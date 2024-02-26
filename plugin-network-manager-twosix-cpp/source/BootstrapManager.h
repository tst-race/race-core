
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

#ifndef __BOOTSTRAP_MANAGER_H__
#define __BOOTSTRAP_MANAGER_H__

#include <IRacePluginNM.h>
#include <IRaceSdkNM.h>

#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "ExtClrMsg.h"

/*
Bootstrapping Messages

Bob:   Introducer node
Alice: new node being introduced
Com:   One of several committee members

Bob -> Com
    type:    link create request (created by Com, loaded by Alice)
    trigger: prepareToBootstrap

Com -> Bob
    type:    link create request response (created by Com, loaded by Alice)
    trigger: receive link create request

Bob -> Alice
    type:    configs (via bootstrapDevice)
    trigger: receive all link create request response, or timeout (maybe?)

Alice -> Bob
    type:    bootstrap package
    trigger: on first start after bootstrapping

Bob -> Com
    type:    add persona
    trigger: received bootstrap package

Alice -> Bob
    type:    link load request (created by Alice, loaded by Com)
    trigger: on first start after bootstrapping

Bob -> Com
    type:    link load request forward (created by Alice, loaded by Com)
    trigger: link load request

Alice -> Bob
    type:    terminate bootstrap link
    trigger: on first start after bootstrapping, after all messages have been
             sent over the bootstrap link
*/

class PluginNMTwoSix;

class BootstrapManager {
public:
    explicit BootstrapManager(PluginNMTwoSix *_plugin) :
        plugin(_plugin),
        messageCounter(1),
        bootstrapHandleGenerator(std::random_device()()),
        bootstrapHandle(0),
        bootstrapDestroyLinkPackageHandle(NULL_RACE_HANDLE) {}

    PluginResponse onPrepareToBootstrap(RaceHandle handle, LinkID linkId,
                                        const std::string &configPath,
                                        const std::vector<std::string> &entranceCommittee);
    PluginResponse onBootstrapFinished(RaceHandle bootstrapHandle, BootstrapState state);
    PluginResponse onBootstrapMessage(const ExtClrMsg &msg);
    PluginResponse onBootstrapPackage(const std::string &persona, const ExtClrMsg &msg,
                                      const std::vector<std::string> &entranceCommittee);
    PluginResponse onBootstrapStart(const std::string &introducer_node,
                                    const std::vector<std::string> &entranceCommittee,
                                    uint64_t bootstrapHandle);

    void onLinkStatusChanged(RaceHandle handle, LinkID linkId, LinkStatus status,
                             LinkProperties properties);
    PluginResponse onConnectionStatusChanged(RaceHandle /*handle*/, const ConnectionID &connId,
                                             ConnectionStatus status, const LinkID &linkId,
                                             const LinkProperties & /*properties*/);
    void onPackageStatusChanged(RaceHandle handle, PackageStatus status, RaceHandle resendHandle);

    bool isBootstrapConnection(const ConnectionID &connId);

public:
    enum BootstrapMessageType {
        UNDEFINED,  // Used for error when parsing
        LINK_CREATE_REQUEST,
        LINK_CREATE_RESPONSE,
        BOOTSTRAP_PACKAGE,
        ADD_PERSONA,
        LINK_LOAD_REQUEST,
        LINK_LOAD_REQUEST_FORWARD,
        DESTROY_LINK,
    };

    struct BootstrapMessage {
        BootstrapMessageType type;
        uint64_t bootstrapHandle = 0;
        uint64_t messageHandle = 0;

        std::vector<std::string> linkAddresses;
        std::vector<std::string> channelGids;

        std::string persona;
        std::string key;
    };

    struct LinkInfo {
        std::string address;
        std::string channel;
        std::string persona;
    };

    struct OutstandingBootstrap {
        RaceHandle sdkHandle;
        uint64_t bootstrapHandle = 0;
        std::string configPath;
        std::vector<uint64_t> outstandingHandles;
        std::vector<LinkInfo> receivedLinks;
        LinkID bootstrapLinkId;
        uint64_t outstandingOpenConnectionHandle;
        ConnectionID bootstrapConnectionId;
    };

    struct OutstandingCreateLink {
        std::vector<RaceHandle> outstandingHandles;
        std::string dest;

        BootstrapMessage message;
    };

    PluginResponse handleBootstrapConnectionOpened(const ConnectionID &connId);

    PluginResponse handleLinkCreateRequest(const BootstrapMessage &msg, const std::string &sender);
    PluginResponse handleLinkCreateResponse(const BootstrapMessage &msg, const std::string &sender);
    PluginResponse handleBootstrapPackage(const BootstrapMessage &msg,
                                          const std::vector<std::string> &entranceCommittee);
    PluginResponse handleAddPersona(const BootstrapMessage &msg, const std::string &sender);
    PluginResponse handleLinkLoadRequest(const BootstrapMessage &msg, const std::string &sender);
    PluginResponse handleLinkLoadRequestForward(const BootstrapMessage &msg,
                                                const std::string &sender);
    PluginResponse handleDestroyLink(const BootstrapMessage &msg, const std::string &sender);

    void destroyBootstrapLinkIfComplete();
    virtual RaceHandle sendBootstrapMsg(const BootstrapMessage &bMsg, const std::string &dest);
    virtual void sendBootstrapPkg(const BootstrapMessage &bMsg, const std::string &destination,
                                  const ConnectionID &connId);
    void writeConfigs(const OutstandingBootstrap &bootstrapInfo);
    BootstrapMessage parseMsg(const ExtClrMsg &msg);
    ExtClrMsg createClrMsg(const BootstrapMessage &bMsg, const std::string &dest);
    BootstrapMessageType bootstrapMessageTypeFromString(const std::string &typeString);
    std::string stringFromBootstrapMessageType(BootstrapMessageType type);

    // public for testing
public:
    PluginNMTwoSix *plugin;

    // create handles for sending messages
    uint64_t messageCounter;

    // creates per-bootstrap handles
    std::default_random_engine bootstrapHandleGenerator;

    // list of prepareToBootstrap calls that we've not got all the responses for
    std::vector<OutstandingBootstrap> outstandingBootstraps;

    // list of handleLinkCreateRequest calls that we've not got all the responses for
    std::vector<OutstandingCreateLink> outstandingCreateLinks;

    // list of links created by committee members, but which don't yet have the persona of the new
    // node associated with them
    std::unordered_map<uint64_t, std::vector<LinkID>> linksToUpdate;

    // If/while this node is being bootstrapped, these contain info about the bootstrapping
    ConnectionID bootstrapConnectionId;
    std::string bootstrapIntroducer;
    uint64_t bootstrapHandle;
    std::vector<std::string> bootstrapEntranceCommittee;
    RaceHandle bootstrapDestroyLinkPackageHandle;
};

#endif
