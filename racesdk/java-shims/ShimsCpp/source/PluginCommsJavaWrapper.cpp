
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

#include "PluginCommsJavaWrapper.h"

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
static const std::string logLabel = "PluginCommsJavaWrapper";

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

PluginCommsJavaWrapper::PluginCommsJavaWrapper(IRaceSdkComms *sdk, const std::string &pluginName,
                                               const std::string &pluginClassName) :
    plugin(nullptr), jvm(JavaShimUtils::getJvm()) {
    RaceLog::logDebug(logLabel, "creating Java wrapper.", "");
    linkNativeMethods(sdk, pluginName, pluginClassName);

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    // Docs for method signatures:
    //     https://docs.oracle.com/javase/1.5.0/docs/guide/jni/spec/types.html#wp276
    // You can also run this command to get method signatures:
    // clang-format off
    //     javap -v -classpath build/LINUX_x86_64/java-shims/ShimsJava/racesdk-java-shims-1.jar ShimsJava.IRacePluginComms
    // clang-format on

    jPluginInitMethodId = JavaIds::getMethodID(
        env, jPluginClass, "init", "(LShimsJava/PluginConfig;)LShimsJava/PluginResponse;");
    jPluginShutdownMethodId =
        JavaIds::getMethodID(env, jPluginClass, "shutdown", "()LShimsJava/PluginResponse;");
    jPluginSendPackageMethodId =
        JavaIds::getMethodID(env, jPluginClass, "sendPackage",
                             "(LShimsJava/RaceHandle;Ljava/lang/String;LShimsJava/"
                             "JEncPkg;DJ)LShimsJava/PluginResponse;");
    jPluginOpenConnectionMethodId =
        JavaIds::getMethodID(env, jPluginClass, "openConnection",
                             "(LShimsJava/RaceHandle;LShimsJava/LinkType;Ljava/lang/String;Ljava/"
                             "lang/String;I)LShimsJava/PluginResponse;");
    jPluginCloseConnectionMethodId = JavaIds::getMethodID(
        env, jPluginClass, "closeConnection",
        "(LShimsJava/RaceHandle;Ljava/lang/String;)LShimsJava/PluginResponse;");
    jPluginDeactivateChannelMethodId = JavaIds::getMethodID(
        env, jPluginClass, "deactivateChannel",
        "(LShimsJava/RaceHandle;Ljava/lang/String;)LShimsJava/PluginResponse;");
    jPluginActivateChannelMethodId = JavaIds::getMethodID(
        env, jPluginClass, "activateChannel",
        "(LShimsJava/RaceHandle;Ljava/lang/String;Ljava/lang/String;)LShimsJava/PluginResponse;");
    jPluginDestroyLinkMethodId = JavaIds::getMethodID(
        env, jPluginClass, "destroyLink",
        "(LShimsJava/RaceHandle;Ljava/lang/String;)LShimsJava/PluginResponse;");
    jPluginCreateLinkMethodId = JavaIds::getMethodID(
        env, jPluginClass, "createLink",
        "(LShimsJava/RaceHandle;Ljava/lang/String;)LShimsJava/PluginResponse;");
    jPluginLoadLinkAddressMethodId = JavaIds::getMethodID(
        env, jPluginClass, "loadLinkAddress",
        "(LShimsJava/RaceHandle;Ljava/lang/String;Ljava/lang/String;)LShimsJava/PluginResponse;");
    jPluginLoadLinkAddressesMethodId = JavaIds::getMethodID(
        env, jPluginClass, "loadLinkAddresses",
        "(LShimsJava/RaceHandle;Ljava/lang/String;[Ljava/lang/String;)LShimsJava/PluginResponse;");
    jPluginCreateLinkFromAddressMethodId = JavaIds::getMethodID(
        env, jPluginClass, "createLinkFromAddress",
        "(LShimsJava/RaceHandle;Ljava/lang/String;Ljava/lang/String;)LShimsJava/PluginResponse;");
    jPluginOnUserInputReceivedMethodId = JavaIds::getMethodID(
        env, jPluginClass, "onUserInputReceived",
        "(LShimsJava/RaceHandle;ZLjava/lang/String;)LShimsJava/PluginResponse;");
    jflushChannelMethodId = JavaIds::getMethodID(
        env, jPluginClass, "flushChannel",
        "(LShimsJava/RaceHandle;Ljava/lang/String;J)LShimsJava/PluginResponse;");
    jserveFilesMethodId =
        JavaIds::getMethodID(env, jPluginClass, "serveFiles",
                             "(Ljava/lang/String;Ljava/lang/String;)LShimsJava/PluginResponse;");
    jcreateBootstrapLinkMethodId = JavaIds::getMethodID(
        env, jPluginClass, "createBootstrapLink",
        "(LShimsJava/RaceHandle;Ljava/lang/String;Ljava/lang/String;)LShimsJava/PluginResponse;");
    jPluginOnUserAcknowledgementReceivedMethodId =
        JavaIds::getMethodID(env, jPluginClass, "onUserAcknowledgementReceived",
                             "(LShimsJava/RaceHandle;)LShimsJava/PluginResponse;");
}

