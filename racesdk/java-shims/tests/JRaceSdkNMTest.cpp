
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
#include <RaceLog.h>
#include <gmock/gmock.h>
#include <jni.h>

#include <exception>
#include <iostream>
#include <memory>

#include "../source/JavaIds.h"
#include "race/mocks/MockRaceSdkNM.h"

using ::testing::Return;

static void logException(JNIEnv *env, jthrowable &jThrowable) {
    if (jThrowable != nullptr) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        RaceLog::logError(
            "JRaceSdkNMTest",
            "Exception caught: " + JavaShimUtils::getMessageFromJthrowable(env, jThrowable), "");
    }
}

class JRaceSdkNMTest : public ::testing::Test {
public:
    JNIEnv *env;
    std::unique_ptr<MockRaceSdkNM> sdk;
    jobject jRaceSdkNM;
    jclass sdkClass;

    virtual void SetUp() override {
        JavaVM *jvm = JavaShimUtils::getJvm();
        JavaShimUtils::getEnv(&env, jvm);
        ASSERT_NE(nullptr, env);
        JavaIds::load(env);

        sdk = std::make_unique<MockRaceSdkNM>();

        // Load shims into java
        jclass libraryLoader = env->FindClass("com/twosix/race/StubLibraryLoader");
        ASSERT_NE(nullptr, libraryLoader);

        // Create Network Manager SDK instance
        sdkClass = env->FindClass("ShimsJava/JRaceSdkNM");
        ASSERT_NE(nullptr, sdkClass);

        jmethodID sdkConstructor = env->GetMethodID(sdkClass, "<init>", "(J)V");
        ASSERT_NE(nullptr, sdkConstructor);

        jlong sdkPtr = reinterpret_cast<jlong>(sdk.get());
        jRaceSdkNM = env->NewObject(sdkClass, sdkConstructor, sdkPtr);
        ASSERT_NE(nullptr, jRaceSdkNM);
    }

    virtual void TearDown() override {
        JavaIds::unload(env);
    }
};

TEST_F(JRaceSdkNMTest, testMakeDir) {
    std::string dirname = "dirname";
    EXPECT_CALL(*sdk, makeDir(dirname)).Times(1).WillOnce(Return(SDK_OK));

    jmethodID jMakeDir =
        env->GetMethodID(sdkClass, "makeDir", "(Ljava/lang/String;)LShimsJava/SdkResponse;");
    ASSERT_NE(jMakeDir, nullptr);

    jstring jDirname = env->NewStringUTF(dirname.c_str());
    jobject jResponse = env->CallObjectMethod(jRaceSdkNM, jMakeDir, jDirname);
    jthrowable jThrowable = env->ExceptionOccurred();
    logException(env, jThrowable);
    ASSERT_EQ(jThrowable, nullptr);

    EXPECT_EQ(JavaShimUtils::jobjectToSdkResponse(env, jResponse).status, SDK_OK);
}

TEST_F(JRaceSdkNMTest, testRemoveDir) {
    std::string dirname = "dirname";
    EXPECT_CALL(*sdk, removeDir(dirname)).Times(1).WillOnce(Return(SDK_OK));

    jmethodID jRemoveDir =
        env->GetMethodID(sdkClass, "removeDir", "(Ljava/lang/String;)LShimsJava/SdkResponse;");
    ASSERT_NE(jRemoveDir, nullptr);

    jstring jDirname = env->NewStringUTF(dirname.c_str());
    jobject jResponse = env->CallObjectMethod(jRaceSdkNM, jRemoveDir, jDirname);
    jthrowable jThrowable = env->ExceptionOccurred();
    logException(env, jThrowable);
    ASSERT_EQ(jThrowable, nullptr);

    EXPECT_EQ(JavaShimUtils::jobjectToSdkResponse(env, jResponse).status, SDK_OK);
}

TEST_F(JRaceSdkNMTest, testListDir) {
    std::string filename = "test";
    std::vector<std::string> data = {"test"};
    EXPECT_CALL(*sdk, listDir(filename)).Times(1).WillOnce(Return(data));

    jmethodID jListDir =
        env->GetMethodID(sdkClass, "listDir", "(Ljava/lang/String;)[Ljava/lang/String;");
    ASSERT_NE(jListDir, nullptr);

    jstring jFilename = env->NewStringUTF(filename.c_str());
    jobjectArray jData =
        static_cast<jobjectArray>(env->CallObjectMethod(jRaceSdkNM, jListDir, jFilename));
    jthrowable jThrowable = env->ExceptionOccurred();
    logException(env, jThrowable);
    ASSERT_EQ(jThrowable, nullptr);

    auto returnedData = JavaShimUtils::jArrayToStringVector(env, jData);
    EXPECT_EQ(returnedData, data);
}

