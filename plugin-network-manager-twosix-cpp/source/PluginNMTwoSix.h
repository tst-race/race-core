
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

#ifndef __PLUGIN_NETWORK_MANAGER_TWOSIX_H__
#define __PLUGIN_NETWORK_MANAGER_TWOSIX_H__

#include <SdkResponse.h>

#include <mutex>  // std::mutex, std::lock_guard
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "BootstrapManager.h"
#include "EncPkg.h"
#include "IRacePluginNM.h"
#include "LinkManager.h"
#include "LinkWizard.h"
#include "Persona.h"
#include "RaceCrypto.h"

#define BEST_LINK 0

class PluginNMTwoSix : public IRacePluginNM {
public:
    struct AddressedMsg {
        const std::string dst;
        const std::string msg;
        std::uint64_t traceId;
        std::uint64_t spanId;
        bool reliable;
        std::size_t linkRank;
    };

    PluginResponse shutdown() override;

    /**
     * @brief Try to open receive connections for all links for all uuids. If a link has multiple
     * personas associated with it, only one connection is opened. If a link has no personas
     * associated with it, no conneciton is opened
     *
     * @param uuids The list of persona UUIDs to open receive connections for
     * @return true if connections for all links for all uuids were opened, false otherwise
     */
    bool openRecvConns(std::vector<std::string> uuids);
    /**
     * @brief Close all receive connections
     */
    void closeRecvConns();
    /**
     * @brief Handle a connection open event - update internal mappings for send connections
     *
     * @param handle RaceHandle identifying the openConnection request this is associated with
     * @param connId ConnectionID of the opened connection
     * @param properties LinkProperties of the link the connection is on
     * @return PluginResponse status of the plugin (always PLUGIN_OK)
     */
    PluginResponse handleConnectionOpened(RaceHandle handle, const ConnectionID &connId,
                                          const LinkProperties &properties);
    /**
     * @brief Handle a connection closed event. Update internal mappings and attempt to open new
     * connections to replace this connection.
     *
     * @param handle RaceHandle identifying the openConnection request this is associated with
     * @param connId ConnectionID of the opened connection
     * @param properties LinkProperties of the link the connection is on
     * @return PluginResponse status of the plugin
     */
    PluginResponse handleConnectionClosed(RaceHandle handle, const ConnectionID &connId,
                                          const LinkID &linkId, const LinkProperties &properties);
    /**
     * @brief Load list of RACE personas from a config file
     *
     * @param configFilePath String path to the config file
     * @return void
     */
    void loadPersonas(const std::string &configFilePath);

    /**
     * @brief Log the size of a formatted message and how much overhead is added
     *
     * @param formattedMessage String of the message to send
     * @param package EncPkg wrapped messag configFilePath String path to the config filee
     * @return void
     */
    void logMessageOverhead(const std::string &formattedMessage, const EncPkg &package);

    /**
     * @brief Parse an ExtClrMsg out of an ePkg
     *
     * @param ePkg The encrypted package to parse
     * @return The parsed cleartext message
     */
    ExtClrMsg parseMsg(const EncPkg &ePkg);

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
     * @brief Notify network manager about a change in the status of a connection. The handle will
     * correspond to a handle returned inside an SdkResponse to a previous openConnection or
     * closeConnection call.
     *
     * @param handle The RaceHandle of the original openConnection or closeConnection call, if that
     * is what caused the change. Otherwise, 0
     * @param status The ConnectionStatus of the connection updated
     * @return PluginResponse the status of the Plugin in response to this call
     */
    PluginResponse onConnectionStatusChanged(RaceHandle handle, ConnectionID connId,
                                             ConnectionStatus status, LinkID linkId,
                                             LinkProperties properties) override;

    /**
     * @brief Notify network manager about a change to the status of a channel
     *
     * @param handle The handle for this callback
     * @param channelGid The name of the channel that changed status
     * @param status The new ChannelStatus of the channel
     * @param properties The ChannelProperties of the channel
     * @return PluginResponse the status of the Plugin in response to this call
     */
    PluginResponse onChannelStatusChanged(RaceHandle handle, std::string channelGid,
                                          ChannelStatus status,
                                          ChannelProperties properties) override;

    /**
     * @brief Notify network manager about a change to the status of a link
     *
     * @param handle The handle for this callback
     * @param channelGid The name of the link that changed status
     * @param status The new LinkStatus of the link
     * @param properties The LinkProperties of the link
     * @return PluginResponse the status of the Plugin in response to this call
     */
    PluginResponse onLinkStatusChanged(RaceHandle handle, LinkID linkId, LinkStatus status,
                                       LinkProperties properties) override;

