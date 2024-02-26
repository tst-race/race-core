
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

#ifndef __STORAGE_ENCRYPTION_INCLUDE_STORAGEENCRYPTION_H_
#define __STORAGE_ENCRYPTION_INCLUDE_STORAGEENCRYPTION_H_

#include <RaceEnums.h>  // StorageEncryptionType

#include <string>  // std::string
#include <vector>  // std::vector

// TODO: move `racesdk/core/source/filesystem.h` to `racesdk/common/include/` instead so this isn't
// duplicated?
#include "filesystem.h"

class StorageEncryption {
public:
    StorageEncryption() = default;

    /**
     * @brief Initialize the type of encryption to be used. Note that this creates a key file that
     * will be used by all instances of this class.
     *
     * @param encType The type of encryption, at the moment either ENC_AES or ENC_NONE.
     * @param passPhrase A user provided passphrase use to generate the key.
     * @param keyDir The directory where the key will be stored.
     */
    void init(RaceEnums::StorageEncryptionType encType, const std::string &passphrase,
              const std::string &keyDir);

    /**
     * @brief
     *
     * @param fullFilePath
     * @return std::vector<std::uint8_t>
     */
    std::vector<std::uint8_t> read(const std::string &fullFilePath);

    /**
     * @brief
     *
     * @param fullFilePath
     * @param data
     */
    void write(const std::string &fullFilePath, const std::vector<std::uint8_t> &data);

    void append(const std::string &fullFilePath, const std::vector<std::uint8_t> &data);

    /**
     * @brief Helper function that determines if a file is encryptable. Files are not encrytable if
     * they exist for testing purposes only
     *
     * @param filename file to check
     * @return true if file can be encrypted
     */
    static bool isFileEncryptable(std::string filename);

protected:
    /**
     * @brief Create a Key object
     *
     * @param encType
     */
    void createKey(RaceEnums::StorageEncryptionType encType, const std::string &passphrase);

    RaceEnums::StorageEncryptionType getEncryptionType();

    std::vector<std::uint8_t> decrypt(const std::vector<std::uint8_t> &ciphertext);

    std::vector<std::uint8_t> decrypt(const std::vector<std::uint8_t> &rawCiphertext,
                                      const std::vector<std::uint8_t> &iv);

    std::vector<std::uint8_t> encrypt(const std::vector<std::uint8_t> &plaintext);

    std::vector<std::uint8_t> encrypt(const std::vector<std::uint8_t> &plaintext,
                                      const std::vector<std::uint8_t> &iv);

protected:
    // TODO: not sure if we really care about preventing copying. might remove these
    StorageEncryption(const StorageEncryption &) {}
    StorageEncryption(StorageEncryption &&) {}
    StorageEncryption &operator=(const StorageEncryption &) {
        return *this;
    }
    StorageEncryption &operator=(StorageEncryption &&) {
        return *this;
    }

private:
    /**
     * @brief Path to the working directory where files are stored, e.g. the salt and passphrase
     * hash.
     *
     */
    fs::path workingDirectory;

    /**
     * @brief The key used for encryption/decryption.
     *
     */
    std::vector<std::uint8_t> fileKey;

public:
    class InvalidPassphrase : public std::runtime_error {
    public:
        explicit InvalidPassphrase(const char *what_arg);
    };
};

#endif
