
//
// Copyright 2023 Two Six Technologies
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "DirectLink.h"

#include <RaceLog.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>  // SIGUSR1
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>

#include "../PluginCommsTwoSixCpp.h"
#include "../base/Connection.h"
#include "../utils/log.h"

/**
 * @brief helper function to convert std::thread::id to a std::string
 *
 * std::thread::id is platform-dependent and does not have an automatic conversion to string
 *
 * However, std::thread::id _does_ have operator<<(), so it's best to use it
 *
 * @param threadId the thread::id you want to represent as a string
 * @return std::string a string representation of the thread's id
 */
static std::string to_string(std::thread::id threadId) {
    std::ostringstream oss;
    oss << std::hex << threadId;
    return oss.str();
}

/**
 * @brief Log the sender of a package given the socket that the message was sent on. The function
 *        will try to log the hostname of the sender, but if that fails it will fallback to just
 *        logging the IP address.
 *
 * @param sock The socket that the sender used to send the package.
 */
inline void logDirectConnectionSender(int sock) {
    struct sockaddr_storage peerAddr;
    socklen_t peerAddrLen = sizeof(peerAddr);
    // Get the address of the sender so that it can be logged for debugging.
    getpeername(sock, reinterpret_cast<struct sockaddr *>(&peerAddr), &peerAddrLen);
    if (peerAddr.ss_family == AF_INET) {
        struct sockaddr_in *s = reinterpret_cast<struct sockaddr_in *>(&peerAddr);
        int peerPort = ntohs(s->sin_port);
        char ipstr[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof(ipstr));

        const uint32_t hostLen = 256;
        char host[hostLen];
        const uint32_t serviceLen = 256;
        char service[serviceLen];
        // Try to get the host name from the IP address. If that fails then just log the IP.
        if (getnameinfo(reinterpret_cast<struct sockaddr *>(&peerAddr), peerAddrLen, host, hostLen,
                        service, serviceLen, 0) == 0) {
            logInfo("directConnectionMonitor: received message from " + std::string(host) + ":" +
                    std::string(service) + " resolves to " + std::string(ipstr) + ":" +
                    std::to_string(peerPort));
        } else {
            logInfo("directConnectionMonitor: received message from " + std::string(ipstr) + ":" +
                    std::to_string(peerPort));
        }
    } else {
        logError("directConnectionMonitor: failed to log sender address. Only IPv4 is supported");
    }
}

DirectLink::DirectLink(IRaceSdkComms *sdk, PluginCommsTwoSixCpp *plugin, Channel *channel,
                       const LinkID &linkId, const LinkProperties &linkProperties,
                       const DirectLinkProfileParser &parser) :
    Link(sdk, plugin, channel, linkId, linkProperties, parser),
    mSocket(0),
    mTerminated(false),
    mHostname(parser.hostname),
    mPort(parser.port) {
    mProperties.linkAddress = this->DirectLink::getLinkAddress();
}

DirectLink::DirectLink(IRaceSdkComms *sdk, PluginCommsTwoSixCpp *plugin, Channel *channel,
                       const LinkID &linkId, const LinkProperties &linkProperties,
                       const std::string &linkAddress) :
    Link(sdk, plugin, channel, linkId, linkProperties, DirectLinkProfileParser()),
    mSocket(0),
    mTerminated(false),
    mHostname(nlohmann::json::parse(linkAddress)["hostname"]),
    mPort(nlohmann::json::parse(linkAddress)["port"]) {
    mProperties.linkAddress = this->DirectLink::getLinkAddress();
}

DirectLink::~DirectLink() {
    shutdownLink();
}

void DirectLink::shutdownInternal() {
    auto properties = getProperties();
    for (const auto &connection : getConnections()) {
        closeConnection(connection->connectionId);
        if (connection->linkType == LT_SEND) {
            mSdk->onConnectionStatusChanged(NULL_RACE_HANDLE, connection->connectionId,
                                            CONNECTION_CLOSED, properties, RACE_BLOCKING);
        }
    }
}

std::shared_ptr<Connection> DirectLink::openConnection(LinkType linkType,
                                                       const ConnectionID &connectionId,
                                                       const std::string &linkHints, int timeout) {
    const std::string loggingPrefix = "DirectLink::openConnection (" + mId + "): ";
    logInfo(loggingPrefix + " called");

    (void)linkHints;

    std::lock_guard<std::mutex> lock(mLinkLock);

    auto connection = std::make_shared<Connection>(connectionId, linkType, shared_from_this(),
                                                   linkHints, timeout);

    mConnections.push_back(connection);

    return connection;
}

