
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

#include "JavaIds.h"

jclass JavaIds::jAppConfigClassId = nullptr;
jclass JavaIds::jChannelPropertiesClassId = nullptr;
jclass JavaIds::jChannelRoleClassId = nullptr;
jclass JavaIds::jChannelStatusClassId = nullptr;
jclass JavaIds::jClrMsgClassId = nullptr;
jclass JavaIds::jConnectionStatusClassId = nullptr;
jclass JavaIds::jConnectionTypeClassId = nullptr;
jclass JavaIds::jDeviceInfoClassId = nullptr;
jclass JavaIds::jEncPkgClassId = nullptr;
jclass JavaIds::jLinkDirectionClassId = nullptr;
jclass JavaIds::jLinkPropertiesClassId = nullptr;
jclass JavaIds::jLinkPropertyPairClassId = nullptr;
jclass JavaIds::jLinkPropertySetClassId = nullptr;
jclass JavaIds::jLinkSideClassId = nullptr;
jclass JavaIds::jLinkStatusClassId = nullptr;
jclass JavaIds::jLinkTypeClassId = nullptr;
jclass JavaIds::jMessageStatusClassId = nullptr;
jclass JavaIds::jNodeTypeClassId = nullptr;
jclass JavaIds::jStorageEncryptionTypeClassId = nullptr;
jclass JavaIds::jUserDisplayTypeClassId = nullptr;
jclass JavaIds::jBootstrapActionTypeClassId = nullptr;
jclass JavaIds::jPackageStatusClassId = nullptr;
jclass JavaIds::jPluginConfigClassId = nullptr;
jclass JavaIds::jPluginResponseClassId = nullptr;
jclass JavaIds::jPluginStatusClassId = nullptr;
jclass JavaIds::jRaceAppClassId = nullptr;
jclass JavaIds::jRaceAppUserResponseClassId = nullptr;
jclass JavaIds::jRaceHandleClassId = nullptr;
jclass JavaIds::jRaceSdkAppClassId = nullptr;
jclass JavaIds::jRaceSdkNMClassId = nullptr;
jclass JavaIds::jRaceSdkCommsClassId = nullptr;
jclass JavaIds::jSdkResponseClassId = nullptr;
jclass JavaIds::jSdkStatusClassId = nullptr;
jclass JavaIds::jSendTypeClassId = nullptr;
jclass JavaIds::jStringClassId = nullptr;
jclass JavaIds::jSupportedChannelsClassId = nullptr;
jclass JavaIds::jTransmissionTypeClassId = nullptr;

#ifdef __ANDROID__
jclass JavaIds::androidAppActivityThreadClassId = nullptr;
#endif

