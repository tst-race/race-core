
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

#include "PluginCommsTwoSixStubTransport.h"

#include <chrono>
#include <nlohmann/json.hpp>

#include "JsonTypes.h"
#include "Link.h"
#include "LinkAddress.h"
#include "log.h"

namespace std {
static std::ostream &operator<<(std::ostream &out, const std::vector<RaceHandle> &handles) {
    return out << nlohmann::json(handles).dump();
}
}  // namespace std

LinkProperties createDefaultLinkProperties(const ChannelProperties &channelProperties) {
    LinkProperties linkProperties;

    linkProperties.linkType = LT_BIDI;
    linkProperties.transmissionType = channelProperties.transmissionType;
    linkProperties.connectionType = channelProperties.connectionType;
    linkProperties.sendType = channelProperties.sendType;
    linkProperties.reliable = channelProperties.reliable;
    linkProperties.isFlushable = channelProperties.isFlushable;
    linkProperties.duration_s = channelProperties.duration_s;
    linkProperties.period_s = channelProperties.period_s;
    linkProperties.mtu = channelProperties.mtu;

    LinkPropertySet worstLinkPropertySet;
    worstLinkPropertySet.bandwidth_bps = 277200;
    worstLinkPropertySet.latency_ms = 3190;
    worstLinkPropertySet.loss = 0.1;
    linkProperties.worst.send = worstLinkPropertySet;
    linkProperties.worst.receive = worstLinkPropertySet;

    linkProperties.expected = channelProperties.creatorExpected;

    LinkPropertySet bestLinkPropertySet;
    bestLinkPropertySet.bandwidth_bps = 338800;
    bestLinkPropertySet.latency_ms = 2610;
    bestLinkPropertySet.loss = 0.1;
    linkProperties.best.send = bestLinkPropertySet;
    linkProperties.best.receive = bestLinkPropertySet;

    linkProperties.supported_hints = channelProperties.supported_hints;
    linkProperties.channelGid = channelProperties.channelGid;

    return linkProperties;
}

PluginCommsTwoSixStubTransport::PluginCommsTwoSixStubTransport(ITransportSdk *sdk) :
    sdk(sdk),
    racePersona(sdk->getActivePersona()),
    channelProperties(sdk->getChannelProperties()),
    defaultLinkProperties(createDefaultLinkProperties(channelProperties)) {
    // No user input requests are needed, so transport is ready right away
    sdk->updateState(COMPONENT_STATE_STARTED);
}

ComponentStatus PluginCommsTwoSixStubTransport::onUserInputReceived(RaceHandle handle,
                                                                    bool answered,
                                                                    const std::string &response) {
    TRACE_METHOD(handle, answered, response);
    // We don't make any user input requests
    return COMPONENT_OK;
}

TransportProperties PluginCommsTwoSixStubTransport::getTransportProperties() {
    TRACE_METHOD();
    return {
        // supportedActions
        {
            {"post", {"*/*"}},
            {"fetch", {}},
        },
    };
}

LinkProperties PluginCommsTwoSixStubTransport::getLinkProperties(const LinkID &linkId) {
    TRACE_METHOD(linkId);
    return links.get(linkId)->getProperties();
}

bool PluginCommsTwoSixStubTransport::preLinkCreate(const std::string &logPrefix, RaceHandle handle,
                                                   const LinkID &linkId,
                                                   LinkSide invalideRoleLinkSide) {
    int numLinks = links.size();
    if (numLinks >= channelProperties.maxLinks) {
        logError(logPrefix + "preLinkCreate: Too many links. links: " + std::to_string(numLinks) +
                 ", maxLinks: " + std::to_string(channelProperties.maxLinks));
        sdk->onLinkStatusChanged(handle, linkId, LINK_DESTROYED, {});
        return false;
    }

    if (channelProperties.currentRole.linkSide == LS_UNDEF ||
        channelProperties.currentRole.linkSide == invalideRoleLinkSide) {
        logError(logPrefix + "preLinkCreate: Invalid role for this call. currentRole: '" +
                 channelProperties.currentRole.roleName +
                 "' linkSide: " + linkSideToString(channelProperties.currentRole.linkSide));
        sdk->onLinkStatusChanged(handle, linkId, LINK_DESTROYED, {});
        return false;
    }

    return true;
}

