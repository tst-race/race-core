
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

#include "ShimsJava_RaceApp.h"

#include <RaceSdk.h>

#include <functional>

#include "IRaceApp.h"
#include "JavaIds.h"
#include "JavaShimUtils.h"
#include "RaceAppWrapper.h"
#include "racetestapp/RaceTestAppOutputLog.h"

inline RaceAppWrapper *getWrapperFromJRaceApp(JNIEnv *env, jobject jApp) {
    long wrapperPtr =
        static_cast<long>(env->GetLongField(jApp, JavaIds::jRaceAppWrapperPointerFieldId));
    return reinterpret_cast<RaceAppWrapper *>(wrapperPtr);
}

template <typename T>
T unwrap(JNIEnv *env, jobject jApp, std::function<T(RaceAppWrapper *)> func) {
    auto wrapper = getWrapperFromJRaceApp(env, jApp);
    if (wrapper == nullptr) {
        const std::string errorMessage = "Native app wrapper pointer is null in RaceApp";
        RaceLog::logError("JavaShims", errorMessage, "");
        jclass excCls = env->FindClass("java/lang/NullPointerException");
        if (excCls != NULL) {
            env->ThrowNew(excCls, errorMessage.c_str());
        }
        return T();
    }
    return func(wrapper);
}

/*
 * Class:     ShimsJava_RaceApp
 * Method:    _jni_initialize
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_ShimsJava_RaceApp__1jni_1initialize(JNIEnv *env, jobject jApp,
                                                                 jlong jOutputPtr, jlong jSdkPtr,
                                                                 jlong jTracerPtr) {
    RaceTestAppOutputLog *outputPtr = reinterpret_cast<RaceTestAppOutputLog *>(jOutputPtr);
    RaceSdk *sdkPtr = reinterpret_cast<RaceSdk *>(jSdkPtr);
    std::shared_ptr<opentracing::Tracer> *tracerPtr =
        reinterpret_cast<std::shared_ptr<opentracing::Tracer> *>(jTracerPtr);
    return reinterpret_cast<jlong>(
        new RaceAppWrapper(*outputPtr, *sdkPtr, *tracerPtr, env, JavaIds::jRaceAppClassId, jApp));
}

/*
 * Class:     ShimsJava_RaceApp
 * Method:    shutdown
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_ShimsJava_RaceApp_shutdown(JNIEnv *env, jobject jApp) {
    RaceAppWrapper *wrapper = getWrapperFromJRaceApp(env, jApp);
    env->SetLongField(jApp, JavaIds::jRaceAppWrapperPointerFieldId, 0);
    if (wrapper != nullptr) {
        RaceLog::logInfo("JavaShims", "Deleting RaceAppWrapper", "");
        delete wrapper;
    }
}

/*
 * Class:     ShimsJava_RaceApp
 * Method:    nativeRequestUserInput
 * Signature:
 * (LShimsJava/RaceHandle;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Z)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_RaceApp_nativeRequestUserInput(
    JNIEnv *env, jobject jApp, jobject jHandle, jstring jPluginId, jstring jKey, jstring jPrompt,
    jboolean jCache) {
    // Convert args
    RaceHandle handle = JavaShimUtils::jobjectToRaceHandle(env, jHandle);
    std::string pluginId = JavaShimUtils::jstring2string(env, jPluginId);
    std::string key = JavaShimUtils::jstring2string(env, jKey);
    std::string prompt = JavaShimUtils::jstring2string(env, jPrompt);
    bool cache = static_cast<bool>(static_cast<jboolean>(jCache));

    auto response = unwrap<SdkResponse>(env, jApp, [&](auto wrapper) {
        return wrapper->nativeRequestUserInput(handle, pluginId, key, prompt, cache);
    });
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_RaceApp
 * Method:    getCachedResponse
 * Signature: (Ljava/lang/String;Ljava/lang/String;)LShimsJava/RaceApp/UserResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_RaceApp_getCachedResponse(JNIEnv *env, jobject jApp,
                                                                   jstring jPluginId,
                                                                   jstring jKey) {
    // Convert args
    std::string pluginId = JavaShimUtils::jstring2string(env, jPluginId);
    std::string key = JavaShimUtils::jstring2string(env, jKey);

    auto [answered, response] = unwrap<std::tuple<bool, std::string>>(
        env, jApp, [&](auto wrapper) { return wrapper->getCachedResponse(pluginId, key); });

    jboolean jAnswered = static_cast<jboolean>(answered);
    jstring jResponse = env->NewStringUTF(response.c_str());
    return env->NewObject(JavaIds::jRaceAppUserResponseClassId,
                          JavaIds::jRaceAppUserResponseConstructorMethodId, jAnswered, jResponse);
}

/*
 * Class:     ShimsJava_RaceApp
 * Method:    getAutoResponse
 * Signature: (Ljava/lang/String;Ljava/lang/String;)LShimsJava/RaceApp/UserResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_RaceApp_getAutoResponse(JNIEnv *env, jobject jApp,
                                                                 jstring jPluginId, jstring jKey) {
    // Convert args
    std::string pluginId = JavaShimUtils::jstring2string(env, jPluginId);
    std::string key = JavaShimUtils::jstring2string(env, jKey);

    auto [answered, response] = unwrap<std::tuple<bool, std::string>>(
        env, jApp, [&](auto wrapper) { return wrapper->getAutoResponse(pluginId, key); });

    jboolean jAnswered = static_cast<jboolean>(answered);
    jstring jResponse = env->NewStringUTF(response.c_str());
    return env->NewObject(JavaIds::jRaceAppUserResponseClassId,
                          JavaIds::jRaceAppUserResponseConstructorMethodId, jAnswered, jResponse);
}

/*
 * Class:     ShimsJava_RaceApp
 * Method:    setCachedResponse
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_ShimsJava_RaceApp_setCachedResponse(JNIEnv *env, jobject jApp,
                                                                    jstring jPluginId, jstring jKey,
                                                                    jstring jResponse) {
    // Convert args
    std::string pluginId = JavaShimUtils::jstring2string(env, jPluginId);
    std::string key = JavaShimUtils::jstring2string(env, jKey);
    std::string response = JavaShimUtils::jstring2string(env, jResponse);

    bool result = unwrap<bool>(env, jApp, [&](auto wrapper) {
        return wrapper->setCachedResponse(pluginId, key, response);
    });

    return static_cast<jboolean>(result);
}
