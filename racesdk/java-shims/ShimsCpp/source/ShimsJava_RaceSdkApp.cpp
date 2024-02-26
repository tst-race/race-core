
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

#include "ShimsJava_RaceSdkApp.h"

#include <functional>
#include <stdexcept>

#include "AppConfig.h"
#include "IRaceApp.h"
#include "IRacePluginComms.h"
#include "IRacePluginNM.h"
#include "JavaIds.h"
#include "JavaShimUtils.h"
#include "RaceLog.h"
#include "RaceSdk.h"

inline RaceSdk *getSdkFromJRaceSdkApp(JNIEnv *env, jobject jSdk) {
    long nativePtr =
        static_cast<long>(env->GetLongField(jSdk, JavaIds::jRaceSdkAppSdkPointerFieldId));
    return reinterpret_cast<RaceSdk *>(nativePtr);
}

template <typename T>
T unwrap(JNIEnv *env, jobject jSdk, std::function<T(RaceSdk *)> func) {
    auto sdk = getSdkFromJRaceSdkApp(env, jSdk);
    if (sdk == nullptr) {
        const std::string errorMessage = "Native SDK pointer is null in RaceSdkApp";
        RaceLog::logError("JavaShims", errorMessage, "");
        jclass excCls = env->FindClass("java/lang/NullPointerException");
        if (excCls != NULL) {
            env->ThrowNew(excCls, errorMessage.c_str());
        }
        return T();
    }
    return func(sdk);
}

JNIEXPORT jlong JNICALL Java_ShimsJava_RaceSdkApp__1jni_1initialize(JNIEnv *env, jobject,
                                                                    jobject jAppConfig,
                                                                    jstring jPassphrase) {
    try {
        AppConfig config = JavaShimUtils::jAppConfigToAppConfig(env, jAppConfig);
        std::string passphrase = JavaShimUtils::jstring2string(env, jPassphrase);
        return reinterpret_cast<jlong>(new RaceSdk(config, passphrase));
    } catch (const StorageEncryption::InvalidPassphrase &err) {
        RaceLog::logError("JavaShims",
                          "Invalid passphrase given to RaceSdk: " + std::string(err.what()), "");
        jclass newExcCls = env->FindClass("ShimsJava/StorageEncryptionInvalidPassphraseException");
        if (newExcCls != NULL) {
            env->ThrowNew(newExcCls, "Invalid passphrase");
        } else {
            RaceLog::logError("JavaShims",
                              "failed to create Java exception of type "
                              "ShimsJava/StorageEncryptionInvalidPassphraseException",
                              "");
        }
    } catch (const std::runtime_error &err) {
        RaceLog::logError(
            "JavaShims",
            "Error creating RaceSdk: " + std::string(err.what()) + " " + typeid(err).name(), "");

        // TODO: idk why this isn't being caught above. Some weird C++/Java/ndk/Android thing that I
        // can't figure out? no idea. spent way too much time debugging this so using this stupid
        // hack to detect if the password is invalid.
        jclass newExcCls;
        if (strcmp(err.what(), "invalid passphrase") == 0) {
            newExcCls = env->FindClass("ShimsJava/StorageEncryptionInvalidPassphraseException");
        } else {
            newExcCls = env->FindClass("java/lang/RuntimeException");
        }

        if (newExcCls != NULL) {
            env->ThrowNew(newExcCls, "Exception thrown when creating RaceSdk");
        } else {
            RaceLog::logError("JavaShims",
                              "failed to create Java exception of type java/lang/RuntimeException",
                              "");
        }
    } catch (const std::exception &err) {
        RaceLog::logError("JavaShims",
                          "Error creating RaceSdk: " + std::string(err.what()) +
                              " ||| type: " + typeid(err).name(),
                          "");
        jclass newExcCls = env->FindClass("java/lang/RuntimeException");
        if (newExcCls != NULL) {
            env->ThrowNew(newExcCls, "Exception thrown when creating RaceSdk");
        } else {
            RaceLog::logError("JavaShims",
                              "failed to create Java exception of type java/lang/RuntimeException",
                              "");
        }
    }
    return 0;
}

