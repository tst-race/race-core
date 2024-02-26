
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

#ifndef _LOADER_PLUGIN_COMMS_JAVA_WRAPPER_H__
#define _LOADER_PLUGIN_COMMS_JAVA_WRAPPER_H__

#include <IRacePluginComms.h>
#include <IRaceSdkComms.h>
#include <jni.h>

#include <iostream>
#include <string>

class PluginCommsJavaWrapper : public IRacePluginComms {
public:
    /**
     * @brief Construct a new Plugin Comms Java Wrapper object. Instantiate Java Plugin.
     *
     * @param sdk sdk Pointer to the sdk instance.
     * @param pluginName The name of the plugin (must match name of dex on Android)
     * @param pluginClassName JNI Signature for Java Plugin Class
     */
    PluginCommsJavaWrapper(IRaceSdkComms *sdk, const std::string &pluginName,
                           const std::string &pluginClassName);

    PluginCommsJavaWrapper(const PluginCommsJavaWrapper &) = delete;
    PluginCommsJavaWrapper &operator=(const PluginCommsJavaWrapper &) = delete;

    /**
     * @brief Destroy the Plugin Comms Java Wrapper object
     *
     */
    virtual ~PluginCommsJavaWrapper();

    /**
     * @brief Initialize the plugin. Set the RaceSdk object and other prep work
     *        to begin allowing calls from core and other plugins.
     *
     * @param pluginConfig Config object containing dynamic config variables (e.g. paths)
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse init(const PluginConfig &pluginConfig) override;

    /**
     * @brief Shutdown the plugin. Close open connections, remove state, etc.
     *
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse shutdown() override;

    /**
     * @brief Send an encrypted package.
     *
     * @param handle The RaceHandle to use for updating package status in
     *      onPackageStatusChanged calls
     * @param connectionId The ID of the connection to use to send the package.
     * @param pkg The encrypted package to send.
     * @param timeoutTimestamp The time the package must be sent by. Measured in seconds since epoch
     * @param timeoutTimestamp The time the package must be sent by. Measured in seconds since epoch
     * @param batchId The batch ID used to group encrypted packages so that they can be flushed at
     * the same time when flushChannel is called. If this value is zero then it can safely be
     * ignored.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse sendPackage(RaceHandle handle, ConnectionID connectionId, EncPkg pkg,
                                       double timeoutTimestamp, uint64_t batchId) override;

    /**
     * @brief Open a connection with a given type on the specified link. Additional configuration
     * info can be provided via the config param.
     *
     * @param handle The RaceHandle to use for onConnectionStatusChanged calls
     * @param linkType The type of link to open: send, receive, or bi-directional.
     * @param linkId The ID of the link that the connection should be opened on.
     * @param linkHints Additional optional configuration information provided by network manager as
     * a stringified JSON Object.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse openConnection(RaceHandle handle, LinkType linkType, LinkID linkId,
                                          std::string linkHints, int32_t sendTimeout) override;

    /**
     * @brief Close a connection with a given ID.
     *
     * @param handle The RaceHandle to use for onConnectionStatusChanged calls
     * @param connectionId The ID of the connection to close.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse closeConnection(RaceHandle handle, ConnectionID connectionId) override;

    /**
     * @brief Destroy a link
     *
     * @param handle The RaceHandle to use for onLinktatusChanged calls
     * @param linkId The LinkID of the link to destroy
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse destroyLink(RaceHandle handle, LinkID linkId) override;

    /**
     * @brief Deactivate a channel
     *
     * @param handle The RaceHandle to use for onChannelStatusChanged calls
     * @param channelGid The name of the channel to deactivate
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse deactivateChannel(RaceHandle handle, std::string channelGid) override;

    /**
     * @brief activate a channel
     *
     * @param handle The RaceHandle to use for onChannelStatusChanged calls
     * @param channelGid The name of the channel to activate
     * @param roleName The name of the role to activate
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse activateChannel(RaceHandle handle, std::string channelGid,
                                           std::string roleName) override;

    /**
     * @brief Create a link on a specified channel
     *
     * @param handle The RaceHandle to use for onLinkStatusChanged calls
     * @param channelGid The name of the channel to create a link on
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse createLink(RaceHandle handle, std::string channelGid) override;

    /**
     * @brief Load a link address on the specified channel
     *
     * @param handle The RaceHandle to use for onLinkStatusChanged calls
     * @param channelGid The name of the channel to load a link on
     * @param linkAddress The link address to load
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse loadLinkAddress(RaceHandle handle, std::string channelGid,
                                           std::string linkAddress) override;

    /**
     * @brief Load a list of link addresses on the specified channel
     *
     * @param handle The RaceHandle to use for onLinkStatusChanged calls
     * @param channelGid The name of the channel to load a link on
     * @param linkAddresses The list of link addresses to load
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse loadLinkAddresses(RaceHandle handle, std::string channelGid,
                                             std::vector<std::string> linkAddress) override;

    /**
     * @brief create link from address specified by genensis configs
     *
     * @param handle The RaceHandle to use for onLinkStatusChanged calls
     * @param channelGid The name of the channel to create a new link for
     * @param linkAddress The LinkAddress used to create this link
     * @return PluginResponse the status of the SDK in response to this call
     */
    virtual PluginResponse createLinkFromAddress(RaceHandle handle, std::string channelGid,
                                                 std::string linkAddress) override;

