
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

#ifndef __NETWORK_MANAGER_WRAPPER_H__
#define __NETWORK_MANAGER_WRAPPER_H__

#include <atomic>
#include <memory>

#include "../include/RaceSdk.h"
#include "Handler.h"
#include "IRacePluginNM.h"
#include "OpenTracingForwardDeclarations.h"

/*
 * NMWrapper: A wrapper for a networkManager plugin that calls associated methods on a separate
 * plugin thread
 */
class NMWrapper : public IRaceSdkNM {
protected:
    RaceSdk &raceSdk;
    std::shared_ptr<opentracing::Tracer> mTracer;
    Handler mThreadHandler;
    // nextPostId is used to identify which post matches with which call/return log.
    std::atomic<uint64_t> nextPostId = 0;

    std::shared_ptr<IRacePluginNM>
        mPlugin;  // unique_ptr requires deleter type as template parameter
    std::string mId;
    std::string mDescription;
    std::string mConfigPath;

    explicit NMWrapper(RaceSdk &sdk, const std::string &name);

    using Interface = IRacePluginNM;
    using SDK = IRaceSdkNM;
    static constexpr const char *createFuncName = "createPluginNM";
    static constexpr const char *destroyFuncName = "destroyPluginNM";
    SDK *getSdk() {
        return this;
    }

public:
    NMWrapper(std::shared_ptr<IRacePluginNM> plugin, std::string id, std::string description,
              RaceSdk &sdk, const std::string &configPath = "");
    virtual ~NMWrapper();

    /* startHandler: start the plugin thread
     *
     * This starts the internally managed thread on which methods of the wrapped plugin are run.
     * Calling a method that executes something on this thread before calling startHandler will
     * schedule the plugin method to be called once startHandler is called.
     *
     * @return tuple<bool, double> tuple containing whether the post was successful, and the
     * proportion of the queue utilized
     */
    virtual void startHandler();

    /* stopHandler: stop the plugin thread
     *
     * This stops the internally managed thread on which methods of the wrapped plugin are run. Any
     * callbacks posted, but not yet completed will be finished. Attempting to post a new callback
     * will fail.
     *
     * @return tuple<bool, double> tuple containing whether the post was successful, and the
     * proportion of the queue utilized
     */
    virtual void stopHandler();

    /* waitForCallbacks: wait for all callbacks to finish
     *
     * Creates a queue and callback with the minimum priority and waits for that to finish. Used for
     * testing.
     */
    virtual void waitForCallbacks();

    /* init: call init on the wrapped plugin
     *
     * init will be run on the current thread instead of the plugin thread.
     */
    virtual bool init(const PluginConfig &pluginConfig);

    /* shutdown: call shutdown on the wrapped plugin, timing out if it takes too long.
     *
     * shutdown will be called on the plugin thread. Shutdown may return before the shutdown method
     * of the wrapped plugin is complete.
     *
     * @return tuple<bool, double> tuple containing whether the post was successful, and the
     * proportion of the queue utilized
     */
    virtual std::tuple<bool, double> shutdown();

    /**
     * @brief Call shutdown on the wrapped plugin, timing out if the call takes longer than the
     * specified timeout.
     *
     * @param timeoutInSeconds Timeout in seconds.
     *
     * @return tuple<bool, double> tuple containing whether the post was successful, and the
     * proportion of the queue utilized
     */
    virtual std::tuple<bool, double> shutdown(std::int32_t timeoutInSeconds);

    /* processClrMsg: call processClrMsg on the wrapped plugin
     *
     * processClrMsg will be called on the plugin thread. processClrMsg may return before the
     * processClrMsg method of the wrapped plugin is complete.
     *
     * @return tuple<bool, double> tuple containing whether the post was successful, and the
     * proportion of the queue utilized
     */
    virtual std::tuple<bool, double> processClrMsg(RaceHandle handle, const ClrMsg &msg,
                                                   int32_t timeout);

    /* processEncPkg: call processEncPkg on the wrapped plugin
     *
     * processEncPkg will be called on the plugin thread. processEncPkg may return before the
     * processEncPkg method of the wrapped plugin is complete.
     *
     * @return tuple<bool, double> tuple containing whether the post was successful, and the
     * proportion of the queue utilized
     */
    virtual std::tuple<bool, double> processEncPkg(RaceHandle handle, const EncPkg &ePkg,
                                                   const std::vector<ConnectionID> &connIDs,
                                                   int32_t timeout);