jmethodID JavaIds::jAppConfigConstructorMethodId = nullptr;
jmethodID JavaIds::jChannelPropertiesConstructorMethodId = nullptr;
jmethodID JavaIds::jChannelPropertiesGetChannelStatusMethodId = nullptr;
jmethodID JavaIds::jChannelPropertiesGetChannelGidMethodId = nullptr;
jmethodID JavaIds::jChannelPropertiesGetConnectionTypeMethodId = nullptr;
jmethodID JavaIds::jChannelPropertiesGetCreatorExpected = nullptr;
jmethodID JavaIds::jChannelPropertiesGetHintsMethodId = nullptr;
jmethodID JavaIds::jChannelPropertiesGetLinkDirectionMethodId = nullptr;
jmethodID JavaIds::jChannelPropertiesGetLoaderExpected = nullptr;
jmethodID JavaIds::jChannelPropertiesGetSendTypeMethodId = nullptr;
jmethodID JavaIds::jChannelPropertiesGetTransmissionTypeMethodId = nullptr;
jmethodID JavaIds::jChannelRoleConstructorMethodId = nullptr;
jmethodID JavaIds::jChannelRoleGetLinkSideMethodId = nullptr;
jmethodID JavaIds::jChannelStatusValueOfStaticMethodId = nullptr;
jmethodID JavaIds::jClrMsgConstructorMethodId = nullptr;
jmethodID JavaIds::jConnectionStatusValueOfStaticMethodId = nullptr;
jmethodID JavaIds::jConnectionTypeOrdinalMethodId = nullptr;
jmethodID JavaIds::jConnectionTypeValueOfStaticMethodId = nullptr;
jmethodID JavaIds::jDeviceInfoConstructorMethodId = nullptr;
jmethodID JavaIds::jEncPkgConstructorMethodId = nullptr;
jmethodID JavaIds::jLinkDirectionOrdinalMethodId = nullptr;
jmethodID JavaIds::jLinkDirectionValueOfStaticMethodId = nullptr;
jmethodID JavaIds::jLinkPropertiesConstructorMethodId = nullptr;
jmethodID JavaIds::jLinkPropertiesGetBestMethodId = nullptr;
jmethodID JavaIds::jLinkPropertiesGetChannelGidMethodId = nullptr;
jmethodID JavaIds::jLinkPropertiesGetConnectionTypeMethodId = nullptr;
jmethodID JavaIds::jLinkPropertiesGetExpectedMethodId = nullptr;
jmethodID JavaIds::jLinkPropertiesGetHintsMethodId = nullptr;
jmethodID JavaIds::jLinkPropertiesGetLinkAddressMethodId = nullptr;
jmethodID JavaIds::jLinkPropertiesGetLinkTypeMethodId = nullptr;
jmethodID JavaIds::jLinkPropertiesGetSendTypeMethodId = nullptr;
jmethodID JavaIds::jLinkPropertiesGetTransmissionTypeMethodId = nullptr;
jmethodID JavaIds::jLinkPropertiesGetWorstMethodId = nullptr;
jmethodID JavaIds::jLinkPropertyPairConstructorMethodId = nullptr;
jmethodID JavaIds::jLinkPropertySetConstructorMethodId = nullptr;
jmethodID JavaIds::jLinkSideValueOfStaticMethodId = nullptr;
jmethodID JavaIds::jLinkStatusValueOfStaticMethodId = nullptr;
jmethodID JavaIds::jLinkTypeOrdinalMethodId = nullptr;
jmethodID JavaIds::jLinkTypeValueOfStaticMethodId = nullptr;
jmethodID JavaIds::jMessageStatusValueOfStaticMethodId = nullptr;
jmethodID JavaIds::jNodeTypeOrdinalMethodId = nullptr;
jmethodID JavaIds::jNodeTypeValueOfStaticMethodId = nullptr;
jmethodID JavaIds::jStorageEncryptionTypeOrdinalMethodId = nullptr;
jmethodID JavaIds::jStorageEncryptionTypeValueOfStaticMethodId = nullptr;
jmethodID JavaIds::jUserDisplayTypeOrdinalMethodId = nullptr;
jmethodID JavaIds::jUserDisplayTypeValueOfStaticMethodId = nullptr;
jmethodID JavaIds::jBootstrapActionTypeOrdinalMethodId = nullptr;
jmethodID JavaIds::jBootstrapActionTypeValueOfStaticMethodId = nullptr;
jmethodID JavaIds::jPackageStatusValueOfStaticMethodId = nullptr;
jmethodID JavaIds::jPluginConfigConstructorMethodId = nullptr;
jmethodID JavaIds::jPluginResponseValueOfStaticMethodId = nullptr;
jmethodID JavaIds::jPluginStatusValueOfStaticMethodId = nullptr;
jmethodID JavaIds::jRaceHandleConstructorMethodId = nullptr;
jmethodID JavaIds::jSdkResponseConstructorMethodId = nullptr;
jmethodID JavaIds::jSdkStatusValueOfStaticMethodId = nullptr;
jmethodID JavaIds::jSendTypeOrdinalMethodId = nullptr;
jmethodID JavaIds::jSendTypeValueOfStaticMethodId = nullptr;
jmethodID JavaIds::jStringGetBytesMethodId = nullptr;
jmethodID JavaIds::jSupportedChannelsConstructorMethodId = nullptr;
jmethodID JavaIds::jSupportedChannelsPutMethodId = nullptr;
jmethodID JavaIds::jTransmissionTypeValueOfStaticMethodId = nullptr;
jmethodID JavaIds::jRaceAppUserResponseConstructorMethodId = nullptr;
#ifdef __ANDROID__
jmethodID JavaIds::currentActivityThreadStaticMethodId = nullptr;
jmethodID JavaIds::getApplicationMethodId = nullptr;
#endif

