
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

#include "RaceRegistryApp.h"

#include <OpenTracingHelpers.h>

#include "racetestapp/raceTestAppHelpers.h"

RaceRegistryApp::RaceRegistryApp(IRaceTestAppOutput &appOutput, IRaceSdkApp &_raceSdk,
                                 std::shared_ptr<opentracing::Tracer> tracer,
                                 RaceRegistry &registry) :
    RaceApp(appOutput, _raceSdk, tracer), registry(registry) {}

void RaceRegistryApp::handleReceivedMessage(ClrMsg msg) {
    auto ctx = spanContextFromClrMsg(msg);
    std::shared_ptr<opentracing::Span> span =
        tracer->StartSpan("receiveMessage", {opentracing::FollowsFrom(ctx.get())});

    try {
        span->SetTag("source", "race registry app");
        span->SetTag("file", __FILE__);
        span->SetTag("messageSize", std::to_string(msg.getMsg().size()));
        span->SetTag("messageHash", rtah::getMessageSignature(msg));
        span->SetTag("messageFrom", msg.getFrom());
        span->SetTag("messageTo", msg.getTo());
        span->SetTag("messageTestId", rtah::testIdFromClrMsg(msg));

        nlohmann::json msgJson = nlohmann::json::parse(msg.getMsg());
        int8_t ampIndex = msgJson["ampIndex"];
        std::string messageBody = msgJson["message"];
        std::string from = msg.getFrom();
        registry.handleRegistryMessage(messageBody, from, ampIndex);
    } catch (std::exception &e) {
        appOutput.writeOutput("Exception while handling message: " + std::string(e.what()));
    }
}
