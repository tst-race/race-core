
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

#include <cstring>

#include "IRacePluginComms.h"
#include "libPluginGolang.h"

/**
 * Custom subclass of the IRacePluginComms interface to invoke Golang cgo-exported functions.
 *
 * We have to use this instead of using the swig director to create the C++ subclass because
 * of the way swig maps back to golang. The C++ swig director class maintains an int handle
 * assigned to it by the golang code in NewDirectorIRacePluginComms. When calls are made to
 * the director instance, it invokes a golang function, passing in this instance handle.
 * Once in golang, this handle is used to lookup the corresponding go struct and invoke
 * functions on it. Since this code is defined in the go shims source that is then compiled
 * into a plugin shared object, the instance handle assignment and lookup is defined in
 * the plugin library--and therefore defined multiple times when multiple golang plugins
 * are used. As a result, the different golang plugins are instantiated correctly, but
 * when the C++ swig director class is invoked, the lookup fails and all calls are directed
 * to the first _loaded_ plugin. Even if the generated swigDirectorLookup, etc. go functions
 * could be compiled into the SDK, it is unlikely it would work as each dynamic library
 * contains an isolated instance of the golang runtime.
 *
 * By implementing the "swig director" class manually and invoking our own golang functions,
 * we avoid this issue. It is very much a hack. And it is very error prone, as it requires
 * re-implementing some of the code that swig would have handled for us. But this allows
 * for multiple golang plugins to be used in RACE.
 */
class PluginCommsTwoSixGolang : public IRacePluginComms {
public:
    static GoString createGoString(const std::string &str) {
        GoString gostr;
        gostr.p = static_cast<char *>(malloc(str.length()));
        memcpy(const_cast<char *>(gostr.p), const_cast<char *>(str.data()), str.length());
        gostr.n = str.length();
        return gostr;
    }

    virtual PluginResponse init(const PluginConfig &pluginConfig) override {
        return static_cast<PluginResponse>(
            PluginCommsGolangInit(reinterpret_cast<GoUintptr>(&pluginConfig)));
    }

    virtual PluginResponse shutdown() override {
        return static_cast<PluginResponse>(PluginCommsGolangShutdown());
    }

    virtual PluginResponse sendPackage(RaceHandle handle, ConnectionID connectionId, EncPkg pkg,
                                       double timeoutTimestamp, uint64_t batchId) override {
        // Make a copy of the EncPkg to avoid a double-free
        EncPkg *encPkg = new EncPkg(static_cast<const EncPkg &>(pkg));
        return static_cast<PluginResponse>(PluginCommsGolangSendPackage(
            handle, createGoString(connectionId), reinterpret_cast<GoUintptr>(encPkg),
            timeoutTimestamp, batchId));
    }

    virtual PluginResponse openConnection(RaceHandle handle, LinkType linkType, LinkID linkId,
                                          std::string linkHints, int32_t sendTimeout) override {
        return static_cast<PluginResponse>(PluginCommsGolangOpenConnection(
            handle, linkType, createGoString(linkId), createGoString(linkHints), sendTimeout));
    }

    virtual PluginResponse closeConnection(RaceHandle handle, ConnectionID connectionId) override {
        return static_cast<PluginResponse>(
            PluginCommsGolangCloseConnection(handle, createGoString(connectionId)));
    }

    virtual PluginResponse destroyLink(RaceHandle handle, LinkID linkId) override {
        return static_cast<PluginResponse>(
            PluginCommsGolangDestroyLink(handle, createGoString(linkId)));
    }

    virtual PluginResponse createLink(RaceHandle handle, std::string channelGid) override {
        return static_cast<PluginResponse>(
            PluginCommsGolangCreateLink(handle, createGoString(channelGid)));
    }

    virtual PluginResponse createLinkFromAddress(RaceHandle handle, std::string channelGid,
                                                 std::string linkAddress) override {
        return static_cast<PluginResponse>(PluginCommsGolangCreateLinkFromAddress(
            handle, createGoString(channelGid), createGoString(linkAddress)));
    }

    virtual PluginResponse loadLinkAddress(RaceHandle handle, std::string channelGid,
                                           std::string linkAddress) override {
        return static_cast<PluginResponse>(PluginCommsGolangLoadLinkAddress(
            handle, createGoString(channelGid), createGoString(linkAddress)));
    }

    virtual PluginResponse loadLinkAddresses(RaceHandle handle, std::string channelGid,
                                             std::vector<std::string> linkAddresses) override {
        return static_cast<PluginResponse>(PluginCommsGolangLoadLinkAddresses(
            handle, createGoString(channelGid), reinterpret_cast<GoUintptr>(&linkAddresses)));
    }

    virtual PluginResponse deactivateChannel(RaceHandle handle, std::string channelGid) override {
        return static_cast<PluginResponse>(
            PluginCommsGolangDeactivateChannel(handle, createGoString(channelGid)));
    }

    virtual PluginResponse activateChannel(RaceHandle handle, std::string channelGid,
                                           std::string roleName) override {
        return static_cast<PluginResponse>(PluginCommsGolangActivateChannel(
            handle, createGoString(channelGid), createGoString(roleName)));
    }

    virtual PluginResponse onUserInputReceived(RaceHandle handle, bool answered,
                                               const std::string &response) override {
        return static_cast<PluginResponse>(
            PluginCommsGolangOnUserInputReceived(handle, answered, createGoString(response)));
    }

    virtual PluginResponse flushChannel(RaceHandle handle, std::string channelGid,
                                        uint64_t batchId) override {
        return static_cast<PluginResponse>(
            PluginCommsGolangFlushChannel(handle, createGoString(channelGid), batchId));
    }

    virtual PluginResponse onUserAcknowledgementReceived(RaceHandle handle) override {
        return static_cast<PluginResponse>(PluginCommsGolangOnUserAcknowledgementReceived(handle));
    }
};

/**
 * @brief Function for SDK to call to create the plugin. Wraps instance
 * initialization
 *
 * @param plugin Pointer to the SDK instance
 */
extern IRacePluginComms *createPluginComms(IRaceSdkComms *sdk) {
    CreatePluginCommsGolang(reinterpret_cast<GoUintptr>(sdk));
    return new PluginCommsTwoSixGolang;
}

/**
 * @brief Function for SDK to call to destroy the plugin. Wraps instance
 * destruction, closing connections, etc.
 *
 * @param plugin Pointer to the plugin instance
 */
extern void destroyPluginComms(IRacePluginComms * /*plugin*/) {
    DestroyPluginCommsGolang();
}

// Set values for loading the object. Used by SDK
const RaceVersionInfo raceVersion = RACE_VERSION;
const char *const racePluginId = "PluginCommsTwoSixGolang";
const char *const racePluginDescription =
    "Plugin Comms Golang Exemplar (Two Six Labs) " BUILD_VERSION;
