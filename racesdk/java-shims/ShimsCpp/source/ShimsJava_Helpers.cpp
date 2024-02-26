
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

#include "ShimsJava_Helpers.h"

#include <OpenTracingHelpers.h>  // createTracer, traceIdFromContext, spanIdFromContext
#include <RaceLog.h>             // RaceLog::logInfo, RaceLog::logError

#include <algorithm>     // std::find_if
#include <chrono>        // std::chrono::system_clock
#include <iomanip>       // std::setfill, std::setw
#include <random>        // std::default_random_engine, std::random_device
#include <sstream>       // std::stringstream
#include <stdexcept>     // std::invalid_argument
#include <system_error>  // std::system_error

#include "JavaShimUtils.h"
#include "racetestapp/RaceTestAppOutputLog.h"

namespace JavaJaeger {
std::shared_ptr<opentracing::Tracer> tracer;
}  // namespace JavaJaeger
namespace JavaShims {
std::shared_ptr<RaceTestAppOutputLog> output;
}

const std::string logLabel = "ShimsJava_Helpers";

/**
 * @brief initialize tracer from file at specified path.
 *
 * @param env JNIEnv passed in by JNI, used to do JNI conversions
 * @param jclass This is passed in by JNI but we don't use it.
 * @param jJaegerConfigPath The path to load the file from
 * @param jActivePersona The active persona on this node.
 */
JNIEXPORT jlong JNICALL Java_ShimsJava_Helpers_createTracer(JNIEnv *env, jclass,
                                                            jstring jJaegerConfigPath,
                                                            jstring jActivePersona) {
    std::string jaegerConfigPath = JavaShimUtils::jstring2string(env, jJaegerConfigPath);
    std::string activePersona = JavaShimUtils::jstring2string(env, jActivePersona);
    RaceLog::logDebug(logLabel, "Debug: Initializing OpenTracing using '" + jaegerConfigPath + "'",
                      "");
    try {
        JavaJaeger::tracer = createTracer(jaegerConfigPath, activePersona);
    } catch (std::exception &e) {
        RaceLog::logDebug(logLabel, "Failed to create tracer: " + std::string(e.what()), "");
    }
    return reinterpret_cast<jlong>(&JavaJaeger::tracer);
}

/**
 * @brief Create RaceTestAppOutputLog and return pointer
 *
 * @param env JNIEnv passed in by JNI, used to do JNI conversions
 * @param jclass This is passed in by JNI but we don't use it.
 * @param jDir Directory to place RaceTestApp output logs
 */
JNIEXPORT jlong JNICALL Java_ShimsJava_Helpers_createRaceTestAppOutputLog(JNIEnv *env, jclass,
                                                                          jstring jDir) {
    std::string dir = JavaShimUtils::jstring2string(env, jDir);
    JavaShims::output = std::make_shared<RaceTestAppOutputLog>(dir);
    return reinterpret_cast<jlong>(JavaShims::output.get());
}