PluginCommsJavaWrapper::~PluginCommsJavaWrapper() {
    RaceLog::logDebug(logLabel, "Destructor Called", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    env->DeleteGlobalRef(jPluginClass);
    env->DeleteGlobalRef(plugin);
    destroyPlugin();
}

void PluginCommsJavaWrapper::linkNativeMethods(IRaceSdkComms *sdk, std::string pluginName,
                                               std::string pluginClassName) {
    RaceLog::logDebug(logLabel, "linkNativeMethods", "");

    if (sdk == nullptr) {
        RaceLog::logError(logLabel,
                          "sdk pointer provided to "
                          "PluginCommsJavaWrapper is nullptr",
                          "");
        return;
    }

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    // Find Comms SDK class
    jclass sdkClass = env->FindClass("ShimsJava/JRaceSdkComms");
    logException(env);
    if (sdkClass == nullptr) {
        const std::string errorMessage =
            "Comms SDK class not found. Make sure racesdk-java-shims jar"
            " exists and is in the class path";
        RaceLog::logError(logLabel, errorMessage, "");
        throw std::runtime_error(errorMessage);
    }

    // Find the Java class for the comms plugin.
    jPluginClass = JavaShimUtils::findDexClass(env, "comms", pluginName, pluginClassName);
    jPluginClass = reinterpret_cast<jclass>(env->NewGlobalRef(jPluginClass));
    logException(env);
    if (jPluginClass == nullptr) {
        const std::string errorMessage = "linkNativeMethods: class not found: " + pluginClassName;
        RaceLog::logError(logLabel, errorMessage, "");
        throw std::runtime_error(errorMessage);
    }

    jmethodID sdkConstructor = env->GetMethodID(sdkClass, "<init>", "(JLjava/lang/String;)V");
    if (sdkConstructor == nullptr) {
        const std::string errorMessage = "Comms SDK constructor not found";
        RaceLog::logError(logLabel, "Comms SDK constructor not found", "");
        throw std::runtime_error(errorMessage);
    }

    jlong sdkPtr = reinterpret_cast<jlong>(sdk);
    jstring jPluginName =
        env->NewStringUTF(pluginName.c_str());  // create java version of pluginName
    jobject sdkObject = env->NewObject(sdkClass, sdkConstructor, sdkPtr, jPluginName);
    logException(env);
    if (sdkObject == nullptr) {
        const std::string errorMessage = "Failed to create Comms SDK";
        RaceLog::logError(logLabel, errorMessage, "");
        throw std::runtime_error(errorMessage);
    }

    // Create comms plugin instance
    jmethodID pluginConstructor =
        env->GetMethodID(jPluginClass, "<init>", "(LShimsJava/JRaceSdkComms;)V");
    if (pluginConstructor == nullptr) {
        const std::string errorMessage = "comms plugin constructor not found";
        RaceLog::logError(logLabel, errorMessage, "");
        throw std::runtime_error(errorMessage);
    }
    jobject local_plugin = env->NewObject(jPluginClass, pluginConstructor, sdkObject);
    plugin = env->NewGlobalRef(local_plugin);
    logException(env);
    if (plugin != nullptr) {
        RaceLog::logDebug(logLabel, "comms plugin instance created", "");
    } else {
        const std::string errorMessage = "Failed to create comms plugin" + pluginClassName;
        RaceLog::logError(logLabel, errorMessage, "");
        throw std::runtime_error(errorMessage);
    }
}

PluginResponse PluginCommsJavaWrapper::init(const PluginConfig &pluginConfig) {
    RaceLog::logDebug(logLabel, "init", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    // Convert args and call init
    jobject jPluginConfig = JavaShimUtils::pluginConfigToJobject(env, pluginConfig);
    jobject jResponse = env->CallObjectMethod(plugin, jPluginInitMethodId, jPluginConfig);
    jthrowable jThrowable = env->ExceptionOccurred();
    if (jThrowable != nullptr) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        RaceLog::logError(logLabel,
                          "Exception caught invoking plugin init: " +
                              JavaShimUtils::getMessageFromJthrowable(env, jThrowable),
                          "");
        return PLUGIN_FATAL;
    }
    return JavaShimUtils::jobjectToPluginResponse(env, jResponse);
}

PluginResponse PluginCommsJavaWrapper::shutdown() {
    RaceLog::logDebug(logLabel, "shutdown", "");
    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    jobject jResponse = env->CallObjectMethod(plugin, jPluginShutdownMethodId);
    jthrowable jThrowable = env->ExceptionOccurred();
    if (jThrowable != nullptr) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        RaceLog::logError(logLabel,
                          "Exception caught invoking plugin shutdown: " +
                              JavaShimUtils::getMessageFromJthrowable(env, jThrowable),
                          "");
        return PLUGIN_FATAL;
    }
    PluginResponse response = JavaShimUtils::jobjectToPluginResponse(env, jResponse);
    destroyPlugin();
    return response;
}