void DirectLink::closeConnection(const ConnectionID &connectionId) {
    logDebug("DirectLink::closeConnection called");
    std::unique_lock<std::mutex> lock(mLinkLock);

    auto connectionIter =
        std::find_if(mConnections.begin(), mConnections.end(),
                     [&connectionId](const std::shared_ptr<Connection> &connection) {
                         return connectionId == connection->connectionId;
                     });

    if (connectionIter == mConnections.end()) {
        logWarning("DirectLink::closeConnection no connection found with ID " + connectionId);
        return;
    }

    LinkType lt = (*connectionIter)->linkType;
    mConnections.erase(connectionIter);

    if (lt == LT_RECV || lt == LT_BIDI) {
        bool hasReceiveConnection = std::any_of(
            mConnections.begin(), mConnections.end(), [](const std::shared_ptr<Connection> &conn) {
                return conn->linkType == LT_RECV || conn->linkType == LT_BIDI;
            });
        logDebug("DirectLink::closeConnection still has open receive connections? " +
                 std::string(hasReceiveConnection ? "true" : "false"));

        // shutdown monitor thread if there are no receive connections
        if (!hasReceiveConnection) {
            if (mSocket != 0) {
                logDebug("Shutting down socket: " + std::to_string(mSocket));
                mTerminated = true;
                ::shutdown(mSocket, SHUT_RDWR);
                logDebug("Socket shutdown.");
            } else {
                logWarning("No socket found for link " + mId);
            }

            if (mMonitorThread.joinable()) {
                logDebug("Joining thread: " + to_string(mMonitorThread.get_id()));
                lock.unlock();
                mMonitorThread.join();
                lock.lock();
                logInfo("Finished shutting down socket");
            }
        }
    }
    logDebug("DirectLink::closeConnection returned");
}

void DirectLink::startConnection(Connection *connection) {
    const std::string loggingPrefix =
        "DirectLink::startConnection (" + connection->connectionId + "): ";
    std::lock_guard<std::mutex> lock(mLinkLock);
    if (connection->linkType == LT_BIDI || connection->linkType == LT_RECV) {
        // if the monitor thread does not exist, we have to create it
        if (!mMonitorThread.joinable()) {
            logDebug(loggingPrefix + "creating thread for receiving link ID: " + mId);
            mTerminated = false;
            mMonitorThread = std::thread(&DirectLink::runMonitor, this);
        } else {
            logDebug(loggingPrefix + "Link " + mId + " already open. Reusing link for connection " +
                     connection->connectionId + ".");
        }
    }
}

