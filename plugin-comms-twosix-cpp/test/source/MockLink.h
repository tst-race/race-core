
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

#ifndef __MOCK_LINK_H_
#define __MOCK_LINK_H_

#include <utility>

#include "../../source/base/Link.h"
#include "gmock/gmock.h"

class MockLink : public Link {
public:
    static LinkProperties getDefaultLinkProperties(LinkType linkType) {
        LinkProperties props;

        props.linkType = linkType;
        props.transmissionType = TT_UNICAST;
        props.connectionType = CT_DIRECT;

        return props;
    }

    explicit MockLink(IRaceSdkComms *sdk, PluginCommsTwoSixCpp *plugin, Channel *channel,
                      const LinkID &linkId, LinkType linkType) :
        Link(sdk, plugin, channel, linkId, getDefaultLinkProperties(linkType), {}) {}

    ~MockLink() {
        shutdownLink();
    }

    MOCK_METHOD(std::shared_ptr<Connection>, openConnection,
                (LinkType linkType, const ConnectionID &connectionId, const std::string &linkHints,
                 int32_t sendTimeout),
                (override));

    MOCK_METHOD(void, closeConnection, (const ConnectionID &connectionId), (override));
    MOCK_METHOD(void, startConnection, (Connection *), (override));
    MOCK_METHOD(PluginResponse, sendPackage,
                (RaceHandle handle, const EncPkg &pkg, double timeoutTimestamp), (override));
    MOCK_METHOD(void, shutdown, (), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<Connection>>, getConnections, (), (override));
    MOCK_METHOD(std::string, getLinkAddress, (), (override));

protected:
    MOCK_METHOD(bool, sendPackageInternal, (RaceHandle handle, const EncPkg &pkg), (override));
};

#endif
