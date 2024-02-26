
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

#ifndef __JAVA_SHIMS_SHIMSCPP_SOURCE_JAVAIDS_H__
#define __JAVA_SHIMS_SHIMSCPP_SOURCE_JAVAIDS_H__

#include <jni.h>

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace JavaIds {

void load(JNIEnv *env);
#ifdef __ANDROID__
jobject getGlobalContext(JNIEnv *env);
#endif
jclass getClassID(JNIEnv *env, const char *javaClassName);
jmethodID getMethodID(JNIEnv *env, jclass classId, const char *name, const char *sig);
void unload(JNIEnv *env);

// Class IDs
extern jclass jAppConfigClassId;
extern jclass jChannelPropertiesClassId;
extern jclass jChannelRoleClassId;
extern jclass jChannelStatusClassId;
extern jclass jClrMsgClassId;
extern jclass jConnectionStatusClassId;
extern jclass jConnectionTypeClassId;
extern jclass jDeviceInfoClassId;
extern jclass jEncPkgClassId;
extern jclass jLinkDirectionClassId;
extern jclass jLinkPropertiesClassId;
extern jclass jLinkPropertyPairClassId;
extern jclass jLinkPropertySetClassId;
extern jclass jLinkSideClassId;
extern jclass jLinkStatusClassId;
extern jclass jLinkTypeClassId;
extern jclass jMessageStatusClassId;
extern jclass jNodeTypeClassId;
extern jclass jStorageEncryptionTypeClassId;
extern jclass jUserDisplayTypeClassId;
extern jclass jBootstrapActionTypeClassId;
extern jclass jPackageStatusClassId;
extern jclass jPluginConfigClassId;
extern jclass jPluginResponseClassId;
extern jclass jPluginStatusClassId;
extern jclass jRaceAppClassId;
extern jclass jRaceAppUserResponseClassId;
extern jclass jRaceHandleClassId;
extern jclass jRaceSdkAppClassId;
extern jclass jRaceSdkNMClassId;
extern jclass jRaceSdkCommsClassId;
extern jclass jSdkResponseClassId;
extern jclass jSdkStatusClassId;
extern jclass jSendTypeClassId;
extern jclass jStringClassId;
extern jclass jSupportedChannelsClassId;
extern jclass jTransmissionTypeClassId;

#ifdef __ANDROID__
extern jclass androidAppActivityThreadClassId;
#endif

// Method IDs
extern jmethodID jAppConfigConstructorMethodId;
extern jmethodID jChannelPropertiesConstructorMethodId;
extern jmethodID jChannelPropertiesGetChannelStatusMethodId;
extern jmethodID jChannelPropertiesGetChannelGidMethodId;
extern jmethodID jChannelPropertiesGetConnectionTypeMethodId;
extern jmethodID jChannelPropertiesGetCreatorExpected;
extern jmethodID jChannelPropertiesGetHintsMethodId;
extern jmethodID jChannelPropertiesGetLinkDirectionMethodId;
extern jmethodID jChannelPropertiesGetLoaderExpected;
extern jmethodID jChannelPropertiesGetSendTypeMethodId;
extern jmethodID jChannelPropertiesGetTransmissionTypeMethodId;
extern jmethodID jChannelRoleConstructorMethodId;
extern jmethodID jChannelRoleGetLinkSideMethodId;
extern jmethodID jChannelStatusValueOfStaticMethodId;
extern jmethodID jClrMsgConstructorMethodId;
extern jmethodID jConnectionStatusValueOfStaticMethodId;
extern jmethodID jConnectionTypeOrdinalMethodId;
extern jmethodID jConnectionTypeValueOfStaticMethodId;
extern jmethodID jDeviceInfoConstructorMethodId;
extern jmethodID jEncPkgConstructorMethodId;
extern jmethodID jLinkDirectionOrdinalMethodId;
extern jmethodID jLinkDirectionValueOfStaticMethodId;
extern jmethodID jLinkPropertiesConstructorMethodId;
extern jmethodID jLinkPropertiesGetBestMethodId;
extern jmethodID jLinkPropertiesGetChannelGidMethodId;
extern jmethodID jLinkPropertiesGetConnectionTypeMethodId;
extern jmethodID jLinkPropertiesGetExpectedMethodId;
extern jmethodID jLinkPropertiesGetHintsMethodId;
extern jmethodID jLinkPropertiesGetLinkAddressMethodId;
extern jmethodID jLinkPropertiesGetLinkTypeMethodId;
extern jmethodID jLinkPropertiesGetSendTypeMethodId;
extern jmethodID jLinkPropertiesGetTransmissionTypeMethodId;
extern jmethodID jLinkPropertiesGetWorstMethodId;
extern jmethodID jLinkPropertyPairConstructorMethodId;
extern jmethodID jLinkPropertySetConstructorMethodId;
extern jmethodID jLinkSideValueOfStaticMethodId;
extern jmethodID jLinkStatusValueOfStaticMethodId;
extern jmethodID jLinkTypeOrdinalMethodId;
extern jmethodID jLinkTypeValueOfStaticMethodId;
extern jmethodID jMessageStatusValueOfStaticMethodId;
extern jmethodID jNodeTypeOrdinalMethodId;
extern jmethodID jNodeTypeValueOfStaticMethodId;
extern jmethodID jStorageEncryptionTypeOrdinalMethodId;
extern jmethodID jStorageEncryptionTypeValueOfStaticMethodId;
extern jmethodID jUserDisplayTypeOrdinalMethodId;
extern jmethodID jUserDisplayTypeValueOfStaticMethodId;
extern jmethodID jBootstrapActionTypeOrdinalMethodId;
extern jmethodID jBootstrapActionTypeValueOfStaticMethodId;
extern jmethodID jPackageStatusValueOfStaticMethodId;
extern jmethodID jPluginConfigConstructorMethodId;
extern jmethodID jPluginResponseValueOfStaticMethodId;
extern jmethodID jPluginStatusValueOfStaticMethodId;
extern jmethodID jRaceHandleConstructorMethodId;
extern jmethodID jSdkResponseConstructorMethodId;
extern jmethodID jSdkStatusValueOfStaticMethodId;
extern jmethodID jSendTypeOrdinalMethodId;
extern jmethodID jSendTypeValueOfStaticMethodId;
extern jmethodID jStringGetBytesMethodId;
extern jmethodID jSupportedChannelsConstructorMethodId;
extern jmethodID jSupportedChannelsPutMethodId;
extern jmethodID jTransmissionTypeValueOfStaticMethodId;
extern jmethodID jRaceAppUserResponseConstructorMethodId;
#ifdef __ANDROID__
extern jmethodID currentActivityThreadStaticMethodId;
extern jmethodID getApplicationMethodId;
#endif

// Field IDs
extern jfieldID jAppConfigNodeTypeFieldId;
extern jfieldID jAppConfigEncryptionTypeFieldId;
extern jfieldID jChannelPropertiesDurationFieldId;
extern jfieldID jChannelPropertiesMtuFieldId;
extern jfieldID jChannelPropertiesMultiAddressableFieldId;
extern jfieldID jChannelPropertiesPeriodFieldId;
extern jfieldID jChannelPropertiesReliableFieldId;
extern jfieldID jChannelPropertiesBootstrapFieldId;
extern jfieldID jChannelPropertiesIsFlushableFieldId;
extern jfieldID jChannelPropertiesMaxLinksFieldId;
extern jfieldID jChannelPropertiesCreatorsPerLoaderFieldId;
extern jfieldID jChannelPropertiesLoadersPerCreatorFieldId;
extern jfieldID jChannelPropertiesRolesFieldId;
extern jfieldID jChannelPropertiesCurrentRoleFieldId;
extern jfieldID jChannelPropertiesMaxSendsPerIntervalFieldId;
extern jfieldID jChannelPropertiesSecondsPerIntervalFieldId;
extern jfieldID jChannelPropertiesIntervalEndTimeFieldId;
extern jfieldID jChannelPropertiesSendsRemainingInIntervalFieldId;
extern jfieldID jChannelRoleRoleNameFieldId;
extern jfieldID jChannelRoleMechanicalTagsFieldId;
extern jfieldID jChannelRoleBehavioralTagsFieldId;
extern jfieldID jChannelRoleLinkSideFieldId;
extern jfieldID jChannelStatusValueFieldId;
extern jfieldID jClrMsgcreateTimeFieldId;
extern jfieldID jClrMsgAmpIndexFieldId;
extern jfieldID jClrMsgFromPersonaFieldId;
extern jfieldID jClrMsgNonceFieldId;
extern jfieldID jClrMsgPlainMsgFieldId;
extern jfieldID jClrMsgSpanIdFieldId;
extern jfieldID jClrMsgToPersonaFieldId;
extern jfieldID jClrMsgTraceIdFieldId;
extern jfieldID jConnectionStatusValueFieldId;
extern jfieldID jDeviceInfoArchitectureFieldId;
extern jfieldID jDeviceInfoNodeTypeFieldId;
extern jfieldID jDeviceInfoPlatformFieldId;
extern jfieldID jEncPkgCipherTextFieldId;
extern jfieldID jEncPkgPackageTypeByteFieldId;
extern jfieldID jEncPkgSpanIdFieldId;
extern jfieldID jEncPkgTraceIdFieldId;
extern jfieldID jLinkPropertiesDurationFieldId;
extern jfieldID jLinkPropertiesMtuFieldId;
extern jfieldID jLinkPropertiesPeriodFieldId;
extern jfieldID jLinkPropertiesReliableFieldId;
extern jfieldID jLinkPropertyPairReceiveFieldId;
extern jfieldID jLinkPropertyPairSendFieldId;
extern jfieldID jLinkPropertySetBandwidthBitsPSFieldId;
extern jfieldID jLinkPropertySetLatencyMsFieldId;
extern jfieldID jLinkPropertySetLossFieldId;
extern jfieldID jLinkStatusValueFieldId;
extern jfieldID jMessageStatusValueFieldId;
extern jfieldID jPackageStatusValueFieldId;
extern jfieldID jPluginConfigAuxDataDirectoryFieldId;
extern jfieldID jPluginConfigEtcDirectoryFieldId;
extern jfieldID jPluginConfigLoggingDirectoryFieldId;
extern jfieldID jPluginConfigPluginConfigFilePathFieldId;
extern jfieldID jPluginConfigTmpDirectoryFieldId;
extern jfieldID jPluginConfigPluginDirectoryFieldId;
extern jfieldID jPluginResponseValueFieldId;
extern jfieldID jPluginStatusValueFieldId;
extern jfieldID jRaceAppWrapperPointerFieldId;
extern jfieldID jRaceHandleValueFieldId;
extern jfieldID jRaceSdkAppSdkPointerFieldId;
extern jfieldID jRaceSdkCommsSdkPointerFieldId;
extern jfieldID jSdkResponseHandleFieldId;
extern jfieldID jSdkResponseQueueUtilizationFieldId;
extern jfieldID jSdkResponseSdkStatusFieldId;
extern jfieldID jSdkStatusValueFieldId;

}  // namespace JavaIds

#endif