bool DirectLink::sendPackageInternal(RaceHandle handle, const EncPkg &pkg) {
    const std::string loggingPrefix = "DirectLink::sendPackage (" + mId + "): ";
    logInfo(loggingPrefix + " called");
    logDebug(loggingPrefix + "    Hostname: " + mHostname);
    logDebug(loggingPrefix + "    Port: " + std::to_string(mPort));

    logDebug("Creating Socket " + mHostname + ":" + std::to_string(mPort));
    const int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        logError(loggingPrefix + "Send Failure: failed to create socket to connect to " +
                 mHostname + ":" + std::to_string(mPort) + ": " + std::to_string(errno) + ": " +
                 std::string(strerror(errno)));
        close(sock);
        mSdk->onPackageStatusChanged(handle, PACKAGE_FAILED_GENERIC, RACE_BLOCKING);

        // package failed, but we don't want to close the link, so return 'success'
        return true;
    }

    logDebug("sendPackageDirectLink: Getting Host " + mHostname + ":" + std::to_string(mPort));
    const struct hostent *he = gethostbyname(mHostname.c_str());
    if (he == nullptr) {
        logError(loggingPrefix + "Failed to get host by name for: " + mHostname);
        close(sock);
        mSdk->onPackageStatusChanged(handle, PACKAGE_FAILED_GENERIC, RACE_BLOCKING);

        // package failed, but we don't want to close the link, so return 'success'
        return true;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(mPort);

    /* copy the network address to sockaddr_in structure */
    memcpy(&serv_addr.sin_addr, he->h_addr_list[0], static_cast<size_t>(he->h_length));

    logDebug("sendPackageDirectLink: Connecting to Host " + mHostname + ":" +
             std::to_string(mPort));
    int retries = 0;
    while (connect(sock, reinterpret_cast<sockaddr *>(&serv_addr), sizeof(serv_addr)) < 0) {
        if (errno == EADDRNOTAVAIL) {
            // This may happen under load due to sockets in timed wait after we call close

            retries++;
            if (retries > 50) {
                logWarning(loggingPrefix +
                           "Connect still failing after 50 retries. Continuing to retry, but "
                           "something may be wrong.");
            } else {
                logInfo(loggingPrefix + "Connect failed due to EADDRNOTAVAIL. retrying");
            }

            // 10 milliseconds should be long enough if this is the only thread creating sockets
            // there's ~30k sockets and 120 second time-wait, so just have to be slower than 4 ms
            // per socket
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        } else {
            logError(loggingPrefix +
                     "Connect Failure: an error occurred while sending a message to " + mHostname +
                     ":" + std::to_string(mPort) + ". connect failed with error: " +
                     std::to_string(errno) + ": " + std::string(strerror(errno)));
            close(sock);
            mSdk->onPackageStatusChanged(handle, PACKAGE_FAILED_GENERIC, RACE_BLOCKING);

            // package failed, but we don't want to close the link, so return 'success'
            return true;
        }
    }

    socklen_t socket_len = sizeof(serv_addr);
    if (getsockname(sock, reinterpret_cast<sockaddr *>(&serv_addr), &socket_len) == -1) {
        logError("sendPackageDirectLink: getsockname failed: " + std::to_string(errno) + ": " +
                 std::string(strerror(errno)));
        mSdk->onPackageStatusChanged(handle, PACKAGE_FAILED_GENERIC, RACE_BLOCKING);

        // package failed, but we don't want to close the link, so return 'success'
        return true;
    } else {
        logDebug("sendPackageDirectLink: Connected to Host " + mHostname + ":" +
                 std::to_string(mPort) + " - sending on portNumber " +
                 std::to_string(static_cast<std::uint32_t>(ntohs(serv_addr.sin_port))));
    }

    logDebug("sendPackageDirectLink: Sending Bytes " + mHostname + ":" + std::to_string(mPort) +
             " - numBytes = " + std::to_string(pkg.getRawData().size()));
    const auto numBytesSent = send(sock, pkg.getRawData().data(), pkg.getRawData().size(), 0);
    if (numBytesSent < 0) {
        logError(loggingPrefix + "Send Failure: an error occurred while sending a message to " +
                 mHostname + ":" + std::to_string(mPort) + ". send failed with error: " +
                 std::to_string(errno) + ": " + std::string(strerror(errno)));
        mSdk->onPackageStatusChanged(handle, PACKAGE_FAILED_GENERIC, RACE_BLOCKING);
        return true;
    } else if (static_cast<size_t>(numBytesSent) != pkg.getRawData().size()) {
        logError(loggingPrefix + "Send Failure: an error occurred while sending a message to " +
                 mHostname + ":" + std::to_string(mPort) + ". attempted to send " +
                 std::to_string(pkg.getRawData().size()) + " bytes but only sent " +
                 std::to_string(numBytesSent));
        mSdk->onPackageStatusChanged(handle, PACKAGE_FAILED_GENERIC, RACE_BLOCKING);

        // package failed, but we don't want to close the link, so return 'success'
        return true;
    }

    logDebug("sendPackageDirectLink: numBytesSent: " + std::to_string(numBytesSent));

    logDebug("Closing Connection " + mHostname + ":" + std::to_string(mPort));
    if (close(sock) != 0) {
        logWarning(loggingPrefix + "Close Failure: an error occurred while closing the socket to " +
                   mHostname + ":" + std::to_string(mPort) + ". close failed with error: " +
                   std::to_string(errno) + ": " + std::string(strerror(errno)));
    }

    mSdk->onPackageStatusChanged(handle, PACKAGE_SENT, RACE_BLOCKING);
    logInfo(loggingPrefix + "returned");
    return true;
}