    /**
     * @brief Notify network manager about a change to the LinkProperties of a Link
     *
     * @param linkId The LinkdID of the link that has been updated
     * @param linkProperties The LinkProperties that were updated
     * @return PluginResponse the status of the Plugin in response to this call
     */
    PluginResponse onLinkPropertiesChanged(LinkID linkId, LinkProperties linkProperties) override;

    /**
     * @brief Notify network manager about a change to the Links associated with a Persona
     *
     * @param recipientPersona The Persona that has changed link associations
     * @param linkType The LinkType of the links (send, recv, bidi)
     * @param links The list of links that are now associated with this persona
     * @return PluginResponse the status of the Plugin in response to this call
     */
    PluginResponse onPersonaLinksChanged(std::string recipientPersona, LinkType linkType,
                                         std::vector<LinkID> links) override;

    /**
     * @brief Notify network manager about received user input response
     *
     * @param handle The handle for this callback
     * @param answered True if the response contains an actual answer to the input prompt, otherwise
     * the response is an empty string and not valid
     * @param response The user response answer to the input prompt
     * @return PluginResponse the status of the Plugin in response to this call
     */
    PluginResponse onUserInputReceived(RaceHandle handle, bool answered,
                                       const std::string &response) override;

    /**
     * @brief Notify the plugin tha the user acknowledged the displayed information
     *
     * @param handle The handle for asynchronously returning the status of this call.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    PluginResponse onUserAcknowledgementReceived(RaceHandle handle) override;

    /**
     * @brief Notify network manager to perform epoch changeover processing
     *
     * @param data Data associated with epoch (network manager implementation specific)
     * @return PluginResponse the status of the Plugin in response to this call
     */
    PluginResponse notifyEpoch(const std::string &data) override;

    /**
     * @brief Post-startup logic after static links have been created and we can communicate with
     * the outside world.
     */
    virtual void onStaticLinksCreated();

    /**
     * @brief Get the preferred link (based on transmission type) for sending to a type of persona,
     *        i.e. client or server.
     *
     * @param potentialLinks The potential links for connecting to the persona.
     * @param recipientPersonaType The type of persona being sent to.
     * @return LinkID The ID of the preferred link, or empty string on error.
     */
    virtual LinkID getPreferredLinkIdForSendingToPersona(
        const std::vector<LinkID> &potentialLinks, const PersonaType recipientPersonaType) = 0;

    /**
     * @brief Check if necessary minimal connections are open
     *
     * @return bool True if necessary connections are open
     */
    virtual bool hasNecessaryConnections() = 0;

    /**
     * @brief Try to apply some hints if the link supports them
     *
     * @param properties The LinkProperties of the link
     * @return stringified JSON of the link hints to send
     */
    std::string tryHints(LinkProperties properties);

    /**
     * @brief Packs a ClrMsg into a string to call sendFormattedMsg(..., std::string &msgString,
     * ...) on
     *
     * @param dstUuid The UUID of the persona to send to
     * @param msg The ClrMsg to process
     *
     * @return The RaceHandle associated with the sent encrypted package.
     */
    virtual RaceHandle sendMsg(const std::string &dstUuid, const ClrMsg &msg) = 0;

    /**
     * @brief Sends a stringified message to the specified destination persona.
     *
     * @param dstUuid The uuid of the persona to send to
     * @param msgString The stringified message to encrypt and send
     * @param traceId The OpenTracing traceId of the original received EncPkg to continue on
     * @param spanId The OpenTracing spanId of the original received EncPkg to continue on
     *
     * @return The RaceHandle associated with the sent encrypted package.
     */
    virtual RaceHandle sendFormattedMsg(const std::string &dstUuid, const std::string &msgString,
                                        const std::uint64_t traceId, const std::uint64_t spanId);

    virtual bool sendBootstrapPkg(const ConnectionID &connId, const std::string &dstUuid,
                                  const std::string &msgString);
    /**
     * @brief Virtual method to insert and re-sort a connection in the list of send connections for
     * a UUID. Implemented by subclasses to do their own prioritizing based on recipient persona
     * type and LinkProperties.
     *
     * @param rankedConnections Sorted list of connections to modify-by-reference
     * @param newConn ConnectionID of the new connection
     * @param newProps Linkproperties of the new connection
     * @param recipientPersonaType PersonaType of the destination of the connection
     * @return void
     */
    virtual void insertConnection(
        std::vector<std::pair<ConnectionID, LinkProperties>> &rankedConnections,
        const ConnectionID &newConn, const LinkProperties &newProps,
        const PersonaType recipientPersonaType) = 0;