PluginResponse PluginCommsJavaWrapper::sendPackage(RaceHandle handle, ConnectionID connectionId,
                                                   EncPkg pkg, double timeoutTimestamp,
                                                   uint64_t batchId) {
    RaceLog::logDebug(logLabel, "sendPackage", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    // Covert params
    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jstring jConnectionId = env->NewStringUTF(connectionId.c_str());
    jobject jEncPkg = JavaShimUtils::encPkgToJobject(env, pkg);
    jdouble jTimeoutTimestamp = static_cast<double>(timeoutTimestamp);
    jlong jBatchId = static_cast<long>(batchId);

    jobject jResponse = env->CallObjectMethod(plugin, jPluginSendPackageMethodId, jHandle,
                                              jConnectionId, jEncPkg, jTimeoutTimestamp, jBatchId);
    jthrowable jThrowable = env->ExceptionOccurred();
    if (jThrowable != nullptr) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        RaceLog::logError(logLabel,
                          "Exception caught invoking plugin sendPackage: " +
                              JavaShimUtils::getMessageFromJthrowable(env, jThrowable),
                          "");
        return PLUGIN_FATAL;
    }
    return JavaShimUtils::jobjectToPluginResponse(env, jResponse);
}

PluginResponse PluginCommsJavaWrapper::openConnection(RaceHandle handle, LinkType linkType,
                                                      LinkID linkId, std::string linkHints,
                                                      int32_t sendTimeout) {
    RaceLog::logDebug(logLabel, "openConnection", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    // convert params
    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jobject jLinkType = JavaShimUtils::linkTypeToJLinkType(env, linkType);
    RaceLog::logDebug(logLabel, "got link type", "");
    jstring jLinkId = env->NewStringUTF(linkId.c_str());
    jstring jlinkHints = env->NewStringUTF(linkHints.c_str());
    jint jSendTimeout = sendTimeout;

    jobject jResponse =
        static_cast<jstring>(env->CallObjectMethod(plugin, jPluginOpenConnectionMethodId, jHandle,
                                                   jLinkType, jLinkId, jlinkHints, jSendTimeout));
    jthrowable jThrowable = env->ExceptionOccurred();
    if (jThrowable != nullptr) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        RaceLog::logError(logLabel,
                          "Exception caught invoking plugin openConnection: " +
                              JavaShimUtils::getMessageFromJthrowable(env, jThrowable),
                          "");
        return PLUGIN_FATAL;
    }
    return JavaShimUtils::jobjectToPluginResponse(env, jResponse);
}

