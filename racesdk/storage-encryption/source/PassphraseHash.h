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

#ifndef __STORAGE_ENCRYPTION_SOURCE_PASSPHRASE_HASH_H_
#define __STORAGE_ENCRYPTION_SOURCE_PASSPHRASE_HASH_H_

#include <vector>   // std::vector
#include <string>  // std::string
#include <cstdint>
#include "filesystem.h"

/**
 * @brief Class to compute and store a hash of the user provided passphrase.
 * 
 */
class PassphraseHash {
public:
    /**
     * @brief Construct a new Passphrase Hash object.
     * 
     * @param dir The directory where the hash file should be stored.
     */
    explicit PassphraseHash(const std::string &dir);

    /**
     * @brief Check if the hash file exists.
     * 
     * @return true If the file exists.
     * @return false If the file does not exist.
     */
    bool exists();

    /**
     * @brief Create a new hash file for a given passphrase and salt.
     * 
     * @param passphrase The passphrase to hash.
     * @param salt The salt used for hashing.
     * 
     * @throw std::runtime_error The hash file already exists.
     * @throw std::runtime_error The function failed to create the hash file.
     * @throw std::runtime_error The function failed to write to the created hash file.
     */
    void create(const std::string &passphrase, const std::vector<std::uint8_t> &salt);

    /**
     * @brief Read the hash value from the file.
     * 
     * @return std::vector<std::uint8_t> The hash value.
     *
     * @throw std::runtime_error The function failed to open or read the hash file.
     */
    std::vector<std::uint8_t> get();

    /**
     * @brief Compare a given passphrase and salt to the existing value in the hash file.
     * 
     * @param passphrase The passphrase used to compare to the existing file.
     * @param salt The salt used to compare to the existing file.
     * @return true The passphrase and salt match the existing file.
     * @return false The passphrase and salt do NOT match the existing file.
     * 
     * @throw std::runtime_error The function failed to open or read the hash file.
     */
    bool compare(const std::string &passphrase, const std::vector<std::uint8_t> &salt);

protected:
    /**
     * @brief Generate a hash for a given passphrase and salt. This is the value that will be written to file.
     * 
     * @param passphrase The passphrase to hash.
     * @param salt The salt used for hashing.
     * @return std::vector<std::uint8_t> The hash.
     */
    std::vector<std::uint8_t> _generateHash(const std::string &passphrase, const std::vector<std::uint8_t> &salt);

private:
    /**
     * @brief The path of the hash file.
     * 
     */
    fs::path hashFilePath;
};

#endif
