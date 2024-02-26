
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

#include <JavaShimUtils.h>
#include <gmock/gmock.h>
#include <jni.h>

#include <iostream>

#include "../source/JavaIds.h"

class javaShimsTestFixture : public ::testing::Test {
public:
    JNIEnv *env;

    virtual void SetUp() override {
        JavaVM *jvm = JavaShimUtils::getJvm();
        JavaShimUtils::getEnv(&env, jvm);
        ASSERT_NE(nullptr, env);
        JavaIds::load(env);
    }

    virtual void TearDown() override {
        JavaIds::unload(env);
    }
};

/**
 * @brief Test ClrMsg Conversion
 *
 */
TEST_F(javaShimsTestFixture, testClrMsg) {
    const std::string plainMsg = "plain msg";
    const std::string fromPersona = "from-persona";
    const std::string toPersona = "to-persona";
    const long createTime = 1000;
    const std::uint32_t nonce = 0;
    const long traceId = 1;
    const long spanId = 2;

    ClrMsg msg(plainMsg, fromPersona, toPersona, createTime, nonce, traceId, spanId);
    jobject jClrMsg = JavaShimUtils::clrMsg_to_jClrMsg(env, msg);
    ClrMsg msg2 = JavaShimUtils::jClrMsg_to_ClrMsg(env, jClrMsg);
    EXPECT_TRUE(msg == msg2);
}

TEST_F(javaShimsTestFixture, testEncPkg) {
    EncPkg origEncPkg(0x8877665544332211, 0x1122113311441155, {0x08, 0x67, 0x53, 0x09});
    origEncPkg.setPackageType(PKG_TYPE_NM);
    jobject jEncPkg = JavaShimUtils::encPkgToJobject(env, origEncPkg);
    EncPkg convertedEncPkg = JavaShimUtils::jobjectToEncPkg(env, jEncPkg);
    EXPECT_EQ(0x8877665544332211, convertedEncPkg.getTraceId());
    EXPECT_EQ(0x1122113311441155, convertedEncPkg.getSpanId());
    RawData expected = {0x08, 0x67, 0x53, 0x09};
    EXPECT_EQ(expected, convertedEncPkg.getCipherText());
    EXPECT_EQ(origEncPkg.getPackageType(), convertedEncPkg.getPackageType());
    EXPECT_EQ(convertedEncPkg.getPackageType(), PKG_TYPE_NM);
}

TEST_F(javaShimsTestFixture, testRaceHandle) {
    RaceHandle origHandle = 0x8877665544332211;
    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, origHandle);
    RaceHandle convertedHandle = JavaShimUtils::jobjectToRaceHandle(env, jHandle);
    EXPECT_EQ(0x8877665544332211, convertedHandle);
}

TEST_F(javaShimsTestFixture, testSdkResponse) {
    SdkResponse origResponse;
    origResponse.handle = 0x8877665544332211;
    origResponse.status = SDK_OK;
    origResponse.queueUtilization = 0.2;

    jobject jResponse = JavaShimUtils::sdkResponseToJobject(env, origResponse);
    SdkResponse convertedResponse = JavaShimUtils::jobjectToSdkResponse(env, jResponse);

    EXPECT_EQ(0x8877665544332211, convertedResponse.handle);
    EXPECT_NEAR(0.2, convertedResponse.queueUtilization, 0.000001);
    EXPECT_EQ(SDK_OK, convertedResponse.status);
}

TEST_F(javaShimsTestFixture, testPluginResponse) {
    PluginResponse origPluginResponse = PLUGIN_OK;
    jobject jPluginResponse = JavaShimUtils::pluginResponseToJobject(env, origPluginResponse);
    PluginResponse convertedPluginResponse =
        JavaShimUtils::jobjectToPluginResponse(env, jPluginResponse);
    EXPECT_EQ(PLUGIN_OK, convertedPluginResponse);
}

TEST_F(javaShimsTestFixture, testPackageStatus) {
    PackageStatus origPackageStatus = PACKAGE_RECEIVED;
    jobject jPackageStatus = JavaShimUtils::packageStatusToJobject(env, origPackageStatus);
    PackageStatus convertedPackageStatus =
        JavaShimUtils::jobjectToPackageStatus(env, jPackageStatus);
    EXPECT_EQ(PACKAGE_RECEIVED, convertedPackageStatus);
}

TEST_F(javaShimsTestFixture, testConnectionStatus) {
    ConnectionStatus origConnStatus = CONNECTION_OPEN;
    jobject jConnStatus = JavaShimUtils::connectionStatusToJobject(env, origConnStatus);
    ConnectionStatus convertedConnStatus =
        JavaShimUtils::jobjectToConnectionStatus(env, jConnStatus);
    EXPECT_EQ(CONNECTION_OPEN, convertedConnStatus);
}

