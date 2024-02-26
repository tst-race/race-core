
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

#include "JavaShimUtils.h"

#include <RaceLog.h>
#include <pthread.h>

#include <cstring>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "JavaIds.h"

#define ENABLE_DEBUG_LOGGING 0

static pthread_key_t key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

static std::string logLabel = "JavaShimUtils";
static JavaVM *g_jvm = nullptr;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *) {
    RaceLog::logDebug(logLabel, "JNI_OnLoad called", "");
    g_jvm = jvm;

    JNIEnv *env;
    if (!JavaShimUtils::getEnv(&env, g_jvm)) {
        RaceLog::logError(logLabel, "JNI_OnLoad: failed to get JVM!", "");
        return JNI_ERR;
    }

    try {
        JavaIds::load(env);
    } catch (const std::runtime_error &e) {
        RaceLog::logError(logLabel, "JNI_OnLoad: " + std::string(e.what()), "");
        return JNI_ERR;
    }

    RaceLog::logDebug(logLabel, "JNI_OnLoad returned", "");
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *, void *) {
    RaceLog::logDebug(logLabel, "JNI_OnUnload called", "");

    JNIEnv *env;
    if (!JavaShimUtils::getEnv(&env, g_jvm)) {
        RaceLog::logError(logLabel, "JNI_OnUnload: failed to get JVM!", "");
    }

    JavaIds::unload(env);
}

std::string getStringFieldFromClassObject(JNIEnv *env, jobject &classObject,
                                          const std::string &fieldName) {
    const jclass cls = env->GetObjectClass(classObject);

    jfieldID fieldId = env->GetFieldID(cls, fieldName.c_str(), "Ljava/lang/String;");
    if (fieldId == nullptr) {
        RaceLog::logError(logLabel, "failed to find field ID for field: " + fieldName, "");
        return "";
    }

    jstring stringObject = static_cast<jstring>(env->GetObjectField(classObject, fieldId));
    const std::string result = JavaShimUtils::jstring2string(env, stringObject);
    return result;
}

void setStringFieldFromClassObject(JNIEnv *env, jobject &classObject, const std::string &fieldName,
                                   std::string value) {
    const jclass cls = env->GetObjectClass(classObject);

    jfieldID fieldId = env->GetFieldID(cls, fieldName.c_str(), "Ljava/lang/String;");
    if (fieldId == nullptr) {
        RaceLog::logError(logLabel, "failed to find field ID for field: " + fieldName, "");
    }

    jstring jValue = env->NewStringUTF(value.c_str());
    env->SetObjectField(classObject, fieldId, jValue);
}

bool getBooleanFieldFromClassObject(JNIEnv *env, jobject &classObject,
                                    const std::string &fieldName) {
    const jclass cls = env->GetObjectClass(classObject);

    jfieldID fieldId = env->GetFieldID(cls, fieldName.c_str(), "Z");
    if (fieldId == nullptr) {
        RaceLog::logError(logLabel, "failed to find field ID for field: " + fieldName, "");
        return "";
    }

    jboolean booleanObject = static_cast<jboolean>(env->GetBooleanField(classObject, fieldId));
    const bool result = static_cast<bool>(booleanObject);
    return result;
}

long getLongFieldFromClassObject(JNIEnv *env, jobject &classObject, const std::string &fieldName) {
    const jclass cls = env->GetObjectClass(classObject);
    jfieldID fieldId = env->GetFieldID(cls, fieldName.c_str(), "J");
    if (fieldId == nullptr) {
        RaceLog::logError(
            logLabel,
            "getLongFieldFromClassObject: failed to find field ID for field: " + fieldName, "");
        return 0;
    }

    const long result = static_cast<long>(env->GetLongField(classObject, fieldId));
    return result;
}

jclass JavaShimUtils::findClass(JNIEnv *env, std::string className) {
    return JavaIds::getClassID(env, className.c_str());
}

#ifndef __ANDROID__
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

/**
 * @brief Create java classpath string by looking for jar files in given searchDirectory
 *
 * @param searchDirectory Path to start search
 * @param recursive whether or not to recursively search sub directories
 *
 * @return classPath
 */
static std::string getClassPath(std::string searchDirectory, bool recursive) {
    std::string classPath = "";

    try {
        for (const fs::directory_entry &entry : fs::directory_iterator(searchDirectory)) {
            if (recursive && fs::is_directory(entry)) {
                classPath += getClassPath(entry.path().string(), recursive);
            } else if (fs::is_regular_file(entry) && entry.path().extension() == ".jar") {
                classPath += ":" + entry.path().string();
            }
        }
    } catch (const std::exception &e) {
        RaceLog::logError("getClassPath",
                          "Failed to open path '" + searchDirectory + "': " + std::string(e.what()),
                          "");
    }

    return classPath;
}

/**
 * @brief Create java.library.path string by looking for SOs files in given searchDirectory
 *
 * @param searchDirectory Path to start search
 *
 * @return libraryPath
 */
static std::string getLibraryPath(std::string searchDirectory) {
    std::string libraryPath = "";

    try {
        for (const fs::directory_entry &entry : fs::directory_iterator(searchDirectory)) {
            if (fs::is_directory(entry)) {
                libraryPath += getLibraryPath(entry.path().string());
            } else if (fs::is_regular_file(entry) && entry.path().extension() == ".so") {
                libraryPath += ":" + entry.path().parent_path().string();
            }
        }
    } catch (const std::exception &e) {
        RaceLog::logError("getLibraryPath",
                          "Failed to open path '" + searchDirectory + "': " + std::string(e.what()),
                          "");
    }

    return libraryPath;
}
#endif

JavaVM *JavaShimUtils::getJvm() {
#ifndef __ANDROID__
    if (g_jvm == nullptr) {
        JNIEnv *env = nullptr;
        JavaVMInitArgs vm_args;  // Arguments for the JVM (see below)

        std::string compiler = std::string("-Djava.compiler=NONE");
        // classpath requires '.' for unit tests
        std::string classPath =
            "-Djava.class.path=." + getClassPath("/usr/local/lib/race/java", true) +
            getClassPath("/usr/local/lib/race/network-manager", true) +
            getClassPath("/usr/local/lib/race/comms", true) +
            getClassPath("/usr/local/lib/race/core", true) + getClassPath(".", false);
        std::string libraryPath = "-Djava.library.path=.:/usr/local/lib" +
                                  getLibraryPath("/usr/local/lib/race/network-manager") +
                                  getLibraryPath("/usr/local/lib/race/comms") +
                                  getLibraryPath("/usr/local/lib/race/core") +
                                  getLibraryPath("/usr/local/lib/race/core/race/lib");

        // A list of options to build a JVM from C++
        JavaVMOption options[3];

        // data() because it requires non-const char*
        options[0].optionString = strdup(compiler.data());
        options[1].optionString = strdup(classPath.data());
        options[2].optionString = strdup(libraryPath.data());

        RaceLog::logDebug(logLabel, "jvm class path: ", "");
        RaceLog::logDebug(logLabel, options[1].optionString, "");

        RaceLog::logDebug(logLabel, "jvm java.library.path: ", "");
        RaceLog::logDebug(logLabel, options[2].optionString, "");

        // Setting the arguments to create a JVM...
        memset(&vm_args, 0, sizeof(vm_args));
        vm_args.version = JNI_VERSION_1_6;
        vm_args.nOptions = 3;
        vm_args.options = options;
        vm_args.ignoreUnrecognized = true;

        long status = JNI_CreateJavaVM(&g_jvm, reinterpret_cast<void **>(&env), &vm_args);

        if (status == JNI_OK) {
            jint ver = env->GetVersion();
            std::stringstream ss;
            ss << ((ver >> 16) & 0x0f) << "." << (ver & 0x0f) << std::endl;
            RaceLog::logInfo(logLabel, "JVM load succeeded: Version " + ss.str(), "");
        } else if (status == JNI_ERR) {
            std::string message = "Failed to create JVM: unknown error";
            RaceLog::logError(logLabel, message, "");
            throw std::runtime_error(message);
        } else if (status == JNI_EDETACHED) {
            std::string message = "Failed to create JVM: not attached";
            RaceLog::logDebug(logLabel, message, "");
            throw std::runtime_error(message);
        } else if (status == JNI_EVERSION) {
            std::string message = "Failed to create JVM: version not supported";
            RaceLog::logError(logLabel, message, "");
            throw std::runtime_error(message);
        } else if (status == JNI_ENOMEM) {
            std::string message = "Failed to create JVM: not enough memory";
            RaceLog::logError(logLabel, message, "");
            throw std::runtime_error(message);
        } else if (status == JNI_EEXIST) {
            std::string message = "Failed to create JVM: VM already created";
            RaceLog::logError(logLabel, message, "");
            throw std::runtime_error(message);
        } else if (status == JNI_EINVAL) {
            std::string message = "Failed to create JVM: invalid arguments";
            RaceLog::logError(logLabel, message, "");
            throw std::runtime_error(message);
        } else {
            std::string message =
                "Failed to create JVM: unknown return code: " + std::to_string(status);
            RaceLog::logError(logLabel, message, "");
            throw std::runtime_error(message);
        }
    }
#endif

    return g_jvm;
}

