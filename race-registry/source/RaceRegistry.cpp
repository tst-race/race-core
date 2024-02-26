
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

#include "RaceRegistry.h"

#include <racetestapp/raceTestAppHelpers.h>

#include <nlohmann/json.hpp>

RaceRegistry::RaceRegistry(IRaceSdkApp &_raceSdk, std::shared_ptr<opentracing::Tracer> tracer) :
    raceSdk(_raceSdk), tracer(tracer) {}

void RaceRegistry::sendResponse(const std::string &msg, const std::string &destination,
                                int8_t ampIndex) {
    std::shared_ptr<opentracing::Span> span = tracer->StartSpan("sendMessage");

    ClrMsg clrMsg = ClrMsg(msg, raceSdk.getActivePersona(), destination,
                           rtah::getTimeInMicroseconds(), 0, ampIndex);

    span->SetTag("source", "racetestapp");
    span->SetTag("file", __FILE__);
    span->SetTag("messageSize", std::to_string(clrMsg.getMsg().size()));
    span->SetTag("messageHash", rtah::getMessageSignature(clrMsg));
    span->SetTag("messageFrom", clrMsg.getFrom());
    span->SetTag("messageTo", clrMsg.getTo());
    span->SetTag("messageTestId", rtah::testIdFromClrMsg(clrMsg));

    raceSdk.sendClientMessage(clrMsg);
}

/* This method should be implemented with registry specific logic */
void RaceRegistry::handleRegistryMessage(const std::string &msg, const std::string &persona,
                                         int8_t ampIndex) {
    rtah::logError("Received handleRegistryMessage stub called");

    // msg should be modified as appropriate. Persona and ampIndex should be passed unmodified
    sendResponse(msg, persona, ampIndex);
}