    /**
     * @brief Writes the network manager configs to disk.
     *
     * If the network manager config contained any bootstrap information, that will be cleared
     * before writing to disk.
     */
    virtual void writeConfigs() = 0;

    /**
     * @brief Notify this node that a new node is part of this node's exit committee. Unsupported on
     * client nodes.
     *
     * @param persona persona to add
     * @param key aes key used for encryption when sending to that node
     */
    virtual void addClient(const std::string &persona, const RawData &key) = 0;

    /**
     * @brief Open connection for all personas on link
     *
     * @param linkId link to open connections on
     * @param properties properties of the link
     */
    bool openConnectionsForLink(const LinkID &linkId, const LinkProperties &properties);

    /**
     * @brief Use the LinkWizard to request additional links if insufficient links exist
     * for the number of desired connections.
     *
     * @param uuidList The list of UUIDs to open links to send to.
     * @return false if an error was received from the SDK on openConnection call, otherwise true
     */
    virtual bool invokeLinkWizard(std::unordered_map<std::string, Persona> personas) = 0;

    /**
     * @brief Get a list of channel IDs for all expected links to the specified destination.
     *
     * @param uuid Destination UUID
     * @return List of expected channel IDs
     */
    virtual std::vector<std::string> getExpectedChannels(const std::string &uuid) = 0;

    /**
     * @brief Get the configs that were passed into init
     *
     * @param persona persona to add
     * @param key aes key used for encryption when sending to that node
     */
    virtual PluginConfig getConfigs();

    /**
     * @brief Get the path to the Jaeger config file
     *
     * @return Path to jaeger config file
     */
    virtual std::string getJaegerConfigPath();

    /**
     * @brief Get sdk
     *
     * @return the sdk
     */
    virtual IRaceSdkNM *getSdk();

    /**
     * @brief Get persona of this node
     *
     * @return the sdk
     */
    virtual std::string getUuid();

    /**
     * @brief Get encryptorused to encrypt messages
     *
     * @return the sdk
     */
    virtual RaceCrypto getEncryptor();

    virtual LinkManager *getLinkManager();
    /**
     * @brief Get aes key used to encrypt messages for this node
     *
     * @return the sdk
     */
    virtual std::vector<uint8_t> getAesKeyForSelf();

protected:
    PluginNMTwoSix(IRaceSdkNM *sdk, PersonaType personaType);

    /**
     * @brief Sends a stringified message to the specified destination persona.
     *
     * @param dstUuid The uuid of the persona to send to
     * @param msgString The stringified message to encrypt and send
     * @param traceId The OpenTracing traceId of the original received EncPkg to continue on
     * @param spanId The OpenTracing spanId of the original received EncPkg to continue on
     * @param linkRank The rank of the link to use, zero being highest rank.
     *
     * @return The RaceHandle associated with the sent encrypted package, or NULL_RACE_HANDLE if the
     * function fails.
     */
    RaceHandle sendFormattedMsg(const std::string &dstUuid, const std::string &msgString,
                                const std::uint64_t traceId, const std::uint64_t spanId,
                                const std::size_t linkRank);

    IRaceSdkNM *raceSdk;
    std::unordered_map<std::string, Persona> uuidToPersonaMap;
    std::unordered_set<ConnectionID> recvConnectionSet;
    std::unordered_map<RaceHandle, std::pair<std::vector<std::string>, LinkType>>
        openingConnectionsMap;
    std::unordered_map<std::string, std::vector<std::pair<ConnectionID, LinkProperties>>>
        uuidToConnectionsMap;
    std::unordered_map<ConnectionID, std::vector<std::string>> connectionToUuidMap;
    std::mutex connectionLock;

    RaceCrypto encryptor;
    std::unordered_map<RaceHandle, const AddressedMsg> resendMap;
    std::unordered_map<std::string, Persona> uuidsToSendTo;
    std::string raceUuid;
    PersonaType myPersonaType;
    LinkWizard linkWizard;
    bool useLinkWizard;
    bool linkWizardInitialized;
    double lookbackSeconds;
    PluginConfig config;

    BootstrapManager bootstrap;
    LinkManager linkManager;
};

#endif