    /**
     * Notify network manager that a device needs to be bootstrapped. The network manager should
     * generate the necessary configs and determine what plugins to use. Once everything necessary
     * has been prepared, the networkManager should call sdk::bootstrapDevice.
     *
     * @param handle The RaceHandle that should be handed to the bootstrapNode call later
     * @param linkId the link id of the bootstrap link
     * @param configPath A path to the directory to store configs in
     * @param deviceInfo Information about the device being bootstrapped
     *
     * @return tuple<bool, double> tuple containing whether the post was successful, and the
     * proportion of the queue utilized
     */
    virtual std::tuple<bool, double> prepareToBootstrap(RaceHandle handle, LinkID linkId,
                                                        std::string configPath,
                                                        DeviceInfo deviceInfo, int32_t timeout);

    /**
     * Inform the network manager when the package from a bootstrapped node is receive. The network
     * manager should should preform necessary steps to introduce the node the network.
     *
     * @param persona The persona of the new bootstrapped node
     * @param pkg The package received from the bootstrapped node
     * @return PluginResponse the status of the Plugin in response to this call
     */
    virtual std::tuple<bool, double> onBootstrapPkgReceived(std::string persona, RawData pkg,
                                                            int32_t timeout);

    /**
     * Inform the network manager bootstrapping finished, failed or cancelled. The network manager
     * should preform necessary steps to cancel its internal bootstrap process.
     *
     * @return success or failure of the Plugin in response to this call
     */
    virtual bool onBootstrapFinished(RaceHandle bootstrapHandle, BootstrapState state);

    /**
     * @brief Notify network manager about a change in package status. The handle will correspond to
     * a handle returned inside an SdkResponse to a previous sendEncryptedPackage call.
     *
     * @param handle The RaceHandle of the previous sendEncryptedPackage call
     * @param status The PackageStatus of the package updated
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     *
     * @return tuple<bool, double> tuple containing whether the post was successful, and the
     * proportion of the queue utilized
     */
    virtual std::tuple<bool, double> onPackageStatusChanged(RaceHandle handle, PackageStatus status,
                                                            int32_t timeout);

    /**
     * @brief Notify network manager about a change in the status of a connection. The handle will
     * correspond to a handle returned inside an SdkResponse to a previous openConnection or
     * closeConnection call.
     *
     * @param handle The RaceHandle of the original openConnection or closeConnection call, if that
     * is what caused the change. Otherwise, 0
     * @param connId The ConnectionID of the connection that changed status
     * @param status The ConnectionStatus of the connection updated
     * @param properties LinkProperties of the Link this connection is on
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     *
     * @return tuple<bool, double> tuple containing whether the post was successful, and the
     * proportion of the queue utilized
     */
    virtual std::tuple<bool, double> onConnectionStatusChanged(
        RaceHandle handle, const ConnectionID &connId, ConnectionStatus status,
        const LinkID &linkId, const LinkProperties &properties, int32_t timeout);

    /**
     * @brief Notify network manager via the SDK that the stauts of a link has changed
     *
     * @param handle The RaceHandle identifying the network manager call it corresponds to (if any)
     * @param channelGid The linkId the link this status pertains to
     * @param status The new LinkStatus
     * @param properties The LinkProperties of the link
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     *
     * @return tuple<bool, double> tuple containing whether the post was successful, and the
     * proportion of the queue utilized
     */
    virtual std::tuple<bool, double> onLinkStatusChanged(RaceHandle handle, LinkID linkId,
                                                         LinkStatus status,
                                                         LinkProperties properties,
                                                         int32_t timeout);

    /**
     * @brief Notify network manager via the SDK that the stauts of a channel has changed
     *
     * @param handle The RaceHandle identifying the network manager call it corresponds to (if any)
     * @param channelGid The name of the channel this status pertains to
     * @param status The new ChannelStatus
     * @param properties The ChannelProperties of the channel
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     *
     * @return tuple<bool, double> tuple containing whether the post was successful, and the
     * proportion of the queue utilized
     */
    virtual std::tuple<bool, double> onChannelStatusChanged(RaceHandle handle,
                                                            const std::string &channelGid,
                                                            ChannelStatus status,
                                                            const ChannelProperties &properties,
                                                            int32_t timeout);