TEST_F(JRaceSdkNMTest, testReadFile) {
    std::string filename = "example filename";
    RawData data = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8};
    EXPECT_CALL(*sdk, readFile(filename)).Times(1).WillOnce(Return(data));

    jmethodID jReadFile = env->GetMethodID(sdkClass, "readFile", "(Ljava/lang/String;)[B");
    ASSERT_NE(jReadFile, nullptr);

    jstring jFilename = env->NewStringUTF(filename.c_str());
    jbyteArray jData =
        static_cast<jbyteArray>(env->CallObjectMethod(jRaceSdkNM, jReadFile, jFilename));
    jthrowable jThrowable = env->ExceptionOccurred();
    logException(env, jThrowable);
    ASSERT_EQ(jThrowable, nullptr);

    auto returnedData = JavaShimUtils::jByteArrayToRawData(env, jData);
    EXPECT_EQ(returnedData, data);
}

TEST_F(JRaceSdkNMTest, testAppendFile) {
    std::string filename = "example filename";
    RawData data = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8};
    EXPECT_CALL(*sdk, appendFile(filename, data)).Times(1).WillOnce(Return(SDK_OK));

    jmethodID jAppendFile =
        env->GetMethodID(sdkClass, "appendFile", "(Ljava/lang/String;[B)LShimsJava/SdkResponse;");
    ASSERT_NE(jAppendFile, nullptr);

    jstring jFilename = env->NewStringUTF(filename.c_str());
    jbyteArray jData = JavaShimUtils::rawDataToJByteArray(env, data);
    jobject jResponse = env->CallObjectMethod(jRaceSdkNM, jAppendFile, jFilename, jData);
    jthrowable jThrowable = env->ExceptionOccurred();
    logException(env, jThrowable);
    ASSERT_EQ(jThrowable, nullptr);

    EXPECT_EQ(JavaShimUtils::jobjectToSdkResponse(env, jResponse).status, SDK_OK);
}

TEST_F(JRaceSdkNMTest, testWriteFile) {
    std::string filename = "example filename";
    RawData data = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8};
    EXPECT_CALL(*sdk, writeFile(filename, data)).Times(1).WillOnce(Return(SDK_OK));

    jmethodID jWriteFile =
        env->GetMethodID(sdkClass, "writeFile", "(Ljava/lang/String;[B)LShimsJava/SdkResponse;");
    ASSERT_NE(jWriteFile, nullptr);

    jstring jFilename = env->NewStringUTF(filename.c_str());
    jbyteArray jData = JavaShimUtils::rawDataToJByteArray(env, data);
    jobject jResponse = env->CallObjectMethod(jRaceSdkNM, jWriteFile, jFilename, jData);
    jthrowable jThrowable = env->ExceptionOccurred();
    logException(env, jThrowable);
    ASSERT_EQ(jThrowable, nullptr);

    EXPECT_EQ(JavaShimUtils::jobjectToSdkResponse(env, jResponse).status, SDK_OK);
}

TEST_F(JRaceSdkNMTest, bootstrapDevice) {
    RaceHandle handle = 4567;
    std::vector<std::string> plugins = {"ExampleComms-1", "ExampleComms-2"};
    EXPECT_CALL(*sdk, bootstrapDevice(handle, plugins)).Times(1).WillOnce(Return(SDK_OK));

    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jobjectArray jPlugins = JavaShimUtils::stringVectorToJArray(env, plugins);

    jmethodID jBootstrapDeviceMethodId =
        env->GetMethodID(sdkClass, "bootstrapDevice",
                         "(LShimsJava/RaceHandle;[Ljava/lang/String;)LShimsJava/SdkResponse;");
    ASSERT_NE(jBootstrapDeviceMethodId, nullptr);

    jobject jResponse =
        env->CallObjectMethod(jRaceSdkNM, jBootstrapDeviceMethodId, jHandle, jPlugins);
    jthrowable jThrowable = env->ExceptionOccurred();
    logException(env, jThrowable);
    ASSERT_EQ(jThrowable, nullptr);

    EXPECT_EQ(JavaShimUtils::jobjectToSdkResponse(env, jResponse).status, SDK_OK);
}

TEST_F(JRaceSdkNMTest, sendBootstrapPkg) {
    std::string connectionId = "connection id";
    std::string persona = "persona";
    RawData data = {12, 34, 56, 78, 90};
    int timeout = RACE_BLOCKING;
    EXPECT_CALL(*sdk, sendBootstrapPkg(connectionId, persona, data, timeout))
        .Times(1)
        .WillOnce(Return(SDK_OK));

    jmethodID jSendBootstrapPkgMethodId =
        env->GetMethodID(sdkClass, "sendBootstrapPkg",
                         "(Ljava/lang/String;Ljava/lang/String;[BI)LShimsJava/SdkResponse;");
    ASSERT_NE(jSendBootstrapPkgMethodId, nullptr);

    jstring jConnectionId = env->NewStringUTF(connectionId.c_str());
    jstring jPersona = env->NewStringUTF(persona.c_str());
    jbyteArray jData = JavaShimUtils::rawDataToJByteArray(env, data);
    jint jTimeout = static_cast<jint>(timeout);
    jobject jResponse = env->CallObjectMethod(jRaceSdkNM, jSendBootstrapPkgMethodId, jConnectionId,
                                              jPersona, jData, jTimeout);
    jthrowable jThrowable = env->ExceptionOccurred();
    logException(env, jThrowable);
    ASSERT_EQ(jThrowable, nullptr);

    EXPECT_EQ(JavaShimUtils::jobjectToSdkResponse(env, jResponse).status, SDK_OK);
}