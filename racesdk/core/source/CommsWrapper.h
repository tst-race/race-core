
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

#ifndef __COMMS_WRAPPER_H__
#define __COMMS_WRAPPER_H__

#include <atomic>
#include <memory>

#include "../include/RaceSdk.h"
#include "Handler.h"
#include "IRacePluginComms.h"
#include "OpenTracingForwardDeclarations.h"

/*
 * CommsWrapper: A wrapper for a comms plugin that calls associated methods on a separate plugin
 * thread
 */
class CommsWrapper : public IRaceSdkComms {
private:
    RaceSdk &raceSdk;
    std::shared_ptr<opentracing::Tracer> mTracer;
    Handler mThreadHandler;
    // nextPostId is used to identify which post matches with which call/return log.
    std::atomic<uint64_t> nextPostId = 0;

    // used to prevent a race condition with multiple connections being opened / removed at once
    std::mutex mConnectionSendTimeoutMapMutex;

    // map a connection id to the send timeout specified when opening it
    // this is protected by mConnectionSendTimeoutMapMutex
    std::unordered_map<std::string, double> mConnectionSendTimeoutMap;

    // used to prevent a race condition with multiple connections being opened at once
    std::mutex mHandlePriorityTimeoutMapMutex;

    // map handle to the priority and send timout requested for the connection
    // this is protected by mHandlePriorityMapMutex
    std::unordered_map<RaceHandle, std::pair<int, double>> mHandlePriorityTimeoutMap;

    // indicates the current state of the plugin. Once shutdown, calls into the plugin will result
    // in an SDK_SHUTTING_DOWN response
    enum State { CONSTRUCTED, INITIALIZED, SHUTDOWN };
    std::atomic<State> state{CONSTRUCTED};

protected:
    // unique_ptr requires deleter type as template parameter
    std::shared_ptr<IRacePluginComms> mPlugin;
    std::string mId;
    std::string mDescription;
    std::string mConfigPath;

    CommsWrapper(RaceSdk &sdk, const std::string &name);

    using Interface = IRacePluginComms;
    using SDK = IRaceSdkComms;
    static constexpr const char *createFuncName = "createPluginComms";
    static constexpr const char *destroyFuncName = "destroyPluginComms";
    SDK *getSdk() {
        return this;
    }

public:
    CommsWrapper(std::shared_ptr<IRacePluginComms> plugin, std::string id, std::string description,
                 RaceSdk &sdk, const std::string &configPath = "");
    virtual ~CommsWrapper();

    const static std::int32_t WAIT_FOREVER = 0;

    /* startHandler: start the plugin thread
     *
     * This starts the internally managed thread on which methods of the wrapped plugin are run.
     * Calling a method that executes something on this thread before calling startHandler will
     * schedule the plugin method to be called once startHandler is called.
     */
    void startHandler();

    /* stopHandler: stop the plugin thread
     *
     * This stops the internally managed thread on which methods of the wrapped plugin are run. Any
     * callbacks posted, but not yet completed will be finished. Attempting to post a new callback
     * will fail.
     */
    void stopHandler();

    /* waitForCallbacks: wait for all callbacks to finish
     *
     * Creates a queue and callback with the minimum priority and waits for that to finish. Used for
     * testing.
     */
    void waitForCallbacks();

    /* init: call init on the wrapped plugin
     *
     * init will be run on the current thread instead of the plugin thread.
     */
    bool init(const PluginConfig &pluginConfig);

    /* shutdown: call shutdown on the wrapped plugin, timing out if it takes too long.
     *
     * shutdown will be called on the plugin thread. Shutdown may return before the shutdown method
     * of the wrapped plugin is complete.
     *
     * @return bool indicating whether or not the call was successful
     */
    bool shutdown();

    /**
     * @brief Call shutdown on the wrapped plugin, timing out if the call takes longer than the
     * specified timeout.
     *
     * @param timeoutInSeconds Timeout in seconds.
     *
     * @return tuple<bool, double> tuple containing whether the post was successful, and the
     * proportion of the queue utilized
     */
    bool shutdown(std::int32_t timeoutInSeconds);

