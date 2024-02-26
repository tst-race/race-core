
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

#ifndef __PERSISTENT_STORAGE_HELPERS_H__
#define __PERSISTENT_STORAGE_HELPERS_H__

#include <sstream>
#include <string>

#include "IComponentSdkBase.h"
#include "log.h"

namespace psh {

/**
 * @brief Save a value to persistent storage. This storage is per node. The value may be
 * retreived by passing the same key to readValue. This vesion stores a single int
 *
 * @param sdk The sdk instance used to access persistent storage APIs
 * @param key The key that may be used to retrieve the value
 * @param value The value associated with the key
 * @return true on success, false on failure
 */
template <typename T>
bool saveValue(IComponentSdkBase *sdk, const std::string &key, T value);

/**
 * @brief Read an integer from persistent storage. If the key is not found, return a specified
 * default value.
 *
 * @param sdk The sdk instance used to access persistent storage APIs
 * @param key The key to look up the value for
 * @param defaultValue The value to return if the key is not found
 * @return the found value if found, or defaultValue if not
 */
template <typename T>
T readValue(IComponentSdkBase *sdk, const std::string &key, T defaultValue);

}  // namespace psh

template <typename T>
bool psh::saveValue(IComponentSdkBase *sdk, const std::string &key, T value) {
    std::string valueString = std::to_string(value);
    return sdk->writeFile(key, {valueString.begin(), valueString.end()}).status == CM_OK;
}

template <typename T>
T psh::readValue(IComponentSdkBase *sdk, const std::string &key, T default_value) {
    const std::string loggingPrefix = "psh::readValue (" + key + "): ";

    std::vector<uint8_t> valueData = sdk->readFile(key);
    if (valueData.size() == 0) {
        return default_value;
    }

    std::string valueString(valueData.begin(), valueData.end());
    logDebug(loggingPrefix + "key: " + key + " value: " + valueString);
    std::stringstream ss(valueString);

    T val;
    ss >> val;

    if (ss) {
        return val;
    } else {
        logError(loggingPrefix + "Could not read value of type " + typeid(T).name() + " from key " +
                 key);
        return default_value;
    }
}

#endif
