
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

#include "RaceAppWrapper.h"

#include <OpenTracingHelpers.h>  // createTracer
#include <RaceLog.h>
#include <dlfcn.h>

#include <iostream>

#include "JavaIds.h"
#include "JavaShimUtils.h"

#define TRACE_METHOD(...) TRACE_METHOD_BASE(RaceAppJavaWrapper, ##__VA_ARGS__)
static const std::string logLabel = "RaceAppJavaWrapper";

RaceAppWrapper::RaceAppWrapper(RaceTestAppOutputLog &output, IRaceSdkApp &raceSdk,
                               std::shared_ptr<opentracing::Tracer> tracer, JNIEnv *env,
                               jclass jRaceAppClassIn, jobject jRaceAppIn) :
    RaceApp(output, raceSdk, tracer),
    jRaceAppClass(reinterpret_cast<jclass>(env->NewGlobalRef(jRaceAppClassIn))),
    jRaceApp(reinterpret_cast<jobject>(env->NewGlobalRef(jRaceAppIn))),
    jHandleReceivedMessageMethodId(JavaIds::getMethodID(env, jRaceAppClass, "handleReceivedMessage",
                                                        "(LShimsJava/JClrMsg;)V")),
    jAddMessageToUIMethodId(
        JavaIds::getMethodID(env, jRaceAppClass, "addMessageToUI", "(LShimsJava/JClrMsg;)V")),
    jOnMessageStatusChangedMethodId(
        JavaIds::getMethodID(env, jRaceAppClass, "onMessageStatusChanged",
                             "(LShimsJava/RaceHandle;LShimsJava/MessageStatus;)V")),
    jOnSdkStatusChangedMethodId(
        JavaIds::getMethodID(env, jRaceAppClass, "onSdkStatusChanged", "(Ljava/lang/String;)V")),
    jDisplayInfoToUserMethodId(
        JavaIds::getMethodID(env, jRaceAppClass, "displayInfoToUser",
                             "(LShimsJava/RaceHandle;Ljava/lang/String;LShimsJava/"
                             "UserDisplayType;)LShimsJava/SdkResponse;")),
    jDisplayBootstrapInfoToUserMethodId(JavaIds::getMethodID(
        env, jRaceAppClass, "displayBootstrapInfoToUser",
        "(LShimsJava/RaceHandle;Ljava/lang/String;LShimsJava/"
        "UserDisplayType;LShimsJava/BootstrapActionType;)LShimsJava/SdkResponse;")),
    jRequestUserInputMethodId(JavaIds::getMethodID(
        env, jRaceAppClass, "requestUserInput",
        "(LShimsJava/RaceHandle;Ljava/lang/String;Ljava/lang/String;Ljava/lang/"
        "String;Z)LShimsJava/SdkResponse;")) {}

RaceAppWrapper::~RaceAppWrapper() {}

void RaceAppWrapper::handleReceivedMessage(ClrMsg msg) {
    TRACE_METHOD();

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, JavaShimUtils::getJvm());
    RaceLog::logDebug(logLabel, "got Env", "");

    // Convert ClrMsg
    jobject jClrMsg = JavaShimUtils::clrMsg_to_jClrMsg(env, msg);
    RaceLog::logDebug(logLabel, "Converted ClrMsg", "");

    // Call method
    // call parent implementation first
    RaceApp::handleReceivedMessage(msg);
    env->CallVoidMethod(jRaceApp, jHandleReceivedMessageMethodId, jClrMsg);
}

void RaceAppWrapper::addMessageToUI(const ClrMsg &msg) {
    TRACE_METHOD();

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, JavaShimUtils::getJvm());
    RaceLog::logDebug(logLabel, "got Env", "");

    // Convert ClrMsg
    jobject jClrMsg = JavaShimUtils::clrMsg_to_jClrMsg(env, msg);
    RaceLog::logDebug(logLabel, "Converted ClrMsg", "");

    // Call method
    // call parent implementation first
    RaceApp::addMessageToUI(msg);
    env->CallVoidMethod(jRaceApp, jAddMessageToUIMethodId, jClrMsg);
}