    /* sendPackage: call sendPackage on the wrapped plugin
     *
     * sendPackage will be called on the plugin thread. sendPackage may return before the
     * sendPackage method of the wrapped plugin is complete.
     *
     * @return SdkResponse object that contains whether the post was successful, and the
     * proportion of the queue utilized, and the RaceHandle that callbacks are going to use to
     * respond
     */
    SdkResponse sendPackage(RaceHandle handle, const ConnectionID &connectionId, const EncPkg &pkg,
                            int32_t timeout, uint64_t batchId);

    /* openConnection: call openConnection on the wrapped plugin
     *
     * openConnection will be called on the plugin thread. openConnection will not return
     * until the openConnection method of the wrapped plugin is complete.
     *
     * @return SdkResponse object that contains whether the post was successful, and the
     * proportion of the queue utilized, and the RaceHandle that callbacks are going to use to
     * respond
     */
    SdkResponse openConnection(RaceHandle handle, LinkType linkType, const LinkID &linkId,
                               const std::string &linkHints, int32_t priority, int32_t sendTimeout,
                               int32_t timeout);

    /* closeConnection: call closeConnection on the wrapped plugin
     *
     * closeConnection will be called on the plugin thread. closeConnection may return before the
     * closeConnection method of the wrapped plugin is complete.
     *
     * @return SdkResponse object that contains whether the post was successful, and the
     * proportion of the queue utilized, and the RaceHandle that callbacks are going to use to
     * respond
     */
    SdkResponse closeConnection(RaceHandle handle, const ConnectionID &connectionId,
                                int32_t timeout);

    SdkResponse createLink(RaceHandle handle, const std::string &channelGid, std::int32_t timeout);
    SdkResponse createBootstrapLink(RaceHandle handle, const std::string &channelGid,
                                    const std::string &passphrase, std::int32_t timeout);
    SdkResponse loadLinkAddress(RaceHandle handle, const std::string &channelGid,
                                const std::string &linkAddress, std::int32_t timeout);
    SdkResponse loadLinkAddresses(RaceHandle handle, const std::string &channelGid,
                                  std::vector<std::string> linkAddresses, std::int32_t timeout);

    SdkResponse createLinkFromAddress(RaceHandle handle, const std::string &channelGid,
                                      const std::string &linkAddress, std::int32_t timeout);
    SdkResponse destroyLink(RaceHandle handle, const LinkID &linkId, std::int32_t timeout);
    SdkResponse deactivateChannel(RaceHandle handle, const std::string &channelGid,
                                  std::int32_t timeout);
    SdkResponse activateChannel(RaceHandle handle, const std::string &channelGid,
                                const std::string &roleName, std::int32_t timeout);

    SdkResponse serveFiles(LinkID linkId, std::string path, int32_t timeout);

    SdkResponse flushChannel(RaceHandle handle, std::string channelGid, uint64_t batchId,
                             int32_t timeout);

    /**
     * @brief Notify comms about received user input response
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
    std::tuple<bool, double> onUserInputReceived(RaceHandle handle, bool answered,
                                                 const std::string &response, int32_t timeout);

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
     * @brief Get the id of the wrapped IRacePluginComms
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
     * @brief Get the description string of the wrapped IRacePluginComms
     *
     * @return const std::string & The description of the wrapped plugin
     */
    const std::string &getDescription() const {
        return mDescription;
    }

    // Comms SDK wrapping

    // IRaceSdkCommon
    virtual RawData getEntropy(std::uint32_t numBytes) override;
    virtual std::string getActivePersona() override;
    virtual SdkResponse asyncError(RaceHandle handle, PluginResponse status) override;
    virtual ChannelProperties getChannelProperties(std::string channelGid) override;
    virtual std::vector<ChannelProperties> getAllChannelProperties() override;

    /**
     * @brief Create the directory of directoryPath, including any directories in the path that do
     * not yet exist
     * @param directoryPath the path of the directory to create.
     *
     * @return SdkResponse indicator of success or failure of the create
     */
    virtual SdkResponse makeDir(const std::string &directoryPath) override;