PluginResponse PluginCommsJavaWrapper::closeConnection(RaceHandle handle,
                                                       ConnectionID connectionId) {
    RaceLog::logDebug(logLabel, "closeConnection", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jstring jConnectionId = env->NewStringUTF(connectionId.c_str());
    jobject jResponse =
        env->CallObjectMethod(plugin, jPluginCloseConnectionMethodId, jHandle, jConnectionId);
    jthrowable jThrowable = env->ExceptionOccurred();
    if (jThrowable != nullptr) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        RaceLog::logError(logLabel,
                          "Exception caught invoking plugin closeConnection: " +
                              JavaShimUtils::getMessageFromJthrowable(env, jThrowable),
                          "");
        return PLUGIN_FATAL;
    }
    return JavaShimUtils::jobjectToPluginResponse(env, jResponse);
}

PluginResponse PluginCommsJavaWrapper::destroyLink(RaceHandle handle, LinkID linkId) {
    RaceLog::logDebug(logLabel, "destroyLink", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);
    // Find destroyLink method
    jmethodID pluginDestroyLink =
        env->GetMethodID(jPluginClass, "destroyLink",
                         "(LShimsJava/RaceHandle;Ljava/lang/String;)LShimsJava/PluginResponse;");
    if (pluginDestroyLink == nullptr) {
        RaceLog::logError(logLabel, "Could not find destroyLink", "");
        return PLUGIN_FATAL;
    }
    RaceLog::logDebug(logLabel, "destroyLink method found", "");
    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jstring jLinkId = env->NewStringUTF(linkId.c_str());
    jobject jResponse = env->CallObjectMethod(plugin, jPluginDestroyLinkMethodId, jHandle, jLinkId);
    jthrowable jThrowable = env->ExceptionOccurred();
    if (jThrowable != nullptr) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        RaceLog::logError(logLabel,
                          "Exception caught invoking plugin destroyLink: " +
                              JavaShimUtils::getMessageFromJthrowable(env, jThrowable),
                          "");
        return PLUGIN_FATAL;
    }
    return JavaShimUtils::jobjectToPluginResponse(env, jResponse);
}

PluginResponse PluginCommsJavaWrapper::deactivateChannel(RaceHandle handle,
                                                         std::string channelGid) {
    RaceLog::logDebug(logLabel, "deactivateChannel", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jstring jChannelGid = env->NewStringUTF(channelGid.c_str());
    jobject jResponse =
        env->CallObjectMethod(plugin, jPluginDeactivateChannelMethodId, jHandle, jChannelGid);
    jthrowable jThrowable = env->ExceptionOccurred();
    if (jThrowable != nullptr) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        RaceLog::logError(logLabel,
                          "Exception caught invoking plugin deactivateChannel: " +
                              JavaShimUtils::getMessageFromJthrowable(env, jThrowable),
                          "");
        return PLUGIN_FATAL;
    }
    return JavaShimUtils::jobjectToPluginResponse(env, jResponse);
}