void DirectLink::runMonitor() {
    const std::string loggingPrefix = "DirectLink::runMonitor (" + mId + "): ";
    logInfo(loggingPrefix + ": called");
    logDebug(loggingPrefix + ": Monitoring direct link: " + mId + " on " + mHostname + ":" +
             std::to_string(mPort));

    const std::chrono::seconds secondsToSleepBetweenRetries{5};
    while (true) {
        int server_fd;
        struct sockaddr_in address;
        int opt = 1;
        int addrlen = sizeof(address);

        // Creating socket file descriptor
        logDebug(loggingPrefix + ": opening socket");
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
            logError(loggingPrefix + ": Receive Failure: Socket Creation: Errno: " +
                     std::to_string(errno) + ": " + std::string(strerror(errno)));
            return;
        }
        mSocket = server_fd;
        logDebug(loggingPrefix + ": socket opened: " + std::to_string(server_fd));

        // Forcefully attaching socket to the port 8080
        logDebug(loggingPrefix + ": calling setsockopt");
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
            logError(loggingPrefix + ": Receive Failure: Getsockopt: Errno: " +
                     std::to_string(errno) + ": " + std::string(strerror(errno)));
            return;
        }
        logDebug(loggingPrefix + ": setsockopt returned");
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(mPort);

        // Forcefully attaching socket to the port 8080
        logDebug(loggingPrefix + ": calling bind");
        if (bind(server_fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
            logError(loggingPrefix + ": Receive Failure: Bind: Errno: " + std::to_string(errno) +
                     ": " + std::string(strerror(errno)));
            return;
        }
        logDebug(loggingPrefix + ": bind returned");
        logDebug(loggingPrefix + ": calling listen");

        const std::int32_t backlogSize = 128;
        if (listen(server_fd, backlogSize) < 0) {
            logError(loggingPrefix + ": Receive Failure: Listen: Errno: " + std::to_string(errno) +
                     ": " + std::string(strerror(errno)));
            return;
        }
        logDebug(loggingPrefix + ": listen returned");

        constexpr int bufferSize = 1024;
        std::uint8_t buffer[bufferSize];
        ssize_t numRead;
        RawData data;
        logInfo(loggingPrefix + ": waiting to receive...");
        while (true) {
            int sock = accept(server_fd, reinterpret_cast<sockaddr *>(&address),
                              reinterpret_cast<socklen_t *>(&addrlen));
            if (sock == -1) {
                if ((errno == ENETDOWN || errno == EPROTO || errno == ENOPROTOOPT ||
                     errno == EHOSTDOWN || errno == ENONET || errno == EHOSTUNREACH ||
                     errno == EOPNOTSUPP || errno == ENETUNREACH)) {
                    logWarning(loggingPrefix + ": accept failed with errno = " +
                               std::to_string(errno) + ". retrying...");
                    continue;
                }
                break;
            }
            logDebug(loggingPrefix + ": New socket connection: " + std::to_string(sock));

            logDirectConnectionSender(sock);

            do {
                std::memset(buffer, 0, bufferSize);
                numRead = read(sock, buffer, bufferSize);
                if (numRead > 0) {
                    data.insert(data.end(), buffer, buffer + std::size_t(numRead));
                } else if (numRead < 0) {
                    logError(loggingPrefix + ": read failure: errno: " + std::to_string(errno) +
                             ": " + std::string(strerror(errno)));
                    data.clear();
                    break;
                }
            } while (numRead > 0);

            close(sock);

            logInfo(loggingPrefix + ": received package on " + mHostname + ":" +
                    std::to_string(mPort) + " of size " + std::to_string(data.size()) +
                    " bytes on link " + mId);
            if (data.size() > 0) {
                EncPkg package(data);
                logDebug(loggingPrefix + ": Received encrypted package");

                std::vector<ConnectionID> connIds;

                for (const auto &conn : getConnections()) {
                    // cppcheck-suppress useStlAlgorithm
                    connIds.push_back(conn->connectionId);
                }

                receivePackageWithCorruption(package, connIds, RACE_BLOCKING);
                data.clear();
            }
            logInfo(loggingPrefix + ": waiting to receive...");
        }

        if (!mTerminated) {
            logError(loggingPrefix + ": unexpected accept failure: errno: " +
                     std::to_string(errno) + ": " + std::string(strerror(errno)));
        }

        close(server_fd);

        logDebug(loggingPrefix + ": socket closed");

        if (mTerminated) {
            break;
        }

        logDebug(loggingPrefix + ": retrying...");
        std::this_thread::sleep_for(secondsToSleepBetweenRetries);
    }

    logDebug(loggingPrefix + "Closing LinkID: " + mId);
    for (const auto &connection : getConnections()) {
        logDebug("\tCauses closure of connectionId " + connection->connectionId);
        mSdk->onConnectionStatusChanged(NULL_RACE_HANDLE, connection->connectionId,
                                        CONNECTION_CLOSED, mProperties, RACE_BLOCKING);
    }
    logInfo(loggingPrefix + " returned");
}

std::string DirectLink::getLinkAddress() {
    nlohmann::json address;
    address["hostname"] = mHostname;
    address["port"] = mPort;
    return address.dump();
}