/*
 * Class:     ShimsJava_RaceSdkApp
 * Method:    shutdown
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_ShimsJava_RaceSdkApp_shutdown(JNIEnv *env, jobject jSdk) {
    RaceSdk *sdk = getSdkFromJRaceSdkApp(env, jSdk);
    env->SetLongField(jSdk, JavaIds::jRaceSdkAppSdkPointerFieldId, 0);
    if (sdk != nullptr) {
        RaceLog::logInfo("JavaShims", "Deleting RaceSdk", "");
        delete sdk;
    }
}

/*
 * Class:     ShimsJava_RaceSdkApp
 * Method:    getAppConfig
 * Signature: ()LShimsJava/AppConfig;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_RaceSdkApp_getAppConfig(JNIEnv *env, jobject jSdk) {
    auto config = unwrap<AppConfig>(env, jSdk, [](auto sdk) { return sdk->getAppConfig(); });
    return JavaShimUtils::appConfigToJobject(env, config);
}

JNIEXPORT jboolean JNICALL Java_ShimsJava_RaceSdkApp_initRaceSystem(JNIEnv *env, jobject jSdk,
                                                                    jlong raceApp) {
    IRaceApp *app = reinterpret_cast<IRaceApp *>(raceApp);
    bool result = unwrap<bool>(env, jSdk, [&](auto sdk) { return sdk->initRaceSystem(app); });
    return static_cast<jboolean>(result);
}

/*
 * Class:     ShimsJava_RaceSdkApp
 * Method:    prepareToBootstrap
 * Signature:
 * (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_RaceSdkApp_prepareToBootstrap(
    JNIEnv *env, jobject jSdk, jstring jPlatform, jstring jArchitecture, jstring jNodeType,
    jstring jPassphrase, jstring jBootstrapChannelId) {
    DeviceInfo deviceInfo;
    deviceInfo.platform = JavaShimUtils::jstring2string(env, jPlatform);
    deviceInfo.architecture = JavaShimUtils::jstring2string(env, jArchitecture);
    deviceInfo.nodeType = JavaShimUtils::jstring2string(env, jNodeType);

    std::string passphrase = JavaShimUtils::jstring2string(env, jPassphrase);
    std::string bootstrapChannelId = JavaShimUtils::jstring2string(env, jBootstrapChannelId);

    RaceHandle handle = unwrap<RaceHandle>(env, jSdk, [&](auto sdk) {
        return sdk->prepareToBootstrap(deviceInfo, passphrase, bootstrapChannelId);
    });
    return JavaShimUtils::raceHandleToJobject(env, handle);
}

/*
 * Class:     ShimsJava_RaceSdkApp
 * Method:    getContacts
 * Signature: ()[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_ShimsJava_RaceSdkApp_getContacts(JNIEnv *env, jobject jSdk) {
    auto contacts =
        unwrap<std::vector<std::string>>(env, jSdk, [](auto sdk) { return sdk->getContacts(); });
    return JavaShimUtils::stringVectorToJArray(env, contacts);
}

/*
 * Class:     ShimsJava_RaceSdkApp
 * Method:    isConnected
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_ShimsJava_RaceSdkApp_isConnected(JNIEnv *env, jobject jSdk) {
    bool isConnected = unwrap<bool>(env, jSdk, [](auto sdk) { return sdk->isConnected(); });
    return static_cast<jboolean>(isConnected);
}

/*
 * Class:     ShimsJava_RaceSdkApp
 * Method:    getActivePersona
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_ShimsJava_RaceSdkApp_getActivePersona(JNIEnv *env, jobject jSdk) {
    auto activePersona =
        unwrap<std::string>(env, jSdk, [](auto sdk) { return sdk->getActivePersona(); });
    return env->NewStringUTF(activePersona.c_str());
}

/*
 * Class:     ShimsJava_RaceSdkApp
 * Method:    cancelBootstrap
 * Signature: Signature: (LShimsJava/bootstrapHandle)V
 */
JNIEXPORT void JNICALL Java_ShimsJava_RaceSdkApp_cancelBootstrap(JNIEnv *env, jobject jSdk,
                                                                 jobject jbootstrapHandle) {
    RaceHandle handle = JavaShimUtils::jobjectToRaceHandle(env, jbootstrapHandle);
    RaceSdk *sdk = getSdkFromJRaceSdkApp(env, jSdk);
    sdk->cancelBootstrap(handle);
}

