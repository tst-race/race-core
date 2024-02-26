
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

#ifndef __RACE_PRINTERS_H_
#define __RACE_PRINTERS_H_

#include "../../source/CommsWrapper.h"
#include "../../source/NMWrapper.h"
#include "BootstrapManager.h"
#include "RaceSdk.h"
#include "nlohmann/json.hpp"

using nlohmann::json;

// valgrind complains about the built-in gtest generic object printer, so we need our own
[[maybe_unused]] static std::ostream &operator<<(std::ostream &os, const CommsWrapper &wrapper) {
    os << "CommsWrapper: " << wrapper.getId() << "\n";

    return os;
}

[[maybe_unused]] static std::ostream &operator<<(std::ostream &os,
                                                 const SdkResponse & /*response*/) {
    os << "<SdkResponse>"
       << "\n";
    return os;
}

[[maybe_unused]] static std::ostream &operator<<(std::ostream &os, const AppConfig & /*config*/) {
    os << "<AppConfig>"
       << "\n";
    return os;
}

[[maybe_unused]] static std::ostream &operator<<(std::ostream &os, const RaceConfig & /*config*/) {
    os << "<RaceConfig>"
       << "\n";
    return os;
}

[[maybe_unused]] static std::ostream &operator<<(std::ostream &os,
                                                 const ChannelProperties & /*props*/) {
    os << "<ChannelProperties>"
       << "\n";

    return os;
}

[[maybe_unused]] static std::ostream &operator<<(std::ostream &os,
                                                 const LinkProperties &properties) {
    os << "linkType: " << static_cast<unsigned int>(properties.linkType) << ", ";
    os << "transmissionType: " << static_cast<unsigned int>(properties.transmissionType) << ", ";
    os << "reliable: " << static_cast<unsigned int>(properties.reliable) << std::endl;

    return os;
}

[[maybe_unused]] static std::ostream &operator<<(std::ostream &os,
                                                 const PluginConfig &pluginConfig) {
    os << "{" << pluginConfig.etcDirectory << ", " << pluginConfig.loggingDirectory << ", "
       << pluginConfig.auxDataDirectory << ", " << pluginConfig.tmpDirectory << ", "
       << pluginConfig.pluginDirectory << "}" << std::endl;

    return os;
}

[[maybe_unused]] static std::ostream &operator<<(std::ostream &os, const DeviceInfo &deviceInfo) {
    os << "{" << deviceInfo.platform << ", " << deviceInfo.architecture << ", "
       << deviceInfo.nodeType << "}" << std::endl;

    return os;
}

[[maybe_unused]] static std::ostream &operator<<(std::ostream &os, const ClrMsg &clrMsg) {
    os << "ClrMsg{"
       << " msg: " << clrMsg.getMsg() << ", from: " << clrMsg.getFrom()
       << ", to: " << clrMsg.getTo() << ", timestamp: " << clrMsg.getTime()
       << ", nonce: " << clrMsg.getNonce()
       << ", ampIndex: " << static_cast<int>(clrMsg.getAmpIndex()) << " }" << std::endl;
    return os;
}

[[maybe_unused]] static std::ostream &operator<<(std::ostream &os, const EncPkg & /*pkg*/) {
    os << "<EncPkg>" << std::endl;
    return os;
}

[[maybe_unused]] static std::ostream &operator<<(std::ostream &os,
                                                 const BootstrapInfo &bootstrapInfo) {
    json obj;
    obj["deviceInfo"] = {{"platform", bootstrapInfo.deviceInfo.platform},
                         {"arch", bootstrapInfo.deviceInfo.architecture},
                         {"node_type", bootstrapInfo.deviceInfo.nodeType}};
    obj["state"] = bootstrapInfo.state.load();
    obj["prepareBootstrapHandle"] = bootstrapInfo.prepareBootstrapHandle;
    obj["createdLinkHandle"] = bootstrapInfo.createdLinkHandle;
    obj["connectionHandle"] = bootstrapInfo.connectionHandle;
    obj["passphrase"] = bootstrapInfo.passphrase;
    obj["bootstrapChannelId"] = bootstrapInfo.bootstrapChannelId;

    // Bootstrap path contains a timestamp that changes every run. This printer is used for log
    // expect which requires output to be deterministic. Commenting this out is a hack to get it
    // working for now.
    // obj["bootstrapPath"] = bootstrapInfo.bootstrapPath;

    obj["commsPlugins"] = bootstrapInfo.commsPlugins;
    obj["bootstrapLink"] = bootstrapInfo.bootstrapLink;
    obj["bootstrapConnection"] = bootstrapInfo.bootstrapConnection;

    os << "BootstrapInfo" << obj.dump(4) << std::endl;
    return os;
}

[[maybe_unused]] static std::ostream &operator<<(std::ostream &os, const NMWrapper &wrapper) {
    os << "<NMWrapper: " << wrapper.getId() << ">" << std::endl;
    return os;
}

#endif
