
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

#ifndef __USER_INPUT_RESPONSE_CACHE_H__
#define __USER_INPUT_RESPONSE_CACHE_H__

#include <nlohmann/json.hpp>
#include <string>

#include "IRaceSdkApp.h"

/**
 * Cache of user input response using the SDK persisted storage APIs.
 */
class UserInputResponseCache {
private:
    IRaceSdkApp &raceSdk;
    nlohmann::json cache;

public:
    explicit UserInputResponseCache(IRaceSdkApp &sdk);

    virtual ~UserInputResponseCache() {}

    /**
     * @brief Reads contents of the cache file into memory.
     *
     * @return True if successful
     */
    virtual bool readCache();

    /**
     * @brief Retrieves the cached response for the given prompt for the specified plugin.
     *
     * @param pluginId Plugin ID for which to get a response (or "Common" for common values)
     * @param prompt User input prompt/key
     * @return Cached user response value
     * @throw std::out_of_range if no cache value exists
     */
    virtual std::string getResponse(const std::string &pluginId, const std::string &prompt);

    /**
     * @brief Caches the given response for the given prompt for the specified plugin.
     * @param pluginId Plugin ID for which to get a response (or "Common" for common values)
     * @param prompt User input prompt/key
     * @param response User input response value to cache
     * @return True if successful
     */
    virtual bool cacheResponse(const std::string &pluginId, const std::string &prompt,
                               const std::string &response);

    /**
     * @brief Clears all values in the cache.
     * @return True if successful
     */
    virtual bool clearCache();

private:
    bool writeCache();
};

#endif  // __USER_INPUT_RESPONSE_CACHE_H__