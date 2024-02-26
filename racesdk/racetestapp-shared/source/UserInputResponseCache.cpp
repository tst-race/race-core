
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

#include "racetestapp/UserInputResponseCache.h"

#include "IRaceSdkApp.h"
#include "racetestapp/raceTestAppHelpers.h"

using json = nlohmann::json;

static std::string fileName = "user-input-response-cache.json";

UserInputResponseCache::UserInputResponseCache(IRaceSdkApp &sdk) : raceSdk(sdk) {}

std::string UserInputResponseCache::getResponse(const std::string &pluginId,
                                                const std::string &prompt) {
    auto key = pluginId + "." + prompt;
    try {
        return cache.at(key).get<std::string>();
    } catch (std::exception &error) {
        rtah::logDebug("No cache entry for " + key + ": " + std::string(error.what()));
        throw std::out_of_range(key);
    }
}

bool UserInputResponseCache::cacheResponse(const std::string &pluginId, const std::string &prompt,
                                           const std::string &response) {
    auto key = pluginId + "." + prompt;
    cache[key] = response;
    return writeCache();
}

bool UserInputResponseCache::clearCache() {
    cache = json::object();
    return writeCache();
}

bool UserInputResponseCache::readCache() {
    cache = json::object();
    auto raw = raceSdk.readFile(fileName);
    if (not raw.empty()) {
        std::string contents(raw.begin(), raw.end());
        try {
            cache = json::parse(contents);
            return true;
        } catch (std::exception &error) {
            rtah::logWarning("Unable to parse user response cache: " + std::string(error.what()));
        }
    }
    return false;
}

bool UserInputResponseCache::writeCache() {
    auto contents = cache.dump();
    auto response = raceSdk.writeFile(fileName, {contents.begin(), contents.end()});
    if (response.status != SDK_OK) {
        rtah::logWarning("Failed to write to user response cache");
    }
    return response.status == SDK_OK;
}
