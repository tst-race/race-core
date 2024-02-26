
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

#ifndef __I_RACE_PLUGIN_NETWORK_MANAGER_H_
#define __I_RACE_PLUGIN_NETWORK_MANAGER_H_

#include <string>
#include <vector>

#include "BootstrapState.h"
#include "ChannelStatus.h"
#include "ClrMsg.h"
#include "ConnectionStatus.h"
#include "DeviceInfo.h"
#include "EncPkg.h"
#include "IRaceSdkNM.h"
#include "LinkStatus.h"
#include "PackageStatus.h"
#include "PluginConfig.h"
#include "PluginResponse.h"
#include "RacePluginExports.h"

class IRacePluginNM {
public:
    /**
     * @brief Destroy the IRacePluginNM object
     *
     */
    virtual ~IRacePluginNM() {}

    /**
     * @brief Initialize the plugin. Set the RaceSdk object and other prep work
     *        to begin allowing calls from core and other plugins.
     *
     * @param pluginConfig Config object containing dynamic config variables (e.g. paths)
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse init(const PluginConfig &pluginConfig) = 0;

    /**
     * @brief Shutdown the plugin. Close open connections, remove state, etc.
     *
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse shutdown() = 0;

    /**
     * @brief Given a cleartext message, do everything necessary to encrypt and send the encrypted
     * package out on the correct Transport, etc.
     *
     * @param handle The RaceHandle for this call
     * @param msg The clear text message to process.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse processClrMsg(RaceHandle handle, const ClrMsg &msg) = 0;

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
    virtual PluginResponse processEncPkg(RaceHandle handle, const EncPkg &ePkg,
                                         const std::vector<ConnectionID> &connIDs) = 0;

    /**
     * @brief Notify network manager that a device needs to be bootstrapped. network manager should
     * generate the necessary configs and determine what plugins to use. Once everything necessary
     * has been prepared, the network manager should call sdk::bootstrapDevice.
     *
     * @param handle The RaceHandle that should be handed to the bootstrapNode call later
     * @param linkId the link id of the bootstrap link
     * @param configPath A path to the directory to store configs in
     * @param deviceInfo Information about the device being bootstrapped
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse prepareToBootstrap(RaceHandle handle, LinkID linkId,
                                              std::string configPath, DeviceInfo deviceInfo) = 0;

    /**
     * @brief Inform the network manager when the package from a bootstrapped node is received. The
     * network manager should should perform necessary steps to introduce the node the network.
     *
     * @param persona The persona of the new bootstrapped node
     * @param pkg The package received from the bootstrapped node
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse onBootstrapPkgReceived(std::string persona, RawData pkg) = 0;

    /**
     * @brief Inform the network manager that bootstrapping finished. The network manager should
     * should perform necessary steps to stop the bootstrap process internally.
     *
     * @param bootstrapHandle bootstrap handle
     * @param state the final bootstrapping state (finished, cancelled, or failed)
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse onBootstrapFinished(RaceHandle /*bootstrapHandle*/,
                                               BootstrapState /*state*/) {
        return PluginResponse::PLUGIN_ERROR;
    }

    /**
     * @brief Notify network manager about a change in package status. The handle will correspond to
     * a handle returned inside an SdkResponse to a previous sendEncryptedPackage call.
     *
     * @param handle The RaceHandle of the previous sendEncryptedPackage call
     * @param status The PackageStatus of the package updated
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse onPackageStatusChanged(RaceHandle handle, PackageStatus status) = 0;

    /**
     * @brief Notify network manager about a change in the status of a connection. The handle will
     * correspond to a handle returned inside an SdkResponse to a previous openConnection or
     * closeConnection call.
     *
     * @param handle The RaceHandle of the original openConnection or closeConnection call, if that
     * is what caused the change. Otherwise, 0
     * @param connId The ConnectionID of the connection
     * @param status The ConnectionStatus of the connection updated
     * @param linkId The LinkID of the link this connection is on
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse onConnectionStatusChanged(RaceHandle handle, ConnectionID connId,
                                                     ConnectionStatus status, LinkID linkId,
                                                     LinkProperties properties) = 0;

    /**
     * @brief Notify network manager about a change to the LinkProperties of a Link
     *
     * @param linkId The LinkdID of the link that has been updated
     * @param linkProperties The LinkProperties that were updated
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse onLinkPropertiesChanged(LinkID linkId,
                                                   LinkProperties linkProperties) = 0;

    /**
     * @brief Notify network manager about a change to the Links associated with a Persona
     *
     * @param recipientPersona The Persona that has changed link associations
     * @param linkType The LinkType of the links (send, recv, bidi)
     * @param links The list of links that are now associated with this persona
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse onPersonaLinksChanged(std::string recipientPersona, LinkType linkType,
                                                 std::vector<LinkID> links) = 0;

    /**
     * @brief Notify network manager about a change to the status of a channel
     *
     * @param handle The handle for this callback
     * @param channelGid The name of the channel that changed status
     * @param status The new ChannelStatus of the channel
     * @param properties The ChannelProperties of the channel
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse onChannelStatusChanged(RaceHandle handle, std::string channelGid,
                                                  ChannelStatus status,
                                                  ChannelProperties properties) = 0;

    /**
     * @brief Notify network manager about a change to the status of a link
     *
     * @param handle The handle for this callback
     * @param channelGid The name of the link that changed status
     * @param status The new LinkStatus of the link
     * @param properties The LinkProperties of the link
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse onLinkStatusChanged(RaceHandle handle, LinkID linkId, LinkStatus status,
                                               LinkProperties properties) = 0;

    /**
     * @brief Notify network manager about received user input response
     *
     * @param handle The handle for this callback
     * @param answered True if the response contains an actual answer to the input prompt, otherwise
     * the response is an empty string and not valid
     * @param response The user response answer to the input prompt
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse onUserInputReceived(RaceHandle handle, bool answered,
                                               const std::string &response) = 0;

    /**
     * @brief Notify the plugin that the user acknowledged the displayed information
     *
     * @param handle The handle for asynchronously returning the status of this call.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse onUserAcknowledgementReceived(RaceHandle handle) = 0;

    /**
     * @brief Notify network manager to perform epoch changeover processing
     *
     * @param data Data associated with epoch (network manager implementation specific)
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse notifyEpoch(const std::string &data) = 0;
};

extern "C" EXPORT IRacePluginNM *createPluginNM(IRaceSdkNM *sdk);
extern "C" EXPORT void destroyPluginNM(IRacePluginNM *plugin);

#endif
