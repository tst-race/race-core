
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

#include "../../include/StorageEncryption.h"
#include "../../include/filesystem.h"
#include "gmock/gmock.h"

TEST(decrypt, decrypt_of_encrypt) {
    StorageEncryption storageEncryption;
    storageEncryption.init(RaceEnums::StorageEncryptionType::ENC_AES, "myWeakPassphrase",
                           "/tmp/race");

    const std::vector<std::uint8_t> dataToWrite = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    // Files for writing must use an absolute path.
    fs::path outputFile = fs::current_path() / "some-encrypted-file-for-testing";
    const std::string outputFileName = outputFile.string();

    storageEncryption.write(outputFileName, dataToWrite);
    const std::vector<std::uint8_t> dataReadFromFile = storageEncryption.read(outputFileName);

    EXPECT_THAT(dataToWrite, ::testing::ContainerEq(dataReadFromFile));
}