ComponentStatus PluginCommsTwoSixStubTransport::postLinkCreate(const std::string &logPrefix,
                                                               RaceHandle handle,
                                                               const LinkID &linkId,
                                                               const std::shared_ptr<Link> &link,
                                                               LinkStatus linkStatus) {
    if (link == nullptr) {
        logError(logPrefix + "postLinkCreate: link was null");
        sdk->onLinkStatusChanged(handle, linkId, LINK_DESTROYED, {});
        return COMPONENT_ERROR;
    }

    links.add(link);
    sdk->onLinkStatusChanged(handle, linkId, linkStatus, {});

    return COMPONENT_OK;
}

std::shared_ptr<Link> PluginCommsTwoSixStubTransport::createLinkInstance(
    const LinkID &linkId, const LinkAddress &address, const LinkProperties &properties) {
    auto link = std::make_shared<Link>(linkId, address, properties, sdk);
    link->start();
    return link;
}

ComponentStatus PluginCommsTwoSixStubTransport::createLink(RaceHandle handle,
                                                           const LinkID &linkId) {
    TRACE_METHOD(handle, linkId);
    if (not preLinkCreate(logPrefix, handle, linkId, LS_LOADER)) {
        return COMPONENT_ERROR;
    }

    LinkAddress address;
    address.hashtag = "cpp_" + racePersona + "_" + std::to_string(nextAvailableHashTag++);
    std::chrono::duration<double> sinceEpoch =
        std::chrono::high_resolution_clock::now().time_since_epoch();
    address.timestamp = sinceEpoch.count();

    LinkProperties properties = defaultLinkProperties;

    auto link = createLinkInstance(linkId, address, properties);

    return postLinkCreate(logPrefix, handle, linkId, link, LINK_CREATED);
}

ComponentStatus PluginCommsTwoSixStubTransport::loadLinkAddress(RaceHandle handle,
                                                                const LinkID &linkId,
                                                                const std::string &linkAddress) {
    TRACE_METHOD(handle, linkId, linkAddress);
    if (not preLinkCreate(logPrefix, handle, linkId, LS_CREATOR)) {
        return COMPONENT_OK;
    }

    LinkAddress address = nlohmann::json::parse(linkAddress);
    LinkProperties properties = defaultLinkProperties;
    auto link = createLinkInstance(linkId, address, properties);

    return postLinkCreate(logPrefix, handle, linkId, link, LINK_LOADED);
}

ComponentStatus PluginCommsTwoSixStubTransport::loadLinkAddresses(
    RaceHandle handle, const LinkID &linkId, const std::vector<std::string> & /* linkAddresses */) {
    TRACE_METHOD(handle, linkId);

    // We do not support multi-address loading
    sdk->onLinkStatusChanged(handle, linkId, LINK_DESTROYED, {});
    return COMPONENT_ERROR;
}

ComponentStatus PluginCommsTwoSixStubTransport::createLinkFromAddress(
    RaceHandle handle, const LinkID &linkId, const std::string &linkAddress) {
    TRACE_METHOD(handle, linkId, linkAddress);
    if (not preLinkCreate(logPrefix, handle, linkId, LS_LOADER)) {
        return COMPONENT_OK;
    }

    LinkAddress address = nlohmann::json::parse(linkAddress);
    LinkProperties properties = defaultLinkProperties;
    auto link = createLinkInstance(linkId, address, properties);

    return postLinkCreate(logPrefix, handle, linkId, link, LINK_CREATED);
}

ComponentStatus PluginCommsTwoSixStubTransport::destroyLink(RaceHandle handle,
                                                            const LinkID &linkId) {
    TRACE_METHOD(handle, linkId);

    auto link = links.remove(linkId);
    if (not link) {
        logError(logPrefix + "link with ID '" + linkId + "' does not exist");
        return COMPONENT_ERROR;
    }

    link->shutdown();

    return COMPONENT_OK;
}

std::vector<EncodingParameters> PluginCommsTwoSixStubTransport::getActionParams(
    const Action &action) {
    TRACE_METHOD(action.actionId, action.json);

    try {
        ActionJson actionParams = nlohmann::json::parse(action.json);
        switch (actionParams.type) {
            case ACTION_FETCH:
                return {};
            case ACTION_POST:
                return {{actionParams.linkId, "*/*", true, {}}};
            default:
                logError(logPrefix +
                         "Unrecognized action type: " + nlohmann::json(actionParams.type).dump());
                break;
        }
    } catch (nlohmann::json::exception &err) {
        logError(logPrefix + "Error in action JSON: " + err.what());
    }

    sdk->updateState(COMPONENT_STATE_FAILED);
    return {};
}