    /**
     * @brief Notify network manager about a change to the LinkProperties of a Link
     *
     * @param linkId The LinkdID of the link that has been updated
     * @param linkProperties The LinkProperties that were updated
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     *
     * @return tuple<bool, double> tuple containing whether the post was successful, and the
     * proportion of the queue utilized
     */
    virtual std::tuple<bool, double> onLinkPropertiesChanged(LinkID linkId,
                                                             const LinkProperties &linkProperties,
                                                             int32_t timeout);

    /**
     * @brief Notify network manager about a change to the Links associated with a Persona
     *
     * @param recipientPersona The Persona that has changed link associations
     * @param linkType The LinkType of the links (send, recv, bidi)
     * @param links The list of links that are now associated with this persona
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     *
     * @return tuple<bool, double> tuple containing whether the post was successful, and the
     * proportion of the queue utilized
     */
    virtual std::tuple<bool, double> onPersonaLinksChanged(std::string recipientPersona,
                                                           LinkType linkType,
                                                           const std::vector<LinkID> &links,
                                                           int32_t timeout);

    /**
     * @brief Notify network manager about received user input response
     *
     * @param handle The handle for this callback
     * @param answered True if the response contains an actual answer to the input prompt, otherwise
     * the response is an empty string and not valid
     * @param response The user response answer to the input prompt
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     *
     * @return tuple<bool, double> tuple containing whether the post was successful, and the
     * proportion of the queue utilized
     */
    virtual std::tuple<bool, double> onUserInputReceived(RaceHandle handle, bool answered,
                                                         const std::string &response,
                                                         int32_t timeout);

    /**
     * @brief Notify the plugin that the user acknowledged the displayed information
     *
     * @param handle The handle for asynchronously returning the status of this call.
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     * @return tuple<bool, double> tuple containing whether the post was successful, and the
     * proportion of the queue utilized
     */
    virtual std::tuple<bool, double> onUserAcknowledgementReceived(RaceHandle handle,
                                                                   int32_t timeout);

    /**
     * @brief Notify network manager to perform epoch changeover processing
     *
     * @param data Data associated with epoch (network manager implementation specific)
     * @param timeout Timeout in milliseconds to block, 0 indicates a nonblocking call. The value
     * RACE_BLOCKING indicates the call will block forever. Other negative values are invalid
     * arguments
     *
     * @return tuple<bool, double> tuple containing whether the post was successful, and the
     * proportion of the queue utilized
     */
    std::tuple<bool, double> notifyEpoch(const std::string &data, int32_t timeout);

    /**
     * @brief Get the id of the wrapped IRacePluginNM
     *
     * @return const std::string & The id of the wrapped plugin
     */
    const std::string &getId() const {
        return mId;
    }

    /**
     * @brief Get the config path of the wrapped IRacePluginNM. The config path is the relative
     * location of the path containing configuration files to be used by the plugin.
     *
     * @return const std::string & The id of the wrapped plugin
     */
    const std::string &getConfigPath() const {
        return !mConfigPath.empty() ? mConfigPath : mId;
    }

    /**
     * @brief Get the description string of the wrapped IRacePluginNM
     *
     * @return const std::string & The description of the wrapped plugin
     */
    const std::string &getDescription() const {
        return mDescription;
    }

    /**
     * @brief Check if the wrapped network manager plugin is the test harness or a real network
     * manager plugin.
     *
     * @return True if the wrapped plugin is the test harness
     */
    virtual bool isTestHarness() const {
        return false;
    }

    // IRaceSdkCommon
    virtual RawData getEntropy(std::uint32_t numBytes) override;
    virtual std::string getActivePersona() override;
    virtual SdkResponse asyncError(RaceHandle handle, PluginResponse status) override;
    virtual SdkResponse makeDir(const std::string &directoryPath) override;
    virtual SdkResponse removeDir(const std::string &directoryPath) override;
    virtual std::vector<std::string> listDir(const std::string &directoryPath) override;
    virtual std::vector<std::uint8_t> readFile(const std::string &filepath) override;
    virtual SdkResponse appendFile(const std::string &filepath,
                                   const std::vector<std::uint8_t> &data) override;
    virtual SdkResponse writeFile(const std::string &filepath,
                                  const std::vector<std::uint8_t> &data) override;

