
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

#include <RaceEnums.h>
#include <StorageEncryption.h>

// TODO: move `racesdk/core/source/filesystem.h` to `racesdk/common/include/` instead so this isn't
// duplicated?
#include <filesystem.h>
#include <string.h>  // strcmp

#include <exception>
#include <fstream>
#include <iostream>

#include "filesystem.h"

const std::string &keyDir = "/etc/race";

std::vector<std::uint8_t> readFile(const std::string &fileName) {
    // Open the file for reading, in binary mode, and start at the end.
    std::ifstream file(fileName, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + fileName);
    }

    // Get the file size.
    const std::streampos fileSize = file.tellg();
    if (fileSize <= 0) {
        return {};
    }
    // Move the file pointer to the beginning of the file.
    file.seekg(0, std::ios::beg);
    // Read the file and store it in a vector.
    std::vector<std::uint8_t> fileData(static_cast<size_t>(fileSize));
    file.read(reinterpret_cast<char *>(fileData.data()),
              static_cast<int64_t>(static_cast<std::int64_t>(fileSize)));

    return fileData;
}

int encrypt(const std::string &source, const std::string &dest, const std::string &passphrase) {
    std::cout << "encrypt " << source << " " << dest << std::endl;

    // Encrypt the file and save it in dest.
    try {
        StorageEncryption se;
        se.init(RaceEnums::StorageEncryptionType::ENC_AES, passphrase, keyDir);
        // The write function requres an absolute path for the destination file, so provide one.
        // TODO: do we want to require this for the write function? This happens because we are
        // creating the directory if it does not exist, which fails for file names with no path
        // behind them/ The SDK should be handling plugin storage sandboxing, so we don't really
        // need to require this. Just need to decide. Probably cleaner so not have the encryption
        // library handle this.
        se.write(fs::absolute(dest).string(), readFile(source));
    } catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

void writeFile(const std::string &fileName, const std::vector<std::uint8_t> &data) {
    // Open the file and overwrite any existing data.
    std::ofstream file(fileName, std::ofstream::trunc);
    if (file.fail()) {
        throw std::runtime_error("failed to open file: " + fileName);
    }

    file.write(reinterpret_cast<const char *>(data.data()), static_cast<std::int64_t>(data.size()));
    if (file.fail()) {
        throw std::runtime_error("failed to write to file: " + fileName);
    }
}

int decrypt(const std::string &source, const std::string &dest, const std::string &passphrase) {
    std::cout << "decrypt " << source << " " << dest << std::endl;

    // Decrypt the file and save it in dest.
    try {
        StorageEncryption se;
        se.init(RaceEnums::StorageEncryptionType::ENC_AES, passphrase, keyDir);
        writeFile(dest, se.read(source));
    } catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

void printUsage(const std::string &programName) {
    // TODO: should we make the output file name be optional? If not provided, just [en|de]crypt in
    // the same location with some alternate file extension?

    // std::cout << "usage: " << programName << " init --encryption [none|aes]" << std::endl;
    // std::cout << "       " << programName << " init --config [race config json file]" <<
    // std::endl;

    std::cout << "usage: " << programName << " [encrypt|decrypt] src dst passphrase" << std::endl;
    std::cout << std::endl;
    std::cout << "NOTE: application currently only supports AES encryption." << std::endl;
}

int main(int argc, char *argv[]) {
    const int requiredNumArgs = 5;
    if (argc == requiredNumArgs && strcmp(argv[1], "encrypt") == 0) {
        return encrypt(argv[2], argv[3], argv[4]);
    } else if (argc == requiredNumArgs && strcmp(argv[1], "decrypt") == 0) {
        return decrypt(argv[2], argv[3], argv[4]);
    } else {
        printUsage(argv[0]);
        return 1;
    }
}
