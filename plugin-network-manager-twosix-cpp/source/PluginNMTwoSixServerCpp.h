
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

#ifndef __PLUGIN_NETWORK_MANAGER_TWOSIX_SERVER_CPP_H__
#define __PLUGIN_NETWORK_MANAGER_TWOSIX_SERVER_CPP_H__

#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <unordered_set>
#include <vector>

#include "ConfigNMTwoSix.h"
#include "ExtClrMsg.h"
#include "PluginNMTwoSix.h"

namespace bmi = boost::multi_index;
using OrderedUuidSet = bmi::multi_index_container<
    MsgUuid, bmi::indexed_by<bmi::sequenced<>,
                             bmi::ordered_unique<bmi::tag<struct unique>, bmi::identity<MsgUuid>>>>;

class PluginNMTwoSixServerCpp : public PluginNMTwoSix {
public:
    explicit PluginNMTwoSixServerCpp(IRaceSdkNM *sdk);
    ~PluginNMTwoSixServerCpp() override;

    /**
     * @brief Initialize the plugin. Set the RaceSdk object and other prep work
     *        to begin allowing calls from core and other plugins.
     *
     * @param pluginConfig Config object containing dynamic config variables (e.g. paths)
     * @return PluginResponse the status of the Plugin in response to this call
     */
    PluginResponse init(const PluginConfig &pluginConfig) override;

    /**
     * @brief Given a cleartext message, do everything necessary to encrypt and send the encrypted
     * package out on the correct Transport, etc.
     *
     * @param handle The RaceHandle for this call
     * @param msg The clear text message to process.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    PluginResponse processClrMsg(RaceHandle handle, const ClrMsg &msg) override;

    /**
     * @brief Given an encrypted package, do everything necessary to either display it to the user,
     * forward it (if this is a server), or just read it (if this message was intended for the
     * network manager module).
     *
     * @param handle The RaceHandle for this call
     * @param ePkg The encrypted package to process.
     * @param connIDs List of connection IDs that the package may have come in on. Since multiple
     * logical connection IDs can be used for the same physical connection, all logical connection
     * IDs are provided. Note that only one of the connection IDs will actually be associated with
     * this package.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    PluginResponse processEncPkg(RaceHandle handle, const EncPkg &ePkg,
                                 const std::vector<ConnectionID> &connIDs) override;

    /**
     * @brief Read the configuration files and instantiate data.
     */
    void loadConfigs();

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
                              const std::pair<ConnectionID, LinkProperties> &pair2,
                              const PersonaType recipientPersonaType);

    /**
     * @brief Comparator for two LinkProperties.
     * For client destinations it prefers CT_INDIRECT, then not CT_UNDEF, then expected send
     * bandwidth. For server destinations, it prefers not CT_UNDEF, then expected send bandwidth.
     *
     * @param prop1 LinkProperties of first connection
     * @param prop2 LinkProperties of second connection
     * @param recipientPersonaType PersonaType of the destination of the connection
     * @return true if prop1 is higher-priority than prop2, false otherwise
     */
    static bool rankLinkProperties(const LinkProperties &prop1, const LinkProperties &prop2,
                                   const PersonaType recipientPersonaType);

    /**
     * @brief Check if we have at least 1 connection to each ring server hop
     *
     * @return bool True if necessary connections are open
     */
    bool hasNecessaryConnections() override;

    /**
     * @brief Get the preferredtransmission type for sending to a specific persona, based on whether
     * they are a client or server.
     *
     * @param persona the persona to get the preferred transmission type for
     * @return The preferred transmission type
     */
    TransmissionType getPreferredTransmissionType(std::string persona);

    /**
     * @brief Example function for performer use
     */
    void exampleEncPkgWithoutPrecursor();

    /**
     * @brief Use the LinkWizard to request additional links if insufficient links exist
     * for the number of desired connections.
     *
     * @param personas The map of UUID -> Persona to open links to send to.
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
     * @brief Adds the UUID to the staleUuids structure; trims 10% of the oldest UUIDs if the size
     * exceeeds maxStaleUuids
     *
     * @param uuid The MsgUuid uuid to add to staleUuids
     */
    void addStaleUuid(MsgUuid uuid);

    /**
     * @brief Adds the UUID to the floodedUuids structure; trims 10% of the oldest UUIDs if the size
     * exceeeds maxFloodedUuids.
     *
     * @param uuid The MsgUuid uuid to add to floodedUuids
     */
    void addFloodedUuid(MsgUuid uuid);

    /**
     * ROUTING METHODS
     **/

    /**
     * @brief Determines whether the msg should cause a new committee ring msg or be forwarded to
     * the client / other committees.
     *
     * @param msg The ExtClrMsg to process
     */
    void routeMsg(ExtClrMsg &msg);

    /**
     * @brief Checks if the msg has already been seen by this node; if not, calls sendToRings, else
     * drops
     *
     * @param msg The ExtClrMsg to process
     */
    void startRingMsg(const ExtClrMsg &msg);

    /**
     * @brief Sends the msg out on each ring this node knows about, setting the ringTtl and ringIdx
     * variables appropriately for each.
     *
     * @param msg The ExtClrMsg to process
     */
    void sendToRings(const ExtClrMsg &msg);

    /**
     * @brief Handles a received ring msg to either forward it along the ring (if ringTtl > 0),
     * forward to other committees, or forward to a client.
     *
     * @param msg The msg to process
     */
    void handleRingMsg(ExtClrMsg &msg);

    /**
     * @brief Resets ringTtl and ringIdx, appends this committee to committeesVisited, and sends to
     * some committees this node knows about. If this node knows about fewer than floodingFactor
     * committees, it _also_ forwards the msg along its rings with ringTtl=0 to prompt them to
     * forward.
     *
     * @param msg The ExtClrMsg to process
     */
    void forwardToNewCommittees(ExtClrMsg &msg);

    /**
     * @brief Sends a stringified message to the specified destination persona.
     *
     * @param dstUuid The uuid of the persona to send to
     * @param msgString The stringified message to encrypt and send
     */
    void sendMsg(const std::string &dstUuid, const ExtClrMsg &msg);

    /**
     * @brief Packs a ClrMsg into a string to call sendFormattedMsg(..., std::string &msgString,
     * ...) on
     *
     * @param dstUuid The UUID of the persona to send to
     * @param msg The ClrMsg to process
     */
    RaceHandle sendMsg(const std::string &dstUuid, const ClrMsg &msg) override;

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
    virtual PluginResponse prepareToBootstrap(RaceHandle handle, LinkID linkId,
                                              std::string configPath,
                                              DeviceInfo deviceInfo) override;

    /**
     * @brief Inform the network manager when the package from a bootstrapped node is receive. The
     * network manager should perform necessary steps to introduce the node the networks.
     *
     * @param persona The persona of the new bootstrapped node
     * @param pkg The package received from the bootstrapped node
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse onBootstrapPkgReceived(std::string persona, RawData pkg) override;

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
     * @param key aes key used for encryption
     */
    virtual void addClient(const std::string &persona, const RawData &key) override;

private:
    ConfigNMTwoSixServer serverConfig;
    OrderedUuidSet staleUuids;
    OrderedUuidSet floodedUuids;
};

#endif
