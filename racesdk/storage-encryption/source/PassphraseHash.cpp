
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

#include "PassphraseHash.h"

#include <openssl/sha.h>

#include <fstream>  // std::ifstream, std::ofstream

PassphraseHash::PassphraseHash(const std::string &dir) : hashFilePath(dir + "/passphrase_hash") {}

bool PassphraseHash::exists() {
    return fs::exists(hashFilePath);
}

void PassphraseHash::create(const std::string &passphrase, const std::vector<std::uint8_t> &salt) {
    if (this->exists()) {
        throw std::runtime_error("hash already exists: " + hashFilePath.string());
    }
    auto hash = this->_generateHash(passphrase, salt);
    std::ofstream outputFile(hashFilePath.native(), std::ofstream::trunc);
    if (outputFile.fail()) {
        throw std::runtime_error("failed to create passphrase hash file: " + hashFilePath.string());
    }

    outputFile.write(reinterpret_cast<const char *>(hash.data()),
                     static_cast<std::int64_t>(hash.size()));
    if (outputFile.fail()) {
        throw std::runtime_error("failed to write to passphrase hash file: " +
                                 hashFilePath.string());
    }
}

std::vector<std::uint8_t> PassphraseHash::get() {
    std::ifstream file(hashFilePath.native());
    const size_t fileSize = fs::file_size(hashFilePath);
    std::vector<std::uint8_t> hash(fileSize);
    file.read(reinterpret_cast<char *>(hash.data()), static_cast<std::int64_t>(fileSize));
    if (file.fail()) {
        throw std::runtime_error("failed to read existing hash: " + hashFilePath.string());
    }
    return hash;
}

bool PassphraseHash::compare(const std::string &passphrase, const std::vector<std::uint8_t> &salt) {
    auto existingHash = this->get();
    auto newHash = this->_generateHash(passphrase, salt);
    return existingHash == newHash;
}

std::vector<std::uint8_t> PassphraseHash::_generateHash(const std::string &passphrase,
                                                        const std::vector<std::uint8_t> &salt) {
    if (passphrase.empty()) {
        return {};
    }
    std::vector<std::uint8_t> hash(SHA256_DIGEST_LENGTH);
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, salt.data(), salt.size());
    SHA256_Update(&sha256, passphrase.c_str(), passphrase.size());
    SHA256_Final(hash.data(), &sha256);
    return hash;
}