PluginResponse PluginCommsJavaWrapper::activateChannel(RaceHandle handle, std::string channelGid,
                                                       std::string roleName) {
    RaceLog::logDebug(logLabel, "activateChannel", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jstring jChannelGid = env->NewStringUTF(channelGid.c_str());
    jstring jRoleName = env->NewStringUTF(roleName.c_str());
    jobject jResponse = env->CallObjectMethod(plugin, jPluginActivateChannelMethodId, jHandle,
                                              jChannelGid, jRoleName);
    jthrowable jThrowable = env->ExceptionOccurred();
    if (jThrowable != nullptr) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        RaceLog::logError(logLabel,
                          "Exception caught invoking plugin activateChannel: " +
                              JavaShimUtils::getMessageFromJthrowable(env, jThrowable),
                          "");
        return PLUGIN_FATAL;
    }
    return JavaShimUtils::jobjectToPluginResponse(env, jResponse);
}

PluginResponse PluginCommsJavaWrapper::createLink(RaceHandle handle, std::string channelGid) {
    RaceLog::logDebug(logLabel, "createLink", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    // convert params
    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jstring jChannelGid = env->NewStringUTF(channelGid.c_str());

    jobject jResponse = static_cast<jstring>(
        env->CallObjectMethod(plugin, jPluginCreateLinkMethodId, jHandle, jChannelGid));
    jthrowable jThrowable = env->ExceptionOccurred();
    if (jThrowable != nullptr) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        RaceLog::logError(logLabel,
                          "Exception caught invoking plugin createLink: " +
                              JavaShimUtils::getMessageFromJthrowable(env, jThrowable),
                          "");
        return PLUGIN_FATAL;
    }
    return JavaShimUtils::jobjectToPluginResponse(env, jResponse);
}

PluginResponse PluginCommsJavaWrapper::loadLinkAddress(RaceHandle handle, std::string channelGid,
                                                       std::string linkAddress) {
    RaceLog::logDebug(logLabel, "loadLinkAddress", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    // convert params
    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jstring jChannelGid = env->NewStringUTF(channelGid.c_str());
    jstring jLinkAddress = env->NewStringUTF(linkAddress.c_str());

    jobject jResponse = static_cast<jstring>(env->CallObjectMethod(
        plugin, jPluginLoadLinkAddressMethodId, jHandle, jChannelGid, jLinkAddress));
    jthrowable jThrowable = env->ExceptionOccurred();
    if (jThrowable != nullptr) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        RaceLog::logError(logLabel,
                          "Exception caught invoking plugin loadLinkAddress: " +
                              JavaShimUtils::getMessageFromJthrowable(env, jThrowable),
                          "");
        return PLUGIN_FATAL;
    }
    return JavaShimUtils::jobjectToPluginResponse(env, jResponse);
}

PluginResponse PluginCommsJavaWrapper::loadLinkAddresses(RaceHandle handle, std::string channelGid,
                                                         std::vector<std::string> linkAddresses) {
    RaceLog::logDebug(logLabel, "loadLinkAddresses", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    // convert params
    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jstring jChannelGid = env->NewStringUTF(channelGid.c_str());
    jobjectArray jLinkAddresses = JavaShimUtils::stringVectorToJArray(env, linkAddresses);

    jobject jResponse = static_cast<jstring>(env->CallObjectMethod(
        plugin, jPluginLoadLinkAddressesMethodId, jHandle, jChannelGid, jLinkAddresses));
    jthrowable jThrowable = env->ExceptionOccurred();
    if (jThrowable != nullptr) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        RaceLog::logError(logLabel,
                          "Exception caught invoking plugin loadLinkAddresses: " +
                              JavaShimUtils::getMessageFromJthrowable(env, jThrowable),
                          "");
        return PLUGIN_FATAL;
    }
    return JavaShimUtils::jobjectToPluginResponse(env, jResponse);
}

PluginResponse PluginCommsJavaWrapper::createLinkFromAddress(RaceHandle handle,
                                                             std::string channelGid,
                                                             std::string linkAddress) {
    RaceLog::logDebug(logLabel, "createLinkFromAddress", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    // convert params
    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jstring jChannelGid = env->NewStringUTF(channelGid.c_str());
    jstring jLinkAddress = env->NewStringUTF(linkAddress.c_str());

    jobject jResponse = static_cast<jstring>(env->CallObjectMethod(
        plugin, jPluginCreateLinkFromAddressMethodId, jHandle, jChannelGid, jLinkAddress));
    jthrowable jThrowable = env->ExceptionOccurred();
    if (jThrowable != nullptr) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        RaceLog::logError(logLabel,
                          "Exception caught invoking plugin createLinkFromAddress: " +
                              JavaShimUtils::getMessageFromJthrowable(env, jThrowable),
                          "");
        return PLUGIN_FATAL;
    }
    return JavaShimUtils::jobjectToPluginResponse(env, jResponse);
}