TEST_F(javaShimsTestFixture, testLinkType) {
    LinkType origLinkType = LT_BIDI;
    jobject jLinkType = JavaShimUtils::linkTypeToJLinkType(env, origLinkType);
    LinkType convertedLinkType = JavaShimUtils::jobjectToLinkType(env, jLinkType);
    EXPECT_EQ(LT_BIDI, convertedLinkType);
}

TEST_F(javaShimsTestFixture, testConnectionTypeDirect) {
    ConnectionType origConnectionType = CT_DIRECT;
    jobject jConnectionType =
        JavaShimUtils::connectionTypeToJConnectionType(env, origConnectionType);
    ConnectionType convertedConnectionType =
        JavaShimUtils::jobjectToConnectionType(env, jConnectionType);
    EXPECT_EQ(CT_DIRECT, convertedConnectionType);
}

TEST_F(javaShimsTestFixture, testConnectionTypeIndirect) {
    ConnectionType origConnectionType = CT_INDIRECT;
    jobject jConnectionType =
        JavaShimUtils::connectionTypeToJConnectionType(env, origConnectionType);
    ConnectionType convertedConnectionType =
        JavaShimUtils::jobjectToConnectionType(env, jConnectionType);
    EXPECT_EQ(CT_INDIRECT, convertedConnectionType);
}

TEST_F(javaShimsTestFixture, testConnectionTypeMixed) {
    ConnectionType origConnectionType = CT_MIXED;
    jobject jConnectionType =
        JavaShimUtils::connectionTypeToJConnectionType(env, origConnectionType);
    ConnectionType convertedConnectionType =
        JavaShimUtils::jobjectToConnectionType(env, jConnectionType);
    EXPECT_EQ(CT_MIXED, convertedConnectionType);
}

TEST_F(javaShimsTestFixture, testConnectionTypeLocal) {
    ConnectionType origConnectionType = CT_LOCAL;
    jobject jConnectionType =
        JavaShimUtils::connectionTypeToJConnectionType(env, origConnectionType);
    ConnectionType convertedConnectionType =
        JavaShimUtils::jobjectToConnectionType(env, jConnectionType);
    EXPECT_EQ(CT_LOCAL, convertedConnectionType);
}

TEST_F(javaShimsTestFixture, testLinkProperties) {
    LinkPropertySet origLinkPropertySetSend;
    origLinkPropertySetSend.bandwidth_bps = 5;
    origLinkPropertySetSend.latency_ms = 8;
    origLinkPropertySetSend.loss = 29.4;

    LinkPropertySet origLinkPropertySetReceive;
    origLinkPropertySetReceive.bandwidth_bps = 7;
    origLinkPropertySetReceive.latency_ms = 11;
    origLinkPropertySetReceive.loss = 15.2;

    LinkPropertyPair origLinkPropertyPair;
    origLinkPropertyPair.send = origLinkPropertySetSend;
    origLinkPropertyPair.receive = origLinkPropertySetReceive;

    LinkProperties origLinkProperties;
    origLinkProperties.linkType = LT_RECV;
    origLinkProperties.connectionType = CT_DIRECT;
    origLinkProperties.transmissionType = TT_UNICAST;
    origLinkProperties.reliable = true;
    origLinkProperties.duration_s = 13;
    origLinkProperties.period_s = 3;
    origLinkProperties.mtu = 1800;
    origLinkProperties.worst = origLinkPropertyPair;
    origLinkProperties.expected = origLinkPropertyPair;
    origLinkProperties.best = origLinkPropertyPair;
    origLinkProperties.supported_hints = {"hint1", "hint2", "hint3"};
    origLinkProperties.linkAddress = "myLinkAddress";
    origLinkProperties.channelGid = "myChannelGid";

    // Convert to Java...
    jobject jLinkProperties = JavaShimUtils::linkPropertiesToJobject(env, origLinkProperties);
    // ...and back
    LinkProperties convertedLinkProperties =
        JavaShimUtils::jLinkPropertiesToLinkProperties(env, jLinkProperties);

    EXPECT_TRUE(convertedLinkProperties.reliable);
    EXPECT_EQ(13, convertedLinkProperties.duration_s);
    EXPECT_EQ(3, convertedLinkProperties.period_s);
    EXPECT_EQ(1800, convertedLinkProperties.mtu);
    EXPECT_EQ(LT_RECV, convertedLinkProperties.linkType);
    EXPECT_EQ(CT_DIRECT, convertedLinkProperties.connectionType);
    EXPECT_EQ(TT_UNICAST, convertedLinkProperties.transmissionType);
    ASSERT_EQ(3, convertedLinkProperties.supported_hints.size());
    EXPECT_EQ("hint1", convertedLinkProperties.supported_hints.at(0));
    EXPECT_EQ("hint2", convertedLinkProperties.supported_hints.at(1));
    EXPECT_EQ("hint3", convertedLinkProperties.supported_hints.at(2));
    EXPECT_EQ(5, convertedLinkProperties.worst.send.bandwidth_bps);
    EXPECT_EQ(8, convertedLinkProperties.worst.send.latency_ms);
    EXPECT_FLOAT_EQ(29.4, convertedLinkProperties.worst.send.loss);
    EXPECT_EQ(7, convertedLinkProperties.best.receive.bandwidth_bps);
    EXPECT_EQ(11, convertedLinkProperties.best.receive.latency_ms);
    EXPECT_FLOAT_EQ(15.2, convertedLinkProperties.best.receive.loss);
    EXPECT_EQ(convertedLinkProperties.linkAddress, "myLinkAddress");
    EXPECT_EQ(convertedLinkProperties.channelGid, "myChannelGid");
}

