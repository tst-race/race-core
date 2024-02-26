
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

#include "ShimsJava_JRaceSdkComms.h"

#include <RaceLog.h>

#include <sstream>

#include "ChannelProperties.h"
#include "IRaceSdkComms.h"
#include "JavaIds.h"
#include "JavaShimUtils.h"
#include "LinkProperties.h"
#include "RaceEnums.h"
#include "SdkResponse.h"

static std::string logLabel = "JRaceSdkComms";

/**
 * @brief Get the sdk pointer from the jobject.
 *
 * @param env The JNI env variable.
 * @param level The jSdk jobject.
 * @return Get cpp pointer from java plugin name to sdk pointer map.
 */
inline IRaceSdkComms *getjSdkCppPointer(JNIEnv *env, jobject jSdk) {
    long sdkPointer =
        static_cast<long>(env->GetLongField(jSdk, JavaIds::jRaceSdkCommsSdkPointerFieldId));
    return reinterpret_cast<IRaceSdkComms *>(sdkPointer);
}

/*
 * Class:     JRaceSdkComms
 * Method:    _jni_initialize
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_ShimsJava_JRaceSdkComms__1jni_1initialize(JNIEnv * /* env */,
                                                                      jobject /* jSdk */,
                                                                      jlong /* sdkPointer */,
                                                                      jstring /* pluginName */) {
    RaceLog::logDebug(logLabel, "Java_JRaceSdkComms__1jni_1initialize", "");
}

/*
 * Class:     ShimsJava_JRaceSdkComms
 * Method:    getBlockingTimeout
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_ShimsJava_JRaceSdkComms_getBlockingTimeout(JNIEnv *, jclass) {
    return static_cast<jint>(RACE_BLOCKING);
}

/*
 * Class:     JRaceSdkComms
 * Method:    getEntropy
 * Signature: (JI)Ljava/util/Vector;
 */
JNIEXPORT jbyteArray JNICALL Java_ShimsJava_JRaceSdkComms_getEntropy(JNIEnv *env, jobject jSdk,
                                                                     jint i) {
    RaceLog::logDebug(logLabel, "Java_JRaceSdkComms_getEntropy", "");

    IRaceSdkComms *sdkPointer = getjSdkCppPointer(env, jSdk);

    RawData sdkEntropy = sdkPointer->getEntropy(static_cast<uint32_t>(i));

    RaceLog::logDebug(logLabel, "getEntropy: Package size = " + std::to_string(sdkEntropy.size()),
                      "");
    RaceLog::logDebug(logLabel, "got entropy", "");
    jbyteArray entropy = env->NewByteArray(i);
    RaceLog::logDebug(logLabel, "created jByteArray", "");
    for (jsize element = 0; element < i; element++) {
        RaceLog::logDebug(logLabel, "setting array region", "");
        jbyte b = static_cast<jbyte>(sdkEntropy[static_cast<size_t>(element)]);
        env->SetByteArrayRegion(entropy, element, 1, &b);
    }
    RaceLog::logDebug(logLabel, "Java_JRaceSdkComms_getEntropy returing", "");
    return entropy;
}

/*
 * Class:     JRaceSdkComms
 * Method:    getActivePersona
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_ShimsJava_JRaceSdkComms_getActivePersona(JNIEnv *env, jobject jSdk) {
    RaceLog::logDebug(logLabel, "Java_JRaceSdkComms_getActivePersona", "");

    IRaceSdkComms *sdkPointer = getjSdkCppPointer(env, jSdk);

    // replaced shared sdk with local sdkPointer accessed from map
    return env->NewStringUTF(sdkPointer->getActivePersona().c_str());
}

/*
 * Class:     ShimsJava_JRaceSdkComms
 * Method:    getChannelProperties
 * Signature: (java/lang/String;)LShimsJava/JChannelProperties;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkComms_getChannelProperties(JNIEnv *env,
                                                                            jobject jSdk,
                                                                            jstring jChannelGid) {
    static const std::string functionLogLabel =
        logLabel + ": Java_ShimsJava_JRaceSdkComms_getChannelProperties";
    RaceLog::logDebug(functionLogLabel, "called", "");

    IRaceSdkComms *sdkPointer = getjSdkCppPointer(env, jSdk);

    const std::string channelGid = JavaShimUtils::jstring2string(env, jChannelGid);
    ChannelProperties properties = sdkPointer->getChannelProperties(channelGid);
    jobject jProperties = JavaShimUtils::channelPropertiesToJobject(env, properties);
    if (jProperties == nullptr) {
        RaceLog::logError(functionLogLabel, "failed to convert link properties", "");
    }
    RaceLog::logDebug(functionLogLabel, "returned", "");
    return jProperties;
}

/*
 * Class:     ShimsJava_JRaceSdkComms
 * Method:    getAllChannelProperties
 * Signature: ()[LShimsJava/JChannelProperties;
 */
