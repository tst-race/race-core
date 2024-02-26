
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

#include "PluginNMJavaWrapper.h"

#include <RaceLog.h>

#include <sstream>

#include "JavaIds.h"
#include "JavaShimUtils.h"

// This would have been defined as a static member variable of the plugin wrapper class,
// however that leads to a double-free on deinitialization of the application. Since the
// library is loaded automatically by C++, and then again in Java via System.loadLibrary,
// the string is referenced twice and deleted twice. That being said, this class is not
// actually needed from Java, and does not need to be part of the library loaded by Java.
// This class could be split out to a separate library that is only loaded by C++.
static const std::string logLabel = "PluginNMJavaWrapper";

static void logException(JNIEnv *env) {
    jthrowable jThrowable = env->ExceptionOccurred();
    if (jThrowable != nullptr) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        RaceLog::logError("JniException",
                          "Exception: " + JavaShimUtils::getMessageFromJthrowable(env, jThrowable),
                          "");
    }
}

static PluginResponse pluginFatalOrResponse(JNIEnv *env, jobject &jResponse) {
    jthrowable jThrowable = env->ExceptionOccurred();
    if (jThrowable != nullptr) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        RaceLog::logError(logLabel,
                          "Exception caught invoking plugin method: " +
                              JavaShimUtils::getMessageFromJthrowable(env, jThrowable),
                          "");
        return PLUGIN_FATAL;
    }

    return JavaShimUtils::jobjectToPluginResponse(env, jResponse);
}

PluginNMJavaWrapper::PluginNMJavaWrapper(IRaceSdkNM *sdk, const std::string &pluginName,
                                         const std::string &pluginClassIn) :
    plugin(nullptr), jvm(JavaShimUtils::getJvm()) {
    RaceLog::logDebug(logLabel, "creating Java wrapper.", "");
    linkNativeMethods(sdk, pluginName, pluginClassIn);

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    // Docs for method signatures:
    //     https://docs.oracle.com/javase/1.5.0/docs/guide/jni/spec/types.html#wp276
    // You can also run this command to get method signatures:
    //     javap -v -classpath build/LINUX_x86_64/java-shims/ShimsJava/racesdk-java-shims-1.jar
    //     ShimsJava.IRacePluginNM

    jPluginInitMethodId = JavaIds::getMethodID(
        env, jPluginClass, "init", "(LShimsJava/PluginConfig;)LShimsJava/PluginResponse;");
    jPluginShutdownMethodId =
        JavaIds::getMethodID(env, jPluginClass, "shutdown", "()LShimsJava/PluginResponse;");
    jPluginProcessClsMsgMethodId = JavaIds::getMethodID(
        env, jPluginClass, "processClrMsg",
        "(LShimsJava/RaceHandle;LShimsJava/JClrMsg;)LShimsJava/PluginResponse;");
    jPluginProcessEncPkgMethodId = JavaIds::getMethodID(
        env, jPluginClass, "processEncPkg",
        "(LShimsJava/RaceHandle;LShimsJava/JEncPkg;[Ljava/lang/String;)LShimsJava/PluginResponse;");
    jPluginPrepareToBootstrapMethodId = JavaIds::getMethodID(
        env, jPluginClass, "prepareToBootstrap",
        "(LShimsJava/RaceHandle;Ljava/lang/String;Ljava/lang/String;LShimsJava/"
        "DeviceInfo;)LShimsJava/PluginResponse;");
    jPluginOnBootstrapKeyReceivedMethodId =
        JavaIds::getMethodID(env, jPluginClass, "onBootstrapKeyReceived",
                             "(Ljava/lang/String;[B)LShimsJava/PluginResponse;");
    jPluginOnPackageStatusChangedMethodId = JavaIds::getMethodID(
        env, jPluginClass, "onPackageStatusChanged",
        "(LShimsJava/RaceHandle;LShimsJava/PackageStatus;)LShimsJava/PluginResponse;");
    jPluginOnConnectionStatusChangedMethodId = JavaIds::getMethodID(
        env, jPluginClass, "onConnectionStatusChanged",
        "(LShimsJava/RaceHandle;Ljava/lang/String;LShimsJava/ConnectionStatus;Ljava/lang/"
        "String;LShimsJava/JLinkProperties;)LShimsJava/PluginResponse;");
    jPluginOnLinkStatusChangedMethodId =
        JavaIds::getMethodID(env, jPluginClass, "onLinkStatusChanged",
                             "(LShimsJava/RaceHandle;Ljava/lang/String;LShimsJava/"
                             "LinkStatus;LShimsJava/JLinkProperties;)LShimsJava/PluginResponse;");
    jPluginOnChannelStatusChangedMethodId = JavaIds::getMethodID(
        env, jPluginClass, "onChannelStatusChanged",
        "(LShimsJava/RaceHandle;Ljava/lang/String;LShimsJava/ChannelStatus;LShimsJava/"
        "JChannelProperties;)LShimsJava/PluginResponse;");
    jPluginOnLinkPropertiesChangedMethodId = JavaIds::getMethodID(
        env, jPluginClass, "onLinkPropertiesChanged",
        "(Ljava/lang/String;LShimsJava/JLinkProperties;)LShimsJava/PluginResponse;");
    jPluginOnPersonaLinksChangedMethodId = JavaIds::getMethodID(
        env, jPluginClass, "onPersonaLinksChanged",
        "(Ljava/lang/String;LShimsJava/LinkType;[Ljava/lang/String;)LShimsJava/PluginResponse;");
    jPluginOnUserInputReceivedMethodId = JavaIds::getMethodID(
        env, jPluginClass, "onUserInputReceived",
        "(LShimsJava/RaceHandle;ZLjava/lang/String;)LShimsJava/PluginResponse;");
    jPluginNotifyEpochMethodId = JavaIds::getMethodID(
        env, jPluginClass, "notifyEpoch", "(Ljava/lang/String;)LShimsJava/PluginResponse;");
    jPluginOnUserAcknowledgementReceivedMethodId =
        JavaIds::getMethodID(env, jPluginClass, "onUserAcknowledgementReceived",
                             "(LShimsJava/RaceHandle;)LShimsJava/PluginResponse;");
}