/*
 * Class:     ShimsJava_RaceSdkApp
 * Method:    onUserAcknowledgementReceived
 * Signature: (LShimsJava/RaceHandle;)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_RaceSdkApp_onUserAcknowledgementReceived(JNIEnv *env,
                                                                                  jobject jSdk,
                                                                                  jobject jHandle) {
    RaceHandle handle = JavaShimUtils::jobjectToRaceHandle(env, jHandle);
    auto response = unwrap<SdkResponse>(
        env, jSdk, [&](auto sdk) { return sdk->onUserAcknowledgementReceived(handle); });
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_RaceSdkApp
 * Method:    onUserInputReceived
 * Signature: (LShimsJava/RaceHandle;ZLjava/lang/String;)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_RaceSdkApp_onUserInputReceived(JNIEnv *env, jobject jSdk,
                                                                        jobject jHandle,
                                                                        jboolean jAnswered,
                                                                        jstring jResponse) {
    RaceHandle handle = JavaShimUtils::jobjectToRaceHandle(env, jHandle);
    bool answered = static_cast<bool>(jAnswered);
    std::string response = JavaShimUtils::jstring2string(env, jResponse);

    auto sdkResponse = unwrap<SdkResponse>(
        env, jSdk, [&](auto sdk) { return sdk->onUserInputReceived(handle, answered, response); });
    return JavaShimUtils::sdkResponseToJobject(env, sdkResponse);
}

/*
 * Class:     ShimsJava_RaceSdkApp
 * Method:    getInitialEnabledChannels
 * Signature: ()[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_ShimsJava_RaceSdkApp_getInitialEnabledChannels(JNIEnv *env,
                                                                                   jobject jSdk) {
    auto channelGids = unwrap<std::vector<std::string>>(
        env, jSdk, [](auto sdk) { return sdk->getInitialEnabledChannels(); });
    return JavaShimUtils::stringVectorToJArray(env, channelGids);
}

/*
 * Class:     ShimsJava_RaceSdkApp
 * Method:    setEnabledChannels
 * Signature: ([Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_ShimsJava_RaceSdkApp_setEnabledChannels(JNIEnv *env, jobject jSdk,
                                                                        jobjectArray jChannelGids) {
    auto channelGids = JavaShimUtils::jArrayToStringVector(env, jChannelGids);
    bool result =
        unwrap<bool>(env, jSdk, [&](auto sdk) { return sdk->setEnabledChannels(channelGids); });
    return static_cast<jboolean>(result);
}

/*
 * Class:     ShimsJava_RaceSdkApp
 * Method:    enableChannel
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_ShimsJava_RaceSdkApp_enableChannel(JNIEnv *env, jobject jSdk,
                                                                   jstring jChannelGid) {
    std::string channelGid = JavaShimUtils::jstring2string(env, jChannelGid);
    bool result = unwrap<bool>(env, jSdk, [&](auto sdk) { return sdk->enableChannel(channelGid); });
    return static_cast<jboolean>(result);
}

/*
 * Class:     ShimsJava_RaceSdkApp
 * Method:    disableChannel
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_ShimsJava_RaceSdkApp_disableChannel(JNIEnv *env, jobject jSdk,
                                                                    jstring jChannelGid) {
    std::string channelGid = JavaShimUtils::jstring2string(env, jChannelGid);
    bool result =
        unwrap<bool>(env, jSdk, [&](auto sdk) { return sdk->disableChannel(channelGid); });
    return static_cast<jboolean>(result);
}

/*
 * Class:     ShimsJava_RaceSdkApp
 * Method:    getAllChannelProperties
 * Signature: ()[LShimsJava/JChannelProperties;
 */
JNIEXPORT jobjectArray JNICALL Java_ShimsJava_RaceSdkApp_getAllChannelProperties(JNIEnv *env,
                                                                                 jobject jSdk) {
    auto channels = unwrap<std::vector<ChannelProperties>>(
        env, jSdk, [](auto sdk) { return sdk->getAllChannelProperties(); });
    jobjectArray result = JavaShimUtils::channelPropertiesVectorToJArray(env, channels);
    return result;
}