    /**
     * @brief Recurively remove the directory of directoryPath
     * @param directoryPath the path of the directory to remove.
     *
     * @return SdkResponse indicator of success or failure of the removal
     */
    virtual SdkResponse removeDir(const std::string &directoryPath) override;

    /**
     * @brief List the contents (directories and files) of the directory path
     * @param directoryPath the path of the directory to list.
     *
     * @return std::vector<std::string> list of directories and files
     */
    virtual std::vector<std::string> listDir(const std::string &directoryPath) override;

    /**
     * @brief Read the contents of a file in this plugin's storage.
     * @param filename The string name of the file to be read.
     *
     * @return std::vector<uint8_t> of the file contents
     * error occurs
     */
    virtual std::vector<std::uint8_t> readFile(const std::string &filename) override;

    /**
     * @brief Append the contents of data to filename in this plugin's storage.
     * @param filname The string name of the file to be appended to (or written).
     * @param data The string of data to append to the file.
     *
     * @return SdkResponse indicator of success or failure of the append.
     */
    virtual SdkResponse appendFile(const std::string &filename,
                                   const std::vector<std::uint8_t> &data) override;

    /**
     * @brief Write the contents of data to filename in this plugin's storage (overwriting if file
     * exists)
     * @param filname The string name of the file to be written.
     * @param data The string of data to write to the file.
     *
     * @return SdkResponse indicator of success or failure of the write.
     */
    virtual SdkResponse writeFile(const std::string &filename,
                                  const std::vector<std::uint8_t> &data) override;

    // IRaceSdkComms
    virtual SdkResponse onPackageStatusChanged(RaceHandle handle, PackageStatus status,
                                               int32_t timeout) override;
    virtual SdkResponse onConnectionStatusChanged(RaceHandle handle, ConnectionID connId,
                                                  ConnectionStatus status,
                                                  LinkProperties properties,
                                                  int32_t timeout) override;
    virtual SdkResponse onLinkStatusChanged(RaceHandle handle, LinkID linkId, LinkStatus status,
                                            LinkProperties properties, int32_t timeout) override;
    virtual SdkResponse onChannelStatusChanged(RaceHandle handle, std::string channelGid,
                                               ChannelStatus status, ChannelProperties properties,
                                               int32_t timeout) override;
    virtual SdkResponse updateLinkProperties(LinkID linkId, LinkProperties properties,
                                             int32_t timeout) override;
    virtual ConnectionID generateConnectionId(LinkID linkId) override;
    virtual LinkID generateLinkId(std::string channelGid) override;
    virtual SdkResponse receiveEncPkg(const EncPkg &pkg, const std::vector<ConnectionID> &connIDs,
                                      int32_t timeout) override;
    virtual SdkResponse requestPluginUserInput(const std::string &key, const std::string &prompt,
                                               bool cache) override;
    virtual SdkResponse requestCommonUserInput(const std::string &key) override;
    virtual SdkResponse displayInfoToUser(const std::string &data,
                                          RaceEnums::UserDisplayType displayType) override;
    virtual SdkResponse displayBootstrapInfoToUser(
        const std::string &data, RaceEnums::UserDisplayType displayType,
        RaceEnums::BootstrapActionType actionType) override;

    virtual SdkResponse unblockQueue(ConnectionID connId) override;

protected:
    /* createQueue: create a new queue on the handler thread
     *
     * Used to create a new queue for each connection. Post from sendPackage and closeConnection are
     * put on this new queue. These posts are fairly scheduled so one blocking connection does not
     * cause all connections to be blocked.
     */
    void createQueue(const std::string &name, int priority);

    /* removeQueue: stop the plugin thread
     *
     * Remove an existing queue. No more posts may happen on the removed queue. Existing posts are
     * still queued and will be completed.
     */
    void removeQueue(const std::string &name);

    /* makeResponse: make an SdkResponse from the provided arguments
     *
     * constructs an SdkResponse with the correct status and queue utilization for the provided
     * arguments. If a non OK status is returned, it will log a warning with the functionName and
     * status.
     */
    SdkResponse makeResponse(const std::string &functionName, bool success, size_t queueUtil,
                             RaceHandle handle);
};

#endif