    /**
     * @brief Notify comms about received user input response
     *
     * @param handle The handle for this callback
     * @param answered True if the response contains an actual answer to the input prompt, otherwise
     * the response is an empty string and not valid
     * @param response The user response answer to the input prompt
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse onUserInputReceived(RaceHandle handle, bool answered,
                                               const std::string &response) override;

    /**
     * @brief Serve files located in the specified directory. The files served are associated with
     * the specified link and may stop being served when the link is closed.
     *
     * @param linkId The link to serve the files on
     * @param path A path to the directory containing the files to serve
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse serveFiles(LinkID /*linkId*/, std::string /*path*/) override;

    /**
     * @brief Create a bootstrap link of the channel specified with the specified passphrase
     *
     * @param handle The handle for asynchronously returning the status of this call.
     * @param channelGid The name of the channel to create a link for
     * @param passphrase The passphrase to use with the link
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse createBootstrapLink(RaceHandle /*handle*/, std::string /*channelGid*/,
                                               std::string /*passphrase*/) override;

    virtual PluginResponse flushChannel(RaceHandle handle, std::string channelGid,
                                        uint64_t batchId) override;

    /**
     * @brief Notify the plugin that the user acknowledged the displayed information
     *
     * @param handle The handle for asynchronously returning the status of this call.
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual PluginResponse onUserAcknowledgementReceived(RaceHandle handle) override;

private:
    /**
     * @brief Map static JNI native methods to C++ functions.
     *
     * @param sdk sdk Pointer to the sdk instance to pass to plugin
     * @param pluginName The name of the plugin
     * @param pluginClassName JNI Signature for Java Plugin Class
     */
    void linkNativeMethods(IRaceSdkComms *sdk, std::string pluginName, std::string pluginClassName);

    /**
     * @brief destroy Java Plugin object
     *
     */
    void destroyPlugin();

private:
    jclass jPluginClass;
    jobject plugin;
    JavaVM *jvm;

    jmethodID jPluginInitMethodId;
    jmethodID jPluginShutdownMethodId;
    jmethodID jPluginSendPackageMethodId;
    jmethodID jPluginOpenConnectionMethodId;
    jmethodID jPluginCloseConnectionMethodId;
    jmethodID jPluginDeactivateChannelMethodId;
    jmethodID jPluginActivateChannelMethodId;
    jmethodID jPluginDestroyLinkMethodId;
    jmethodID jPluginCreateLinkMethodId;
    jmethodID jPluginLoadLinkAddressMethodId;
    jmethodID jPluginLoadLinkAddressesMethodId;
    jmethodID jPluginCreateLinkFromAddressMethodId;
    jmethodID jPluginOnUserInputReceivedMethodId;
    jmethodID jPluginOnUserAcknowledgementReceivedMethodId;
    jmethodID jflushChannelMethodId;
    jmethodID jserveFilesMethodId;
    jmethodID jcreateBootstrapLinkMethodId;
};

#endif
