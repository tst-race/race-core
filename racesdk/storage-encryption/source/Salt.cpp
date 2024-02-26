
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

#include "Salt.h"

#include <openssl/rand.h>  // RAND_bytes

#include <fstream>

#include "filesystem.h"

#define SALT_SIZE 32

std::vector<std::uint8_t> Salt::get(const std::string &dir, const std::string &fileName) {
    std::vector<std::uint8_t> saltData(SALT_SIZE);

    fs::path saltFilePath(dir + "/" + fileName);

    if (fs::exists(saltFilePath)) {
        std::ifstream file(saltFilePath.native());
        const size_t fileSize = fs::file_size(saltFilePath);
        std::vector<std::uint8_t> readData(fileSize);
        file.read(reinterpret_cast<char *>(readData.data()),
                  static_cast<int64_t>(static_cast<std::int64_t>(fileSize)));
        if (file.fail()) {
            throw std::runtime_error("Failed to read file: " + saltFilePath.string());
        }
        return readData;
    } else {
        RAND_bytes(saltData.data(), SALT_SIZE);
        std::ofstream outputFile(saltFilePath.native(), std::ofstream::trunc);
        if (outputFile.fail()) {
            throw std::runtime_error("Failed to open file for writing: " + saltFilePath.string());
        }

        outputFile.write(reinterpret_cast<const char *>(saltData.data()),
                         static_cast<std::int64_t>(saltData.size()));
        if (outputFile.fail()) {
            throw std::runtime_error("Failed to write file: " + saltFilePath.string());
        }
        return saltData;
    }
}