    // IRaceSdkNM
    virtual SdkResponse sendEncryptedPackage(EncPkg ePkg, ConnectionID connectionId,
                                             uint64_t batchId, int32_t timeout) override;
    virtual SdkResponse presentCleartextMessage(ClrMsg msg) override;
    virtual SdkResponse onPluginStatusChanged(PluginStatus status) override;
    virtual SdkResponse openConnection(LinkType linkType, LinkID linkId, std::string linkHints,
                                       int32_t priority, int32_t sendTimeout,
                                       int32_t timeout) override;
    virtual SdkResponse closeConnection(ConnectionID connectionId, int32_t timeout) override;
    virtual std::vector<LinkID> getLinksForPersonas(std::vector<std::string> recipientPersonas,
                                                    LinkType linkType) override;
    virtual std::vector<LinkID> getLinksForChannel(std::string channelGid) override;
    virtual LinkID getLinkForConnection(ConnectionID connectionId) override;
    virtual LinkProperties getLinkProperties(LinkID linkId) override;
    virtual std::map<std::string, ChannelProperties> getSupportedChannels() override;
    virtual ChannelProperties getChannelProperties(std::string channelGid) override;
    virtual std::vector<ChannelProperties> getAllChannelProperties() override;
    virtual SdkResponse deactivateChannel(std::string channelGid, std::int32_t timeout) override;
    virtual SdkResponse activateChannel(std::string channelGid, std::string roleName,
                                        std::int32_t timeout) override;
    virtual SdkResponse destroyLink(LinkID linkId, std::int32_t timeout) override;
    virtual SdkResponse createLink(std::string channelGid, std::vector<std::string> personas,
                                   std::int32_t timeout) override;
    virtual SdkResponse loadLinkAddress(std::string channelGid, std::string linkAddress,
                                        std::vector<std::string> personas,
                                        std::int32_t timeout) override;
    virtual SdkResponse loadLinkAddresses(std::string channelGid,
                                          std::vector<std::string> linkAddresses,
                                          std::vector<std::string> personas,
                                          std::int32_t timeout) override;
    virtual SdkResponse createLinkFromAddress(std::string channelGid, std::string linkAddress,
                                              std::vector<std::string> personas,
                                              std::int32_t timeout) override;
    virtual SdkResponse bootstrapDevice(RaceHandle handle,
                                        std::vector<std::string> commsChannels) override;
    virtual SdkResponse bootstrapFailed(RaceHandle handle) override;
    virtual SdkResponse setPersonasForLink(std::string linkId,
                                           std::vector<std::string> personas) override;
    virtual std::vector<std::string> getPersonasForLink(std::string linkId) override;
    virtual SdkResponse onMessageStatusChanged(RaceHandle handle, MessageStatus status) override;
    virtual SdkResponse sendBootstrapPkg(ConnectionID connectionId, std::string persona,
                                         RawData key, int32_t timout) override;
    virtual SdkResponse requestPluginUserInput(const std::string &key, const std::string &prompt,
                                               bool cache) override;
    virtual SdkResponse requestCommonUserInput(const std::string &key) override;
    virtual SdkResponse flushChannel(ConnectionID connId, uint64_t batchId,
                                     std::int32_t timeout) override;
    virtual SdkResponse displayInfoToUser(const std::string &data,
                                          RaceEnums::UserDisplayType displayType) override;

protected:
    /* construct: called by the constructors to perform initializations logic
     *
     * Logs and creates the necessary queues.
     */
    void construct();

    /* createQueue: create a new queue on the handler thread
     *
     * Used to create different queues for callbacks vs received messages / packages
     */
    void createQueue(const std::string &name, int priority);

    /* removeQueue: stop the plugin thread
     *
     * Used to remove previously create queues. No queues are expected to be removed but method
     * added for completeness.
     */
    void removeQueue(const std::string &name);
};

#endif