jfieldID JavaIds::jAppConfigNodeTypeFieldId = nullptr;
jfieldID JavaIds::jAppConfigEncryptionTypeFieldId = nullptr;
jfieldID JavaIds::jChannelPropertiesDurationFieldId = nullptr;
jfieldID JavaIds::jChannelPropertiesMtuFieldId = nullptr;
jfieldID JavaIds::jChannelPropertiesMultiAddressableFieldId = nullptr;
jfieldID JavaIds::jChannelPropertiesPeriodFieldId = nullptr;
jfieldID JavaIds::jChannelPropertiesReliableFieldId = nullptr;
jfieldID JavaIds::jChannelPropertiesBootstrapFieldId = nullptr;
jfieldID JavaIds::jChannelPropertiesIsFlushableFieldId = nullptr;
jfieldID JavaIds::jChannelPropertiesMaxLinksFieldId = nullptr;
jfieldID JavaIds::jChannelPropertiesCreatorsPerLoaderFieldId = nullptr;
jfieldID JavaIds::jChannelPropertiesLoadersPerCreatorFieldId = nullptr;
jfieldID JavaIds::jChannelPropertiesRolesFieldId = nullptr;
jfieldID JavaIds::jChannelPropertiesCurrentRoleFieldId = nullptr;
jfieldID JavaIds::jChannelPropertiesMaxSendsPerIntervalFieldId = nullptr;
jfieldID JavaIds::jChannelPropertiesSecondsPerIntervalFieldId = nullptr;
jfieldID JavaIds::jChannelPropertiesIntervalEndTimeFieldId = nullptr;
jfieldID JavaIds::jChannelPropertiesSendsRemainingInIntervalFieldId = nullptr;
jfieldID JavaIds::jChannelRoleRoleNameFieldId = nullptr;
jfieldID JavaIds::jChannelRoleMechanicalTagsFieldId = nullptr;
jfieldID JavaIds::jChannelRoleBehavioralTagsFieldId = nullptr;
jfieldID JavaIds::jChannelRoleLinkSideFieldId = nullptr;
jfieldID JavaIds::jChannelStatusValueFieldId = nullptr;
jfieldID JavaIds::jClrMsgcreateTimeFieldId = nullptr;
jfieldID JavaIds::jClrMsgAmpIndexFieldId = nullptr;
jfieldID JavaIds::jClrMsgFromPersonaFieldId = nullptr;
jfieldID JavaIds::jClrMsgNonceFieldId = nullptr;
jfieldID JavaIds::jClrMsgPlainMsgFieldId = nullptr;
jfieldID JavaIds::jClrMsgSpanIdFieldId = nullptr;
jfieldID JavaIds::jClrMsgToPersonaFieldId = nullptr;
jfieldID JavaIds::jClrMsgTraceIdFieldId = nullptr;
jfieldID JavaIds::jConnectionStatusValueFieldId = nullptr;
jfieldID JavaIds::jDeviceInfoArchitectureFieldId;
jfieldID JavaIds::jDeviceInfoNodeTypeFieldId;
jfieldID JavaIds::jDeviceInfoPlatformFieldId;
jfieldID JavaIds::jEncPkgCipherTextFieldId = nullptr;
jfieldID JavaIds::jEncPkgPackageTypeByteFieldId = nullptr;
jfieldID JavaIds::jEncPkgSpanIdFieldId = nullptr;
jfieldID JavaIds::jEncPkgTraceIdFieldId = nullptr;
jfieldID JavaIds::jLinkPropertiesDurationFieldId = nullptr;
jfieldID JavaIds::jLinkPropertiesMtuFieldId = nullptr;
jfieldID JavaIds::jLinkPropertiesPeriodFieldId = nullptr;
jfieldID JavaIds::jLinkPropertiesReliableFieldId = nullptr;
jfieldID JavaIds::jLinkPropertyPairReceiveFieldId = nullptr;
jfieldID JavaIds::jLinkPropertyPairSendFieldId = nullptr;
jfieldID JavaIds::jLinkPropertySetBandwidthBitsPSFieldId = nullptr;
jfieldID JavaIds::jLinkPropertySetLatencyMsFieldId = nullptr;
jfieldID JavaIds::jLinkPropertySetLossFieldId = nullptr;
jfieldID JavaIds::jLinkStatusValueFieldId = nullptr;
jfieldID JavaIds::jMessageStatusValueFieldId = nullptr;
jfieldID JavaIds::jPackageStatusValueFieldId = nullptr;
jfieldID JavaIds::jPluginConfigAuxDataDirectoryFieldId = nullptr;
jfieldID JavaIds::jPluginConfigEtcDirectoryFieldId = nullptr;
jfieldID JavaIds::jPluginConfigLoggingDirectoryFieldId = nullptr;
jfieldID JavaIds::jPluginConfigTmpDirectoryFieldId = nullptr;
jfieldID JavaIds::jPluginConfigPluginDirectoryFieldId = nullptr;
jfieldID JavaIds::jPluginResponseValueFieldId = nullptr;
jfieldID JavaIds::jPluginStatusValueFieldId = nullptr;
jfieldID JavaIds::jRaceAppWrapperPointerFieldId = nullptr;
jfieldID JavaIds::jRaceHandleValueFieldId = nullptr;
jfieldID JavaIds::jRaceSdkAppSdkPointerFieldId = nullptr;
jfieldID JavaIds::jRaceSdkCommsSdkPointerFieldId = nullptr;
jfieldID JavaIds::jSdkResponseHandleFieldId = nullptr;
jfieldID JavaIds::jSdkResponseQueueUtilizationFieldId = nullptr;
jfieldID JavaIds::jSdkResponseSdkStatusFieldId = nullptr;
jfieldID JavaIds::jSdkStatusValueFieldId = nullptr;

static std::unordered_set<jclass *> cachedClassIds;
static std::unordered_set<jmethodID *> cachedMethodIds;
static std::unordered_set<jfieldID *> cachedFieldIds;

void getAndCacheClassID(JNIEnv *env, const char *javaClassName, jclass *classId) {
    if (*classId != nullptr) {
        return;
    }

    jclass tempLocalClassRef = JavaIds::getClassID(env, javaClassName);

    if (tempLocalClassRef == nullptr) {
        throw std::runtime_error("getAndCacheClassID: failed to find class: " +
                                 std::string(javaClassName));
    }
    *classId = static_cast<jclass>(env->NewGlobalRef(tempLocalClassRef));
    env->DeleteLocalRef(tempLocalClassRef);
    cachedClassIds.insert(classId);
}

void getAndCacheMethodID(JNIEnv *env, jclass classId, const char *name, const char *sig,
                         jmethodID *methodId) {
    if (*methodId != nullptr) {
        return;
    }

    *methodId = JavaIds::getMethodID(env, classId, name, sig);
    cachedMethodIds.insert(methodId);
}

void getStaticMethodID(JNIEnv *env, jclass classId, const char *name, const char *sig,
                       jmethodID *methodId) {
    if (*methodId != nullptr) {
        return;
    }
    *methodId = env->GetStaticMethodID(classId, name, sig);
    if (*methodId == nullptr) {
        throw std::runtime_error(
            "getStaticMethodID: failed to get static method ID: method name: " + std::string(name) +
            " signature: " + std::string(sig));
    }
    cachedMethodIds.insert(methodId);
}

void getFieldID(JNIEnv *env, jclass classId, const char *fieldName, const char *sig,
                jfieldID *fieldId) {
    if (*fieldId != nullptr) {
        return;
    }
    *fieldId = env->GetFieldID(classId, fieldName, sig);
    if (*fieldId == nullptr) {
        throw std::runtime_error("getFieldID: failed to get field ID: field name: " +
                                 std::string(fieldName) + " signature: " + std::string(sig));
    }
    cachedFieldIds.insert(fieldId);
}

