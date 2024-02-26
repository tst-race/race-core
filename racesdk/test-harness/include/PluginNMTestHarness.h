
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

#ifndef __SOURCE_PLUGIN_NETWORK_MANAGER_TEST_HARNESS_H__
#define __SOURCE_PLUGIN_NETWORK_MANAGER_TEST_HARNESS_H__

#include <string>
#include <tuple>
#include <unordered_map>

#include "IRacePluginNM.h"

class PluginNMTestHarness : public IRacePluginNM {
public:
    struct AddressedPkg {
        std::string dst;
        std::string channelGid;
        std::string linkId;
        EncPkg pkg;
    };

    explicit PluginNMTestHarness(IRaceSdkNM *sdk);
    virtual ~PluginNMTestHarness();

    virtual PluginResponse processNMBypassMsg(RaceHandle handle, const std::string &route,
                                              const ClrMsg &msg);
    virtual PluginResponse openRecvConnection(RaceHandle handle, const std::string &persona,
                                              const std::string &route);
    virtual PluginResponse rpcDeactivateChannel(const std::string &channelGid);
    virtual PluginResponse rpcDestroyLink(const std::string &linkId);
    virtual PluginResponse rpcCloseConnection(const std::string &connectionId);

    PluginResponse init(const PluginConfig &pluginConfig) override;
    PluginResponse shutdown() override;
    PluginResponse processClrMsg(RaceHandle handle, const ClrMsg &msg) override;
    PluginResponse processEncPkg(RaceHandle handle, const EncPkg &ePkg,
                                 const std::vector<std::string> &connIDs) override;

    PluginResponse prepareToBootstrap(RaceHandle handle, LinkID linkId, std::string configPath,
                                      DeviceInfo deviceInfo) override;

    PluginResponse onBootstrapPkgReceived(std::string persona, RawData pkg) override;
    PluginResponse onConnectionStatusChanged(RaceHandle handle, ConnectionID connId,
                                             ConnectionStatus status, LinkID linkId,
                                             LinkProperties properties) override;
    PluginResponse onLinkStatusChanged(RaceHandle handle, LinkID connId, LinkStatus status,
                                       LinkProperties properties) override;
    PluginResponse onChannelStatusChanged(RaceHandle handle, std::string channelGid,
                                          ChannelStatus status,
                                          ChannelProperties properties) override;
    PluginResponse onLinkPropertiesChanged(LinkID linkId, LinkProperties linkProperties) override;
    PluginResponse onPersonaLinksChanged(std::string recipientPersona, LinkType linkType,
                                         std::vector<LinkID> links) override;
    PluginResponse onPackageStatusChanged(RaceHandle handle, PackageStatus status) override;
    PluginResponse onUserInputReceived(RaceHandle handle, bool answered,
                                       const std::string &response) override;
    PluginResponse onUserAcknowledgementReceived(RaceHandle handle) override;
    PluginResponse notifyEpoch(const std::string &data) override;
    static std::string getDescription();

protected:
    static std::tuple<std::string, LinkID, ConnectionID> splitRoute(const std::string &route);
    LinkID getLinkForChannel(const std::string &persona, const std::string &channelGid,
                             LinkType linkType);
    PluginResponse openConnAndQueueToSend(const AddressedPkg &addrPkg);
    void logDebug(const std::string &message, const std::string &stackTrace = "");
    void logInfo(const std::string &message, const std::string &stackTrace = "");
    void logError(const std::string &message, const std::string &stackTrace = "");
    void logMessageOverhead(const ClrMsg &message, const EncPkg &package);

protected:
    IRaceSdkNM *raceSdk;
    std::string activePersona;
    std::unordered_map<RaceHandle, const AddressedPkg> sendMap;
    std::unordered_map<RaceHandle, LinkID> pendingRecvConns;
    std::unordered_map<ConnectionID, LinkID> recvConnIds;
};

#endif
