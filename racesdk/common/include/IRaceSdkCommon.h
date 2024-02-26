
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

#ifndef __I_RACE_SDK_COMMON_H_
#define __I_RACE_SDK_COMMON_H_

#include <cstdint>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "ChannelProperties.h"
#include "EncPkg.h"
#include "PluginResponse.h"
#include "RaceEnums.h"
#include "SdkResponse.h"

/**
 * @brief This constant may be used by either network manager or comms to specify that a function
 * taking a timeout should never time out.
 */
const int32_t RACE_BLOCKING = std::numeric_limits<int32_t>::min();

/**
 * @brief This constant may be used by network manager to indicate that a connection should never
 * have packages timeout on it (packages can still fail, just not timeout)
 */
const int32_t RACE_UNLIMITED = std::numeric_limits<int32_t>::min();

/**
 * @brief This constant is used by network manager to specify that a batch ID is null, i.e. on calls
 * to sendEncryptedPackage.
 */
const uint64_t RACE_BATCH_ID_NULL = 0;

class IRaceSdkCommon {
public:
    /**
     * @brief Destroy the IRaceSdkCommon object
     *
     */
    virtual ~IRaceSdkCommon() {}

    /**
     * @brief Query the system for entropy. Entropy will be gathered from system calls as well as
     * external sources, e.g. user scribbling on screen or reading from a device's microphone.
     *
     * @param numBytes
     * @return RawData
     */
    virtual RawData getEntropy(std::uint32_t numBytes) = 0;

    /**
     * @brief Get the active persona for the RACE System. Currently persona should
     *        should not change, but we are building for the possibility it will in the future.
     *
     * @return std::string return the active persona
     */
    virtual std::string getActivePersona() = 0;

    /**
     * @brief Get the ChannelProperties for a particular channel
     *
     * @param channelGid The name of the channel
     * @return ChannelProperties the properties for the channel
     */
    virtual ChannelProperties getChannelProperties(std::string channelGid) = 0;

    /**
     * @brief Get Channel Properties for all channels. This may be used instead of
     * getSupportedChannels to get channels regardless of what state they're in
     * (getSupportedChannels only returns channels in the AVAILABLE state).
     *
     * @return std::vector<ChannelProperties> the properties for all channels
     */
    virtual std::vector<ChannelProperties> getAllChannelProperties() = 0;

    /**
     * @brief Notify the SDK of an error that occured in an asynchronous call
     *
     * @param race_handle The handle associated with the async call
     * @param plugin_response The status of the async call
     *
     * @return SdkResponse The status of the SDK in response to this call
     */
    virtual SdkResponse asyncError(RaceHandle handle, PluginResponse status) = 0;

    /**
     * @brief Create the directory of directoryPath, including any directories in the path that do
     * not yet exist
     * @param directoryPath the path of the directory to create.
     *
     * @return SdkResponse indicator of success or failure of the create
     */
    virtual SdkResponse makeDir(const std::string &directoryPath) = 0;

    /**
     * @brief Recurively remove the directory of directoryPath
     * @param directoryPath the path of the directory to remove.
     *
     * @return SdkResponse indicator of success or failure of the removal
     */
    virtual SdkResponse removeDir(const std::string &directoryPath) = 0;

    /**
     * @brief List the contents (directories and files) of the directory path
     * @param directoryPath the path of the directory to list.
     *
     * @return std::vector<std::string> list of directories and files
     */
    virtual std::vector<std::string> listDir(const std::string &directoryPath) = 0;

    /**
     * @brief Read the contents of a file in this plugin's storage.
     * @param filename The string path of the file to be read.
     *
     * @return std::vector<uint8_t> of the file contents
     */
    virtual std::vector<std::uint8_t> readFile(const std::string &filepath) = 0;

    /**
     * @brief Append the contents of data to filename in this plugin's storage. Directories are
     * created as-needed.
     * @param filename The string path of the file to be appended to (or written).
     * @param data The string of data to append to the file.
     *
     * @return SdkResponse indicator of success or failure of the append.
     */
    virtual SdkResponse appendFile(const std::string &filepath,
                                   const std::vector<std::uint8_t> &data) = 0;

    /**
     * @brief Write the contents of data to filename in this plugin's storage (overwriting if file
     * exists). Directories are created as-needed.
     * @param filname The string path of the file to be written.
     * @param data The string of data to write to the file.
     *
     * @return SdkResponse indicator of success or failure of the write.
     */
    virtual SdkResponse writeFile(const std::string &filepath,
                                  const std::vector<std::uint8_t> &data) = 0;
};

#endif