void JavaShimUtils::destroyJvm() {
    RaceLog::logDebug(logLabel, "Destroy JVM called", "");
    if (g_jvm != nullptr) {
        g_jvm->DestroyJavaVM();
        g_jvm = nullptr;
    }
}

jclass JavaShimUtils::findDexClass(JNIEnv *env, const std::string &pluginType, std::string dexName,
                                   const std::string &className) {
#ifdef __ANDROID__
    // get android class loader
    jobject jAndroidContext = JavaIds::getGlobalContext(env);
    jclass jContextClass = env->FindClass("android/content/Context");
    jmethodID jGetClassLoader =
        env->GetMethodID(jContextClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject jClassLoaderObject = env->CallObjectMethod(jAndroidContext, jGetClassLoader);

    jclass jDexClassLoaderClass = env->FindClass("dalvik/system/DexClassLoader");
    if (jDexClassLoaderClass == nullptr) {
        RaceLog::logError(logLabel, "DexClassLoader class not found", "");
        return nullptr;
    }
    jmethodID jDexClassLoaderConstructor = env->GetMethodID(
        jDexClassLoaderClass, "<init>",
        "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/ClassLoader;)V");
    if (jDexClassLoaderConstructor == nullptr) {
        RaceLog::logError(logLabel, "DexClassLoader constructor not found", "");
        return nullptr;
    }

    dexName = "/data/data/com.twosix.race/race/artifacts/" + pluginType + "/" + dexName + "/" +
              dexName + ".dex";
    jstring jDexName = env->NewStringUTF(dexName.c_str());

    jobject jDexClassLoader = env->NewObject(jDexClassLoaderClass, jDexClassLoaderConstructor,
                                             jDexName, NULL, NULL, jClassLoaderObject);
    if (jDexClassLoader == nullptr) {
        RaceLog::logError(logLabel, "Failed to create DexClassLoader", "");
        return nullptr;
    }

    // Find java plugin
    jmethodID jLoadClass = env->GetMethodID(jDexClassLoaderClass, "loadClass",
                                            "(Ljava/lang/String;)Ljava/lang/Class;");
    jstring jClassName = env->NewStringUTF(className.c_str());
    return jclass(env->CallObjectMethod(jDexClassLoader, jLoadClass, jClassName));
#else
    (void)pluginType;
    (void)dexName;
    // Find Java class
    return env->FindClass(className.c_str());
#endif
}

void JavaShimUtils::make_key() {
    (void)pthread_key_create(&key, &ThreadDestructor);
}

std::string JavaShimUtils::jstring2string(JNIEnv *env, jstring jStr) {
    if (jStr == nullptr) {
        return "";
    }

    const jbyteArray stringJbytes = static_cast<jbyteArray>(
        env->CallObjectMethod(jStr, JavaIds::jStringGetBytesMethodId, env->NewStringUTF("UTF-8")));

    size_t length = static_cast<size_t>(env->GetArrayLength(stringJbytes));
    jbyte *pBytes = env->GetByteArrayElements(stringJbytes, NULL);

    std::string ret = std::string(reinterpret_cast<char *>(pBytes), length);
    env->ReleaseByteArrayElements(stringJbytes, pBytes, JNI_ABORT);

    env->DeleteLocalRef(stringJbytes);

    return ret;
}

std::vector<std::uint8_t> JavaShimUtils::jobjectToVectorUint8(JNIEnv *env, jobject &theJavaObject) {
    jbyteArray *javaObjectArray = reinterpret_cast<jbyteArray *>(&theJavaObject);

    jbyte *javaObjectBytes = env->GetByteArrayElements(*javaObjectArray, nullptr);

    const std::int32_t javaObjectSize =
        static_cast<std::int32_t>(env->GetArrayLength(*javaObjectArray));

    std::vector<std::uint8_t> result;
    for (std::int32_t index = 0; index < javaObjectSize; ++index) {
        result.push_back(static_cast<unsigned char>(javaObjectBytes[index]));
    }

    env->ReleaseByteArrayElements(*javaObjectArray, javaObjectBytes, 0);

    return result;
}

LinkType JavaShimUtils::jobjectToLinkType(JNIEnv *env, jobject &javaLinkType) {
    const jint value = env->CallIntMethod(javaLinkType, JavaIds::jLinkTypeOrdinalMethodId);
    return static_cast<LinkType>(value);
}

jobject JavaShimUtils::linkTypeToJLinkType(JNIEnv *env, LinkType linkType) {
    return env->CallStaticObjectMethod(JavaIds::jLinkTypeClassId,
                                       JavaIds::jLinkTypeValueOfStaticMethodId,
                                       static_cast<jint>(linkType));
}

LinkDirection JavaShimUtils::jobjectToLinkDirection(JNIEnv *env, jobject &javaLinkDirection) {
    const jint value =
        env->CallIntMethod(javaLinkDirection, JavaIds::jLinkDirectionOrdinalMethodId);
    return static_cast<LinkDirection>(value);
}

jobject JavaShimUtils::linkDirectionToJLinkDirection(JNIEnv *env, LinkDirection linkDirection) {
    return env->CallStaticObjectMethod(JavaIds::jLinkDirectionClassId,
                                       JavaIds::jLinkDirectionValueOfStaticMethodId,
                                       static_cast<jint>(linkDirection));
}

jobject JavaShimUtils::transmissionTypeToJobject(JNIEnv *env, TransmissionType transmissionType) {
    return env->CallStaticObjectMethod(JavaIds::jTransmissionTypeClassId,
                                       JavaIds::jTransmissionTypeValueOfStaticMethodId,
                                       static_cast<jint>(transmissionType));
}

jobject JavaShimUtils::linkSideToJobject(JNIEnv *env, LinkSide linkSide) {
    return env->CallStaticObjectMethod(JavaIds::jLinkSideClassId,
                                       JavaIds::jLinkSideValueOfStaticMethodId,
                                       static_cast<jint>(linkSide));
}

ConnectionType JavaShimUtils::jobjectToConnectionType(JNIEnv *env, jobject &javaConnectionType) {
    const jint value =
        env->CallIntMethod(javaConnectionType, JavaIds::jConnectionTypeOrdinalMethodId);
    return static_cast<ConnectionType>(value);
}

jobject JavaShimUtils::connectionTypeToJConnectionType(JNIEnv *env, ConnectionType connectionType) {
    return env->CallStaticObjectMethod(JavaIds::jConnectionTypeClassId,
                                       JavaIds::jConnectionTypeValueOfStaticMethodId,
                                       static_cast<jint>(connectionType));
}

SendType JavaShimUtils::jobjectToSendType(JNIEnv *env, jobject &javaSendType) {
    const jint value = env->CallIntMethod(javaSendType, JavaIds::jSendTypeOrdinalMethodId);
    return static_cast<SendType>(value);
}

jobject JavaShimUtils::sendTypeToJSendType(JNIEnv *env, SendType sendType) {
    return env->CallStaticObjectMethod(JavaIds::jSendTypeClassId,
                                       JavaIds::jSendTypeValueOfStaticMethodId,
                                       static_cast<jint>(sendType));
}

RaceHandle JavaShimUtils::jobjectToRaceHandle(JNIEnv *env, jobject &jRaceHandle) {
    return static_cast<RaceHandle>(
        static_cast<long>(env->GetLongField(jRaceHandle, JavaIds::jRaceHandleValueFieldId)));
}

jobject JavaShimUtils::raceHandleToJobject(JNIEnv *env, RaceHandle handle) {
    jlong jHandle = static_cast<jlong>(handle);
    return env->NewObject(JavaIds::jRaceHandleClassId, JavaIds::jRaceHandleConstructorMethodId,
                          jHandle);
}

jobject JavaShimUtils::sdkStatusToJobject(JNIEnv *env, SdkStatus status) {
    return env->CallStaticObjectMethod(JavaIds::jSdkStatusClassId,
                                       JavaIds::jSdkStatusValueOfStaticMethodId,
                                       static_cast<jint>(status));
}

SdkStatus JavaShimUtils::jobjectToSdkStatus(JNIEnv *env, jobject &jStatus) {
    if (jStatus == nullptr) {
        return SDK_INVALID;
    }
    return static_cast<SdkStatus>(
        static_cast<std::int32_t>(env->GetIntField(jStatus, JavaIds::jSdkStatusValueFieldId)));
}

jobject JavaShimUtils::sdkResponseToJobject(JNIEnv *env, const SdkResponse &response) {
    jobject status = JavaShimUtils::sdkStatusToJobject(env, response.status);
    jdouble queueUtilization = static_cast<jdouble>(response.queueUtilization);
    jobject handle = JavaShimUtils::raceHandleToJobject(env, response.handle);
    return env->NewObject(JavaIds::jSdkResponseClassId, JavaIds::jSdkResponseConstructorMethodId,
                          status, queueUtilization, handle);
}

SdkResponse JavaShimUtils::jobjectToSdkResponse(JNIEnv *env, jobject &jResponse) {
    jobject jStatus = env->GetObjectField(jResponse, JavaIds::jSdkResponseSdkStatusFieldId);
    jdouble jQueueUtilization = static_cast<double>(
        env->GetDoubleField(jResponse, JavaIds::jSdkResponseQueueUtilizationFieldId));
    jobject jHandle = env->GetObjectField(jResponse, JavaIds::jSdkResponseHandleFieldId);

    SdkResponse response;
    response.status = JavaShimUtils::jobjectToSdkStatus(env, jStatus);
    response.queueUtilization = static_cast<double>(jQueueUtilization);
    response.handle = JavaShimUtils::jobjectToRaceHandle(env, jHandle);
    return response;
}

jobject JavaShimUtils::packageStatusToJobject(JNIEnv *env, PackageStatus status) {
    return env->CallStaticObjectMethod(JavaIds::jPackageStatusClassId,
                                       JavaIds::jPackageStatusValueOfStaticMethodId,
                                       static_cast<jint>(status));
}

PackageStatus JavaShimUtils::jobjectToPackageStatus(JNIEnv *env, jobject &jPackageStatus) {
    if (jPackageStatus == nullptr) {
        return PACKAGE_INVALID;
    }
    return static_cast<PackageStatus>(static_cast<std::int32_t>(
        env->GetIntField(jPackageStatus, JavaIds::jPackageStatusValueFieldId)));
}

ConnectionStatus JavaShimUtils::jobjectToConnectionStatus(JNIEnv *env, jobject &jConnectionStatus) {
    if (jConnectionStatus == nullptr) {
        return CONNECTION_INVALID;
    }
    return static_cast<ConnectionStatus>(static_cast<std::int32_t>(
        env->GetIntField(jConnectionStatus, JavaIds::jConnectionStatusValueFieldId)));
}

jobject JavaShimUtils::connectionStatusToJobject(JNIEnv *env, ConnectionStatus status) {
    return env->CallStaticObjectMethod(JavaIds::jConnectionStatusClassId,
                                       JavaIds::jConnectionStatusValueOfStaticMethodId,
                                       static_cast<jint>(status));
}

LinkStatus JavaShimUtils::jobjectToLinkStatus(JNIEnv *env, jobject &jLinkStatus) {
    if (jLinkStatus == nullptr) {
        return LINK_UNDEF;
    }
    return static_cast<LinkStatus>(
        static_cast<std::int32_t>(env->GetIntField(jLinkStatus, JavaIds::jLinkStatusValueFieldId)));
}

jobject JavaShimUtils::linkStatusToJobject(JNIEnv *env, LinkStatus status) {
    return env->CallStaticObjectMethod(JavaIds::jLinkStatusClassId,
                                       JavaIds::jLinkStatusValueOfStaticMethodId,
                                       static_cast<jint>(status));
}

ChannelStatus JavaShimUtils::jobjectToChannelStatus(JNIEnv *env, jobject &jChannelStatus) {
    if (jChannelStatus == nullptr) {
        return CHANNEL_UNDEF;
    }
    return static_cast<ChannelStatus>(static_cast<std::int32_t>(
        env->GetIntField(jChannelStatus, JavaIds::jChannelStatusValueFieldId)));
}

jobject JavaShimUtils::channelStatusToJobject(JNIEnv *env, ChannelStatus status) {
    return env->CallStaticObjectMethod(JavaIds::jChannelStatusClassId,
                                       JavaIds::jChannelStatusValueOfStaticMethodId,
                                       static_cast<jint>(status));
}

PluginStatus JavaShimUtils::jobjectToPluginStatus(JNIEnv *env, jobject &jPluginStatus) {
    if (jPluginStatus == nullptr) {
        return PLUGIN_UNDEF;
    }
    return static_cast<PluginStatus>(static_cast<std::int32_t>(
        env->GetIntField(jPluginStatus, JavaIds::jPluginStatusValueFieldId)));
}

jobject JavaShimUtils::pluginStatusToJobject(JNIEnv *env, PluginStatus status) {
    return env->CallStaticObjectMethod(JavaIds::jPluginStatusClassId,
                                       JavaIds::jPluginStatusValueOfStaticMethodId,
                                       static_cast<jint>(status));
}

jobject JavaShimUtils::pluginResponseToJobject(JNIEnv *env, PluginResponse pluginResponse) {
    return env->CallStaticObjectMethod(JavaIds::jPluginResponseClassId,
                                       JavaIds::jPluginResponseValueOfStaticMethodId,
                                       static_cast<jint>(pluginResponse));
}

PluginResponse JavaShimUtils::jobjectToPluginResponse(JNIEnv *env, jobject &jPluginResponse) {
    if (jPluginResponse == nullptr) {
        return PLUGIN_INVALID;
    }
    return static_cast<PluginResponse>(static_cast<std::int32_t>(
        env->GetIntField(jPluginResponse, JavaIds::jPluginResponseValueFieldId)));
}

jobject JavaShimUtils::pluginConfigToJobject(JNIEnv *env, PluginConfig pluginConfig) {
    jobject jPluginConfig =
        env->NewObject(JavaIds::jPluginConfigClassId, JavaIds::jPluginConfigConstructorMethodId,
                       env->NewStringUTF(pluginConfig.etcDirectory.c_str()),
                       env->NewStringUTF(pluginConfig.loggingDirectory.c_str()),
                       env->NewStringUTF(pluginConfig.auxDataDirectory.c_str()),
                       env->NewStringUTF(pluginConfig.tmpDirectory.c_str()),
                       env->NewStringUTF(pluginConfig.pluginDirectory.c_str()));
    return jPluginConfig;
}

PluginConfig JavaShimUtils::jobjectToPluginConfig(JNIEnv *env, jobject &jPluginConfig) {
    PluginConfig pluginConfig;
    pluginConfig.etcDirectory =
        jstring2string(env, static_cast<jstring>(env->GetObjectField(
                                jPluginConfig, JavaIds::jPluginConfigEtcDirectoryFieldId)));
    pluginConfig.loggingDirectory =
        jstring2string(env, static_cast<jstring>(env->GetObjectField(
                                jPluginConfig, JavaIds::jPluginConfigLoggingDirectoryFieldId)));
    pluginConfig.auxDataDirectory =
        jstring2string(env, static_cast<jstring>(env->GetObjectField(
                                jPluginConfig, JavaIds::jPluginConfigAuxDataDirectoryFieldId)));
    pluginConfig.tmpDirectory =
        jstring2string(env, static_cast<jstring>(env->GetObjectField(
                                jPluginConfig, JavaIds::jPluginConfigTmpDirectoryFieldId)));
    pluginConfig.pluginDirectory =
        jstring2string(env, static_cast<jstring>(env->GetObjectField(
                                jPluginConfig, JavaIds::jPluginConfigPluginDirectoryFieldId)));

    return pluginConfig;
}

jobject JavaShimUtils::messageStatusToJobject(JNIEnv *env, MessageStatus status) {
    return env->CallStaticObjectMethod(JavaIds::jMessageStatusClassId,
                                       JavaIds::jMessageStatusValueOfStaticMethodId,
                                       static_cast<jint>(status));
}

MessageStatus JavaShimUtils::jobjectToMessageStatus(JNIEnv *env, jobject &jMessageStatus) {
    if (jMessageStatus == nullptr) {
        return MS_UNDEF;
    }
    return static_cast<MessageStatus>(static_cast<std::int32_t>(
        env->GetIntField(jMessageStatus, JavaIds::jMessageStatusValueFieldId)));
}

jobject JavaShimUtils::deviceInfoToJobject(JNIEnv *env, DeviceInfo deviceInfo) {
    jobject jPluginConfig =
        env->NewObject(JavaIds::jDeviceInfoClassId, JavaIds::jDeviceInfoConstructorMethodId,
                       env->NewStringUTF(deviceInfo.platform.c_str()),
                       env->NewStringUTF(deviceInfo.architecture.c_str()),
                       env->NewStringUTF(deviceInfo.nodeType.c_str()));
    return jPluginConfig;
}

DeviceInfo JavaShimUtils::jobjectToDeviceInfo(JNIEnv *env, jobject &jDeviceInfo) {
    if (jDeviceInfo == nullptr) {
        return {};
    }

    DeviceInfo deviceInfo;
    deviceInfo.platform =
        jstring2string(env, static_cast<jstring>(env->GetObjectField(
                                jDeviceInfo, JavaIds::jDeviceInfoPlatformFieldId)));
    deviceInfo.architecture =
        jstring2string(env, static_cast<jstring>(env->GetObjectField(
                                jDeviceInfo, JavaIds::jDeviceInfoArchitectureFieldId)));
    deviceInfo.nodeType =
        jstring2string(env, static_cast<jstring>(env->GetObjectField(
                                jDeviceInfo, JavaIds::jDeviceInfoNodeTypeFieldId)));
    return deviceInfo;
}

bool JavaShimUtils::getEnv(JNIEnv **env, JavaVM *jvm) {
    (void)pthread_once(&key_once, make_key);
    if (pthread_getspecific(key) == NULL) {
        pthread_setspecific(key, jvm);
    }

    int getEnvStat = jvm->GetEnv(reinterpret_cast<void **>(env), JNI_VERSION_1_6);
    if (getEnvStat == JNI_OK) {
#if ENABLE_DEBUG_LOGGING
        RaceLog::logDebug(logLabel, "GetEnv: already attached", "");
#endif
        return true;
    } else if (getEnvStat == JNI_ERR) {
        RaceLog::logError(logLabel, "GetEnv: unknown error", "");
    } else if (getEnvStat == JNI_EDETACHED) {
        RaceLog::logDebug(logLabel, "GetEnv: not attached", "");
#ifdef __ANDROID__
        if (jvm->AttachCurrentThread(env, NULL) != 0) {
#else
        if (jvm->AttachCurrentThread(reinterpret_cast<void **>(env), NULL) != 0) {
#endif
            RaceLog::logDebug(logLabel, "GetEnv: Failed to attach", "");
        }
        return true;
    } else if (getEnvStat == JNI_EVERSION) {
        RaceLog::logError(logLabel, "GetEnv: version not supported", "");
    } else if (getEnvStat == JNI_ENOMEM) {
        RaceLog::logError(logLabel, "GetEnv: not enough memory", "");
    } else if (getEnvStat == JNI_EEXIST) {
        RaceLog::logError(logLabel, "GetEnv: VM already created", "");
    } else if (getEnvStat == JNI_EINVAL) {
        RaceLog::logError(logLabel, "GetEnv: invalid arguments", "");
    } else {
        RaceLog::logError(logLabel, "GetEnv: unknown return code " + std::to_string(getEnvStat),
                          "");
    }
    return false;
}

void JavaShimUtils::ThreadDestructor(void *ptr) {
    JavaVM *jvm = reinterpret_cast<JavaVM *>(ptr);
    jvm->DetachCurrentThread();
}

std::string JavaShimUtils::getMessageFromJthrowable(JNIEnv *env, jthrowable &throwable) {
    jclass throwableClass = env->GetObjectClass(throwable);
    jmethodID getMessage = env->GetMethodID(throwableClass, "getMessage", "()Ljava/lang/String;");
    if (getMessage == nullptr) {
        RaceLog::logError(logLabel, "failed to find throwable getMessage", "");
    }
    jstring jMessage = static_cast<jstring>(env->CallObjectMethod(throwable, getMessage));
    if (jMessage == nullptr) {
        RaceLog::logWarning(logLabel, "no message for throwable", "");
    }
    std::string message = jstring2string(env, jMessage);
    if (message.empty()) {
        jmethodID getCause =
            env->GetMethodID(throwableClass, "getCause", "()Ljava/lang/Throwable;");
        if (getCause == nullptr) {
            RaceLog::logError(logLabel, "failed to find throwable getCause", "");
        } else {
            jthrowable jCause = static_cast<jthrowable>(env->CallObjectMethod(throwable, getCause));
            if (jCause != nullptr) {
                return getMessageFromJthrowable(env, jCause);
            } else {
                RaceLog::logWarning(logLabel, "no cause for throwable", "");
                jmethodID toString =
                    env->GetMethodID(throwableClass, "toString", "()Ljava/lang/String;");
                if (toString == nullptr) {
                    RaceLog::logError(logLabel, "failed to find throwable toString", "");
                } else {
                    jMessage = static_cast<jstring>(env->CallObjectMethod(throwable, toString));
                    if (jMessage == nullptr) {
                        RaceLog::logWarning(logLabel, "no string representation for throwable", "");
                    }
                    return jstring2string(env, jMessage);
                }
            }
        }
    }
    return message;
}

ClrMsg JavaShimUtils::jClrMsg_to_ClrMsg(JNIEnv *env, jobject jClrMsg) {
    const std::string plainMsg = jstring2string(
        env, static_cast<jstring>(env->GetObjectField(jClrMsg, JavaIds::jClrMsgPlainMsgFieldId)));
    const std::string fromPersona = jstring2string(
        env,
        static_cast<jstring>(env->GetObjectField(jClrMsg, JavaIds::jClrMsgFromPersonaFieldId)));
    const std::string toPersona = jstring2string(
        env, static_cast<jstring>(env->GetObjectField(jClrMsg, JavaIds::jClrMsgToPersonaFieldId)));
    const long createTime =
        static_cast<long>(env->GetLongField(jClrMsg, JavaIds::jClrMsgcreateTimeFieldId));
    const std::int32_t nonce =
        static_cast<std::int32_t>(env->GetIntField(jClrMsg, JavaIds::jClrMsgNonceFieldId));
    const std::int32_t ampIndex =
        static_cast<std::int32_t>(env->GetIntField(jClrMsg, JavaIds::jClrMsgAmpIndexFieldId));
    const long traceId =
        static_cast<long>(env->GetLongField(jClrMsg, JavaIds::jClrMsgTraceIdFieldId));
    const long spanId =
        static_cast<long>(env->GetLongField(jClrMsg, JavaIds::jClrMsgSpanIdFieldId));

    ClrMsg msg(plainMsg, fromPersona, toPersona, static_cast<std::int64_t>(createTime), nonce,
               ampIndex, static_cast<std::uint64_t>(traceId), static_cast<std::uint64_t>(spanId));
    return msg;
}

jobject JavaShimUtils::clrMsg_to_jClrMsg(JNIEnv *env, const ClrMsg &clrMsg) {
    jstring jPlainMsg = env->NewStringUTF(clrMsg.getMsg().c_str());
    jstring jFromPersona = env->NewStringUTF(clrMsg.getFrom().c_str());
    jstring jToPersona = env->NewStringUTF(clrMsg.getTo().c_str());
    jlong jCreateTime = static_cast<jlong>(clrMsg.getTime());
    jint jNonce = static_cast<jint>(clrMsg.getNonce());
    jbyte jAmpIndex = static_cast<jbyte>(clrMsg.getAmpIndex());
    jlong jTraceId = static_cast<jlong>(clrMsg.getTraceId());
    jlong jSpanId = static_cast<jlong>(clrMsg.getSpanId());

    jobject jClrMsg =
        env->NewObject(JavaIds::jClrMsgClassId, JavaIds::jClrMsgConstructorMethodId, jPlainMsg,
                       jFromPersona, jToPersona, jCreateTime, jNonce, jAmpIndex, jTraceId, jSpanId);
    return jClrMsg;
}

EncPkg JavaShimUtils::jobjectToEncPkg(JNIEnv *env, jobject jEncPkg) {
    jlong jTraceId = static_cast<long>(env->GetLongField(jEncPkg, JavaIds::jEncPkgTraceIdFieldId));
    jlong jSpanId = static_cast<long>(env->GetLongField(jEncPkg, JavaIds::jEncPkgSpanIdFieldId));
    PackageType jPkgType = static_cast<PackageType>(
        static_cast<jbyte>(env->GetByteField(jEncPkg, JavaIds::jEncPkgPackageTypeByteFieldId)));

    jobject jCipherText = env->GetObjectField(jEncPkg, JavaIds::jEncPkgCipherTextFieldId);

    RawData cipherText = JavaShimUtils::jobjectToVectorUint8(env, jCipherText);

    EncPkg ePkg(static_cast<uint64_t>(jTraceId), static_cast<uint64_t>(jSpanId), cipherText);
    ePkg.setPackageType(jPkgType);
    return ePkg;
}

jobject JavaShimUtils::encPkgToJobject(JNIEnv *env, const EncPkg &encPkg) {
    jlong jTraceId = static_cast<jlong>(encPkg.getTraceId());
    jlong jSpanId = static_cast<jlong>(encPkg.getSpanId());
    jbyte packageType = static_cast<jbyte>(encPkg.getPackageType());

    RawData cipherText = encPkg.getCipherText();
    jsize numBytes = static_cast<jsize>(cipherText.size());
    jbyteArray jCipherText = env->NewByteArray(numBytes);
    // This works because std::vector uses contiguous internal memory
    env->SetByteArrayRegion(jCipherText, 0, numBytes,
                            reinterpret_cast<const jbyte *>(cipherText.data()));

    jobject jEncPkg = env->NewObject(JavaIds::jEncPkgClassId, JavaIds::jEncPkgConstructorMethodId,
                                     jTraceId, jSpanId, jCipherText, packageType);
    return jEncPkg;
}

RaceEnums::NodeType JavaShimUtils::jobjectToNodeType(JNIEnv *env, jobject javaNodeType) {
    const jint value = env->CallIntMethod(javaNodeType, JavaIds::jNodeTypeOrdinalMethodId);
    return static_cast<RaceEnums::NodeType>(value);
}

jobject JavaShimUtils::nodeTypeToJNodeType(JNIEnv *env, RaceEnums::NodeType nodeType) {
    return env->CallStaticObjectMethod(JavaIds::jNodeTypeClassId,
                                       JavaIds::jNodeTypeValueOfStaticMethodId,
                                       static_cast<jint>(nodeType));
}

RaceEnums::StorageEncryptionType JavaShimUtils::jobjectToStorageEncryptionType(
    JNIEnv *env, jobject javaStorageEncryptionType) {
    const jint value = env->CallIntMethod(javaStorageEncryptionType,
                                          JavaIds::jStorageEncryptionTypeOrdinalMethodId);
    return static_cast<RaceEnums::StorageEncryptionType>(value);
}

jobject JavaShimUtils::StorageEncryptionTypeToJStorageEncryptionType(
    JNIEnv *env, RaceEnums::StorageEncryptionType storageEncryptionType) {
    return env->CallStaticObjectMethod(JavaIds::jStorageEncryptionTypeClassId,
                                       JavaIds::jStorageEncryptionTypeValueOfStaticMethodId,
                                       static_cast<jint>(storageEncryptionType));
}

RaceEnums::UserDisplayType JavaShimUtils::jobjectToUserDisplayType(JNIEnv *env,
                                                                   jobject javaUserDisplayType) {
    const jint value =
        env->CallIntMethod(javaUserDisplayType, JavaIds::jUserDisplayTypeOrdinalMethodId);
    return static_cast<RaceEnums::UserDisplayType>(value);
}

jobject JavaShimUtils::userDisplayTypeToJUserDisplayType(
    JNIEnv *env, RaceEnums::UserDisplayType userDisplayType) {
    return env->CallStaticObjectMethod(JavaIds::jUserDisplayTypeClassId,
                                       JavaIds::jUserDisplayTypeValueOfStaticMethodId,
                                       static_cast<jint>(userDisplayType));
}

RaceEnums::BootstrapActionType JavaShimUtils::jobjectToBootstrapActionType(
    JNIEnv *env, jobject javaBootstrapActionType) {
    const jint value =
        env->CallIntMethod(javaBootstrapActionType, JavaIds::jBootstrapActionTypeOrdinalMethodId);
    return static_cast<RaceEnums::BootstrapActionType>(value);
}

jobject JavaShimUtils::bootstrapActionTypeToJBootstrapActionType(
    JNIEnv *env, RaceEnums::BootstrapActionType bootstrapActionType) {
    return env->CallStaticObjectMethod(JavaIds::jBootstrapActionTypeClassId,
                                       JavaIds::jBootstrapActionTypeValueOfStaticMethodId,
                                       static_cast<jint>(bootstrapActionType));
}

AppConfig JavaShimUtils::jAppConfigToAppConfig(JNIEnv *env, jobject jAppConfig) {
    AppConfig config = AppConfig();
    config.persona = getStringFieldFromClassObject(env, jAppConfig, "persona");
    config.appDir = getStringFieldFromClassObject(env, jAppConfig, "appDir");
    config.pluginArtifactsBaseDir =
        getStringFieldFromClassObject(env, jAppConfig, "pluginArtifactsBaseDir");
    config.platform = getStringFieldFromClassObject(env, jAppConfig, "platform");
    config.architecture = getStringFieldFromClassObject(env, jAppConfig, "architecture");
    config.environment = getStringFieldFromClassObject(env, jAppConfig, "environment");
    // Config Files
    config.configTarPath = getStringFieldFromClassObject(env, jAppConfig, "configTarPath");
    config.baseConfigPath = getStringFieldFromClassObject(env, jAppConfig, "baseConfigPath");
    // Testing specific files (user-responses.json, jaeger-config.json, voa.json)
    config.etcDirectory = getStringFieldFromClassObject(env, jAppConfig, "etcDirectory");
    config.voaConfigPath = getStringFieldFromClassObject(env, jAppConfig, "voaConfigPath");
    config.jaegerConfigPath = getStringFieldFromClassObject(env, jAppConfig, "jaegerConfigPath");
    config.userResponsesFilePath =
        getStringFieldFromClassObject(env, jAppConfig, "userResponsesFilePath");
    // Bootstrap dirs
    config.bootstrapFilesDirectory =
        getStringFieldFromClassObject(env, jAppConfig, "bootstrapFilesDirectory");
    config.bootstrapCacheDirectory =
        getStringFieldFromClassObject(env, jAppConfig, "bootstrapCacheDirectory");
    // Others
    config.sdkFilePath = getStringFieldFromClassObject(env, jAppConfig, "sdkFilePath");

    config.tmpDirectory = getStringFieldFromClassObject(env, jAppConfig, "tmpDirectory");
    config.logDirectory = getStringFieldFromClassObject(env, jAppConfig, "logDirectory");
    config.logFilePath = getStringFieldFromClassObject(env, jAppConfig, "logFilePath");
    config.appPath = getStringFieldFromClassObject(env, jAppConfig, "appPath");

    jobject nodeType = env->GetObjectField(jAppConfig, JavaIds::jAppConfigNodeTypeFieldId);
    config.nodeType = jobjectToNodeType(env, nodeType);

    jobject encryptionType =
        env->GetObjectField(jAppConfig, JavaIds::jAppConfigEncryptionTypeFieldId);
    config.encryptionType = jobjectToStorageEncryptionType(env, encryptionType);

    return config;
}

jobject JavaShimUtils::appConfigToJobject(JNIEnv *env, const AppConfig &appConfig) {
    jobject jAppConfig =
        env->NewObject(JavaIds::jAppConfigClassId, JavaIds::jAppConfigConstructorMethodId);

    setStringFieldFromClassObject(env, jAppConfig, "persona", appConfig.persona);
    setStringFieldFromClassObject(env, jAppConfig, "appDir", appConfig.appDir);
    setStringFieldFromClassObject(env, jAppConfig, "pluginArtifactsBaseDir",
                                  appConfig.pluginArtifactsBaseDir);
    setStringFieldFromClassObject(env, jAppConfig, "platform", appConfig.platform);
    setStringFieldFromClassObject(env, jAppConfig, "architecture", appConfig.architecture);
    setStringFieldFromClassObject(env, jAppConfig, "environment", appConfig.environment);
    // Config Files
    setStringFieldFromClassObject(env, jAppConfig, "configTarPath", appConfig.configTarPath);
    setStringFieldFromClassObject(env, jAppConfig, "baseConfigPath", appConfig.baseConfigPath);
    // Testing specific files (user-responses.json, jaeger-config.json, voa.json)
    setStringFieldFromClassObject(env, jAppConfig, "etcDirectory", appConfig.etcDirectory);
    setStringFieldFromClassObject(env, jAppConfig, "jaegerConfigPath", appConfig.jaegerConfigPath);
    setStringFieldFromClassObject(env, jAppConfig, "userResponsesFilePath",
                                  appConfig.userResponsesFilePath);
    setStringFieldFromClassObject(env, jAppConfig, "voaConfigPath", appConfig.voaConfigPath);
    // Bootstrap Directories
    setStringFieldFromClassObject(env, jAppConfig, "bootstrapFilesDirectory",
                                  appConfig.bootstrapFilesDirectory);
    setStringFieldFromClassObject(env, jAppConfig, "bootstrapCacheDirectory",
                                  appConfig.bootstrapCacheDirectory);
    // Others
    setStringFieldFromClassObject(env, jAppConfig, "sdkFilePath", appConfig.sdkFilePath);

    setStringFieldFromClassObject(env, jAppConfig, "tmpDirectory", appConfig.tmpDirectory);
    setStringFieldFromClassObject(env, jAppConfig, "logDirectory", appConfig.logDirectory);
    setStringFieldFromClassObject(env, jAppConfig, "logFilePath", appConfig.logFilePath);
    setStringFieldFromClassObject(env, jAppConfig, "appPath", appConfig.appPath);

    // TODO: what about .nodeType and .encryptionType? set these using the setter methods in
    // AppConfigs

    return jAppConfig;
}

LinkProperties JavaShimUtils::jLinkPropertiesToLinkProperties(JNIEnv *env, jobject jLinkProps) {
    LinkProperties linkProperties;

    // get the link type
    jint jLinkType = env->CallIntMethod(jLinkProps, JavaIds::jLinkPropertiesGetLinkTypeMethodId);
    linkProperties.linkType = static_cast<LinkType>(jLinkType);

    // get the connection type
    jint jConnectionType =
        env->CallIntMethod(jLinkProps, JavaIds::jLinkPropertiesGetConnectionTypeMethodId);
    linkProperties.connectionType = static_cast<ConnectionType>(jConnectionType);

    // get the send type
    jint jSendType = env->CallIntMethod(jLinkProps, JavaIds::jLinkPropertiesGetSendTypeMethodId);
    linkProperties.sendType = static_cast<SendType>(jSendType);

    // get the transmission type
    jint jTransmissionType =
        env->CallIntMethod(jLinkProps, JavaIds::jLinkPropertiesGetTransmissionTypeMethodId);
    linkProperties.transmissionType = static_cast<TransmissionType>(jTransmissionType);

    linkProperties.reliable = static_cast<bool>(static_cast<jboolean>(
        env->GetBooleanField(jLinkProps, JavaIds::jLinkPropertiesReliableFieldId)));
    linkProperties.duration_s = static_cast<std::int32_t>(
        env->GetIntField(jLinkProps, JavaIds::jLinkPropertiesDurationFieldId));
    linkProperties.period_s = static_cast<std::int32_t>(
        env->GetIntField(jLinkProps, JavaIds::jLinkPropertiesPeriodFieldId));
    linkProperties.mtu =
        static_cast<std::int32_t>(env->GetIntField(jLinkProps, JavaIds::jLinkPropertiesMtuFieldId));

    jobject jWorst = env->CallObjectMethod(jLinkProps, JavaIds::jLinkPropertiesGetWorstMethodId);
    jobject jExpected = env->CallObjectMethod(jLinkProps, JavaIds::jLinkPropertiesGetBestMethodId);
    jobject jBest = env->CallObjectMethod(jLinkProps, JavaIds::jLinkPropertiesGetExpectedMethodId);
    linkProperties.worst = JavaShimUtils::jLinkPropertyPairToLinkPropertyPair(env, jWorst);
    linkProperties.expected = JavaShimUtils::jLinkPropertyPairToLinkPropertyPair(env, jExpected);
    linkProperties.best = JavaShimUtils::jLinkPropertyPairToLinkPropertyPair(env, jBest);

    // get supported hints
    jobjectArray hints = static_cast<jobjectArray>(
        env->CallObjectMethod(jLinkProps, JavaIds::jLinkPropertiesGetHintsMethodId));
    jsize stringCount = env->GetArrayLength(hints);
    for (jsize i = 0; i < stringCount; i++) {
        jstring string = static_cast<jstring>(env->GetObjectArrayElement(hints, i));
        std::string rawString = JavaShimUtils::jstring2string(env, string);
        linkProperties.supported_hints.push_back(rawString);
    }

    jstring jChannelGid = static_cast<jstring>(
        env->CallObjectMethod(jLinkProps, JavaIds::jLinkPropertiesGetChannelGidMethodId));
    linkProperties.channelGid = JavaShimUtils::jstring2string(env, jChannelGid);
    jstring jLinkAddress = static_cast<jstring>(
        env->CallObjectMethod(jLinkProps, JavaIds::jLinkPropertiesGetLinkAddressMethodId));
    linkProperties.linkAddress = JavaShimUtils::jstring2string(env, jLinkAddress);
    RaceLog::logDebug(logLabel, "jLinkPropertiesToLinkProperties: returned", "");

    return linkProperties;
}

jobject JavaShimUtils::linkPropertiesToJobject(JNIEnv *env, const LinkProperties &properties) {
    jobject jLinkType = JavaShimUtils::linkTypeToJLinkType(env, properties.linkType);
    if (jLinkType == nullptr) {
        RaceLog::logError(logLabel, "Unable to convert link type", "");
        return nullptr;
    }

    jobject jConnectionType =
        JavaShimUtils::connectionTypeToJConnectionType(env, properties.connectionType);
    if (jConnectionType == nullptr) {
        RaceLog::logError(logLabel, "Unable to convert connection type", "");
        return nullptr;
    }

    jobject jSendType = JavaShimUtils::sendTypeToJSendType(env, properties.sendType);
    if (jSendType == nullptr) {
        RaceLog::logError(logLabel, "Unable to convert send type", "");
        return nullptr;
    }

    jobject jTransmissionType =
        JavaShimUtils::transmissionTypeToJobject(env, properties.transmissionType);
    if (jTransmissionType == nullptr) {
        RaceLog::logError(logLabel, "Unable to convert transmission type", "");
        return nullptr;
    }

    // Create the supported_hints
    jobjectArray supportedHints = static_cast<jobjectArray>(env->NewObjectArray(
        properties.supported_hints.size(), JavaIds::jStringClassId, env->NewStringUTF("")));
    if (supportedHints == nullptr) {
        RaceLog::logError(logLabel, "failed to create supported hints array", "");
        return nullptr;
    }
    for (unsigned int index = 0; index < properties.supported_hints.size(); ++index) {
        env->SetObjectArrayElement(supportedHints, static_cast<jint>(index),
                                   env->NewStringUTF(properties.supported_hints[index].c_str()));
    }
    jstring channelGid = env->NewStringUTF(properties.channelGid.c_str());
    jstring linkAddress = env->NewStringUTF(properties.linkAddress.c_str());

    // Create the Java LinkProperties.
    jobject jLinkProperties = env->NewObject(
        JavaIds::jLinkPropertiesClassId, JavaIds::jLinkPropertiesConstructorMethodId,
        JavaShimUtils::linkPropertyPairToJobject(env, properties.worst),
        JavaShimUtils::linkPropertyPairToJobject(env, properties.expected),
        JavaShimUtils::linkPropertyPairToJobject(env, properties.best),
        static_cast<jboolean>(properties.reliable), static_cast<jboolean>(properties.isFlushable),
        channelGid, linkAddress, supportedHints, jLinkType, jTransmissionType, jConnectionType,
        jSendType, static_cast<jint>(properties.duration_s), static_cast<jint>(properties.period_s),
        static_cast<jint>(properties.mtu));
    if (jLinkProperties == nullptr) {
        RaceLog::logError(logLabel, "failed to construct jLinkProperties", "");
        return nullptr;
    }

    RaceLog::logDebug(logLabel, "linkPropertiesToJobject: returned", "");
    return jLinkProperties;
}

ChannelProperties JavaShimUtils::jChannelPropertiesToChannelProperties(JNIEnv *env,
                                                                       jobject jChannelProps) {
    ChannelProperties channelProperties;

    // get the channel status
    jint jChannelStatus =
        env->CallIntMethod(jChannelProps, JavaIds::jChannelPropertiesGetChannelStatusMethodId);
    channelProperties.channelStatus = static_cast<ChannelStatus>(jChannelStatus);

    // get the link direction
    jint jLinkDirection =
        env->CallIntMethod(jChannelProps, JavaIds::jChannelPropertiesGetLinkDirectionMethodId);
    channelProperties.linkDirection = static_cast<LinkDirection>(jLinkDirection);

    // get the connection type
    jint jConnectionType =
        env->CallIntMethod(jChannelProps, JavaIds::jChannelPropertiesGetConnectionTypeMethodId);
    channelProperties.connectionType = static_cast<ConnectionType>(jConnectionType);

    // get the connection type
    jint jSendType =
        env->CallIntMethod(jChannelProps, JavaIds::jChannelPropertiesGetSendTypeMethodId);
    channelProperties.sendType = static_cast<SendType>(jSendType);

    // get the transmission type
    jint jTransmissionType =
        env->CallIntMethod(jChannelProps, JavaIds::jChannelPropertiesGetTransmissionTypeMethodId);
    channelProperties.transmissionType = static_cast<TransmissionType>(jTransmissionType);

    // get the current role
    jobjectArray jRoles = static_cast<jobjectArray>(
        env->GetObjectField(jChannelProps, JavaIds::jChannelPropertiesRolesFieldId));

    jobject jCurrentRole =
        env->GetObjectField(jChannelProps, JavaIds::jChannelPropertiesCurrentRoleFieldId);

    channelProperties.multiAddressable = static_cast<bool>(static_cast<jboolean>(
        env->GetBooleanField(jChannelProps, JavaIds::jChannelPropertiesMultiAddressableFieldId)));
    channelProperties.reliable = static_cast<bool>(static_cast<jboolean>(
        env->GetBooleanField(jChannelProps, JavaIds::jChannelPropertiesReliableFieldId)));
    channelProperties.bootstrap = static_cast<bool>(static_cast<jboolean>(
        env->GetBooleanField(jChannelProps, JavaIds::jChannelPropertiesBootstrapFieldId)));
    channelProperties.isFlushable = static_cast<bool>(static_cast<jboolean>(
        env->GetBooleanField(jChannelProps, JavaIds::jChannelPropertiesIsFlushableFieldId)));
    channelProperties.duration_s = static_cast<std::int32_t>(
        env->GetIntField(jChannelProps, JavaIds::jChannelPropertiesDurationFieldId));
    channelProperties.period_s = static_cast<std::int32_t>(
        env->GetIntField(jChannelProps, JavaIds::jChannelPropertiesPeriodFieldId));
    channelProperties.mtu = static_cast<std::int32_t>(
        env->GetIntField(jChannelProps, JavaIds::jChannelPropertiesMtuFieldId));
    channelProperties.maxLinks = static_cast<std::int32_t>(
        env->GetIntField(jChannelProps, JavaIds::jChannelPropertiesMaxLinksFieldId));
    channelProperties.creatorsPerLoader = static_cast<std::int32_t>(
        env->GetIntField(jChannelProps, JavaIds::jChannelPropertiesCreatorsPerLoaderFieldId));
    channelProperties.loadersPerCreator = static_cast<std::int32_t>(
        env->GetIntField(jChannelProps, JavaIds::jChannelPropertiesLoadersPerCreatorFieldId));
    channelProperties.maxSendsPerInterval = static_cast<std::int32_t>(
        env->GetIntField(jChannelProps, JavaIds::jChannelPropertiesMaxSendsPerIntervalFieldId));
    channelProperties.secondsPerInterval = static_cast<std::int32_t>(
        env->GetIntField(jChannelProps, JavaIds::jChannelPropertiesSecondsPerIntervalFieldId));
    channelProperties.intervalEndTime = static_cast<std::uint64_t>(
        env->GetLongField(jChannelProps, JavaIds::jChannelPropertiesIntervalEndTimeFieldId));
    channelProperties.sendsRemainingInInterval = static_cast<std::int32_t>(env->GetIntField(
        jChannelProps, JavaIds::jChannelPropertiesSendsRemainingInIntervalFieldId));

    jobject jCreatorExpected =
        env->CallObjectMethod(jChannelProps, JavaIds::jChannelPropertiesGetCreatorExpected);
    jobject jLoaderExpected =
        env->CallObjectMethod(jChannelProps, JavaIds::jChannelPropertiesGetLoaderExpected);
    channelProperties.creatorExpected =
        JavaShimUtils::jLinkPropertyPairToLinkPropertyPair(env, jCreatorExpected);
    channelProperties.loaderExpected =
        JavaShimUtils::jLinkPropertyPairToLinkPropertyPair(env, jLoaderExpected);

    // get supported hints
    jobjectArray hints = static_cast<jobjectArray>(
        env->CallObjectMethod(jChannelProps, JavaIds::jChannelPropertiesGetHintsMethodId));
    jsize stringCount = env->GetArrayLength(hints);
    for (jsize i = 0; i < stringCount; i++) {
        jstring string = static_cast<jstring>(env->GetObjectArrayElement(hints, i));
        std::string rawString = JavaShimUtils::jstring2string(env, string);
        channelProperties.supported_hints.push_back(rawString);
    }
    jstring jChannelGid = static_cast<jstring>(
        env->CallObjectMethod(jChannelProps, JavaIds::jChannelPropertiesGetChannelGidMethodId));

    channelProperties.roles = JavaShimUtils::jRolesVectorToRolesVector(env, jRoles);
    channelProperties.currentRole = JavaShimUtils::jChannelRoleToChannelRole(env, jCurrentRole);

    channelProperties.channelGid = JavaShimUtils::jstring2string(env, jChannelGid);

    return channelProperties;
}

jobject JavaShimUtils::channelPropertiesToJobject(JNIEnv *env,
                                                  const ChannelProperties &properties) {
    jobject jChannelStatus = JavaShimUtils::channelStatusToJobject(env, properties.channelStatus);
    if (jChannelStatus == nullptr) {
        RaceLog::logError(logLabel, "Unable to convert channel status", "");
        return nullptr;
    }

    jobject jLinkDirection =
        JavaShimUtils::linkDirectionToJLinkDirection(env, properties.linkDirection);
    if (jLinkDirection == nullptr) {
        RaceLog::logError(logLabel, "Unable to convert link direction", "");
        return nullptr;
    }

    jobject jConnectionType =
        JavaShimUtils::connectionTypeToJConnectionType(env, properties.connectionType);
    if (jConnectionType == nullptr) {
        RaceLog::logError(logLabel, "Unable to convert connection type", "");
        return nullptr;
    }

    jobject jSendType = JavaShimUtils::sendTypeToJSendType(env, properties.sendType);
    if (jSendType == nullptr) {
        RaceLog::logError(logLabel, "Unable to convert connection type", "");
        return nullptr;
    }

    jobject jTransmissionType =
        JavaShimUtils::transmissionTypeToJobject(env, properties.transmissionType);
    if (jTransmissionType == nullptr) {
        RaceLog::logError(logLabel, "Unable to convert transmission type", "");
        return nullptr;
    }

    jobject jRoles = JavaShimUtils::rolesVectorToJRolesVector(env, properties.roles);
    if (jRoles == nullptr) {
        RaceLog::logError(logLabel, "Unable to convert channel role", "");
        return nullptr;
    }

    jobject jChannelRole = JavaShimUtils::channelRoleToJobject(env, properties.currentRole);
    if (jChannelRole == nullptr) {
        RaceLog::logError(logLabel, "Unable to convert channel role", "");
        return nullptr;
    }

    // Create the supported_hints
    jobjectArray supportedHints = static_cast<jobjectArray>(env->NewObjectArray(
        properties.supported_hints.size(), JavaIds::jStringClassId, env->NewStringUTF("")));
    if (supportedHints == nullptr) {
        RaceLog::logError(logLabel, "failed to create supported hints array", "");
        return nullptr;
    }
    for (unsigned int index = 0; index < properties.supported_hints.size(); ++index) {
        env->SetObjectArrayElement(supportedHints, static_cast<jint>(index),
                                   env->NewStringUTF(properties.supported_hints[index].c_str()));
    }

    jstring channelGid = env->NewStringUTF(properties.channelGid.c_str());

    // Create the Java ChannelProperties.
    jobject creatorExpected =
        JavaShimUtils::linkPropertyPairToJobject(env, properties.creatorExpected);
    jobject loaderExpected =
        JavaShimUtils::linkPropertyPairToJobject(env, properties.loaderExpected);
    jobject jChannelProperties = env->NewObject(
        JavaIds::jChannelPropertiesClassId, JavaIds::jChannelPropertiesConstructorMethodId,
        jChannelStatus, creatorExpected, loaderExpected, static_cast<jboolean>(properties.reliable),
        static_cast<jboolean>(properties.bootstrap), static_cast<jboolean>(properties.isFlushable),
        static_cast<jboolean>(properties.multiAddressable), supportedHints, channelGid,
        jLinkDirection, jTransmissionType, jConnectionType, jSendType,
        static_cast<jint>(properties.duration_s), static_cast<jint>(properties.period_s),
        static_cast<jint>(properties.mtu), static_cast<jint>(properties.maxLinks),
        static_cast<jint>(properties.creatorsPerLoader),
        static_cast<jint>(properties.loadersPerCreator), jRoles, jChannelRole,
        static_cast<jint>(properties.maxSendsPerInterval),
        static_cast<jint>(properties.secondsPerInterval),
        static_cast<jlong>(properties.intervalEndTime),
        static_cast<jint>(properties.sendsRemainingInInterval));
    if (jChannelProperties == nullptr) {
        RaceLog::logError(logLabel, "failed to construct jChannelProperties", "");
        return nullptr;
    }

    return jChannelProperties;
}

LinkPropertySet JavaShimUtils::jLinkPropertySetToLinkPropertySet(JNIEnv *env,
                                                                 jobject jLinkPropSet) {
    LinkPropertySet ls;

    ls.bandwidth_bps = static_cast<std::int32_t>(
        env->GetIntField(jLinkPropSet, JavaIds::jLinkPropertySetBandwidthBitsPSFieldId));
    ls.latency_ms = static_cast<std::int32_t>(
        env->GetIntField(jLinkPropSet, JavaIds::jLinkPropertySetLatencyMsFieldId));
    ls.loss =
        static_cast<float>(env->GetFloatField(jLinkPropSet, JavaIds::jLinkPropertySetLossFieldId));

    return ls;
}

jobject JavaShimUtils::linkPropertySetToJobject(JNIEnv *env, const LinkPropertySet &propertySet) {
    jclass jLinkPropertySetClass = JavaIds::jLinkPropertySetClassId;
    jobject jLinkPropertySet =
        env->NewObject(jLinkPropertySetClass, JavaIds::jLinkPropertySetConstructorMethodId,
                       propertySet.bandwidth_bps, propertySet.latency_ms, propertySet.loss);
    if (jLinkPropertySet == nullptr) {
        RaceLog::logError(logLabel, "failed to construct jLinkPropertySetClass", "");
        return nullptr;
    }
    return jLinkPropertySet;
}

LinkPropertyPair JavaShimUtils::jLinkPropertyPairToLinkPropertyPair(JNIEnv *env,
                                                                    jobject jLinkPropPair) {
    LinkPropertyPair lp;
    jobject jSend = env->GetObjectField(jLinkPropPair, JavaIds::jLinkPropertyPairSendFieldId);
    jobject jReceive = env->GetObjectField(jLinkPropPair, JavaIds::jLinkPropertyPairReceiveFieldId);

    lp.send = JavaShimUtils::jLinkPropertySetToLinkPropertySet(env, jSend);
    lp.receive = JavaShimUtils::jLinkPropertySetToLinkPropertySet(env, jReceive);

    return lp;
}

jobject JavaShimUtils::linkPropertyPairToJobject(JNIEnv *env,
                                                 const LinkPropertyPair &propertyPair) {
    jobject jLinkPropertyPair = env->NewObject(
        JavaIds::jLinkPropertyPairClassId, JavaIds::jLinkPropertyPairConstructorMethodId,
        JavaShimUtils::linkPropertySetToJobject(env, propertyPair.send),
        JavaShimUtils::linkPropertySetToJobject(env, propertyPair.receive));
    if (jLinkPropertyPair == nullptr) {
        RaceLog::logError(logLabel, "failed to construct jLinkPropertyPair", "");
        return nullptr;
    }
    return jLinkPropertyPair;
}

jobject JavaShimUtils::rolesVectorToJRolesVector(JNIEnv *env,
                                                 const std::vector<ChannelRole> &roles) {
    jobjectArray jRoles;
    jsize len = roles.size();
    jRoles = env->NewObjectArray(len, JavaIds::jChannelRoleClassId, 0);
    for (size_t i = 0; i < roles.size(); i++) {
        env->SetObjectArrayElement(jRoles, static_cast<jsize>(i),
                                   channelRoleToJobject(env, roles[i]));
    }
    return jRoles;
}

std::vector<ChannelRole> JavaShimUtils::jRolesVectorToRolesVector(JNIEnv *env,
                                                                  jobjectArray &jRoles) {
    std::vector<ChannelRole> roles;

    jsize size = env->GetArrayLength(jRoles);

    for (jsize i = 0; i < size; ++i) {
        jobject jRole = env->GetObjectArrayElement(jRoles, i);
        roles.push_back(jChannelRoleToChannelRole(env, jRole));
    }
    return roles;
}

jobject JavaShimUtils::channelRoleToJobject(JNIEnv *env, const ChannelRole &role) {
    jobject jLinkSide = JavaShimUtils::linkSideToJobject(env, role.linkSide);
    if (jLinkSide == nullptr) {
        RaceLog::logError(logLabel, "Unable to convert link side", "");
        return nullptr;
    }

    jobject channelRole =
        env->NewObject(JavaIds::jChannelRoleClassId, JavaIds::jChannelRoleConstructorMethodId,
                       env->NewStringUTF(role.roleName.c_str()),
                       JavaShimUtils::stringVectorToJArray(env, role.mechanicalTags),
                       JavaShimUtils::stringVectorToJArray(env, role.behavioralTags), jLinkSide);

    if (channelRole == nullptr) {
        RaceLog::logError(logLabel, "failed to construct channelRole", "");
        return nullptr;
    }
    return channelRole;
}

ChannelRole JavaShimUtils::jChannelRoleToChannelRole(JNIEnv *env, jobject &jRole) {
    ChannelRole role;

    jstring jRoleName =
        static_cast<jstring>(env->GetObjectField(jRole, JavaIds::jChannelRoleRoleNameFieldId));
    jobjectArray jMechanicalTags = static_cast<jobjectArray>(
        env->GetObjectField(jRole, JavaIds::jChannelRoleMechanicalTagsFieldId));
    jobjectArray jBehavioralTags = static_cast<jobjectArray>(
        env->GetObjectField(jRole, JavaIds::jChannelRoleBehavioralTagsFieldId));

    role.roleName = JavaShimUtils::jstring2string(env, jRoleName);
    role.mechanicalTags = JavaShimUtils::jArrayToStringVector(env, jMechanicalTags);
    role.behavioralTags = JavaShimUtils::jArrayToStringVector(env, jBehavioralTags);

    jint jLinkSide = env->CallIntMethod(jRole, JavaIds::jChannelRoleGetLinkSideMethodId);
    role.linkSide = static_cast<LinkSide>(jLinkSide);

    return role;
}

jobjectArray JavaShimUtils::stringVectorToJArray(JNIEnv *env,
                                                 std::vector<std::string> stringArray) {
    jstring str;
    jobjectArray jStringArray = 0;
    jsize len = stringArray.size();
    unsigned int i;
    jStringArray = env->NewObjectArray(len, JavaIds::jStringClassId, 0);
    for (i = 0; i < stringArray.size(); i++) {
        str = env->NewStringUTF(stringArray[i].c_str());
        env->SetObjectArrayElement(jStringArray, static_cast<jsize>(i), str);
    }
    return jStringArray;
}

jobjectArray JavaShimUtils::channelPropertiesVectorToJArray(
    JNIEnv *env, std::vector<ChannelProperties> propertiesArray) {
    jsize len = propertiesArray.size();
    jobjectArray jPropertiesArray = env->NewObjectArray(len, JavaIds::jChannelPropertiesClassId, 0);
    for (unsigned int i = 0; i < propertiesArray.size(); i++) {
        jobject props = channelPropertiesToJobject(env, propertiesArray[i]);
        env->SetObjectArrayElement(jPropertiesArray, static_cast<jsize>(i), props);
    }
    return jPropertiesArray;
}

jobject JavaShimUtils::supportedChannelsToJobject(
    JNIEnv *env, const std::map<std::string, ChannelProperties> &supportedChannels) {
    jobject jSupportedChannels = env->NewObject(JavaIds::jSupportedChannelsClassId,
                                                JavaIds::jSupportedChannelsConstructorMethodId);
    for (auto gidProps : supportedChannels) {
        jstring channelGid = env->NewStringUTF(gidProps.first.c_str());
        jobject props = JavaShimUtils::channelPropertiesToJobject(env, gidProps.second);
        env->CallObjectMethod(jSupportedChannels, JavaIds::jSupportedChannelsPutMethodId,
                              channelGid, props);
    }
    return jSupportedChannels;
}

std::vector<std::string> JavaShimUtils::jArrayToStringVector(JNIEnv *env, jobjectArray &jArray) {
    std::vector<std::string> stringArray;

    jsize size = env->GetArrayLength(jArray);

    for (jsize i = 0; i < size; ++i) {
        jstring javaString = static_cast<jstring>(env->GetObjectArrayElement(jArray, i));
        std::string cString = JavaShimUtils::jstring2string(env, javaString);
        stringArray.push_back(cString);
    }
    return stringArray;
}

RawData JavaShimUtils::jByteArrayToRawData(JNIEnv *env, jbyteArray jData) {
    jbyte *jDataBytes = env->GetByteArrayElements(jData, nullptr);
    jsize jDataSize = env->GetArrayLength(jData);
    RawData data = RawData(jDataBytes, jDataBytes + jDataSize);

    return data;
}

jbyteArray JavaShimUtils::rawDataToJByteArray(JNIEnv *env, const RawData &data) {
    jsize filesize = static_cast<jsize>(data.size());
    jbyteArray jData = env->NewByteArray(filesize);
    env->SetByteArrayRegion(jData, 0, filesize, reinterpret_cast<const jbyte *>(data.data()));

    return jData;
}

void JavaShimUtils::setTraceAndSpanIdForJClrMsg(JNIEnv *env, jobject jClrMsg, int64_t traceId,
                                                int64_t spanId) {
    env->SetLongField(jClrMsg, JavaIds::jClrMsgTraceIdFieldId, static_cast<jlong>(traceId));
    env->SetLongField(jClrMsg, JavaIds::jClrMsgSpanIdFieldId, static_cast<jlong>(spanId));
}
