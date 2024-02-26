
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

#include "ShimsJava_JRaceSdkNM.h"

#include <RaceLog.h>

#include <sstream>

#include "ChannelProperties.h"
#include "IRaceSdkNM.h"
#include "JavaShimUtils.h"
#include "LinkProperties.h"

namespace JRaceSdkNM {
IRaceSdkNM *sdk;
}

static std::string logLabel = "JRaceSdkNM";

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    _jni_initialize
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_ShimsJava_JRaceSdkNM__1jni_1initialize(JNIEnv *, jobject,
                                                                   jlong sdkPointer) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM__1jni_1initialize: called", "");
    JRaceSdkNM::sdk = reinterpret_cast<IRaceSdkNM *>(sdkPointer);
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM__1jni_1initialize: returned", "");
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    getBlockingTimeout
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_ShimsJava_JRaceSdkNM_getBlockingTimeout(JNIEnv *, jclass) {
    return static_cast<jint>(RACE_BLOCKING);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    getUnlimitedTimeout
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_ShimsJava_JRaceSdkNM_getUnlimitedTimeout(JNIEnv *, jclass) {
    return static_cast<jint>(RACE_UNLIMITED);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    getEntropy
 * Signature: (I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_ShimsJava_JRaceSdkNM_getEntropy(JNIEnv *env, jobject, jint size) {
    // TODO: how can we test this? Write a test "plugin" class that calls the function and returns
    // the value? Write a test in Java? -GP
    RaceLog::logDebug(logLabel, "Java_JRaceSdkNM_getEntropy: called", "");
    RawData sdkEntropy = JRaceSdkNM::sdk->getEntropy(static_cast<uint32_t>(size));
    jbyteArray entropy = env->NewByteArray(static_cast<jsize>(size));
    if (entropy == nullptr) {
        RaceLog::logError(logLabel, "failed to create Java byte array for entropy", "");
        return nullptr;
    }

    for (jsize index = 0; index < size; ++index) {
        jbyte oneByteOfEntropy = static_cast<jbyte>(sdkEntropy[static_cast<size_t>(index)]);
        env->SetByteArrayRegion(entropy, index, 1, &oneByteOfEntropy);
    }

    RaceLog::logDebug(logLabel, "Java_JRaceSdkNM_getEntropy: returned", "");
    return entropy;
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    getActivePersona
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_ShimsJava_JRaceSdkNM_getActivePersona(JNIEnv *env, jobject) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_getActivePersona: called", "");
    jstring persona = env->NewStringUTF(JRaceSdkNM::sdk->getActivePersona().c_str());
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_getActivePersona: returned", "");
    return persona;
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    removeDir
 * Signature: (Ljava/lang/String;)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_removeDir(JNIEnv *env, jobject,
                                                              jstring jFilepath) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_removeDir: called", "");

    std::string filepath = JavaShimUtils::jstring2string(env, jFilepath);
    SdkResponse response = JRaceSdkNM::sdk->removeDir(filepath);

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_removeDir: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    makeDir
 * Signature: (Ljava/lang/String;)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_makeDir(JNIEnv *env, jobject,
                                                            jstring jFilepath) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_makeDir: called", "");

    std::string filepath = JavaShimUtils::jstring2string(env, jFilepath);
    SdkResponse response = JRaceSdkNM::sdk->makeDir(filepath);

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_makeDir: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    listDir
 * Signature: (Ljava/lang/String)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_ShimsJava_JRaceSdkNM_listDir(JNIEnv *env, jobject,
                                                                 jstring jFilepath) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_listdir: called", "");

    std::string filepath = JavaShimUtils::jstring2string(env, jFilepath);
    std::vector<std::string> contents = JRaceSdkNM::sdk->listDir(filepath);
    jobjectArray jContents = JavaShimUtils::stringVectorToJArray(env, contents);

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_listdir: returned", "");
    return jContents;
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    readFile
 * Signature: (Ljava/lang/String)[B;
 */
JNIEXPORT jbyteArray JNICALL Java_ShimsJava_JRaceSdkNM_readFile(JNIEnv *env, jobject,
                                                                jstring jFilename) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_readFile: called", "");

    std::string filename = JavaShimUtils::jstring2string(env, jFilename);
    RawData data = JRaceSdkNM::sdk->readFile(filename);
    jbyteArray jData = JavaShimUtils::rawDataToJByteArray(env, data);

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_readFile: returned", "");
    return jData;
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    appendFile
 * Signature: (Ljava/lang/String;[B)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_appendFile(JNIEnv *env, jobject,
                                                               jstring jFilename,
                                                               jbyteArray jData) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_appendFile: called", "");

    std::string filename = JavaShimUtils::jstring2string(env, jFilename);
    RawData data = JavaShimUtils::jByteArrayToRawData(env, jData);
    SdkResponse response = JRaceSdkNM::sdk->appendFile(filename, data);

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_appendFile: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    writeFile
 * Signature: (Ljava/lang/String;[B)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_writeFile(JNIEnv *env, jobject,
                                                              jstring jFilename, jbyteArray jData) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_writeFile: called", "");

    std::string filename = JavaShimUtils::jstring2string(env, jFilename);
    RawData data = JavaShimUtils::jByteArrayToRawData(env, jData);
    SdkResponse response = JRaceSdkNM::sdk->writeFile(filename, data);

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_writeFile: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    requestPluginUserInput
 * Signature: (Ljava/lang/String;Ljava/lang/String;Z)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_requestPluginUserInput(JNIEnv *env, jobject,
                                                                           jstring jKey,
                                                                           jstring jPrompt,
                                                                           jboolean jCache) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_requestPluginUserInput: called", "");

    std::string key = JavaShimUtils::jstring2string(env, jKey);
    std::string prompt = JavaShimUtils::jstring2string(env, jPrompt);
    bool cache = static_cast<bool>(jCache);
    SdkResponse response = JRaceSdkNM::sdk->requestPluginUserInput(key, prompt, cache);

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_requestPluginUserInput: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    requestCommonUserInput
 * Signature: (Ljava/lang/String;)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_requestCommonUserInput(JNIEnv *env, jobject,
                                                                           jstring jKey) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_requestCommonUserInput: called", "");

    std::string key = JavaShimUtils::jstring2string(env, jKey);
    SdkResponse response = JRaceSdkNM::sdk->requestCommonUserInput(key);

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_requestCommonUserInput: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    getLinkProperties
 * Signature: (java/lang/String;)LShimsJava/JLinkProperties;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_getLinkProperties(JNIEnv *env, jobject,
                                                                      jstring jLinkId) {
    static const std::string functionLogLabel =
        logLabel + ": Java_ShimsJava_JRaceSdkNM_getLinkProperties";
    RaceLog::logDebug(functionLogLabel, "called", "");

    const LinkID linkId = JavaShimUtils::jstring2string(env, jLinkId);
    LinkProperties properties = JRaceSdkNM::sdk->getLinkProperties(linkId);
    jobject jProperties = JavaShimUtils::linkPropertiesToJobject(env, properties);
    if (jProperties == nullptr) {
        RaceLog::logError(functionLogLabel, "failed to convert link properties", "");
    }
    RaceLog::logDebug(functionLogLabel, "returned", "");
    return jProperties;
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    getChannelProperties
 * Signature: (java/lang/String;)LShimsJava/JChannelProperties;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_getChannelProperties(JNIEnv *env, jobject,
                                                                         jstring jChannelGid) {
    static const std::string functionLogLabel =
        logLabel + ": Java_ShimsJava_JRaceSdkNM_getChannelProperties";
    RaceLog::logDebug(functionLogLabel, "called", "");

    const std::string channelGid = JavaShimUtils::jstring2string(env, jChannelGid);
    ChannelProperties properties = JRaceSdkNM::sdk->getChannelProperties(channelGid);
    jobject jProperties = JavaShimUtils::channelPropertiesToJobject(env, properties);
    if (jProperties == nullptr) {
        RaceLog::logError(functionLogLabel, "failed to convert link properties", "");
    }
    RaceLog::logDebug(functionLogLabel, "returned", "");
    return jProperties;
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    getAllChannelProperties
 * Signature: ()[LShimsJava/JChannelProperties;
 */
JNIEXPORT jobjectArray JNICALL Java_ShimsJava_JRaceSdkNM_getAllChannelProperties(JNIEnv *env,
                                                                                 jobject) {
    static const std::string functionLogLabel =
        logLabel + ": Java_ShimsJava_JRaceSdkNM_getAllChannelProperties";
    RaceLog::logDebug(functionLogLabel, "called", "");
    std::vector<ChannelProperties> properties = JRaceSdkNM::sdk->getAllChannelProperties();
    jobjectArray jProperties = JavaShimUtils::channelPropertiesVectorToJArray(env, properties);
    if (jProperties == nullptr) {
        RaceLog::logError(functionLogLabel, "failed to convert link properties", "");
    }
    RaceLog::logDebug(functionLogLabel, "returned", "");
    return jProperties;
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    getSupportedChannels
 * Signature: (java/lang/String;)LShimsJava/JChannelProperties;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_getSupportedChannels(JNIEnv *env, jobject) {
    static const std::string functionLogLabel =
        logLabel + ": Java_ShimsJava_JRaceSdkNM_getSupportedChannels";
    RaceLog::logDebug(functionLogLabel, "called", "");

    std::map<std::string, ChannelProperties> supportedChannels =
        JRaceSdkNM::sdk->getSupportedChannels();
    jobject jSupportedChannels = JavaShimUtils::supportedChannelsToJobject(env, supportedChannels);
    if (jSupportedChannels == nullptr) {
        RaceLog::logError(functionLogLabel, "failed to convert supported channels", "");
    }
    RaceLog::logDebug(functionLogLabel, "returned", "");
    return jSupportedChannels;
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    sendEncryptedPackage
 * Signature: (LShimsJava/JEncPkg;Ljava/lang/String;I)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_sendEncryptedPackage(
    JNIEnv *env, jobject, jobject ePkg, jstring connectionId, jlong jBatchId, jint jTimeout) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_sendEncryptedPackage: called", "");

    EncPkg package = JavaShimUtils::jobjectToEncPkg(env, ePkg);

    std::string connId = JavaShimUtils::jstring2string(env, connectionId);
    uint64_t batchId = static_cast<uint64_t>(jBatchId);
    int timeout = static_cast<int>(jTimeout);
    SdkResponse response = JRaceSdkNM::sdk->sendEncryptedPackage(package, connId, batchId, timeout);

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_sendEncryptedPackage: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    presentCleartextMessage
 * Signature: (LShimsJava/JClrMsg;)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_presentCleartextMessage(JNIEnv *env, jobject,
                                                                            jobject jClrMsg) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_presentCleartextMessage: called", "");

    const ClrMsg clrMsg = JavaShimUtils::jClrMsg_to_ClrMsg(env, jClrMsg);
    SdkResponse response = JRaceSdkNM::sdk->presentCleartextMessage(clrMsg);

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_presentCleartextMessage: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    getLinksForPersonas
 * Signature: ([Ljava/lang/String;LShimsJava/JLinkProperties/LinkType;)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_ShimsJava_JRaceSdkNM_getLinksForPersonas(
    JNIEnv *env, jobject, jobjectArray recipientPersonas, jobject jLinkType) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_getLinksForPersonas: called", "");
    const jsize numPersonas = env->GetArrayLength(recipientPersonas);
    std::vector<std::string> personas;
    for (int i = 0; i < numPersonas; ++i) {
        jstring persona = static_cast<jstring>(env->GetObjectArrayElement(recipientPersonas, i));
        personas.push_back(JavaShimUtils::jstring2string(env, persona));
    }

    LinkType linkType = JavaShimUtils::jobjectToLinkType(env, jLinkType);

    std::vector<std::string> links = JRaceSdkNM::sdk->getLinksForPersonas(personas, linkType);

    jobjectArray result = JavaShimUtils::stringVectorToJArray(env, links);

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_getLinksForPersonas: returned", "");

    return result;
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    getLinksForChannel
 * Signature: (Ljava/lang/String;)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_ShimsJava_JRaceSdkNM_getLinksForChannel(JNIEnv *env, jobject,
                                                                            jstring jChannelGid) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_getLinksForChannel: called", "");

    std::string channelGid = JavaShimUtils::jstring2string(env, jChannelGid);
    std::vector<std::string> links = JRaceSdkNM::sdk->getLinksForChannel(channelGid);

    jobjectArray result = JavaShimUtils::stringVectorToJArray(env, links);

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_getLinksForChannel: returned", "");

    return result;
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    getPersonasForLink
 * Signature: (Ljava/lang/String;)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_ShimsJava_JRaceSdkNM_getPersonasForLink(JNIEnv *env, jobject,
                                                                            jstring jLinkId) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_getPersonasForLink: called", "");

    const LinkID linkId = JavaShimUtils::jstring2string(env, jLinkId);
    std::vector<std::string> personas = JRaceSdkNM::sdk->getPersonasForLink(linkId);
    jobjectArray result = JavaShimUtils::stringVectorToJArray(env, personas);
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_getPersonasForLink: returned", "");

    return result;
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    setPersonasForLink
 * Signature: (Ljava/lang/String;[Ljava/lang/String;)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_setPersonasForLink(JNIEnv *env, jobject,
                                                                       jstring jLinkId,
                                                                       jobjectArray jPersonas) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_setPersonasForLink: called", "");
    const LinkID linkId = JavaShimUtils::jstring2string(env, jLinkId);

    const jsize numPersonas = env->GetArrayLength(jPersonas);
    std::vector<std::string> personas;
    for (int i = 0; i < numPersonas; ++i) {
        jstring persona = static_cast<jstring>(env->GetObjectArrayElement(jPersonas, i));
        personas.push_back(JavaShimUtils::jstring2string(env, persona));
    }

    SdkResponse response = JRaceSdkNM::sdk->setPersonasForLink(linkId, personas);
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_setPersonasForLink: returned", "");

    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    openConnection
 * Signature:
 * (LShimsJava/JLinkProperties/LinkType;Ljava/lang/String;Ljava/lang/String;III)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_openConnection(
    JNIEnv *env, jobject, jobject jLinkType, jstring jLinkId, jstring jLinkHints, jint jPriority,
    jint jSendTimeout, jint jTimeout) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_openConnection: called", "");

    LinkType linkType = JavaShimUtils::jobjectToLinkType(env, jLinkType);
    std::string linkId = JavaShimUtils::jstring2string(env, jLinkId);
    std::string linkHints = JavaShimUtils::jstring2string(env, jLinkHints);
    SdkResponse response = JRaceSdkNM::sdk->openConnection(
        linkType, linkId, linkHints, static_cast<std::int32_t>(jPriority),
        static_cast<std::int32_t>(jSendTimeout), static_cast<std::int32_t>(jTimeout));

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_openConnection: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    closeConnection
 * Signature: (Ljava/lang/String;I)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_closeConnection(JNIEnv *env, jobject,
                                                                    jstring jConnectionId,
                                                                    jint jTimeout) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_closeConnection: called", "");
    std::string connId = JavaShimUtils::jstring2string(env, jConnectionId);
    SdkResponse response =
        JRaceSdkNM::sdk->closeConnection(connId, static_cast<std::int32_t>(jTimeout));
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_closeConnection: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    destroyLink
 * Signature: (Ljava/lang/String;I)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_destroyLink(JNIEnv *env, jobject,
                                                                jstring jLinkId, jint jTimeout) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_destroyLink: called", "");

    std::string linkId = JavaShimUtils::jstring2string(env, jLinkId);
    SdkResponse response =
        JRaceSdkNM::sdk->destroyLink(linkId, static_cast<std::int32_t>(jTimeout));

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_destroyLink: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    deactivateChannel
 * Signature: (Ljava/lang/String;I)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_deactivateChannel(JNIEnv *env, jobject,
                                                                      jstring jChannelGid,
                                                                      jint jTimeout) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_deactivateChannel: called", "");

    std::string channelGid = JavaShimUtils::jstring2string(env, jChannelGid);
    SdkResponse response =
        JRaceSdkNM::sdk->deactivateChannel(channelGid, static_cast<std::int32_t>(jTimeout));

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_deactivateChannel: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    activateChannel
 * Signature: (Ljava/lang/String;I)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_activateChannel(JNIEnv *env, jobject,
                                                                    jstring jChannelGid,
                                                                    jstring jRoleName,
                                                                    jint jTimeout) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_activateChannel: called", "");

    std::string channelGid = JavaShimUtils::jstring2string(env, jChannelGid);
    std::string roleName = JavaShimUtils::jstring2string(env, jRoleName);
    SdkResponse response =
        JRaceSdkNM::sdk->activateChannel(channelGid, roleName, static_cast<std::int32_t>(jTimeout));

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_activateChannel: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    createLink
 * Signature: (Ljava/lang/String;I)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_createLink(JNIEnv *env, jobject,
                                                               jstring jChannelGid,
                                                               jobjectArray jPersonas,
                                                               jint jTimeout) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_createLink: called", "");

    std::string channelGid = JavaShimUtils::jstring2string(env, jChannelGid);
    std::vector<std::string> personas = JavaShimUtils::jArrayToStringVector(env, jPersonas);
    SdkResponse response =
        JRaceSdkNM::sdk->createLink(channelGid, personas, static_cast<std::int32_t>(jTimeout));

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_createLink: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    createLinkFromAddress
 * Signature: (Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;I)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_createLinkFromAddress(JNIEnv *env, jobject,
                                                                          jstring jChannelGid,
                                                                          jstring jLinkAddress,
                                                                          jobjectArray jPersonas,
                                                                          jint jTimeout) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_createLinkFromAddress: called", "");

    std::string channelGid = JavaShimUtils::jstring2string(env, jChannelGid);
    std::string linkAddress = JavaShimUtils::jstring2string(env, jLinkAddress);
    std::vector<std::string> personas = JavaShimUtils::jArrayToStringVector(env, jPersonas);
    SdkResponse response = JRaceSdkNM::sdk->createLinkFromAddress(
        channelGid, linkAddress, personas, static_cast<std::int32_t>(jTimeout));

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_createLinkFromAddress: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    loadLinkAddress
 * Signature: (Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;I)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_loadLinkAddress(JNIEnv *env, jobject,
                                                                    jstring jChannelGid,
                                                                    jstring jLinkAddress,
                                                                    jobjectArray jPersonas,
                                                                    jint jTimeout) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_loadLinkAddress: called", "");

    std::string channelGid = JavaShimUtils::jstring2string(env, jChannelGid);
    std::string linkAddress = JavaShimUtils::jstring2string(env, jLinkAddress);
    std::vector<std::string> personas = JavaShimUtils::jArrayToStringVector(env, jPersonas);
    SdkResponse response = JRaceSdkNM::sdk->loadLinkAddress(channelGid, linkAddress, personas,
                                                            static_cast<std::int32_t>(jTimeout));

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_loadLinkAddress: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    loadLinkAddresses
 * Signature: (Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/String;I)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_loadLinkAddresses(JNIEnv *env, jobject,
                                                                      jstring jChannelGid,
                                                                      jobjectArray jLinkAddresses,
                                                                      jobjectArray jPersonas,
                                                                      jint jTimeout) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_loadLinkAddresses: called", "");

    std::string channelGid = JavaShimUtils::jstring2string(env, jChannelGid);
    std::vector<std::string> linkAddresses =
        JavaShimUtils::jArrayToStringVector(env, jLinkAddresses);
    std::vector<std::string> personas = JavaShimUtils::jArrayToStringVector(env, jPersonas);
    SdkResponse response = JRaceSdkNM::sdk->loadLinkAddresses(channelGid, linkAddresses, personas,
                                                              static_cast<std::int32_t>(jTimeout));

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_loadLinkAddresses: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    bootstrapDevice
 * Signature:
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_bootstrapDevice(JNIEnv *env, jobject,
                                                                    jobject jHandle,
                                                                    jobjectArray jCommsPlugins) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_bootstrapDevice: called", "");

    RaceHandle handle = JavaShimUtils::jobjectToRaceHandle(env, jHandle);
    std::vector<std::string> commsPlugins = JavaShimUtils::jArrayToStringVector(env, jCommsPlugins);
    SdkResponse response = JRaceSdkNM::sdk->bootstrapDevice(handle, commsPlugins);

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_bootstrapDevice: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    bootstrapFailed
 * Signature:
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_bootstrapFailed(JNIEnv *env, jobject,
                                                                    jobject jHandle) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_bootstrapFailed: called", "");

    RaceHandle handle = JavaShimUtils::jobjectToRaceHandle(env, jHandle);
    SdkResponse response = JRaceSdkNM::sdk->bootstrapFailed(handle);

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_bootstrapFailed: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    sendBootstrapPkg
 * Signature:
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_sendBootstrapPkg(
    JNIEnv *env, jobject, jstring jConnectionId, jstring jPersona, jbyteArray jKey, jint jTimeout) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_sendBootstrapPkg: called", "");

    std::string connectionId = JavaShimUtils::jstring2string(env, jConnectionId);
    std::string persona = JavaShimUtils::jstring2string(env, jPersona);
    RawData key = JavaShimUtils::jByteArrayToRawData(env, jKey);
    SdkResponse response = JRaceSdkNM::sdk->sendBootstrapPkg(connectionId, persona, key,
                                                             static_cast<std::int32_t>(jTimeout));

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_sendBootstrapPkg: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    onPluginStatusChanged
 * Signature: (LShimsJava/PluginStatus)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_onPluginStatusChanged(JNIEnv *env, jobject,
                                                                          jobject jPluginStatus) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_onPluginStatusChanged: called", "");

    PluginStatus pluginStatus = JavaShimUtils::jobjectToPluginStatus(env, jPluginStatus);
    SdkResponse response = JRaceSdkNM::sdk->onPluginStatusChanged(pluginStatus);

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_onPluginStatusChanged: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    onMessageStatusChanged
 * Signature: (LShimsJava/RaceHandle;LShimsJava/MessageStatus;)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_onMessageStatusChanged(JNIEnv *env, jobject,
                                                                           jobject handle,
                                                                           jobject status) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_onMessageStatusChanged: called", "");

    SdkResponse response =
        JRaceSdkNM::sdk->onMessageStatusChanged(JavaShimUtils::jobjectToRaceHandle(env, handle),
                                                JavaShimUtils::jobjectToMessageStatus(env, status));

    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_onMessageStatusChanged: returned", "");
    return JavaShimUtils::sdkResponseToJobject(env, response);
}

/*
 * Class:     ShimsJava_JRaceSdkNM
 * Method:    displayInfoToUser
 * Signature: (Ljava/lang/String;LShimsJava/UserDisplayType;)LShimsJava/SdkResponse;
 */
JNIEXPORT jobject JNICALL Java_ShimsJava_JRaceSdkNM_displayInfoToUser(JNIEnv *env, jobject,
                                                                      jstring jData,
                                                                      jobject jDisplayType) {
    RaceLog::logDebug(logLabel, "Java_ShimsJava_JRaceSdkNM_displayInfoToUser", "");
    std::string data = JavaShimUtils::jstring2string(env, jData);
    RaceEnums::UserDisplayType displayType =
        JavaShimUtils::jobjectToUserDisplayType(env, jDisplayType);

    SdkResponse response = JRaceSdkNM::sdk->displayInfoToUser(data, displayType);
    return JavaShimUtils::sdkResponseToJobject(env, response);
}