void JavaIds::load(JNIEnv *env) {
    getAndCacheClassID(env, "java/lang/String", &jStringClassId);
    getAndCacheClassID(env, "java/util/HashMap", &jSupportedChannelsClassId);
    getAndCacheClassID(env, "ShimsJava/AppConfig", &jAppConfigClassId);
    getAndCacheClassID(env, "ShimsJava/ChannelRole", &jChannelRoleClassId);
    getAndCacheClassID(env, "ShimsJava/ChannelStatus", &jChannelStatusClassId);
    getAndCacheClassID(env, "ShimsJava/ConnectionStatus", &jConnectionStatusClassId);
    getAndCacheClassID(env, "ShimsJava/ConnectionType", &jConnectionTypeClassId);
    getAndCacheClassID(env, "ShimsJava/DeviceInfo", &jDeviceInfoClassId);
    getAndCacheClassID(env, "ShimsJava/JChannelProperties", &jChannelPropertiesClassId);
    getAndCacheClassID(env, "ShimsJava/JClrMsg", &jClrMsgClassId);
    getAndCacheClassID(env, "ShimsJava/JEncPkg", &jEncPkgClassId);
    getAndCacheClassID(env, "ShimsJava/JLinkProperties", &jLinkPropertiesClassId);
    getAndCacheClassID(env, "ShimsJava/RaceSdkApp", &jRaceSdkAppClassId);
    getAndCacheClassID(env, "ShimsJava/JRaceSdkNM", &jRaceSdkNMClassId);
    getAndCacheClassID(env, "ShimsJava/JRaceSdkComms", &jRaceSdkCommsClassId);
    getAndCacheClassID(env, "ShimsJava/LinkDirection", &jLinkDirectionClassId);
    getAndCacheClassID(env, "ShimsJava/LinkPropertyPair", &jLinkPropertyPairClassId);
    getAndCacheClassID(env, "ShimsJava/LinkPropertySet", &jLinkPropertySetClassId);
    getAndCacheClassID(env, "ShimsJava/LinkSide", &jLinkSideClassId);
    getAndCacheClassID(env, "ShimsJava/LinkStatus", &jLinkStatusClassId);
    getAndCacheClassID(env, "ShimsJava/LinkType", &jLinkTypeClassId);
    getAndCacheClassID(env, "ShimsJava/MessageStatus", &jMessageStatusClassId);
    getAndCacheClassID(env, "ShimsJava/NodeType", &jNodeTypeClassId);
    getAndCacheClassID(env, "ShimsJava/StorageEncryptionType", &jStorageEncryptionTypeClassId);
    getAndCacheClassID(env, "ShimsJava/UserDisplayType", &jUserDisplayTypeClassId);
    getAndCacheClassID(env, "ShimsJava/BootstrapActionType", &jBootstrapActionTypeClassId);
    getAndCacheClassID(env, "ShimsJava/PackageStatus", &jPackageStatusClassId);
    getAndCacheClassID(env, "ShimsJava/PluginConfig", &jPluginConfigClassId);
    getAndCacheClassID(env, "ShimsJava/PluginResponse", &jPluginResponseClassId);
    getAndCacheClassID(env, "ShimsJava/PluginStatus", &jPluginStatusClassId);
    getAndCacheClassID(env, "ShimsJava/RaceApp", &jRaceAppClassId);
    getAndCacheClassID(env, "ShimsJava/RaceApp$UserResponse", &jRaceAppUserResponseClassId);
    getAndCacheClassID(env, "ShimsJava/RaceHandle", &jRaceHandleClassId);
    getAndCacheClassID(env, "ShimsJava/SdkResponse", &jSdkResponseClassId);
    getAndCacheClassID(env, "ShimsJava/SdkResponse$SdkStatus", &jSdkStatusClassId);
    getAndCacheClassID(env, "ShimsJava/SendType", &jSendTypeClassId);
    getAndCacheClassID(env, "ShimsJava/TransmissionType", &jTransmissionTypeClassId);
#ifdef __ANDROID__
    getAndCacheClassID(env, "android/app/ActivityThread", &androidAppActivityThreadClassId);
#endif

    // Docs for method signatures:
    //     https://docs.oracle.com/javase/1.5.0/docs/guide/jni/spec/types.html#wp276
    // clang-format off
    // You can also run this command to get method signatures:
    //     javap -v -classpath build/LINUX_x86_64/java-shims/ShimsJava/racesdk-java-shims-1.jar ShimsJava.<some Java class>
    // clang-format on

    getAndCacheMethodID(env, jAppConfigClassId, "<init>", "()V", &jAppConfigConstructorMethodId);
    getAndCacheMethodID(env, jLinkPropertyPairClassId, "<init>",
                        "(LShimsJava/LinkPropertySet;LShimsJava/LinkPropertySet;)V",
                        &jLinkPropertyPairConstructorMethodId);
    getAndCacheMethodID(env, jSupportedChannelsClassId, "<init>", "()V",
                        &jSupportedChannelsConstructorMethodId);
    getAndCacheMethodID(env, jSupportedChannelsClassId, "put",
                        "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;",
                        &jSupportedChannelsPutMethodId);
    getAndCacheMethodID(env, jLinkPropertySetClassId, "<init>", "(IIF)V",
                        &jLinkPropertySetConstructorMethodId);
    getAndCacheMethodID(env, jChannelPropertiesClassId, "<init>",
                        "(LShimsJava/ChannelStatus;LShimsJava/LinkPropertyPair;LShimsJava/"
                        "LinkPropertyPair;ZZZZ[Ljava/lang/"
                        "String;Ljava/lang/String;LShimsJava/LinkDirection;LShimsJava/"
                        "TransmissionType;LShimsJava/ConnectionType;LShimsJava/"
                        "SendType;IIIIII[LShimsJava/ChannelRole;LShimsJava/ChannelRole;IIJI)V",
                        &jChannelPropertiesConstructorMethodId);
    getAndCacheMethodID(env, jChannelPropertiesClassId, "getChannelStatusAsInt", "()I",
                        &jChannelPropertiesGetChannelStatusMethodId);
    getAndCacheMethodID(env, jChannelPropertiesClassId, "getLinkDirectionAsInt", "()I",
                        &jChannelPropertiesGetLinkDirectionMethodId);
    getAndCacheMethodID(env, jChannelPropertiesClassId, "getConnectionTypeAsInt", "()I",
                        &jChannelPropertiesGetConnectionTypeMethodId);
    getAndCacheMethodID(env, jChannelPropertiesClassId, "getSendTypeAsInt", "()I",
                        &jChannelPropertiesGetSendTypeMethodId);
    getAndCacheMethodID(env, jChannelPropertiesClassId, "getTransmissionTypeAsInt", "()I",
                        &jChannelPropertiesGetTransmissionTypeMethodId);
    getAndCacheMethodID(env, jChannelPropertiesClassId, "getCreatorExpected",
                        "()LShimsJava/LinkPropertyPair;", &jChannelPropertiesGetCreatorExpected);
    getAndCacheMethodID(env, jChannelPropertiesClassId, "getLoaderExpected",
                        "()LShimsJava/LinkPropertyPair;", &jChannelPropertiesGetLoaderExpected);
    getAndCacheMethodID(env, jChannelPropertiesClassId, "getSupportedHints",
                        "()[Ljava/lang/Object;", &jChannelPropertiesGetHintsMethodId);
    getAndCacheMethodID(env, jChannelPropertiesClassId, "getChannelGid", "()Ljava/lang/String;",
                        &jChannelPropertiesGetChannelGidMethodId);
    getAndCacheMethodID(
        env, jChannelRoleClassId, "<init>",
        "(Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/String;LShimsJava/LinkSide;)V",
        &jChannelRoleConstructorMethodId);
    getAndCacheMethodID(env, jChannelRoleClassId, "getLinkSideAsInt", "()I",
                        &jChannelRoleGetLinkSideMethodId);
    getAndCacheMethodID(
        env, jLinkPropertiesClassId, "<init>",
        "(LShimsJava/LinkPropertyPair;LShimsJava/LinkPropertyPair;LShimsJava/"
        "LinkPropertyPair;ZZLjava/lang/String;Ljava/lang/String;[Ljava/lang/String;LShimsJava/"
        "LinkType;LShimsJava/TransmissionType;LShimsJava/ConnectionType;LShimsJava/SendType;III)V",
        &jLinkPropertiesConstructorMethodId);
    getAndCacheMethodID(env, jLinkPropertiesClassId, "getLinkTypeAsInt", "()I",
                        &jLinkPropertiesGetLinkTypeMethodId);
    getAndCacheMethodID(env, jLinkPropertiesClassId, "getConnectionTypeAsInt", "()I",
                        &jLinkPropertiesGetConnectionTypeMethodId);
    getAndCacheMethodID(env, jLinkPropertiesClassId, "getSendTypeAsInt", "()I",
                        &jLinkPropertiesGetSendTypeMethodId);
    getAndCacheMethodID(env, jLinkPropertiesClassId, "getTransmissionTypeAsInt", "()I",
                        &jLinkPropertiesGetTransmissionTypeMethodId);
    getAndCacheMethodID(env, jLinkPropertiesClassId, "getWorst", "()LShimsJava/LinkPropertyPair;",
                        &jLinkPropertiesGetWorstMethodId);
    getAndCacheMethodID(env, jLinkPropertiesClassId, "getBest", "()LShimsJava/LinkPropertyPair;",
                        &jLinkPropertiesGetBestMethodId);
    getAndCacheMethodID(env, jLinkPropertiesClassId, "getExpected",
                        "()LShimsJava/LinkPropertyPair;", &jLinkPropertiesGetExpectedMethodId);
    getAndCacheMethodID(env, jLinkPropertiesClassId, "getSupportedHints", "()[Ljava/lang/Object;",
                        &jLinkPropertiesGetHintsMethodId);
    getAndCacheMethodID(env, jLinkPropertiesClassId, "getChannelGid", "()Ljava/lang/String;",
                        &jLinkPropertiesGetChannelGidMethodId);
    getAndCacheMethodID(env, jLinkPropertiesClassId, "getLinkAddress", "()Ljava/lang/String;",
                        &jLinkPropertiesGetLinkAddressMethodId);
    getAndCacheMethodID(env, jEncPkgClassId, "<init>", "(JJ[BB)V", &jEncPkgConstructorMethodId);
    getAndCacheMethodID(env, jClrMsgClassId, "<init>",
                        "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;JIBJJ)V",
                        &jClrMsgConstructorMethodId);
    getAndCacheMethodID(env, jDeviceInfoClassId, "<init>",
                        "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
                        &jDeviceInfoConstructorMethodId);
    getAndCacheMethodID(env, jPluginConfigClassId, "<init>",
                        "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/"
                        "String;Ljava/lang/String;)V",
                        &jPluginConfigConstructorMethodId);
    getAndCacheMethodID(env, jSdkResponseClassId, "<init>",
                        "(LShimsJava/SdkResponse$SdkStatus;DLShimsJava/RaceHandle;)V",
                        &jSdkResponseConstructorMethodId);
    getAndCacheMethodID(env, jRaceHandleClassId, "<init>", "(J)V", &jRaceHandleConstructorMethodId);
    getAndCacheMethodID(env, jSendTypeClassId, "ordinal", "()I", &jSendTypeOrdinalMethodId);
    getAndCacheMethodID(env, jConnectionTypeClassId, "ordinal", "()I",
                        &jConnectionTypeOrdinalMethodId);
    getAndCacheMethodID(env, jLinkDirectionClassId, "ordinal", "()I",
                        &jLinkDirectionOrdinalMethodId);
    getAndCacheMethodID(env, jLinkTypeClassId, "ordinal", "()I", &jLinkTypeOrdinalMethodId);
    getAndCacheMethodID(env, jNodeTypeClassId, "ordinal", "()I", &jNodeTypeOrdinalMethodId);
    getAndCacheMethodID(env, jStorageEncryptionTypeClassId, "ordinal", "()I",
                        &jStorageEncryptionTypeOrdinalMethodId);
    getAndCacheMethodID(env, jUserDisplayTypeClassId, "ordinal", "()I",
                        &jUserDisplayTypeOrdinalMethodId);
    getAndCacheMethodID(env, jBootstrapActionTypeClassId, "ordinal", "()I",
                        &jBootstrapActionTypeOrdinalMethodId);
    getAndCacheMethodID(env, jStringClassId, "getBytes", "(Ljava/lang/String;)[B",
                        &jStringGetBytesMethodId);
    getAndCacheMethodID(env, jRaceAppUserResponseClassId, "<init>", "(ZLjava/lang/String;)V",
                        &jRaceAppUserResponseConstructorMethodId);
#ifdef __ANDROID__
    getStaticMethodID(env, androidAppActivityThreadClassId, "currentActivityThread",
                      "()Landroid/app/ActivityThread;", &currentActivityThreadStaticMethodId);
    getAndCacheMethodID(env, androidAppActivityThreadClassId, "getApplication",
                        "()Landroid/app/Application;", &getApplicationMethodId);
#endif
    getStaticMethodID(env, jLinkSideClassId, "valueOf", "(I)LShimsJava/LinkSide;",
                      &jLinkSideValueOfStaticMethodId);
    getStaticMethodID(env, jLinkTypeClassId, "valueOf", "(I)LShimsJava/LinkType;",
                      &jLinkTypeValueOfStaticMethodId);
    getStaticMethodID(env, jNodeTypeClassId, "valueOf", "(I)LShimsJava/NodeType;",
                      &jNodeTypeValueOfStaticMethodId);
    getStaticMethodID(env, jStorageEncryptionTypeClassId, "valueOf",
                      "(I)LShimsJava/StorageEncryptionType;",
                      &jStorageEncryptionTypeValueOfStaticMethodId);
    getStaticMethodID(env, jUserDisplayTypeClassId, "valueOf", "(I)LShimsJava/UserDisplayType;",
                      &jUserDisplayTypeValueOfStaticMethodId);
    getStaticMethodID(env, jBootstrapActionTypeClassId, "valueOf",
                      "(I)LShimsJava/BootstrapActionType;",
                      &jBootstrapActionTypeValueOfStaticMethodId);
    getStaticMethodID(env, jLinkDirectionClassId, "valueOf", "(I)LShimsJava/LinkDirection;",
                      &jLinkDirectionValueOfStaticMethodId);
    getStaticMethodID(env, jTransmissionTypeClassId, "valueOf", "(I)LShimsJava/TransmissionType;",
                      &jTransmissionTypeValueOfStaticMethodId);
    getStaticMethodID(env, jConnectionTypeClassId, "valueOf", "(I)LShimsJava/ConnectionType;",
                      &jConnectionTypeValueOfStaticMethodId);
    getStaticMethodID(env, jSendTypeClassId, "valueOf", "(I)LShimsJava/SendType;",
                      &jSendTypeValueOfStaticMethodId);
    getStaticMethodID(env, jSdkStatusClassId, "valueOf", "(I)LShimsJava/SdkResponse$SdkStatus;",
                      &jSdkStatusValueOfStaticMethodId);
    getStaticMethodID(env, jPackageStatusClassId, "valueOf", "(I)LShimsJava/PackageStatus;",
                      &jPackageStatusValueOfStaticMethodId);
    getStaticMethodID(env, jConnectionStatusClassId, "valueOf", "(I)LShimsJava/ConnectionStatus;",
                      &jConnectionStatusValueOfStaticMethodId);
    getStaticMethodID(env, jLinkStatusClassId, "valueOf", "(I)LShimsJava/LinkStatus;",
                      &jLinkStatusValueOfStaticMethodId);
    getStaticMethodID(env, jChannelStatusClassId, "valueOf", "(I)LShimsJava/ChannelStatus;",
                      &jChannelStatusValueOfStaticMethodId);
    getStaticMethodID(env, jPluginStatusClassId, "valueOf", "(I)LShimsJava/PluginStatus;",
                      &jPluginStatusValueOfStaticMethodId);
    getStaticMethodID(env, jPluginResponseClassId, "valueOf", "(I)LShimsJava/PluginResponse;",
                      &jPluginResponseValueOfStaticMethodId);
    getStaticMethodID(env, jMessageStatusClassId, "valueOf", "(I)LShimsJava/MessageStatus;",
                      &jMessageStatusValueOfStaticMethodId);

    getFieldID(env, jAppConfigClassId, "nodeType", "LShimsJava/NodeType;",
               &jAppConfigNodeTypeFieldId);
    getFieldID(env, jAppConfigClassId, "encryptionType", "LShimsJava/StorageEncryptionType;",
               &jAppConfigEncryptionTypeFieldId);
    getFieldID(env, jLinkPropertySetClassId, "loss", "F", &jLinkPropertySetLossFieldId);

    getFieldID(env, jDeviceInfoClassId, "nodeType", "Ljava/lang/String;",
               &jDeviceInfoPlatformFieldId);
    getFieldID(env, jDeviceInfoClassId, "architecture", "Ljava/lang/String;",
               &jDeviceInfoArchitectureFieldId);
    getFieldID(env, jDeviceInfoClassId, "platform", "Ljava/lang/String;",
               &jDeviceInfoNodeTypeFieldId);
    getFieldID(env, jPluginConfigClassId, "etcDirectory", "Ljava/lang/String;",
               &jPluginConfigEtcDirectoryFieldId);
    getFieldID(env, jPluginConfigClassId, "loggingDirectory", "Ljava/lang/String;",
               &jPluginConfigLoggingDirectoryFieldId);
    getFieldID(env, jPluginConfigClassId, "auxDataDirectory", "Ljava/lang/String;",
               &jPluginConfigAuxDataDirectoryFieldId);
    getFieldID(env, jPluginConfigClassId, "tmpDirectory", "Ljava/lang/String;",
               &jPluginConfigTmpDirectoryFieldId);
    getFieldID(env, jPluginConfigClassId, "pluginDirectory", "Ljava/lang/String;",
               &jPluginConfigPluginDirectoryFieldId);
    getFieldID(env, jClrMsgClassId, "plainMsg", "Ljava/lang/String;", &jClrMsgPlainMsgFieldId);
    getFieldID(env, jClrMsgClassId, "fromPersona", "Ljava/lang/String;",
               &jClrMsgFromPersonaFieldId);
    getFieldID(env, jClrMsgClassId, "toPersona", "Ljava/lang/String;", &jClrMsgToPersonaFieldId);
    getFieldID(env, jLinkPropertiesClassId, "reliable", "Z", &jLinkPropertiesReliableFieldId);
    getFieldID(env, jChannelPropertiesClassId, "reliable", "Z", &jChannelPropertiesReliableFieldId);
    getFieldID(env, jChannelPropertiesClassId, "bootstrap", "Z",
               &jChannelPropertiesBootstrapFieldId);
    getFieldID(env, jChannelPropertiesClassId, "isFlushable", "Z",
               &jChannelPropertiesIsFlushableFieldId);
    getFieldID(env, jChannelPropertiesClassId, "multiAddressable", "Z",
               &jChannelPropertiesMultiAddressableFieldId);
    getFieldID(env, jEncPkgClassId, "packageTypeByte", "B", &jEncPkgPackageTypeByteFieldId);
    getFieldID(env, jClrMsgClassId, "ampIndex", "B", &jClrMsgAmpIndexFieldId);
    getFieldID(env, jClrMsgClassId, "createTime", "J", &jClrMsgcreateTimeFieldId);
    getFieldID(env, jClrMsgClassId, "nonce", "I", &jClrMsgNonceFieldId);
    getFieldID(env, jClrMsgClassId, "traceId", "J", &jClrMsgTraceIdFieldId);
    getFieldID(env, jClrMsgClassId, "spanId", "J", &jClrMsgSpanIdFieldId);
    getFieldID(env, jEncPkgClassId, "traceId", "J", &jEncPkgTraceIdFieldId);
    getFieldID(env, jEncPkgClassId, "spanId", "J", &jEncPkgSpanIdFieldId);
    getFieldID(env, jRaceHandleClassId, "value", "J", &jRaceHandleValueFieldId);
    getFieldID(env, jSdkStatusClassId, "value", "I", &jSdkStatusValueFieldId);
    getFieldID(env, jPackageStatusClassId, "value", "I", &jPackageStatusValueFieldId);
    getFieldID(env, jConnectionStatusClassId, "value", "I", &jConnectionStatusValueFieldId);
    getFieldID(env, jLinkStatusClassId, "value", "I", &jLinkStatusValueFieldId);
    getFieldID(env, jChannelStatusClassId, "value", "I", &jChannelStatusValueFieldId);
    getFieldID(env, jPluginStatusClassId, "value", "I", &jPluginStatusValueFieldId);
    getFieldID(env, jPluginResponseClassId, "value", "I", &jPluginResponseValueFieldId);
    getFieldID(env, jMessageStatusClassId, "value", "I", &jMessageStatusValueFieldId);
    getFieldID(env, jLinkPropertiesClassId, "duration", "I", &jLinkPropertiesDurationFieldId);
    getFieldID(env, jLinkPropertiesClassId, "period", "I", &jLinkPropertiesPeriodFieldId);
    getFieldID(env, jLinkPropertiesClassId, "mtu", "I", &jLinkPropertiesMtuFieldId);
    getFieldID(env, jChannelPropertiesClassId, "duration", "I", &jChannelPropertiesDurationFieldId);
    getFieldID(env, jChannelPropertiesClassId, "period", "I", &jChannelPropertiesPeriodFieldId);
    getFieldID(env, jChannelPropertiesClassId, "mtu", "I", &jChannelPropertiesMtuFieldId);
    getFieldID(env, jChannelPropertiesClassId, "maxLinks", "I", &jChannelPropertiesMaxLinksFieldId);
    getFieldID(env, jChannelPropertiesClassId, "creatorsPerLoader", "I",
               &jChannelPropertiesCreatorsPerLoaderFieldId);
    getFieldID(env, jChannelPropertiesClassId, "loadersPerCreator", "I",
               &jChannelPropertiesLoadersPerCreatorFieldId);
    getFieldID(env, jChannelPropertiesClassId, "roles", "[LShimsJava/ChannelRole;",
               &jChannelPropertiesRolesFieldId);
    getFieldID(env, jChannelPropertiesClassId, "currentRole", "LShimsJava/ChannelRole;",
               &jChannelPropertiesCurrentRoleFieldId);
    getFieldID(env, jChannelPropertiesClassId, "maxSendsPerInterval", "I",
               &jChannelPropertiesMaxSendsPerIntervalFieldId);
    getFieldID(env, jChannelPropertiesClassId, "secondsPerInterval", "I",
               &jChannelPropertiesSecondsPerIntervalFieldId);
    getFieldID(env, jChannelPropertiesClassId, "intervalEndTime", "J",
               &jChannelPropertiesIntervalEndTimeFieldId);
    getFieldID(env, jChannelPropertiesClassId, "sendsRemainingInInterval", "I",
               &jChannelPropertiesSendsRemainingInIntervalFieldId);
    getFieldID(env, jChannelRoleClassId, "roleName", "Ljava/lang/String;",
               &jChannelRoleRoleNameFieldId);
    getFieldID(env, jChannelRoleClassId, "mechanicalTags", "[Ljava/lang/String;",
               &jChannelRoleMechanicalTagsFieldId);
    getFieldID(env, jChannelRoleClassId, "behavioralTags", "[Ljava/lang/String;",
               &jChannelRoleBehavioralTagsFieldId);
    getFieldID(env, jChannelRoleClassId, "linkSide", "LShimsJava/LinkSide;",
               &jChannelRoleLinkSideFieldId);
    getFieldID(env, jLinkPropertySetClassId, "bandwidthBitsPS", "I",
               &jLinkPropertySetBandwidthBitsPSFieldId);
    getFieldID(env, jLinkPropertySetClassId, "latencyMs", "I", &jLinkPropertySetLatencyMsFieldId);
    getFieldID(env, jSdkResponseClassId, "queueUtilization", "D",
               &jSdkResponseQueueUtilizationFieldId);
    getFieldID(env, jSdkResponseClassId, "status", "LShimsJava/SdkResponse$SdkStatus;",
               &jSdkResponseSdkStatusFieldId);
    getFieldID(env, jSdkResponseClassId, "handle", "LShimsJava/RaceHandle;",
               &jSdkResponseHandleFieldId);
    getFieldID(env, jEncPkgClassId, "cipherText", "[B", &jEncPkgCipherTextFieldId);
    getFieldID(env, jLinkPropertyPairClassId, "send", "LShimsJava/LinkPropertySet;",
               &jLinkPropertyPairSendFieldId);
    getFieldID(env, jLinkPropertyPairClassId, "receive", "LShimsJava/LinkPropertySet;",
               &jLinkPropertyPairReceiveFieldId);
    getFieldID(env, jRaceAppClassId, "raceAppWrapperPtr", "J", &jRaceAppWrapperPointerFieldId);
    getFieldID(env, jRaceSdkAppClassId, "nativePtr", "J", &jRaceSdkAppSdkPointerFieldId);
    getFieldID(env, jRaceSdkCommsClassId, "sdkPointer", "J", &jRaceSdkCommsSdkPointerFieldId);
}