PluginResponse PluginCommsJavaWrapper::onUserInputReceived(RaceHandle handle, bool answered,
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

PluginResponse PluginCommsJavaWrapper::serveFiles(LinkID linkId, std::string path) {
    RaceLog::logDebug(logLabel, "serveFiles: called", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    jstring jLinkId = env->NewStringUTF(linkId.c_str());
    jstring jPath = env->NewStringUTF(path.c_str());

    jobject jResponse = env->CallObjectMethod(plugin, jserveFilesMethodId, jLinkId, jPath);
    jthrowable jThrowable = env->ExceptionOccurred();
    if (jThrowable != nullptr) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        RaceLog::logError(logLabel,
                          "Exception caught invoking plugin serveFiles: " +
                              JavaShimUtils::getMessageFromJthrowable(env, jThrowable),
                          "");
        return PLUGIN_FATAL;
    }
    RaceLog::logDebug(logLabel, "serveFiles: returned", "");
    return JavaShimUtils::jobjectToPluginResponse(env, jResponse);
};

PluginResponse PluginCommsJavaWrapper::createBootstrapLink(RaceHandle handle,
                                                           std::string channelGid,
                                                           std::string passphrase) {
    RaceLog::logDebug(logLabel, "createBootstrapLink: called", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jstring jChannelId = env->NewStringUTF(channelGid.c_str());
    jstring jPassPhrase = env->NewStringUTF(passphrase.c_str());

    jobject jResponse = env->CallObjectMethod(plugin, jcreateBootstrapLinkMethodId, jHandle,
                                              jChannelId, jPassPhrase);
    jthrowable jThrowable = env->ExceptionOccurred();
    if (jThrowable != nullptr) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        RaceLog::logError(logLabel,
                          "Exception caught invoking plugin createBootstrapLink: " +
                              JavaShimUtils::getMessageFromJthrowable(env, jThrowable),
                          "");
        return PLUGIN_FATAL;
    }
    RaceLog::logDebug(logLabel, "createBootstrapLink: returned", "");
    return JavaShimUtils::jobjectToPluginResponse(env, jResponse);
};

PluginResponse PluginCommsJavaWrapper::flushChannel(RaceHandle handle, std::string channelGid,
                                                    uint64_t batchId) {
    RaceLog::logDebug(logLabel, "flushChannel: called", "");

    JNIEnv *env;
    JavaShimUtils::getEnv(&env, jvm);

    jobject jHandle = JavaShimUtils::raceHandleToJobject(env, handle);
    jstring jChannelId = env->NewStringUTF(channelGid.c_str());
    jlong jBatchId = static_cast<long>(batchId);

    jobject jResponse =
        env->CallObjectMethod(plugin, jflushChannelMethodId, jHandle, jChannelId, jBatchId);
    jthrowable jThrowable = env->ExceptionOccurred();
    if (jThrowable != nullptr) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        RaceLog::logError(logLabel,
                          "Exception caught invoking plugin flushChannel: " +
                              JavaShimUtils::getMessageFromJthrowable(env, jThrowable),
                          "");
        return PLUGIN_FATAL;
    }
    RaceLog::logDebug(logLabel, "flushChannel: returned", "");
    return JavaShimUtils::jobjectToPluginResponse(env, jResponse);
}

void PluginCommsJavaWrapper::destroyPlugin() {
    plugin = nullptr;
}

PluginResponse PluginCommsJavaWrapper::onUserAcknowledgementReceived(RaceHandle handle) {
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