PluginNMJavaWrapper::~PluginNMJavaWrapper() {
    RaceLog::logDebug(logLabel, "Destructor Called", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    env->DeleteGlobalRef(jPluginClass);
    env->DeleteGlobalRef(plugin);
    destroyPlugin();
}

PluginResponse PluginNMJavaWrapper::init(const PluginConfig &pluginConfig) {
    RaceLog::logDebug(logLabel, "init", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    // Convert args and call init
    jobject jPluginConfig = JavaShimUtils::pluginConfigToJobject(env, pluginConfig);

    jobject jResponse = env->CallObjectMethod(plugin, jPluginInitMethodId, jPluginConfig);
    RaceLog::logDebug(logLabel, "init: returned", "");
    return pluginFatalOrResponse(env, jResponse);
}

PluginResponse PluginNMJavaWrapper::shutdown() {
    RaceLog::logDebug(logLabel, "shutdown: called", "");
    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    jobject jResponse = env->CallObjectMethod(plugin, jPluginShutdownMethodId);
    RaceLog::logDebug(logLabel, "shutdown: returned", "");
    return pluginFatalOrResponse(env, jResponse);
}

PluginResponse PluginNMJavaWrapper::processClrMsg(RaceHandle handle, const ClrMsg &msg) {
    RaceLog::logDebug(logLabel, "processClrMsg: called", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jobject jClrMsg = JavaShimUtils::clrMsg_to_jClrMsg(env, msg);

    jobject jResponse =
        env->CallObjectMethod(plugin, jPluginProcessClsMsgMethodId, jHandle, jClrMsg);
    RaceLog::logDebug(logLabel, "processClrMsg: returned", "");
    return pluginFatalOrResponse(env, jResponse);
}

PluginResponse PluginNMJavaWrapper::processEncPkg(RaceHandle handle, const EncPkg &ePkg,
                                                  const std::vector<ConnectionID> &connIDs) {
    RaceLog::logDebug(logLabel, "processEncPkg: called", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jobject jEncPkg = JavaShimUtils::encPkgToJobject(env, ePkg);
    if (jEncPkg == nullptr) {
        RaceLog::logError(logLabel, "processEncPkg: failed to construct JEncPkg", "");
        return PLUGIN_FATAL;
    }

    jobjectArray javaConnIDs = JavaShimUtils::stringVectorToJArray(env, connIDs);

    jobject jResponse =
        env->CallObjectMethod(plugin, jPluginProcessEncPkgMethodId, jHandle, jEncPkg, javaConnIDs);
    RaceLog::logDebug(logLabel, "processEncPkg: return", "");
    return pluginFatalOrResponse(env, jResponse);
}

PluginResponse PluginNMJavaWrapper::prepareToBootstrap(RaceHandle handle, LinkID linkId,
                                                       std::string configPath,
                                                       DeviceInfo deviceInfo) {
    RaceLog::logDebug(logLabel, "prepareToBootstrap: called", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jstring jLinkId = env->NewStringUTF(linkId.c_str());
    jstring jConfigPath = env->NewStringUTF(configPath.c_str());
    jobject jDeviceInfo = JavaShimUtils::deviceInfoToJobject(env, deviceInfo);

    jobject jResponse = env->CallObjectMethod(plugin, jPluginPrepareToBootstrapMethodId, jHandle,
                                              jLinkId, jConfigPath, jDeviceInfo);
    RaceLog::logDebug(logLabel, "prepareToBootstrap: return", "");
    return pluginFatalOrResponse(env, jResponse);
}

PluginResponse PluginNMJavaWrapper::onBootstrapPkgReceived(std::string persona, RawData pkg) {
    RaceLog::logDebug(logLabel, "onBootstrapKeyReceived: called", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    jstring jPersona = env->NewStringUTF(persona.c_str());
    jobject jPkg = JavaShimUtils::rawDataToJByteArray(env, pkg);

    jobject jResponse =
        env->CallObjectMethod(plugin, jPluginOnBootstrapKeyReceivedMethodId, jPersona, jPkg);
    RaceLog::logDebug(logLabel, "onBootstrapKeyReceived: return", "");
    return pluginFatalOrResponse(env, jResponse);
}

PluginResponse PluginNMJavaWrapper::onPackageStatusChanged(RaceHandle handle,
                                                           PackageStatus status) {
    RaceLog::logDebug(logLabel, "onPackageStatusChanged: called", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jobject jStatus = JavaShimUtils::packageStatusToJobject(env, status);

    jobject jResponse =
        env->CallObjectMethod(plugin, jPluginOnPackageStatusChangedMethodId, jHandle, jStatus);
    return pluginFatalOrResponse(env, jResponse);
}

PluginResponse PluginNMJavaWrapper::onConnectionStatusChanged(RaceHandle handle,
                                                              ConnectionID connId,
                                                              ConnectionStatus status,
                                                              LinkID linkId,
                                                              LinkProperties properties) {
    RaceLog::logDebug(logLabel, "onConnectionStatusChanged: called", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jstring jConnId = env->NewStringUTF(connId.c_str());
    jobject jStatus = JavaShimUtils::connectionStatusToJobject(env, status);
    jstring jLinkId = env->NewStringUTF(linkId.c_str());
    jobject jProperties = JavaShimUtils::linkPropertiesToJobject(env, properties);

    jobject jResponse = env->CallObjectMethod(plugin, jPluginOnConnectionStatusChangedMethodId,
                                              jHandle, jConnId, jStatus, jLinkId, jProperties);
    RaceLog::logDebug(logLabel, "onConnectionStatusChanged: return", "");
    return pluginFatalOrResponse(env, jResponse);
}

PluginResponse PluginNMJavaWrapper::onLinkStatusChanged(RaceHandle handle, LinkID linkId,
                                                        LinkStatus status,
                                                        LinkProperties properties) {
    RaceLog::logDebug(logLabel, "onLinkStatusChanged: called", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jstring jLinkId = env->NewStringUTF(linkId.c_str());
    jobject jStatus = JavaShimUtils::linkStatusToJobject(env, status);
    jobject jProperties = JavaShimUtils::linkPropertiesToJobject(env, properties);

    jobject jResponse = env->CallObjectMethod(plugin, jPluginOnLinkStatusChangedMethodId, jHandle,
                                              jLinkId, jStatus, jProperties);
    RaceLog::logDebug(logLabel, "onLinkStatusChanged: return", "");
    return pluginFatalOrResponse(env, jResponse);
}

PluginResponse PluginNMJavaWrapper::onChannelStatusChanged(RaceHandle handle,
                                                           std::string channelGid,
                                                           ChannelStatus status,
                                                           ChannelProperties properties) {
    RaceLog::logDebug(logLabel, "onChannelStatusChanged: called", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jstring jChannelGid = env->NewStringUTF(channelGid.c_str());
    jobject jStatus = JavaShimUtils::channelStatusToJobject(env, status);
    jobject jProperties = JavaShimUtils::channelPropertiesToJobject(env, properties);

    jobject jResponse = env->CallObjectMethod(plugin, jPluginOnChannelStatusChangedMethodId,
                                              jHandle, jChannelGid, jStatus, jProperties);
    RaceLog::logDebug(logLabel, "onChannelStatusChanged: return", "");
    return pluginFatalOrResponse(env, jResponse);
}

PluginResponse PluginNMJavaWrapper::onLinkPropertiesChanged(LinkID linkId,
                                                            LinkProperties linkProperties) {
    RaceLog::logDebug(logLabel, "onLinkPropertiesChanged: called", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    jstring jLinkId = env->NewStringUTF(linkId.c_str());
    jobject jProperties = JavaShimUtils::linkPropertiesToJobject(env, linkProperties);

    jobject jResponse =
        env->CallObjectMethod(plugin, jPluginOnLinkPropertiesChangedMethodId, jLinkId, jProperties);
    RaceLog::logDebug(logLabel, "onLinkPropertiesChanged: return", "");
    return pluginFatalOrResponse(env, jResponse);
}

PluginResponse PluginNMJavaWrapper::onPersonaLinksChanged(std::string recipientPersona,
                                                          LinkType linkType,
                                                          std::vector<LinkID> links) {
    RaceLog::logDebug(logLabel, "onPersonaLinksChanged: called", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    jstring jRecipientPersona = env->NewStringUTF(recipientPersona.c_str());
    jobject jLinkType = JavaShimUtils::linkTypeToJLinkType(env, linkType);
    jobjectArray jLinks = JavaShimUtils::stringVectorToJArray(env, links);

    jobject jResponse = env->CallObjectMethod(plugin, jPluginOnPersonaLinksChangedMethodId,
                                              jRecipientPersona, jLinkType, jLinks);
    RaceLog::logDebug(logLabel, "onPersonaLinksChanged: return", "");
    return pluginFatalOrResponse(env, jResponse);
}

PluginResponse PluginNMJavaWrapper::onUserInputReceived(RaceHandle handle, bool answered,
                                                        const std::string &response) {
    RaceLog::logDebug(logLabel, "onUserInputReceived: called", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jboolean jAnswered = static_cast<jboolean>(answered);
    jstring jUserResponse = env->NewStringUTF(response.c_str());

    jobject jResponse = env->CallObjectMethod(plugin, jPluginOnUserInputReceivedMethodId, jHandle,
                                              jAnswered, jUserResponse);
    jthrowable jThrowable = env->ExceptionOccurred();
    if (jThrowable != nullptr) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        RaceLog::logError(logLabel,
                          "Exception caught invoking plugin onUserInputReceived: " +
                              JavaShimUtils::getMessageFromJthrowable(env, jThrowable),
                          "");
        return PLUGIN_FATAL;
    }
    RaceLog::logDebug(logLabel, "onUserInputReceived: return", "");
    return JavaShimUtils::jobjectToPluginResponse(env, jResponse);
}

PluginResponse PluginNMJavaWrapper::notifyEpoch(const std::string &data) {
    RaceLog::logDebug(logLabel, "notifyEpoch: called", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    jstring jData = env->NewStringUTF(data.c_str());

    jobject jResponse = env->CallObjectMethod(plugin, jPluginNotifyEpochMethodId, jData);
    jthrowable jThrowable = env->ExceptionOccurred();
    if (jThrowable != nullptr) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        RaceLog::logError(logLabel,
                          "Exception caught invoking plugin notifyEpoch: " +
                              JavaShimUtils::getMessageFromJthrowable(env, jThrowable),
                          "");
        return PLUGIN_FATAL;
    }
    RaceLog::logDebug(logLabel, "onUserInputReceived: return", "");
    return JavaShimUtils::jobjectToPluginResponse(env, jResponse);
}

void PluginNMJavaWrapper::destroyPlugin() {
    // if (plugin != nullptr) {
    // TODO: need to validate that this call actually works. Looks like it
    // does not...
    // destroy_plugin(plugin);
    plugin = nullptr;
    // }
}

void PluginNMJavaWrapper::linkNativeMethods(IRaceSdkNM *sdk, std::string pluginName,
                                            std::string pluginClassName) {
    RaceLog::logDebug(logLabel, "linkNativeMethods: called", "");

    if (sdk == nullptr) {
        const std::string errorMessage = "linkNativeMethods: sdk pointer provided is nullptr";
        RaceLog::logError(logLabel, errorMessage, "");
        throw std::runtime_error(errorMessage);
    }

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    // Create Network Manager SDK instance
    jclass sdkClass = JavaShimUtils::findClass(env, "ShimsJava/JRaceSdkNM");
    logException(env);
    if (sdkClass == nullptr) {
        const std::string errorMessage =
            "linkNativeMethods: Network Manager SDK class not found. Make sure the "
            "racesdk-java-shims jar"
            " exists and is in the class path";
        RaceLog::logError(logLabel, errorMessage, "");
        throw std::runtime_error(errorMessage);
    }

    // Find the Java class for the network manager plugin.
    jPluginClass = JavaShimUtils::findDexClass(env, "networkManager", pluginName, pluginClassName);
    jPluginClass = reinterpret_cast<jclass>(env->NewGlobalRef(jPluginClass));
    logException(env);
    if (jPluginClass == nullptr) {
        const std::string errorMessage = "linkNativeMethods: class not found: " + pluginClassName;
        RaceLog::logError(logLabel, errorMessage, "");
        throw std::runtime_error(errorMessage);
    }
    jmethodID sdkConstructor = env->GetMethodID(sdkClass, "<init>", "(J)V");
    if (sdkConstructor == nullptr) {
        const std::string errorMessage = "Network Manager SDK constructor not found";
        RaceLog::logError(logLabel, errorMessage, "");
        throw std::runtime_error(errorMessage);
    }
    jlong sdkPtr = reinterpret_cast<jlong>(sdk);
    jobject sdkObject = env->NewObject(sdkClass, sdkConstructor, sdkPtr);
    logException(env);
    if (sdkObject == nullptr) {
        const std::string errorMessage = "Failed to create Network Manager SDK";
        RaceLog::logError(logLabel, errorMessage, "");
        throw std::runtime_error(errorMessage);
    }

    // Find the constructor for the network manager plugin Java class.
    jmethodID constructor;
    constructor = env->GetMethodID(jPluginClass, "<init>", "(LShimsJava/JRaceSdkNM;)V");
    if (constructor == nullptr) {
        const std::string errorMessage = "linkNativeMethods: network manager constructor not found";
        RaceLog::logError(logLabel, errorMessage, "");
        throw std::runtime_error(errorMessage);
    }

    // Create an instance of the Java network manager plugin class.
    jobject local_plugin = env->NewObject(jPluginClass, constructor, sdkObject);
    plugin = env->NewGlobalRef(local_plugin);
    logException(env);
    if (plugin == nullptr) {
        jthrowable jThrowable = env->ExceptionOccurred();
        std::string errorMessage =
            "linkNativeMethods: failed to construct instance of network manager class: ";
        if (jThrowable != nullptr) {
            env->ExceptionDescribe();
            env->ExceptionClear();
            errorMessage += JavaShimUtils::getMessageFromJthrowable(env, jThrowable);
        } else {
            errorMessage += "unknown error";
        }
        RaceLog::logError(logLabel, errorMessage, "");
        throw std::runtime_error(errorMessage);
    }

    RaceLog::logDebug(logLabel, "linkNativeMethods: returned", "");
}

PluginResponse PluginNMJavaWrapper::onUserAcknowledgementReceived(RaceHandle handle) {
    RaceLog::logDebug(logLabel, "onUserAcknowledgementReceived: called", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);

    jobject jResponse =
        env->CallObjectMethod(plugin, jPluginOnUserAcknowledgementReceivedMethodId, jHandle);
    jthrowable jThrowable = env->ExceptionOccurred();
    if (jThrowable != nullptr) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        RaceLog::logError(logLabel,
                          "Exception caught invoking plugin onUserAcknowledgementReceived: " +
                              JavaShimUtils::getMessageFromJthrowable(env, jThrowable),
                          "");
        return PLUGIN_FATAL;
    }
    RaceLog::logDebug(logLabel, "onUserAcknowledgementReceived: returned", "");
    return JavaShimUtils::jobjectToPluginResponse(env, jResponse);
}