ComponentStatus PluginCommsTwoSixStubTransport::enqueueContent(
    const EncodingParameters &params, const Action &action, const std::vector<uint8_t> &content) {
    TRACE_METHOD(params.linkId, action.actionId, action.json, content.size());

    if (content.empty()) {
        logDebug(logPrefix + "Skipping enqueue content. Content size is 0.");
        return COMPONENT_OK;
    }

    try {
        ActionJson actionParams = nlohmann::json::parse(action.json);
        // If the action is for a wildcard link id, the component manager chooses the link and
        // specifies it when calling enqueueContent()
        actionToLinkIdMap[action.actionId] = params.linkId;
        switch (actionParams.type) {
            case ACTION_FETCH:
                // Nothing to be queued
                return COMPONENT_OK;

            case ACTION_POST:
                return links.get(params.linkId)->enqueueContent(action.actionId, content);

            default:
                logError(logPrefix +
                         "Unrecognized action type: " + nlohmann::json(actionParams.type).dump());
                break;
        }
    } catch (nlohmann::json::exception &err) {
        logError(logPrefix + "Error in action JSON: " + err.what());
    }

    return COMPONENT_ERROR;
}

ComponentStatus PluginCommsTwoSixStubTransport::dequeueContent(const Action &action) {
    TRACE_METHOD(action.actionId);

    try {
        ActionJson actionParams = nlohmann::json::parse(action.json);
        LinkID linkId = actionParams.linkId == "*" ? actionToLinkIdMap.at(action.actionId) :
                                                     actionParams.linkId;
        actionToLinkIdMap.erase(action.actionId);
        switch (actionParams.type) {
            case ACTION_POST:
                return links.get(linkId)->dequeueContent(action.actionId);

            default:
                // No content associated with any other action types
                return COMPONENT_OK;
        }
    } catch (std::exception &err) {
        logError(logPrefix + "Error: " + err.what());
    }

    return COMPONENT_ERROR;
}

ComponentStatus PluginCommsTwoSixStubTransport::doAction(const std::vector<RaceHandle> &handles,
                                                         const Action &action) {
    TRACE_METHOD(handles, action.actionId);

    try {
        ActionJson actionParams = nlohmann::json::parse(action.json);
        LinkID linkId = actionParams.linkId;

        switch (actionParams.type) {
            case ACTION_FETCH:
                // this map shouldn't contain anything in the fetch case, but just in case, erase it
                actionToLinkIdMap.erase(action.actionId);

                // This exemplar treats wildcard fetches as a fetch on EVERY link
                // Real transports which do NOT "fetch" for all links in a single action
                // (e.g. checking multiple subforums) may need to round-robin fetch for
                // a single link at a time
                if (actionParams.linkId == "*") {
                    ComponentStatus status = COMPONENT_OK;
                    logInfo(logPrefix + "Fetching from all links");
                    auto linkMap = links.getMap();
                    logInfo(logPrefix + "links: " + std::to_string(linkMap.size()));
                    for (auto &link : linkMap) {
                        logInfo(logPrefix + "Fetching from link " + link.first);
                        ComponentStatus thisStatus = link.second->fetch();
                        if (thisStatus == COMPONENT_FATAL) {
                            return COMPONENT_FATAL;
                        } else if (thisStatus != COMPONENT_OK) {
                            // propagate error status, but continue because it's not fatal
                            status = thisStatus;
                        }
                    }
                    return status;
                } else {
                    logInfo(logPrefix + "Fetching from single link");
                    return links.get(linkId)->fetch();
                }

            case ACTION_POST:
                if (linkId == "*") {
                    auto it = actionToLinkIdMap.find(action.actionId);
                    if (it == actionToLinkIdMap.end()) {
                        logInfo(logPrefix +
                                "Skipping action because no link exists for wildcard action");
                        return COMPONENT_OK;
                    } else {
                        linkId = it->second;
                    }
                }
                actionToLinkIdMap.erase(action.actionId);
                return links.get(linkId)->post(std::move(handles), action.actionId);

            default:
                logError(logPrefix +
                         "Unrecognized action type: " + nlohmann::json(actionParams.type).dump());
                break;
        }
    } catch (std::exception &err) {
        logError(logPrefix + "Error: " + err.what());
    }

    return COMPONENT_ERROR;
}

#ifndef TESTBUILD
ITransportComponent *createTransport(const std::string &transport, ITransportSdk *sdk,
                                     const std::string &roleName,
                                     const PluginConfig &pluginConfig) {
    TRACE_FUNCTION(transport, roleName, pluginConfig.pluginDirectory);
    return new PluginCommsTwoSixStubTransport(sdk);
}
void destroyTransport(ITransportComponent *component) {
    TRACE_FUNCTION();
    delete component;
}

const RaceVersionInfo raceVersion = RACE_VERSION;
#endif