#ifdef __ANDROID__
jobject JavaIds::getGlobalContext(JNIEnv *env) {
    jclass activityThread = env->FindClass("android/app/ActivityThread");
    jmethodID currentActivityThread = env->GetStaticMethodID(
        activityThread, "currentActivityThread", "()Landroid/app/ActivityThread;");
    jobject at = env->CallStaticObjectMethod(activityThread, currentActivityThread);

    jmethodID getApplication =
        env->GetMethodID(activityThread, "getApplication", "()Landroid/app/Application;");
    jobject context = env->CallObjectMethod(at, getApplication);
    return context;
}
#endif

jclass JavaIds::getClassID(JNIEnv *env, const char *javaClassName) {
#ifdef __ANDROID__
    // get android class loader
    jobject jAndroidContext = JavaIds::getGlobalContext(env);
    jclass jContextClass = env->FindClass("android/content/Context");
    jmethodID jGetClassLoader =
        env->GetMethodID(jContextClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject jClassLoaderObject = env->CallObjectMethod(jAndroidContext, jGetClassLoader);
    jclass jClassLoaderClass = env->FindClass("java/lang/ClassLoader");
    jmethodID jLoadClass =
        env->GetMethodID(jClassLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");

    // Find java plugin
    jstring jClassName = env->NewStringUTF(javaClassName);
    return jclass(env->CallObjectMethod(jClassLoaderObject, jLoadClass, jClassName));
#else
    return env->FindClass(javaClassName);
#endif
}

jmethodID JavaIds::getMethodID(JNIEnv *env, jclass classId, const char *name, const char *sig) {
    jmethodID methodId = env->GetMethodID(classId, name, sig);
    if (methodId == nullptr) {
        throw std::runtime_error("getMethodID: failed to get method ID: method name: " +
                                 std::string(name) + " signature: " + std::string(sig));
    }
    return methodId;
}

void JavaIds::unload(JNIEnv *env) {
    for (auto javaClass : cachedClassIds) {
        env->DeleteGlobalRef(*javaClass);
        *javaClass = nullptr;
    }
    cachedClassIds.clear();

    for (auto methodId : cachedMethodIds) {
        *methodId = nullptr;
    }
    cachedMethodIds.clear();
}
