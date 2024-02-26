
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

#ifndef __COMPOSITE_WRAPPER_H__
#define __COMPOSITE_WRAPPER_H__

#include <RacePluginExports.h>

#include <cctype>
#include <memory>
#include <sstream>

#include "CommsWrapper.h"
#include "ComponentPlugin.h"
#include "DynamicLibrary.h"
#include "OpenTracingForwardDeclarations.h"
#include "RaceSdk.h"
#include "decomposed-comms/ComponentManager.h"
#include "filesystem.h"
#include "helper.h"

class CompositeWrapper : public CommsWrapper {
public:
    CompositeWrapper(RaceSdk &sdk, Composition composition, const std::string &description,
                     IComponentPlugin &transport, IComponentPlugin &usermodel,
                     const std::unordered_map<std::string, IComponentPlugin *> &encodings) :
        CommsWrapper(sdk, composition.id) {
        TRACE_METHOD();

        auto componentManager =
            std::make_shared<ComponentManager>(*this, composition, transport, usermodel, encodings);

        this->mPlugin = componentManager;
        this->mId = composition.id;
        this->mDescription = description;
        this->mConfigPath = "";

        helper::logDebug("LoaderWrapper: returned");
    }
    // Ensure Parent type has a virtual destructor (required for unique_ptr conversion)
    virtual ~CompositeWrapper() override {
        helper::logDebug("LoaderWrapper::~LoaderWrapper: called");
        this->mPlugin.reset();
        helper::logDebug("LoaderWrapper::~LoaderWrapper: returned");
    }
};

#endif