JNIEXPORT jobjectArray JNICALL Java_ShimsJava_JRaceSdkComms_getAllChannelProperties(JNIEnv *env,
                                                                                    jobject jSdk) {
    static const std::string functionLogLabel =
        logLabel + ": Java_ShimsJava_JRaceSdkComms_getAllChannelProperties";
    RaceLog::logDebug(functionLogLabel, "called", "");

    IRaceSdkComms *sdkPointer = getjSdkCppPointer(env, jSdk);

    std::vector<ChannelProperties> properties = sdkPointer->getAllChannelProperties();
    jobjectArray jProperties = JavaShimUtils::channelPropertiesVectorToJArray(env, properties);
    if (jProperties == nullptr) {
        RaceLog::logError(functionLogLabel, "failed to convert link properties", "");
    }
    RaceLog::logDebug(functionLogLabel, "returned", "");
    return jProperties;
}

/*
 * Class:     ShimsJava_JRaceSdkComms
 * Method:    removeDir
 * Signature: (Ljava/lang/String;)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkComms_removeDir(JNIEnv *env, jobject jSdk,
                                                                 jstring jFilepath) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkComms_removeDir: called", "");

    IRaceSdkComms *sdkPointer = getjSdkCppPointer(env, jSdk);

    std::string filepath = JavaShimUtils::jstring2string(env, jFilepath);
    SdkResponse response = sdkPointer->removeDir(filepath);

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkComms_removeDir: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkComms
 * Method:    makeDir
 * Signature: (Ljava/lang/String;)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkComms_makeDir(JNIEnv *env, jobject jSdk,
                                                               jstring jFilepath) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkComms_makeDir: called", "");

    IRaceSdkComms *sdkPointer = getjSdkCppPointer(env, jSdk);

    std::string filepath = JavaShimUtils::jstring2string(env, jFilepath);
    SdkResponse response = sdkPointer->makeDir(filepath);

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkComms_makeDir: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkComms
 * Method:    listDir
 * Signature: (Ljava/lang/String)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_ShimsJava_JRaceSdkComms_listDir(JNIEnv *env, jobject jSdk,
                                                                    jstring jFilepath) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkComms_listdir: called", "");

    IRaceSdkComms *sdkPointer = getjSdkCppPointer(env, jSdk);

    std::string filepath = JavaShimUtils::jstring2string(env, jFilepath);
    std::vector<std::string> contents = sdkPointer->listDir(filepath);
    jobjectArray jContents = JavaShimUtils::stringVectorToJArray(env, contents);

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkComms_listdir: returned", "");
    return jContents;
}

/*
 * Class:     ShimsJava_JRaceSdkComms
 * Method:    readFile
 * Signature: (Ljava/lang/String)[B;
 */
JNIEXPORT jbyteArray JNICALL Java_ShimsJava_JRaceSdkComms_readFile(JNIEnv *env, jobject jSdk,
                                                                   jstring jFilepath) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkComms_readFile: called", "");

    IRaceSdkComms *sdkPointer = getjSdkCppPointer(env, jSdk);

    std::string filepath = JavaShimUtils::jstring2string(env, jFilepath);
    RawData data = sdkPointer->readFile(filepath);
    jbyteArray jData = JavaShimUtils::rawDataToJByteArray(env, data);

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkComms_readFile: returned", "");
    return jData;
}

