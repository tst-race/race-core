
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

#ifndef __I_COMPONENT_SDK_BASE_H__
#define __I_COMPONENT_SDK_BASE_H__

#include "IComponentBase.h"

class IComponentSdkBase {
public:
    virtual ~IComponentSdkBase() = default;

    virtual std::string getActivePersona() = 0;
    virtual ChannelResponse requestPluginUserInput(const std::string &key,
                                                   const std::string &prompt, bool cache) = 0;
    virtual ChannelResponse requestCommonUserInput(const std::string &key) = 0;

    virtual ChannelResponse updateState(ComponentState state) = 0;

    /**
     * @brief Create the directory of directoryPath, including any directories in the path that do
     * not yet exist
     * @param directoryPath the path of the directory to create.
     *
     * @return Channel response (status and handle)
     */
    virtual ChannelResponse makeDir(const std::string &directoryPath) = 0;

    /**
     * @brief Recurively remove the directory of directoryPath
     * @param directoryPath the path of the directory to remove.
     *
     * @return Channel response (status and handle)
     */
    virtual ChannelResponse removeDir(const std::string &directoryPath) = 0;

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
     * @return Channel response (status and handle)
     */
    virtual ChannelResponse appendFile(const std::string &filepath,
                                       const std::vector<std::uint8_t> &data) = 0;

    /**
     * @brief Write the contents of data to filename in this plugin's storage (overwriting if file
     * exists). Directories are created as-needed.
     * @param filname The string path of the file to be written.
     * @param data The string of data to write to the file.
     *
     * @return Channel response (status and handle)
     */
    virtual ChannelResponse writeFile(const std::string &filepath,
                                      const std::vector<std::uint8_t> &data) = 0;
};

#endif