void RaceAppWrapper::onMessageStatusChanged(RaceHandle handle, MessageStatus status) {
    TRACE_METHOD();

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, JavaShimUtils::getJvm());
    RaceLog::logDebug(logLabel, "got Env", "");

    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jobject jStatus = JavaShimUtils::messageStatusToJobject(env, status);

    // Call method
    // call parent implementation first
    RaceApp::onMessageStatusChanged(handle, status);
    env->CallVoidMethod(jRaceApp, jOnMessageStatusChangedMethodId, jHandle, jStatus);
}

void RaceAppWrapper::onSdkStatusChanged(const nlohmann::json &sdkStatus) {
    TRACE_METHOD();

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, JavaShimUtils::getJvm());
    RaceLog::logDebug(logLabel, "got Env", "");

    // Convert Status
    jstring jSdkStatus = env->NewStringUTF(sdkStatus.dump().c_str());
    RaceLog::logDebug(logLabel, "Converted sdkStatus", "");

    // Call method
    env->CallVoidMethod(jRaceApp, jOnSdkStatusChangedMethodId, jSdkStatus);
}

nlohmann::json RaceAppWrapper::getSdkStatus() {
    return {{}};
}

SdkResponse RaceAppWrapper::displayInfoToUser(RaceHandle handle, const std::string &data,
                                              RaceEnums::UserDisplayType displayType) {
    TRACE_METHOD();

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, JavaShimUtils::getJvm());
    RaceLog::logDebug(logLabel, "got Env", "");

    // Convert Params
    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jstring jData = env->NewStringUTF(data.c_str());
    jobject jDisplayType = JavaShimUtils::userDisplayTypeToJUserDisplayType(env, displayType);

    // Call method
    jobject jResponse =
        env->CallObjectMethod(jRaceApp, jDisplayInfoToUserMethodId, jHandle, jData, jDisplayType);
    return JavaShimUtils::jobjectToSdkResponse(env, jResponse);
}

SdkResponse RaceAppWrapper::displayBootstrapInfoToUser(RaceHandle handle, const std::string &data,
                                                       RaceEnums::UserDisplayType displayType,
                                                       RaceEnums::BootstrapActionType actionType) {
    TRACE_METHOD();

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, JavaShimUtils::getJvm());
    RaceLog::logDebug(logLabel, "got Env", "");

    // Convert Params
    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jstring jData = env->NewStringUTF(data.c_str());
    jobject jDisplayType = JavaShimUtils::userDisplayTypeToJUserDisplayType(env, displayType);
    jobject jActionType = JavaShimUtils::bootstrapActionTypeToJBootstrapActionType(env, actionType);

    // Call method
    jobject jResponse = env->CallObjectMethod(jRaceApp, jDisplayBootstrapInfoToUserMethodId,
                                              jHandle, jData, jDisplayType, jActionType);
    return JavaShimUtils::jobjectToSdkResponse(env, jResponse);
}

SdkResponse RaceAppWrapper::requestUserInput(RaceHandle handle, const std::string &pluginId,
                                             const std::string &key, const std::string &prompt,
                                             bool cache) {
    TRACE_METHOD();

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, JavaShimUtils::getJvm());
    RaceLog::logDebug(logLabel, "got Env", "");

    // Convert Params
    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jstring jPluginId = env->NewStringUTF(pluginId.c_str());
    jstring jKey = env->NewStringUTF(key.c_str());
    jstring jPrompt = env->NewStringUTF(prompt.c_str());
    jboolean jCache = static_cast<jboolean>(cache);

    // Call method
    jobject jResponse = env->CallObjectMethod(jRaceApp, jRequestUserInputMethodId, jHandle,
                                              jPluginId, jKey, jPrompt, jCache);
    return JavaShimUtils::jobjectToSdkResponse(env, jResponse);
}

SdkResponse RaceAppWrapper::nativeRequestUserInput(RaceHandle handle, const std::string &pluginId,
                                                   const std::string &key,
                                                   const std::string &prompt, bool cache) {
    return RaceApp::requestUserInput(handle, pluginId, key, prompt, cache);
}
