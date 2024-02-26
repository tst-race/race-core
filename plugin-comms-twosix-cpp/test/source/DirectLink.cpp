
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

#include "../../source/direct/DirectLink.h"

#include <future>
#include <memory>

#include "MockLink.h"
#include "MockPluginComms.h"
#include "gtest/gtest.h"
#include "race/mocks/MockRaceSdkComms.h"

using ::testing::_;
using ::testing::Return;

// Create two direct links, and have one send to the other.
TEST(DirectLink, DISABLED_test_everything) {
    // The two links have separate mock sdks
    MockRaceSdkComms sdk1;
    // ON_CALL(sdk1, getActivePersona()).WillByDefault(::testing::Return("persona1"));
    MockRaceSdkComms sdk2;
    // ON_CALL(sdk1, getActivePersona()).WillByDefault(::testing::Return("persona2"));

    // the properties for the send and receive links
    LinkProperties sendProperties;
    sendProperties.linkType = LT_SEND;
    sendProperties.transmissionType = TT_UNICAST;
    sendProperties.connectionType = CT_DIRECT;

    LinkProperties recvProperties;
    recvProperties.linkType = LT_RECV;
    recvProperties.transmissionType = TT_UNICAST;
    recvProperties.connectionType = CT_DIRECT;

    DirectLinkProfileParser sendParser;
    sendParser.hostname = "localhost";
    sendParser.port = 12345;

    DirectLinkProfileParser receiveParser;
    receiveParser.hostname = "localhost";
    receiveParser.port = 12345;

    // Create the links. The receive link starts listening on port 12345. The send link will attempt
    // to send to that port.
    auto sendLink = std::make_shared<DirectLink>(
        &sdk1, static_cast<PluginCommsTwoSixCpp *>(nullptr), static_cast<Channel *>(nullptr),
        "LinkID0", sendProperties, sendParser);

    auto recvLink = std::make_shared<DirectLink>(
        &sdk2, static_cast<PluginCommsTwoSixCpp *>(nullptr), static_cast<Channel *>(nullptr),
        "LinkID1", recvProperties, receiveParser);

    // Create a connection on each link
    std::shared_ptr<Connection> connection;
    connection = sendLink->openConnection(LT_SEND, "ConnID0", {}, 1000);
    EXPECT_NE(connection, nullptr);

    connection = recvLink->openConnection(LT_RECV, "ConnID1", {}, 1000);
    EXPECT_NE(connection, nullptr);

    // send a package from one link to the other. Once the package is received, signal the main
    // thread.
    std::promise<void> promise;
    RaceHandle handle = 0;
    EncPkg pkg(1, 2, {0, 1, 2, 3});
    EXPECT_CALL(sdk1, onPackageStatusChanged(handle, PACKAGE_SENT, RACE_BLOCKING));
    EXPECT_CALL(sdk2, receiveEncPkg(pkg, std::vector<std::string>{"ConnID1"}, RACE_BLOCKING))
        .WillOnce([&promise](const EncPkg &, const std::vector<ConnectionID> &, int32_t) {
            promise.set_value();
            return SDK_OK;
        });

    auto reponse = sendLink->sendPackage(handle, pkg, std::numeric_limits<double>::infinity());
    EXPECT_EQ(reponse, PLUGIN_OK);

    // wait for the receiver to signal that it has received the package
    promise.get_future().wait();

    // close the connections
    sendLink->closeConnection("ConnID0");
    recvLink->closeConnection("ConnID1");

    // shutdown the links
    sendLink->shutdown();
    recvLink->shutdown();
}