/**
 * @brief Test LinkPropertySet Conversion
 *
 */
TEST_F(javaShimsTestFixture, testLinkPropertySet) {
    /**
     *  LinkPropertySet
     **/
    jclass jLinkPropertySetClass = env->FindClass("ShimsJava/LinkPropertySet");
    jmethodID jLinkPropertySetConstructor =
        env->GetMethodID(jLinkPropertySetClass, "<init>", "(IIF)V");

    // LinkPropertySet 1
    jint expectedBandwidth = static_cast<jint>(5);
    jint expectedLatency = static_cast<jint>(8);
    jfloat expectedLoss = static_cast<jfloat>(12);
    jobject jLinkPropertySet = env->NewObject(jLinkPropertySetClass, jLinkPropertySetConstructor,
                                              expectedBandwidth, expectedLatency, expectedLoss);
    // LinkPropertySet 2
    jint expectedBandwidth2 = static_cast<jint>(7);
    jint expectedLatency2 = static_cast<jint>(11);
    jfloat expectedLoss2 = static_cast<jfloat>(14);
    jobject jLinkPropertySet2 = env->NewObject(jLinkPropertySetClass, jLinkPropertySetConstructor,
                                               expectedBandwidth2, expectedLatency2, expectedLoss2);

    /**
     *  LinkPropertyPair
     **/
    jclass jLinkPropertyPairClass = env->FindClass("ShimsJava/LinkPropertyPair");
    jmethodID jLinkPropertyPairConstructor =
        env->GetMethodID(jLinkPropertyPairClass, "<init>",
                         "(LShimsJava/LinkPropertySet;LShimsJava/LinkPropertySet;)V");
    // LinkPropertyPair 1
    jobject jLinkPropertyPair = env->NewObject(jLinkPropertyPairClass, jLinkPropertyPairConstructor,
                                               jLinkPropertySet, jLinkPropertySet2);

    /**
     *  LinkProperties
     **/
    std::vector<std::string> expectedHints = {"hint1", "hint2", "hint3"};
    jobjectArray jHints = JavaShimUtils::stringVectorToJArray(env, expectedHints);
    jclass jLinkPropertiesClass = env->FindClass("ShimsJava/JLinkProperties");
    jmethodID jLinkPropertiesConstructor = env->GetMethodID(
        jLinkPropertiesClass, "<init>",
        "(LShimsJava/LinkPropertyPair;LShimsJava/LinkPropertyPair;LShimsJava/"
        "LinkPropertyPair;ZLjava/lang/String;Ljava/lang/String;[Ljava/lang/String;)V");
    bool expectedReliable = true;
    jstring jChannelGid = env->NewStringUTF("expected-channel-gid");
    jstring jLinkAddress = env->NewStringUTF("expected-link-address");
    jobject jLinkProperties =
        env->NewObject(jLinkPropertiesClass, jLinkPropertiesConstructor, jLinkPropertyPair,
                       jLinkPropertyPair, jLinkPropertyPair,
                       static_cast<jboolean>(expectedReliable), jChannelGid, jLinkAddress, jHints);

    // Convert back to C++
    LinkProperties convertedLinkProperties =
        JavaShimUtils::jLinkPropertiesToLinkProperties(env, jLinkProperties);

    // LinkProperties specific fields
    EXPECT_TRUE(convertedLinkProperties.reliable == expectedReliable);
    EXPECT_TRUE(convertedLinkProperties.supported_hints.at(0).compare(expectedHints.at(0)) == 0);
    ASSERT_EQ(convertedLinkProperties.supported_hints.size(), expectedHints.size());
    for (size_t hintIndex = 0; hintIndex < expectedHints.size(); ++hintIndex) {
        EXPECT_EQ(convertedLinkProperties.supported_hints[hintIndex], expectedHints[hintIndex]);
    }

    // LinkPropertyPair Fields
    EXPECT_TRUE(convertedLinkProperties.worst.send.bandwidth_bps ==
                static_cast<int>(expectedBandwidth));
    EXPECT_TRUE(convertedLinkProperties.worst.send.latency_ms == static_cast<int>(expectedLatency));
    EXPECT_TRUE(convertedLinkProperties.best.receive.bandwidth_bps ==
                static_cast<int>(expectedBandwidth2));
    EXPECT_TRUE(convertedLinkProperties.best.receive.latency_ms ==
                static_cast<int>(expectedLatency2));
}

