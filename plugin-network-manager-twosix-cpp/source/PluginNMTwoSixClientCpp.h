
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

#ifndef __PLUGIN_NETWORK_MANAGER_TWOSIX_CLIENT_CPP_H__
#define __PLUGIN_NETWORK_MANAGER_TWOSIX_CLIENT_CPP_H__

#include <atomic>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>

#include "ClearMessagePackageTracker.h"
#include "ConfigNMTwoSix.h"
#include "PluginNMTwoSix.h"

namespace bmi = boost::multi_index;
using OrderedHashSet = bmi::multi_index_container<
    MsgHash, bmi::indexed_by<bmi::sequenced<>,
                             bmi::ordered_unique<bmi::tag<struct unique>, bmi::identity<MsgHash>>>>;

class PluginNMTwoSixClientCpp : public PluginNMTwoSix {
public:
    explicit PluginNMTwoSixClientCpp(IRaceSdkNM *sdk);
    ~PluginNMTwoSixClientCpp() override;
    PluginResponse init(const PluginConfig &pluginConfig) override;
    PluginResponse processClrMsg(RaceHandle handle, const ClrMsg &msg) override;
    PluginResponse processEncPkg(RaceHandle handle, const EncPkg &ePkg,
                                 const std::vector<ConnectionID> &connIDs) override;

    /**
     * @brief Handle a received client message
     *
     * @param parsedMsg the message that was received.
     * @return status of the call
     */
    PluginResponse handleReceivedMsg(const ExtClrMsg &parsedMsg);

    /**
     * @brief Get the preferred link (based on transmission type) for sending to a type of persona,
     *        i.e. client or server.
     *
     * @param potentialLinks The potential links for connecting to the persona.
     * @param recipientPersonaType The type of persona being sent to.
     * @return LinkID The ID of the preferred link, or empty string on error.
     */
    LinkID getPreferredLinkIdForSendingToPersona(const std::vector<LinkID> &potentialLinks,
                                                 const PersonaType recipientPersonaType) override;
    /**
     * @brief Insert and re-sort a connection in the list of send connections for a UUID. Chooses
     * based on rankConnProps ordering.
     *
     * @param rankedConnections Sorted list of connections to modify-by-reference
     * @param newConn ConnectionID of the new connection
     * @param newProps Linkproperties of the new connection
     * @param recipientPersonaType PersonaType of the destination of the connection
     * @return void
     */
    void insertConnection(std::vector<std::pair<ConnectionID, LinkProperties>> &rankedConnections,
                          const ConnectionID &newConn, const LinkProperties &newProps,
                          const PersonaType recipientPersonaType) override;
    /**
     * @brief Comparator for two pairs of ConnectID, LinkProperties pairs. Chooses based on
     * rankLinkProperties of their LinkProperties and recipientPersonaType
     *
     * @param pair1 pair<ConnectionID, LinkProperties> of first connection
     * @param pair2 pair<ConnectionID, LinkProperties> of second connection
     * @param recipientPersonaType PersonaType of the destination of the connection
     * @return true if pair1 is higher-priority than pair2, false otherwise
     */
    static bool rankConnProps(const std::pair<ConnectionID, LinkProperties> &pair1,
                              const std::pair<ConnectionID, LinkProperties> &pair2);

    /**
     * @brief Comparator for two LinkProperties - prefers CT_INDIRECT, then not CT_UNDEF, then
     * expected send bandwidth
     *
     * @param prop1 LinkProperties of first connection
     * @param prop2 LinkProperties of second connection
     * @param recipientPersonaType PersonaType of the destination of the connection
     * @return true if prop1 is higher-priority than prop2, false otherwise
     */
    static bool rankLinkProperties(const LinkProperties &prop1, const LinkProperties &prop2);

    /**
     * @brief Check if we have at least 1 connection to an entrance committee server
     *
     * @return bool True if necessary connections are open
     */
    bool hasNecessaryConnections() override;

    /**
     * @brief Use the LinkWizard to request additional links if insufficient links exist
     * for the number of desired connections.
     *
     * @param uuidList The list of UUIDs to open links to send to.
     * @return false if an error was received from the SDK on openConnection call, otherwise true
     */
    bool invokeLinkWizard(std::unordered_map<std::string, Persona> personas) override;

