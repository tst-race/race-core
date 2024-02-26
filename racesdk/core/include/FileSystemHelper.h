
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

#ifndef _FILE_SYSTEM_HELPER_H__
#define _FILE_SYSTEM_HELPER_H__

#include <StorageEncryption.h>

class FileSystemHelper {
public:
    explicit FileSystemHelper();
    virtual ~FileSystemHelper() = default;

    /**
     * @brief Recursively copy the contents of the source directory and decrypt them before writing
     * into the destination
     * @param srcPath Source directory path
     * @param destPath Destination directory path
     * @param pluginStorageEncryption The Storage Encryption object responsible for
     * encrpting/decrypting the file
     *
     * @return True if successful
     */
    virtual bool copyAndDecryptDir(const std::string &srcPath, const std::string &destPath,
                                   StorageEncryption &pluginStorageEncryption);

    /**
     * @brief Create a new zip file, containing the entire contents of the specified directory
     *
     * @param zipFilePath Absolute path to the zip file to be created
     * @param sourceDirectoryPath Absolute path to the directory to be archived
     * @return True if successful
     */
    virtual bool createZip(const std::string &zipFilePath, const std::string &sourceDirectoryPath);
};

#endif