TEST_F(javaShimsTestFixture, testChannelRole) {
    ChannelRole origRole;
    origRole.roleName = "role-name";
    origRole.mechanicalTags = {"tag1", "tag2", "tag3"};
    origRole.behavioralTags = {"tag4", "tag5"};
    origRole.linkSide = LS_BOTH;

    jobject jChannelRole = JavaShimUtils::channelRoleToJobject(env, origRole);
    ChannelRole convertedRole = JavaShimUtils::jChannelRoleToChannelRole(env, jChannelRole);

    EXPECT_EQ(origRole.roleName, convertedRole.roleName);
    EXPECT_EQ(origRole.mechanicalTags, convertedRole.mechanicalTags);
    EXPECT_EQ(origRole.behavioralTags, convertedRole.behavioralTags);
    EXPECT_EQ(origRole.linkSide, convertedRole.linkSide);
}

TEST_F(javaShimsTestFixture, testChannelProperties) {
    LinkPropertySet origLinkPropertySetSend;
    origLinkPropertySetSend.bandwidth_bps = 5;
    origLinkPropertySetSend.latency_ms = 8;
    origLinkPropertySetSend.loss = 29.4;

    ChannelRole origRole1;
    origRole1.roleName = "role-name";
    origRole1.mechanicalTags = {"tag1", "tag2", "tag3"};
    origRole1.behavioralTags = {"tag4", "tag5"};
    origRole1.linkSide = LS_CREATOR;

    ChannelRole origRole2;
    origRole2.roleName = "role-name";
    origRole2.mechanicalTags = {"tag6", "tag7", "tag8"};
    origRole2.behavioralTags = {"tag9", "tag10"};
    origRole2.linkSide = LS_LOADER;

    LinkPropertySet origLinkPropertySetReceive;
    origLinkPropertySetReceive.bandwidth_bps = 7;
    origLinkPropertySetReceive.latency_ms = 11;
    origLinkPropertySetReceive.loss = 15.2;

    LinkPropertyPair origLinkPropertyPair;
    origLinkPropertyPair.send = origLinkPropertySetSend;
    origLinkPropertyPair.receive = origLinkPropertySetReceive;

    ChannelProperties origChannelProperties;
    origChannelProperties.channelStatus = CHANNEL_ENABLED;
    origChannelProperties.linkDirection = LD_BIDI;
    origChannelProperties.transmissionType = TT_UNICAST;
    origChannelProperties.connectionType = CT_DIRECT;
    origChannelProperties.sendType = ST_STORED_ASYNC;
    origChannelProperties.multiAddressable = true;
    origChannelProperties.reliable = true;
    origChannelProperties.bootstrap = true;
    origChannelProperties.isFlushable = true;
    origChannelProperties.duration_s = 13;
    origChannelProperties.period_s = 3;
    origChannelProperties.mtu = 1800;
    origChannelProperties.creatorExpected = origLinkPropertyPair;
    origChannelProperties.loaderExpected = origLinkPropertyPair;
    origChannelProperties.supported_hints = {"hint1", "hint2", "hint3"};
    origChannelProperties.roles = {origRole1, origRole2};
    origChannelProperties.currentRole = origRole1;
    origChannelProperties.maxSendsPerInterval = 42;
    origChannelProperties.secondsPerInterval = 3600;
    origChannelProperties.intervalEndTime = 8675309;
    origChannelProperties.sendsRemainingInInterval = 7;
    origChannelProperties.channelGid = "myChannelGid";

    // Convert to Java...
    jobject jChannelProperties =
        JavaShimUtils::channelPropertiesToJobject(env, origChannelProperties);
    // ...and back
    ChannelProperties convertedChannelProperties =
        JavaShimUtils::jChannelPropertiesToChannelProperties(env, jChannelProperties);

    EXPECT_EQ(CHANNEL_ENABLED, convertedChannelProperties.channelStatus);
    EXPECT_EQ(LD_BIDI, origChannelProperties.linkDirection);
    EXPECT_EQ(TT_UNICAST, origChannelProperties.transmissionType);
    EXPECT_EQ(CT_DIRECT, origChannelProperties.connectionType);
    EXPECT_EQ(ST_STORED_ASYNC, origChannelProperties.sendType);
    EXPECT_TRUE(convertedChannelProperties.multiAddressable);
    EXPECT_TRUE(convertedChannelProperties.reliable);
    EXPECT_TRUE(convertedChannelProperties.bootstrap);
    EXPECT_TRUE(convertedChannelProperties.isFlushable);
    EXPECT_EQ(13, convertedChannelProperties.duration_s);
    EXPECT_EQ(3, convertedChannelProperties.period_s);
    EXPECT_EQ(1800, convertedChannelProperties.mtu);
    EXPECT_EQ(5, convertedChannelProperties.creatorExpected.send.bandwidth_bps);
    EXPECT_EQ(8, convertedChannelProperties.creatorExpected.send.latency_ms);
    EXPECT_FLOAT_EQ(29.4, convertedChannelProperties.creatorExpected.send.loss);
    EXPECT_EQ(7, convertedChannelProperties.creatorExpected.receive.bandwidth_bps);
    EXPECT_EQ(11, convertedChannelProperties.creatorExpected.receive.latency_ms);
    EXPECT_FLOAT_EQ(15.2, convertedChannelProperties.creatorExpected.receive.loss);
    EXPECT_EQ(5, convertedChannelProperties.loaderExpected.send.bandwidth_bps);
    EXPECT_EQ(8, convertedChannelProperties.loaderExpected.send.latency_ms);
    EXPECT_FLOAT_EQ(29.4, convertedChannelProperties.loaderExpected.send.loss);
    EXPECT_EQ(7, convertedChannelProperties.loaderExpected.receive.bandwidth_bps);
    EXPECT_EQ(11, convertedChannelProperties.loaderExpected.receive.latency_ms);
    EXPECT_FLOAT_EQ(15.2, convertedChannelProperties.loaderExpected.receive.loss);
    ASSERT_EQ(3, convertedChannelProperties.supported_hints.size());
    EXPECT_EQ("hint1", convertedChannelProperties.supported_hints.at(0));
    EXPECT_EQ("hint2", convertedChannelProperties.supported_hints.at(1));
    EXPECT_EQ("hint3", convertedChannelProperties.supported_hints.at(2));
    EXPECT_EQ(origRole1.roleName, convertedChannelProperties.currentRole.roleName);
    EXPECT_EQ(origRole1.mechanicalTags, convertedChannelProperties.currentRole.mechanicalTags);
    EXPECT_EQ(origRole1.behavioralTags, convertedChannelProperties.currentRole.behavioralTags);
    EXPECT_EQ(origRole1.linkSide, convertedChannelProperties.currentRole.linkSide);
    ASSERT_EQ(2, convertedChannelProperties.roles.size());
    EXPECT_EQ(origRole1.roleName, convertedChannelProperties.roles[0].roleName);
    EXPECT_EQ(origRole1.mechanicalTags, convertedChannelProperties.roles[0].mechanicalTags);
    EXPECT_EQ(origRole1.behavioralTags, convertedChannelProperties.roles[0].behavioralTags);
    EXPECT_EQ(origRole1.linkSide, convertedChannelProperties.roles[0].linkSide);
    EXPECT_EQ(origRole2.roleName, convertedChannelProperties.roles[1].roleName);
    EXPECT_EQ(origRole2.mechanicalTags, convertedChannelProperties.roles[1].mechanicalTags);
    EXPECT_EQ(origRole2.behavioralTags, convertedChannelProperties.roles[1].behavioralTags);
    EXPECT_EQ(origRole2.linkSide, convertedChannelProperties.roles[1].linkSide);
    EXPECT_EQ(42, convertedChannelProperties.maxSendsPerInterval);
    EXPECT_EQ(3600, convertedChannelProperties.secondsPerInterval);
    EXPECT_EQ(8675309, convertedChannelProperties.intervalEndTime);
    EXPECT_EQ(7, convertedChannelProperties.sendsRemainingInInterval);
    EXPECT_EQ("myChannelGid", convertedChannelProperties.channelGid);
}
