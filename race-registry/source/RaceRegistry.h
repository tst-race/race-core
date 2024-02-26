
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

#pragma once

#include <IRaceSdkApp.h>
#include <OpenTracingHelpers.h>

class RaceRegistry {
public:
    explicit RaceRegistry(IRaceSdkApp &_raceSdk, std::shared_ptr<opentracing::Tracer> tracer);

    /* This method should be implemented with registry specific logic */
    void handleRegistryMessage(const std::string &msg, const std::string &persona, int8_t ampIndex);

protected:
    void sendResponse(const std::string &msg, const std::string &destination, int8_t ampIndex);

protected:
    IRaceSdkApp &raceSdk;
    std::shared_ptr<opentracing::Tracer> tracer;
};