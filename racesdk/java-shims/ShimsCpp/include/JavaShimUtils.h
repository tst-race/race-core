
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

#ifndef _Java_Shim_Utils
#define _Java_Shim_Utils

#include <AppConfig.h>
#include <ChannelProperties.h>
#include <ChannelStatus.h>
#include <ClrMsg.h>
#include <ConnectionStatus.h>
#include <DeviceInfo.h>
#include <EncPkg.h>
#include <LinkProperties.h>
#include <LinkStatus.h>
#include <MessageStatus.h>
#include <PackageStatus.h>
#include <PluginConfig.h>
#include <PluginResponse.h>
#include <PluginStatus.h>
#include <SdkResponse.h>
#include <jni.h>

#include <map>
#include <string>
#include <vector>

namespace JavaShimUtils {

/*****
 * Converters
 *****/

/**
 * @brief Convert Java ClrMsg to C++ ClrMsg
 *
 * @param env JNI Environment reference
 * @param jClrMsg Java ClrMsg to convert
 * @return ClrMsg
 */
ClrMsg jClrMsg_to_ClrMsg(JNIEnv *env, jobject jClrMsg);

/**
 * @brief Convert C++ ClrMsg to Java ClrMsg
 *
 * @param env JNI Environment reference
 * @param ClrMsg C++ ClrMsg to convert
 * @return jClrMsg
 */
jobject clrMsg_to_jClrMsg(JNIEnv *env, const ClrMsg &clrMsg);

/**
 * @brief Convert Java EncPkg to C++ EncPkg
 *
 * @param env JNI Environment reference
 * @param jEncPkg Java EncPkg to convert
 * @return C++ EncPkg
 */
EncPkg jobjectToEncPkg(JNIEnv *env, jobject jEncPkg);

/**
 * @brief Convert C++ EncPkg to Java EncPkg
 *
 * @param env JNI Environment reference
 * @param encPkg C++ EncPkg to convert
 * @return Java EncPkg
 */
jobject encPkgToJobject(JNIEnv *env, const EncPkg &encPkg);

/**
 * @brief Convert Java NodeType to C++ NodeType
 *
 * @param env JNI Environment reference
 * @param javaNodeType Java NodeType to convert
 * @return NodeType
 */
RaceEnums::NodeType jobjectToNodeType(JNIEnv *env, jobject javaNodeType);

/**
 * @brief Convert C++ NodeType to Java NodeType
 *
 * @param env JNI Environment reference
 * @param nodeType C++ NodeType to convert
 * @return NodeType
 */
jobject nodeTypeToJNodeType(JNIEnv *env, RaceEnums::NodeType nodeType);

/**
 * @brief Convert Java StorageEncryptionType to C++ StorageEncryptionType
 *
 * @param env JNI Environment reference
 * @param javaStorageEncryptionType Java StorageEncryptionType to convert
 * @return StorageEncryptionType
 */
RaceEnums::StorageEncryptionType jobjectToStorageEncryptionType(JNIEnv *env,
                                                                jobject javaStorageEncryptionType);

/**
 * @brief Convert C++ StorageEncryptionType to Java StorageEncryptionType
 *
 * @param env JNI Environment reference
 * @param storageEncryptionType C++ StorageEncryptionType to convert
 * @return StorageEncryptionType
 */
jobject StorageEncryptionTypeToJStorageEncryptionType(
    JNIEnv *env, RaceEnums::StorageEncryptionType storageEncryptionType);

/**
 * @brief Convert Java UserDisplayType to C++ UserDisplayType
 *
 * @param env JNI Environment reference
 * @param javaUserDisplayType Java UserDisplayType to convert
 * @return UserDisplayType
 */
RaceEnums::UserDisplayType jobjectToUserDisplayType(JNIEnv *env, jobject javaUserDisplayType);

/**
 * @brief Convert C++ UserDisplayType to Java UserDisplayType
 *
 * @param env JNI Environment reference
 * @param userDisplayType C++ UserDisplayType to convert
 * @return UserDisplayType
 */
jobject userDisplayTypeToJUserDisplayType(JNIEnv *env, RaceEnums::UserDisplayType userDisplayType);

/**
 * @brief Convert Java BootstrapActionType to C++ BootstrapActionType
 *
 * @param env JNI Environment reference
 * @param javaBootstrapActionType Java BootstrapActionType to convert
 * @return BootstrapActionType
 */
RaceEnums::BootstrapActionType jobjectToBootstrapActionType(JNIEnv *env,
                                                            jobject javaBootstrapActionType);

/**
 * @brief Convert C++ BootstrapActionType to Java BootstrapActionType
 *
 * @param env JNI Environment reference
 * @param javaBootstrapActionType C++ BootstrapActionType to convert
 * @return BootstrapActionType
 */
jobject bootstrapActionTypeToJBootstrapActionType(
    JNIEnv *env, RaceEnums::BootstrapActionType bootstrapActionType);

/**
 * @brief Convert Java AppConfig to C++ AppConfig
 *
 * @param env JNI Environment reference
 * @param jobject Java AppConfig to convert
 * @return AppConfig
 */
AppConfig jAppConfigToAppConfig(JNIEnv *env, jobject jAppConfig);

/**
 * @brief Convert C++ AppConfig to Java AppConfig
 *
 * @param env JNI Environment reference
 * @param appConfig C++ AppConfig to convert
 * @return Java AppConfig
 */
jobject appConfigToJobject(JNIEnv *env, const AppConfig &appConfig);

/**
 * @brief Convert Java LinkProperties to C++ LinkProperties
 *
 * @param env JNI Environment reference
 * @param jobject Java LinkProperties to convert
 * @return LinkProperties
 */
LinkProperties jLinkPropertiesToLinkProperties(JNIEnv *env, jobject jLinkProps);

/**
 * @brief Convert C++ LinkProperties to Java LinkProperties
 *
 * @param env JNI Environment reference
 * @param properties C++ LinkProperties to convert
 * @return Java LinkProperties
 */
jobject linkPropertiesToJobject(JNIEnv *env, const LinkProperties &properties);

/**
 * @brief Convert Java ChannelProperties to C++ ChannelProperties
 *
 * @param env JNI Environment reference
 * @param jobject Java ChannelProperties to convert
 * @return ChannelProperties
 */
ChannelProperties jChannelPropertiesToChannelProperties(JNIEnv *env, jobject jchannelProps);

/**
 * @brief Convert C++ ChannelProperties to Java ChannelProperties
 *
 * @param env JNI Environment reference
 * @param properties C++ ChannelProperties to convert
 * @return Java ChannelProperties
 */
jobject channelPropertiesToJobject(JNIEnv *env, const ChannelProperties &properties);

/**
 * @brief Convert C++ ChannelProperties vector to Java ChannelProperties array
 *
 * @param env JNI Environment reference
 * @param properties C++ ChannelProperties to convert
 * @return Java ChannelProperties
 */
jobjectArray channelPropertiesVectorToJArray(JNIEnv *env,
                                             std::vector<ChannelProperties> propertiesArray);

/**
 * @brief Convert Java LinkPropertySet to C++ LinkPropertySet
 *
 * @param env JNI Environment reference
 * @param jobject Java LinkPropertySet to convert
 * @return LinkPropertySet
 */
LinkPropertySet jLinkPropertySetToLinkPropertySet(JNIEnv *env, jobject jLinkPropSet);

/**
 * @brief Convert C++ LinkPropertySet to Java LinkPropertySet
 *
 * @param env JNI Environment reference
 * @param propertySet C++ LinkPropertySet
 * @return Java LinkPropertySet
 */
jobject linkPropertySetToJobject(JNIEnv *env, const LinkPropertySet &propertySet);

/**
 * @brief Convert Java LinkPropertyPair to C++ LinkPropertyPair
 *
 * @param env JNI Environment reference
 * @param jobject Java LinkPropertyPair to convert
 * @return LinkPropertyPair
 */
LinkPropertyPair jLinkPropertyPairToLinkPropertyPair(JNIEnv *env, jobject jLinkPropPair);

/**
 * @brief Convert C++ LinkPropertyPair to Java LinkPropertyPair
 *
 * @param env JNI Environment reference
 * @param propertyPair C++ LinkPropertyPair
 * @return Java LinkPropertyPair
 */
jobject linkPropertyPairToJobject(JNIEnv *env, const LinkPropertyPair &propertyPair);

/**
 * @brief Convert C++ vector of ChannelRoles to Java array of ChannelRoles
 *
 * @param env JNI Environment reference
 * @param roles C++ vector of ChannelRoles
 * @return Java array of ChannelRoles
 */
jobject rolesVectorToJRolesVector(JNIEnv *env, const std::vector<ChannelRole> &roles);

/**
 * @brief Convert Java array of ChannelRoles to C++ vector of ChannelRoles
 *
 * @param env JNI Environment reference
 * @param jRoles Java array of ChannelRoles
 * @return C++ vector of ChannelRoles
 */
std::vector<ChannelRole> jRolesVectorToRolesVector(JNIEnv *env, jobjectArray &jRoles);

/**
 * @brief Convert C++ ChannelRole to Java ChannelRole
 *
 * @param env JNI Environment reference
 * @param propertyPair C++ ChannelRole
 * @return Java ChannelRole
 */
jobject channelRoleToJobject(JNIEnv *env, const ChannelRole &role);

/**
 * @brief Convert Java ChannelRole to C++ ChannelRole
 *
 * @param env JNI Environment reference
 * @param jobject Java ChannelRole to convert
 * @return ChannelRole
 */
ChannelRole jChannelRoleToChannelRole(JNIEnv *env, jobject &jCurrentRole);

/**
 * @brief Convert Java String to C++ string
 *
 * @param env JNI Environment reference
 * @param jobject Java String to convert
 * @return string
 */
std::string jstring2string(JNIEnv *env, jstring jStr);

/**
 * @brief Convert C++ string vector to Java string array
 *
 * @param env JNI Environment reference
 * @param vector<string> vector to convert
 * @return jobjectArray
 */
jobjectArray stringVectorToJArray(JNIEnv *env, std::vector<std::string> stringArray);

/**
 * @brief Convert C++ map of channelGid to ChannelProperties to Java map
 *
 * @param env JNI Environment reference
 * @param map<string, ChannelProperties> vector to convert
 * @return jobject
 */
jobject supportedChannelsToJobject(
    JNIEnv *env, const std::map<std::string, ChannelProperties> &supportedChannels);
/**
 * @brief Convert Java string array to C++ string vector
 *
 * @param env JNI Environment reference
 * @param jobjectArray java array to convert
 * @return vector<string>
 */
std::vector<std::string> jArrayToStringVector(JNIEnv *env, jobjectArray &jArray);

/**
 * @brief Convert Java byte array to C++ uint8_t vector
 *
 * @param env JNI Environment reference
 * @param jobject java byte array to convert
 * @return vector<uint8_t>
 */
std::vector<std::uint8_t> jobjectToVectorUint8(JNIEnv *env, jobject &theJavaObject);

/**
 * @brief Convert Java LinkType to C++ LinkType
 *
 * @param env JNI Environment reference
 * @param jobject java LinkType to convert
 * @return LinkType
 */
LinkType jobjectToLinkType(JNIEnv *env, jobject &javaLinkType);

/**
 * @brief Convert C++ LinkType to Java LinkType
 *
 * @param env JNI Environment reference
 * @param linkType c++ linkType to convert
 * @return jobject JLinkType
 */
jobject linkTypeToJLinkType(JNIEnv *env, LinkType linkType);

/**
 * @brief Convert Java LinkDirection to C++ LinkDirection
 *
 * @param env JNI Environment reference
 * @param jobject java LinkDirection to convert
 * @return LinkDirection
 */
LinkDirection jobjectToLinkDirection(JNIEnv *env, jobject &javaLinkDirection);

/**
 * @brief Convert C++ LinkDirection to Java LinkDirection
 *
 * @param env JNI Environment reference
 * @param linkDirection c++ linkDirection to convert
 * @return jobject JLinkDirection
 */
jobject linkDirectionToJLinkDirection(JNIEnv *env, LinkDirection linkDirection);

/**
 * @brief Convert C++ ConnectionType to Java ConnectionType
 *
 * @param env JNI Environment reference
 * @param connectionType c++ connectionType to convert
 * @return jobject JConnectionType
 */
jobject connectionTypeToJConnectionType(JNIEnv *env, ConnectionType connectionType);

/**
 * @brief Convert Java ConnectionType to C++ ConnectionType
 *
 * @param env JNI Environment reference
 * @param jobject java ConnectionType to convert
 * @return ConnectionType
 */
ConnectionType jobjectToConnectionType(JNIEnv *env, jobject &javaConnectionType);

/**
 * @brief Convert C++ SendType to Java SendType
 *
 * @param env JNI Environment reference
 * @param sendType c++ sendType to convert
 * @return jobject JSendType
 */
jobject sendTypeToJSendType(JNIEnv *env, SendType sendType);

/**
 * @brief Convert Java SendType to C++ SendType
 *
 * @param env JNI Environment reference
 * @param jobject java SendType to convert
 * @return SendType
 */
SendType jobjectToSendType(JNIEnv *env, jobject &javaSendType);

/**
 * @brief Convert C++ TransmissionType to Java TransmissionType
 *
 * @param env JNI Environment reference
 * @param transmissionType C++ TransmissionType to convert
 * @return Java TransmissionType
 */
jobject transmissionTypeToJobject(JNIEnv *env, TransmissionType transmissionType);

/**
 * @brief Convert C++ LinkSide to Java LinkSide
 *
 * @param env JNI Environment reference
 * @param LinkSide C++ LinkSide to convert
 * @return Java LinkSide
 */
jobject linkSideToJobject(JNIEnv *env, LinkSide linkSide);

/**
 * @brief Convert Java RaceHandle to C++ RaceHandle
 *
 * @param env JNI Environment reference
 * @param jRaceHandle Java RaceHandle to convert
 * @return C++ RaceHandle
 */
RaceHandle jobjectToRaceHandle(JNIEnv *env, jobject &jRaceHandle);

/**
 * @brief Convert C++ RaceHandle to Java RaceHandle
 *
 * @param env JNI Environment reference
 * @param handle C++ RaceHandle to convert
 * @return Java RaceHandle
 */
jobject raceHandleToJobject(JNIEnv *env, RaceHandle handle);

/**
 * @brief Convert C++ SdkStatus to Java SdkStatus
 *
 * @param env JNI Environment reference
 * @param status C++ SdkStatus to convert
 * @return Java SdkStatus
 */
jobject sdkStatusToJobject(JNIEnv *env, SdkStatus status);

/**
 * @brief Convert Java SdkStatus to C++ SdkStatus
 *
 * @param env JNI Environment reference
 * @param status Java SdkStatus to convert
 * @return C++ SdkStatus
 */
SdkStatus jobjectToSdkStatus(JNIEnv *env, jobject &jStatus);

/**
 * @brief Convert C++ SdkResponse to Java SdkResponse
 *
 * @param env JNI Environment reference
 * @param response C++ SdkResponse to convert
 * @return Java SdkResponse
 */
jobject sdkResponseToJobject(JNIEnv *env, const SdkResponse &response);

/**
 * @brief Convert Java SdkResponse to C++ SdkResponse
 *
 * @param env JNI Environment reference
 * @param jResponse Java SdkResponse to convert
 * @return C++ SdkResponse
 */
SdkResponse jobjectToSdkResponse(JNIEnv *env, jobject &jResponse);

/**
 * @brief Convert C++ PackageStatus to Java PackageStatus
 *
 * @param env JNI Environment reference
 * @param status C++ PackageStatus to convert
 * @return Java PackageStatus
 */
jobject packageStatusToJobject(JNIEnv *env, PackageStatus status);

/**
 * @brief Convert Java PackageStatus to C++ PackageStatus
 *
 * @param env JNI Environment reference
 * @param jPackageStatus Java PackageStatus to convert
 * @return C++ PackageStatus
 */
PackageStatus jobjectToPackageStatus(JNIEnv *env, jobject &jPackageStatus);

/**
 * @brief Convert C++ ConnectionStatus to Java ConnectionStatus
 *
 * @param env JNI Environment reference
 * @param status C++ ConnectionStatus to convert
 * @return Java ConnectionStatus
 */
jobject connectionStatusToJobject(JNIEnv *env, ConnectionStatus status);

/**
 * @brief Convert Java ConnectionStatus to C++ ConnectionStatus
 *
 * @param env JNI Environment reference
 * @param jConnectionStatus Java ConnectionStatus to convert
 * @return C++ ConnectionStatus
 */
ConnectionStatus jobjectToConnectionStatus(JNIEnv *env, jobject &jConnectionStatus);

/**
 * @brief Convert C++ LinkStatus to Java LinkStatus
 *
 * @param env JNI Environment reference
 * @param status C++ LinkStatus to convert
 * @return Java LinkStatus
 */
jobject linkStatusToJobject(JNIEnv *env, LinkStatus status);

/**
 * @brief Convert Java LinkStatus to C++ LinkStatus
 *
 * @param env JNI Environment reference
 * @param jLinkStatus Java LinkStatus to convert
 * @return C++ LinkStatus
 */
LinkStatus jobjectToLinkStatus(JNIEnv *env, jobject &jLinkStatus);

/**
 * @brief Convert C++ ChannelStatus to Java ChannelStatus
 *
 * @param env JNI Environment reference
 * @param status C++ ChannelStatus to convert
 * @return Java ChannelStatus
 */
jobject channelStatusToJobject(JNIEnv *env, ChannelStatus status);

/**
 * @brief Convert C++ PluginStatus to Java PluginStatus
 *
 * @param env JNI Environment reference
 * @param status C++ PluginStatus to convert
 * @return Java PluginStatus
 */
PluginStatus jobjectToPluginStatus(JNIEnv *env, jobject &jPluginStatus);

/**
 * @brief Convert C++ PluginStatus to Java PluginStatus
 *
 * @param env JNI Environment reference
 * @param status C++ PluginStatus to convert
 * @return Java PluginStatus
 */
jobject pluginStatusToJobject(JNIEnv *env, PluginStatus status);

/**
 * @brief Convert Java ChannelStatus to C++ ChannelStatus
 *
 * @param env JNI Environment reference
 * @param jChannelStatus Java ChannelStatus to convert
 * @return C++ ChannelStatus
 */
ChannelStatus jobjectToChannelStatus(JNIEnv *env, jobject &jChannelStatus);

/**
 * @brief Convert C++ PluginResponse to Java PluginResponse
 *
 * @param env JNI Environment reference
 * @param pluginResponse C++ PluginResponse to convert
 * @return Java PluginResponse
 */
jobject pluginResponseToJobject(JNIEnv *env, PluginResponse pluginResponse);

/**
 * @brief Convert Java PluginResponse to C++ PluginResponse
 *
 * @param env JNI Environment reference
 * @param jPluginResponse Java PluginResponse to convert
 * @return C++ PluginResponse
 */
PluginResponse jobjectToPluginResponse(JNIEnv *env, jobject &jPluginResponse);

/**
 * @brief Convert C++ PluginConfig to Java PluginConfig
 *
 * @param env JNI Environment reference
 * @param pluginConfig C++ PluginConfig to convert
 * @return Java PluginConfig
 */
jobject pluginConfigToJobject(JNIEnv *env, PluginConfig pluginConfig);

/**
 * @brief Convert Java PluginConfig to C++ PluginConfig
 *
 * @param env JNI Environment reference
 * @param jPluginConfig Java PluginConfig to convert
 * @return C++ PluginConfig
 */
PluginConfig jobjectToPluginConfig(JNIEnv *env, jobject &jPluginConfig);

/**
 * @brief Convert C++ MessageStatus to Java MessageStatus
 *
 * @param env JNI Environment reference
 * @param status C++ MessageStatus to convert
 * @return Java MessageStatus
 */
jobject messageStatusToJobject(JNIEnv *env, MessageStatus status);

/**
 * @brief Convert Java MessageStatus to C++ MessageStatus
 *
 * @param env JNI Environment reference
 * @param jMessageStatus Java MessageStatus to convert
 * @return C++ MessageStatus
 */
MessageStatus jobjectToMessageStatus(JNIEnv *env, jobject &jMessageStatus);

/**
 * @brief Convert Java byte array to C++ RawData
 *
 * @param env JNI Environment reference
 * @param jData Java byte array to convert
 * @return C++ RawData
 */
RawData jByteArrayToRawData(JNIEnv *env, jbyteArray jData);

/**
 * @brief Convert Java byte array to C++ RawData
 *
 * @param env JNI Environment reference
 * @param data C++ RawData to convert
 * @return Jave byte array
 */
jbyteArray rawDataToJByteArray(JNIEnv *env, const RawData &data);

/**
 * @brief Convert C++ DeviceInfo to Java DeviceInfo
 *
 * @param env JNI Environment reference
 * @param deviceInfo C++ DeviceInfo to convert
 * @return Java DeviceInfo
 */
jobject deviceInfoToJobject(JNIEnv *env, DeviceInfo deviceInfo);

/**
 * @brief Convert Java DeviceInfo to C++ DeviceInfo
 *
 * @param env JNI Environment reference
 * @param jDeviceInfo Java DeviceInfo to convert
 * @return C++ DeviceInfo
 */
DeviceInfo jobjectToDeviceInfo(JNIEnv *env, jobject &jDeviceInfo);

/****
 * Get fields from Java
 ****/

/**
 * @brief Get the Message from a Jthrowable object
 *
 * @param env JNI Environment reference
 * @param jthrowable object to retrieve message from
 * @return std::string
 */
std::string getMessageFromJthrowable(JNIEnv *env, jthrowable &throwable);

/*****
 * Java Environment functions
 *****/

/**
 * @brief Find a class in the java environment. This should be used instead of env->FindClass
 * because there is additional logic to handle Android nicely.
 *
 * @param env JNI Environment reference
 * @param className the name of the class to find
 * @return jclass the class that was found, or nullptr if none was found
 */
jclass findClass(JNIEnv *env, std::string className);

/**
 * @brief Find a class in the java environment. This is similar to findClass, but can be used
 * to find a class provided in a dex at runtime. This defaults to the same thing as findClass
 * on non-Android platforms.
 *
 * @param env JNI Environment reference
 * @param pluginType the plugin type to use, should be "network-manager" or "comms"
 * @param dexName the name of the dex to find the class in
 * @param className the name of the class to find
 * @return jclass the class that was found, or nullptr if none was found
 */
jclass findDexClass(JNIEnv *env, const std::string &pluginType, std::string dexName,
                    const std::string &className);

/**
 * @brief Get the Env object. This is neccessary when calling into Java from a new thread.
 *
 * @param env JNI Environment reference
 * @param jvm JNI Virtual Machine reference
 * @return true: env was newly attached
 * @return false: env either failed to attached or was already attached
 */
bool getEnv(JNIEnv **env, JavaVM *jvm);

/**
 * @brief Get the existing jvm for RACE or create one if one doesn't currently exist. On
 * Android, this will return the system jvm.
 *
 * @return JavaVM* the Java VM
 */
JavaVM *getJvm();

/**
 * @brief delete the existing JVM
 *
 *
 */
void destroyJvm();

/**
 * @brief Detached current thread from jvm before destroying thread.
 *
 * @param ptr
 */
void ThreadDestructor(void *ptr);

/**
 * @brief attached ThreadDestructor to thread
 *
 */
void make_key();

/**
 * @brief Set the Trace And Span Id For jClrMsg object.
 *
 * @param env JNI Environment reference
 * @param jClrMsg The clear message object to update
 * @param traceId The trace ID value to apply to the clear messages
 * @param spanId The span ID value to apply to the clear messages
 */
void setTraceAndSpanIdForJClrMsg(JNIEnv *env, jobject jClrMsg, int64_t traceId, int64_t spanId);

}  // namespace JavaShimUtils
#endif