/*
 * Class:     ShimsJava_JRaceSdkComms
 * Method:    appendFile
 * Signature: (Ljava/lang/String;[B)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkComms_appendFile(JNIEnv *env, jobject jSdk,
                                                                  jstring jFilepath,
                                                                  jbyteArray jData) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkComms_appendFile: called", "");

    IRaceSdkComms *sdkPointer = getjSdkCppPointer(env, jSdk);

    std::string filepath = JavaShimUtils::jstring2string(env, jFilepath);
    RawData data = JavaShimUtils::jByteArrayToRawData(env, jData);
    SdkResponse response = sdkPointer->appendFile(filepath, data);

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkComms_appendFile: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkComms
 * Method:    writeFile
 * Signature: (Ljava/lang/String;[B)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkComms_writeFile(JNIEnv *env, jobject jSdk,
                                                                 jstring jFilepath,
                                                                 jbyteArray jData) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkComms_writeFile: called", "");

    IRaceSdkComms *sdkPointer = getjSdkCppPointer(env, jSdk);

    std::string filepath = JavaShimUtils::jstring2string(env, jFilepath);
    RawData data = JavaShimUtils::jByteArrayToRawData(env, jData);
    SdkResponse response = sdkPointer->writeFile(filepath, data);

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkComms_writeFile: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkComms
 * Method:    requestPluginUserInput
 * Signature: (Ljava/lang/String;Ljava/lang/String;Z)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkComms_requestPluginUserInput(
    JNIEnv *env, jobject jSdk, jstring jKey, jstring jPrompt, jboolean jCache) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkComms_requestPluginUserInput: called", "");

    IRaceSdkComms *sdkPointer = getjSdkCppPointer(env, jSdk);

    std::string key = JavaShimUtils::jstring2string(env, jKey);
    std::string prompt = JavaShimUtils::jstring2string(env, jPrompt);
    bool cache = static_cast<bool>(jCache);
    SdkResponse response = sdkPointer->requestPluginUserInput(key, prompt, cache);

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkComms_requestPluginUserInput: returned",
                      "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkComms
 * Method:    requestCommonUserInput
 * Signature: (Ljava/lang/String;)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkComms_requestCommonUserInput(JNIEnv *env,
                                                                              jobject jSdk,
                                                                              jstring jKey) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkComms_requestCommonUserInput: called", "");

    IRaceSdkComms *sdkPointer = getjSdkCppPointer(env, jSdk);

    std::string key = JavaShimUtils::jstring2string(env, jKey);
    SdkResponse response = sdkPointer->requestCommonUserInput(key);

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkComms_requestCommonUserInput: returned",
                      "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkComms
 * Method:    onPackageStatusChanged
 * Signature: (LShimsJava/RaceHandle;LShimsJava/PackageStatus;I)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkComms_onPackageStatusChanged(
    JNIEnv *env, jobject jSdk, jobject jHandle, jobject jPackageStatus, jint jTimeout) {
    RaceLog::logDebug(logLabel, "Java_JRaceSdkComms_onPackageStatusChanged", "");

    IRaceSdkComms *sdkPointer = getjSdkCppPointer(env, jSdk);

    RaceHandle handle = JavaShimUtils::jobjectToRaceHandle(env, jHandle);
    PackageStatus status = JavaShimUtils::jobjectToPackageStatus(env, jPackageStatus);
    int32_t timeout = static_cast<int32_t>(jTimeout);
    return JavaShimUtils::sdkResponseToJobject(
        env, sdkPointer->onPackageStatusChanged(handle, status, timeout));
}

/*
 * Class:     ShimsJava_JRaceSdkComms
 * Method:    onConnectionStatusChanged
 * Signature:
 * (LShimsJava/RaceHandle;Ljava/lang/String;LShimsJava/ConnectionStatus;LShimsJava/JLinkProperties;I)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkComms_onConnectionStatusChanged(
    JNIEnv *env, jobject jSdk, jobject jHandle, jstring jConnId, jobject jConnectionStatus,
    jobject jLinkProperties, jint jTimeout) {
    RaceLog::logDebug(logLabel, "Java_JRaceSdkComms_onConnectionStatusChanged", "");

    IRaceSdkComms *sdkPointer = getjSdkCppPointer(env, jSdk);

    RaceHandle handle = JavaShimUtils::jobjectToRaceHandle(env, jHandle);
    std::string connId = JavaShimUtils::jstring2string(env, jConnId);
    ConnectionStatus status = JavaShimUtils::jobjectToConnectionStatus(env, jConnectionStatus);
    LinkProperties linkProperties =
        JavaShimUtils::jLinkPropertiesToLinkProperties(env, jLinkProperties);
    int32_t timeout = static_cast<int32_t>(jTimeout);
    return JavaShimUtils::sdkResponseToJobject(
        env,
        sdkPointer->onConnectionStatusChanged(handle, connId, status, linkProperties, timeout));
}

/*
 * Class:     ShimsJava_JRaceSdkComms
 * Method:    onLinkStatusChanged
 * Signature:
 * (LShimsJava/RaceHandle;Ljava/lang/String;LShimsJava/LinkStatus;LShimsJava/JLinkProperties;I)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkComms_onLinkStatusChanged(
    JNIEnv *env, jobject jSdk, jobject jHandle, jstring jLinkId, jobject jLinkStatus,
    jobject jLinkProperties, jint jTimeout) {
    RaceLog::logDebug(logLabel, "Java_JRaceSdkComms_onLinkStatusChanged", "");

    IRaceSdkComms *sdkPointer = getjSdkCppPointer(env, jSdk);

    RaceHandle handle = JavaShimUtils::jobjectToRaceHandle(env, jHandle);
    std::string linkId = JavaShimUtils::jstring2string(env, jLinkId);
    LinkStatus status = JavaShimUtils::jobjectToLinkStatus(env, jLinkStatus);
    LinkProperties linkProperties =
        JavaShimUtils::jLinkPropertiesToLinkProperties(env, jLinkProperties);
    int32_t timeout = static_cast<int32_t>(jTimeout);
    return JavaShimUtils::sdkResponseToJobject(
        env, sdkPointer->onLinkStatusChanged(handle, linkId, status, linkProperties, timeout));
}

/*
 * Class:     ShimsJava_JRaceSdkComms
 * Method:    onChannelStatusChanged
 * Signature:
 * (LShimsJava/RaceHandle;Ljava/lang/String;LShimsJava/ChannelStatus;LShimsJava/JChannelProperties;I)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkComms_onChannelStatusChanged(
    JNIEnv *env, jobject jSdk, jobject jHandle, jstring jChannelGid, jobject jChannelStatus,
    jobject jChannelProperties, jint jTimeout) {
    RaceLog::logDebug(logLabel, "Java_JRaceSdkComms_onChannelStatusChanged", "");

    IRaceSdkComms *sdkPointer = getjSdkCppPointer(env, jSdk);

    RaceHandle handle = JavaShimUtils::jobjectToRaceHandle(env, jHandle);
    std::string channelGid = JavaShimUtils::jstring2string(env, jChannelGid);
    ChannelStatus status = JavaShimUtils::jobjectToChannelStatus(env, jChannelStatus);
    ChannelProperties channelProperties =
        JavaShimUtils::jChannelPropertiesToChannelProperties(env, jChannelProperties);
    int32_t timeout = static_cast<int32_t>(jTimeout);
    return JavaShimUtils::sdkResponseToJobject(
        env,
        sdkPointer->onChannelStatusChanged(handle, channelGid, status, channelProperties, timeout));
}

/*
 * Class:     JRaceSdkComms
 * Method:    updateLinkProperties
 * Signature: (Ljava/lang/String;LShimsJava/JLinkProperties;I)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkComms_updateLinkProperties(
    JNIEnv *env, jobject jSdk, jstring jLinkId, jobject jLinkProperties, jint jTimeout) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkComms_updateLinkProperties", "");

    IRaceSdkComms *sdkPointer = getjSdkCppPointer(env, jSdk);

    std::string linkId = JavaShimUtils::jstring2string(env, jLinkId);
    LinkProperties linkProperties =
        JavaShimUtils::jLinkPropertiesToLinkProperties(env, jLinkProperties);
    int32_t timeout = static_cast<int32_t>(jTimeout);
    return JavaShimUtils::sdkResponseToJobject(
        env, sdkPointer->updateLinkProperties(linkId, linkProperties, timeout));
}

/*
 * Class:     ShimsJava_JRaceSdkComms
 * Method:    generateConnectionId
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_ShimsJava_JRaceSdkComms_generateConnectionId(JNIEnv *env,
                                                                            jobject jSdk,
                                                                            jstring jLinkId) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkComms_generateConnectionId", "");

    IRaceSdkComms *sdkPointer = getjSdkCppPointer(env, jSdk);

    std::string linkId = JavaShimUtils::jstring2string(env, jLinkId);
    return env->NewStringUTF(sdkPointer->generateConnectionId(linkId).c_str());
}

/*
 * Class:     ShimsJava_JRaceSdkComms
 * Method:    generateLinkId
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_ShimsJava_JRaceSdkComms_generateLinkId(JNIEnv *env, jobject jSdk,
                                                                      jstring jChannelGid) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkComms_generateLinkId", "");

    IRaceSdkComms *sdkPointer = getjSdkCppPointer(env, jSdk);

    std::string channelGid = JavaShimUtils::jstring2string(env, jChannelGid);
    return env->NewStringUTF(sdkPointer->generateLinkId(channelGid).c_str());
}

/*
 * Class:     ShimsJava_JRaceSdkComms
 * Method:    receiveEncPkg
 * Signature: (LShimsJava/JEncPkg;[Ljava/lang/String;I)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkComms_receiveEncPkg(JNIEnv *env, jobject jSdk,
                                                                     jobject jEncPkg,
                                                                     jobjectArray jConnectionIds,
                                                                     jint jTimeout) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkComms_receiveEncPkg", "");

    IRaceSdkComms *sdkPointer = getjSdkCppPointer(env, jSdk);

    EncPkg encPkg = JavaShimUtils::jobjectToEncPkg(env, jEncPkg);
    RaceLog::logDebug(
        logLabel,
        "receiveEncPkg: called. Package size = " + std::to_string(encPkg.getCipherText().size()),
        "");

    std::vector<ConnectionID> connectionIds =
        JavaShimUtils::jArrayToStringVector(env, jConnectionIds);
    int32_t timeout = static_cast<int32_t>(jTimeout);

    return JavaShimUtils::sdkResponseToJobject(
        env, sdkPointer->receiveEncPkg(encPkg, connectionIds, timeout));
}

/*
 * Class:     ShimsJava_JRaceSdkComms
 * Method:    displayInfoToUser
 * Signature: (Ljava/lang/String;LShimsJava/UserDisplayType;)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkComms_displayInfoToUser(JNIEnv *env, jobject jSdk,
                                                                         jstring jData,
                                                                         jobject jDisplayType) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkComms_displayInfoToUser", "");

    IRaceSdkComms *sdkPointer = getjSdkCppPointer(env, jSdk);

    std::string data = JavaShimUtils::jstring2string(env, jData);
    RaceEnums::UserDisplayType displayType =
        JavaShimUtils::jobjectToUserDisplayType(env, jDisplayType);

    SdkResponse response = sdkPointer->displayInfoToUser(data, displayType);
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkComms
 * Method:    displayBootstrapInfoToUser
 * Signature:
 * (Ljava/lang/String;LShimsJava/UserDisplayType;LShimsJava/BootstrapActionType;)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkComms_displayBootstrapInfoToUser(
    JNIEnv *env, jobject jSdk, jstring jData, jobject jDisplayType, jobject jBootstrapActionType) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkComms_displayBootstrapInfoToUser", "");

    IRaceSdkComms *sdkPointer = getjSdkCppPointer(env, jSdk);

    std::string data = JavaShimUtils::jstring2string(env, jData);
    RaceEnums::UserDisplayType displayType =
        JavaShimUtils::jobjectToUserDisplayType(env, jDisplayType);
    RaceEnums::BootstrapActionType bootstrapActionType =
        JavaShimUtils::jobjectToBootstrapActionType(env, jBootstrapActionType);

    SdkResponse response =
        sdkPointer->displayBootstrapInfoToUser(data, displayType, bootstrapActionType);
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkComms
 * Method:    unblockQueue
 * Signature:
 * (Ljava/lang/String;)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkComms_unblockQueue(JNIEnv *env, jobject jSdk,
                                                                    jstring jConnId) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkComms_unblockQueue", "");

    IRaceSdkComms *sdkPointer = getjSdkCppPointer(env, jSdk);

    std::string connId = JavaShimUtils::jstring2string(env, jConnId);
    SdkResponse response = sdkPointer->unblockQueue(connId);
    return JavaShimUtils::sdkResponseToJobject(env, response);
}
