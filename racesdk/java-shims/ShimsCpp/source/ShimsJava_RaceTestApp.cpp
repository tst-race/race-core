
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

#include "ShimsJava_RaceTestApp.h"

#include <OpenTracingHelpers.h>

#include <chrono>

#include "JavaShimUtils.h"
#include "RaceSdk.h"
#include "racetestapp/Message.h"
#include "racetestapp/RaceTestApp.h"
#include "racetestapp/RaceTestAppOutputLog.h"

namespace JavaShims {
std::shared_ptr<RaceTestApp> raceTestApp;
}

using Clock = std::chrono::system_clock;
using Millis = std::chrono::milliseconds;
using Time = std::chrono::time_point<Clock>;

/*
 * Class:     ShimsJava_RaceTestApp
 * Method:    _jni_initialize
 * Signature: (JJJ)V
 */
JNIEXPORT void JNICALL Java_ShimsJava_RaceTestApp__1jni_1initialize(
    JNIEnv *, jobject, jlong jOutputPtr, jlong jRaceSdkPtr, jlong jRaceAppPtr, jlong jTracerPtr) {
    RaceTestAppOutputLog *outputPtr = reinterpret_cast<RaceTestAppOutputLog *>(jOutputPtr);
    RaceSdk *raceSdkPtr = reinterpret_cast<RaceSdk *>(jRaceSdkPtr);
    RaceApp *raceAppPtr = reinterpret_cast<RaceApp *>(jRaceAppPtr);
    std::shared_ptr<opentracing::Tracer> *tracerPtr =
        reinterpret_cast<std::shared_ptr<opentracing::Tracer> *>(jTracerPtr);
    JavaShims::raceTestApp =
        std::make_shared<RaceTestApp>(*outputPtr, *raceSdkPtr, *raceAppPtr, *tracerPtr);
}

/*
 * Class:     ShimsJava_RaceTestApp
 * Method:    sendMessage
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_ShimsJava_RaceTestApp_sendMessage(JNIEnv *env, jobject, jstring jText,
                                                              jstring jTo) {
    std::string text = JavaShimUtils::jstring2string(env, jText);
    std::string to = JavaShimUtils::jstring2string(env, jTo);
    rta::Message message = rta::Message(text, to, Clock::now(), {}, false, "");
    JavaShims::raceTestApp->sendMessage(message);
}

/*
 * Class:     ShimsJava_RaceTestApp
 * Method:    processRaceTestAppCommand
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT jboolean JNICALL Java_ShimsJava_RaceTestApp_processRaceTestAppCommand(JNIEnv *env,
                                                                                jobject,
                                                                                jstring jCommand) {
    std::string command = JavaShimUtils::jstring2string(env, jCommand);
    return JavaShims::raceTestApp->processRaceTestAppCommand(command);
}