    /**
     * @brief Get a list of channel IDs for all expected links.
     *
     * @param uuid Destination UUID
     * @return List of expected channel IDs
     */
    std::vector<std::string> getExpectedChannels(const std::string &uuid) override;

    /**
     * @brief Packs a ClrMsg into a string to call sendFormattedMsg(..., std::string &msgString,
     * ...) for each destination in the multicast group
     *
     * @param uuidList The UUIDs of the personas in the multicast group
     * @param msg The ClrMsg to process
     * @param linkRank The rank of the link to use, zero being highest rank.
     * @return True if successful
     */
    bool sendMulticastMsg(const std::vector<std::string> &uuidList, const ClrMsg &msg,
                          std::size_t linkRank = 0);

    /**
     * @brief Packs a ClrMsg into a string to call sendFormattedMsg(..., std::string &msgString,
     * ...) on
     *
     * @param dstUuid The UUID of the persona to send to
     * @param msg The ClrMsg to process
     */
    RaceHandle sendMsg(const std::string &dstUuid, const ClrMsg &msg) override;

    /**
     * @brief Notify network manager about a change in package status. The handle will correspond to
     * a handle returned inside an SdkResponse to a previous sendEncryptedPackage call.
     *
     * @param handle The RaceHandle of the previous sendEncryptedPackage call
     * @param status The PackageStatus of the package updated
     * @return PluginResponse the status of the Plugin in response to this call
     */
    PluginResponse onPackageStatusChanged(RaceHandle handle, PackageStatus status) override;

    /**
     * @brief Notify network manager that a device needs to be bootstrapped. network manager should
     * generate the necessary configs and determine what plugins to use. Once everything necessary
     * has been prepared, the network manager should call sdk::boostrapNode.
     *
     * @param handle The RaceHandle that should be handed to the bootstrapNode call later
     * @param linkId the link id of the bootstrap link
     * @param configPath A path to the directory to store configs in
     * @param deviceInfo Information about the device being bootstrapped
     * @return PluginResponse the status of the Plugin in response to this call
     */
    PluginResponse prepareToBootstrap(RaceHandle handle, LinkID linkId, std::string configPath,
                                      DeviceInfo deviceInfo) override;

    /**
     * @brief Stop and cleanup bootstrap process
     *
     * @param bootstrapHandle bootstrap handle
     * @param state the final bootstrapping state (finished, cancelled, or failed)
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse onBootstrapFinished(RaceHandle bootstrapHandle,
                                               BootstrapState state) override;

    /**
     * @brief Inform the network manager when the package from a bootstrapped node is receive. The
     * network manager should perform necessary steps to introduce the node the networks.
     *
     * @param persona The persona of the new bootstrapped node
     * @param pkg The package received from the bootstrapped node
     * @return PluginResponse the status of the Plugin in response to this call
     */
    PluginResponse onBootstrapPkgReceived(std::string persona, RawData pkg) override;

    /**
     * @brief Writes the network manager configs to disk.
     *
     * If the network manager config contained any bootstrap information, that will be cleared
     * before writing to disk.
     */
    virtual void writeConfigs() override;

    /**
     * @brief Notify this node that a new node is part of this node's exit committee. Unsupported on
     * client nodes.
     *
     * @param persona persona to add
     * @param key aes key used for encryption when sending to that node
     */
    virtual void addClient(const std::string &persona, const RawData &key) override;

protected:
    /**
     * @brief Read the plugin configuration file
     */
    void loadConfigs();

private:
    ConfigNMTwoSixClient clientConfig;
    OrderedHashSet seenMessages;
    ClearMessagePackageTracker messageStatusTracker;
    std::atomic<uint64_t> nextBatchId{0};

    /**
     * @brief Adds the hash to the seenMessages structure; trims 10% of the oldest hashes if the
     * size exceeds maxSeenMessages
     *
     * @param hash The MsgHash to add to seenMessages
     */
    void addSeenMessage(MsgHash hash);
};

#endif
