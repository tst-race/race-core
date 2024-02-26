
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

#include "../../../source/helper.h"

#include <openssl/rand.h>

#include <algorithm>
#include <fstream>  // std::ifstream
#include <sstream>  // std::stringstream

#include "../../common/race_printers.h"
#include "gtest/gtest.h"

TEST(doesConnectionIncludeGivenPersonas, true_for_empty_personas) {
    EXPECT_TRUE(helper::doesConnectionIncludeGivenPersonas({}, {}));
}

TEST(doesConnectionIncludeGivenPersonas, true_for_empty_given_personas) {
    std::vector<std::string> connectionProfilePersonas = {"A"};
    const std::vector<std::string> givenPersonas = {};
    EXPECT_TRUE(
        helper::doesConnectionIncludeGivenPersonas(connectionProfilePersonas, givenPersonas));
}

TEST(doesConnectionIncludeGivenPersonas, true_when_inputs_are_equal) {
    std::vector<std::string> connectionProfilePersonas = {"A"};
    const std::vector<std::string> givenPersonas = {"A"};
    EXPECT_TRUE(
        helper::doesConnectionIncludeGivenPersonas(connectionProfilePersonas, givenPersonas));
}

TEST(doesConnectionIncludeGivenPersonas, should_handle_out_of_order) {
    std::vector<std::string> connectionProfilePersonas = {"B", "A", "C"};
    const std::vector<std::string> givenPersonas = {"A", "B"};
    EXPECT_TRUE(
        helper::doesConnectionIncludeGivenPersonas(connectionProfilePersonas, givenPersonas));
}

TEST(doesConnectionIncludeGivenPersonas, should_handle_repeats) {
    std::vector<std::string> connectionProfilePersonas = {"B", "A", "C", "C", "C"};
    const std::vector<std::string> givenPersonas = {"A", "A", "A", "A", "A", "A"};
    EXPECT_TRUE(
        helper::doesConnectionIncludeGivenPersonas(connectionProfilePersonas, givenPersonas));
}

TEST(doesConnectionIncludeGivenPersonas, expect_false_1) {
    std::vector<std::string> connectionProfilePersonas = {"A"};
    const std::vector<std::string> givenPersonas = {"A", "B"};
    EXPECT_FALSE(
        helper::doesConnectionIncludeGivenPersonas(connectionProfilePersonas, givenPersonas));
}

TEST(doesConnectionIncludeGivenPersonas, expect_false_2) {
    std::vector<std::string> connectionProfilePersonas = {"B", "A", "C"};
    const std::vector<std::string> givenPersonas = {"A", "B", "D"};
    EXPECT_FALSE(
        helper::doesConnectionIncludeGivenPersonas(connectionProfilePersonas, givenPersonas));
}

TEST(writeFile, read_write_nokey) {
    StorageEncryption storageEncryption;
    storageEncryption.init(RaceEnums::StorageEncryptionType::ENC_AES, "myWeakPassphrase", "/tmp/");

    std::string datapath = "data";
    std::string pluginId = "testPlugin";
    std::string filename = "test";
    fs::create_directory(datapath);
    fs::create_directory(fs::path(datapath) / pluginId);
    std::string msg = "Hello World";
    std::vector<unsigned char> bytes = {msg.begin(), msg.end()};

    helper::writeFile(filename, pluginId, datapath, bytes, storageEncryption);

    EXPECT_EQ(helper::readFile(filename, pluginId, datapath, storageEncryption), bytes);
    fs::remove_all(datapath);
}

TEST(writeFile, write_twice_overwrites) {
    StorageEncryption storageEncryption;
    storageEncryption.init(RaceEnums::StorageEncryptionType::ENC_AES, "myWeakPassphrase",
                           "/tmp/race");

    std::string datapath = "data";
    std::string pluginId = "testPlugin";
    std::string filename = "test";
    fs::create_directory(datapath);
    fs::create_directory(fs::path(datapath) / pluginId);
    std::string msg = "Hello World";
    std::string msg2 = "Goodbye, cruel world.";
    std::vector<unsigned char> bytes = {msg.begin(), msg.end()};
    std::vector<unsigned char> bytes2 = {msg2.begin(), msg2.end()};

    helper::writeFile(filename, pluginId, datapath, bytes, storageEncryption);
    EXPECT_EQ(helper::readFile(filename, pluginId, datapath, storageEncryption), bytes);
    helper::writeFile(filename, pluginId, datapath, bytes2, storageEncryption);
    EXPECT_EQ(helper::readFile(filename, pluginId, datapath, storageEncryption), bytes2);
    fs::remove_all(datapath);
}

TEST(writeFile, append_appends) {
    StorageEncryption storageEncryption;
    storageEncryption.init(RaceEnums::StorageEncryptionType::ENC_AES, "myWeakPassphrase",
                           "/tmp/race/");

    std::string datapath = "data";
    std::string pluginId = "testPlugin";
    std::string filename = "test";
    fs::create_directory(datapath);
    fs::create_directory(fs::path(datapath) / pluginId);
    std::string msg = "Hello World";
    std::string msg2 = "Goodbye, cruel world.";
    std::vector<unsigned char> bytes = {msg.begin(), msg.end()};
    std::vector<unsigned char> bytes2 = {msg2.begin(), msg2.end()};
    std::vector<unsigned char> appended = {bytes.begin(), bytes.end()};
    appended.insert(appended.end(), bytes2.begin(), bytes2.end());

    helper::writeFile(filename, pluginId, datapath, bytes, storageEncryption);
    helper::appendFile(filename, pluginId, datapath, bytes2, storageEncryption);
    const auto actualReadData = helper::readFile(filename, pluginId, datapath, storageEncryption);
    EXPECT_EQ(actualReadData, appended)
        << "Expected: " << std::string(appended.begin(), appended.end()) << std::endl
        << "Actual:   " << std::string(actualReadData.begin(), actualReadData.end());
    fs::remove_all(datapath);
}

TEST(writeFile, lists_directories) {
    std::string datapath = "data";
    std::string pluginId = "testPlugin";
    std::string dir = "dir";
    fs::create_directory(datapath);
    fs::create_directory(fs::path(datapath) / pluginId);
    fs::create_directory(fs::path(datapath) / pluginId / dir);
    std::vector<std::string> contents = {"dir"};
    EXPECT_EQ(helper::listDir("", pluginId, datapath), contents);
    fs::remove_all(datapath);
}

TEST(writeFile, makes_directories) {
    std::string datapath = "data";
    std::string pluginId = "testPlugin";
    std::string dir = "dir/testdir";
    fs::create_directory(datapath);
    fs::create_directory(fs::path(datapath) / pluginId);
    EXPECT_EQ(helper::makeDir(dir, pluginId, datapath), true);
    std::vector<std::string> contents = {"dir"};
    EXPECT_EQ(helper::listDir("", pluginId, datapath), contents);
    std::vector<std::string> dirContents = {"testdir"};
    EXPECT_EQ(helper::listDir("dir", pluginId, datapath), dirContents);
    fs::remove_all(datapath);
}

TEST(writeFile, creates_directories) {
    StorageEncryption storageEncryption;
    storageEncryption.init(RaceEnums::StorageEncryptionType::ENC_AES, "myWeakPassphrase", "/tmp");

    std::string datapath = "data";
    std::string pluginId = "testPlugin";
    std::string filepath = "dir/test";
    std::string filepath2 = "dir/test2";
    fs::create_directory(datapath);
    fs::create_directory(fs::path(datapath) / pluginId);
    std::string msg = "Hello World";
    std::vector<unsigned char> bytes = {msg.begin(), msg.end()};

    helper::writeFile(filepath, pluginId, datapath, bytes, storageEncryption);
    helper::writeFile(filepath2, pluginId, datapath, bytes, storageEncryption);

    std::vector<std::string> contents = {"dir"};
    EXPECT_EQ(helper::listDir("", pluginId, datapath), contents);
    std::vector<std::string> dirContents = {"test", "test2"};
    auto actualDirContents = helper::listDir("dir", pluginId, datapath);
    std::sort(actualDirContents.begin(), actualDirContents.end());
    EXPECT_EQ(actualDirContents, dirContents);

    EXPECT_EQ(helper::readFile(filepath, pluginId, datapath, storageEncryption), bytes);
    EXPECT_EQ(helper::readFile(filepath2, pluginId, datapath, storageEncryption), bytes);
    fs::remove_all(datapath);
}

TEST(writeFile, remove_file) {
    StorageEncryption storageEncryption;
    storageEncryption.init(RaceEnums::StorageEncryptionType::ENC_AES, "myWeakPassphrase",
                           "/tmp/race/");

    std::string datapath = "data";
    std::string pluginId = "testPlugin";
    std::string filepath = "dir/test";
    fs::create_directory(datapath);
    fs::create_directory(fs::path(datapath) / pluginId);
    std::string msg = "Hello World";
    std::vector<unsigned char> bytes = {msg.begin(), msg.end()};

    helper::writeFile(filepath, pluginId, datapath, bytes, storageEncryption);

    EXPECT_EQ(helper::removeDir(filepath, pluginId, datapath), true);
    EXPECT_EQ(helper::readFile(filepath, pluginId, datapath, storageEncryption),
              std::vector<unsigned char>());
    fs::remove_all(datapath);
}

TEST(writeFile, remove_directory) {
    StorageEncryption storageEncryption;
    storageEncryption.init(RaceEnums::StorageEncryptionType::ENC_AES, "myWeakPassphrase",
                           "/tmp/race/");

    std::string datapath = "data";
    std::string pluginId = "testPlugin";
    std::string filepath = "dir/test";
    fs::create_directory(datapath);
    fs::create_directory(fs::path(datapath) / pluginId);
    std::string msg = "Hello World";
    std::vector<unsigned char> bytes = {msg.begin(), msg.end()};

    helper::writeFile(filepath, pluginId, datapath, bytes, storageEncryption);

    std::vector<std::string> contents = {"dir"};
    EXPECT_EQ(helper::listDir("", pluginId, datapath), contents);
    EXPECT_EQ(helper::removeDir("dir", pluginId, datapath), true);
    EXPECT_EQ(helper::readFile(filepath, pluginId, datapath, storageEncryption),
              std::vector<unsigned char>());
    EXPECT_EQ(helper::listDir("", pluginId, datapath), std::vector<std::string>());
    fs::remove_all(datapath);
}

/////////////////////////////////////////////////////////////////////////////////
// convertToHexString
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Fixture class for testing convertFromHexString
 *
 */
class ConvertToHexStringParamTest :
    public testing::TestWithParam<std::pair<size_t, std::string>> {};

TEST_P(ConvertToHexStringParamTest, converts_values) {
    const std::pair<size_t, std::string> testCaseValues = GetParam();
    const std::string result = helper::convertToHexString(testCaseValues.first);
    EXPECT_EQ(result, testCaseValues.second);
}

INSTANTIATE_TEST_SUITE_P(success_cases, ConvertToHexStringParamTest,
                         ::testing::Values(std::make_pair(0, "0"), std::make_pair(1, "1"),
                                           std::make_pair(15, "f"), std::make_pair(4095, "fff")));

class ConvertToHexStringWithPaddingParamTest : public ConvertToHexStringParamTest {};

TEST_P(ConvertToHexStringWithPaddingParamTest, converts_values) {
    const size_t paddingLength = 5;
    const std::pair<size_t, std::string> testCaseValues = GetParam();
    const std::string result = helper::convertToHexString(testCaseValues.first, paddingLength);
    EXPECT_EQ(result, testCaseValues.second);
}

INSTANTIATE_TEST_SUITE_P(success_cases, ConvertToHexStringWithPaddingParamTest,
                         ::testing::Values(std::make_pair(0, "00000"), std::make_pair(1, "00001"),
                                           std::make_pair(15, "0000f"),
                                           std::make_pair(4095, "00fff")));

////////////////////////////////////////////////////////////////////////////////
// convertFromHexString
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Fixture class for testing convertToHexString.
 *
 */
class ConvertFromHexStringParamTest :
    public testing::TestWithParam<std::pair<std::string, size_t>> {};

TEST_P(ConvertFromHexStringParamTest, converts_values) {
    const std::pair<std::string, size_t> testCaseValues = GetParam();
    const size_t result = helper::convertFromHexString(testCaseValues.first);
    EXPECT_EQ(result, testCaseValues.second);
}

INSTANTIATE_TEST_SUITE_P(success_cases, ConvertFromHexStringParamTest,
                         ::testing::Values(std::make_pair("0", 0), std::make_pair("1", 1),
                                           std::make_pair("f", 15), std::make_pair("fff", 4095),
                                           std::make_pair("000", 0), std::make_pair("001", 1),
                                           std::make_pair("00f", 15), std::make_pair("f00", 3840)));

INSTANTIATE_TEST_SUITE_P(failure_cases, ConvertFromHexStringParamTest,
                         ::testing::Values(std::make_pair("", 0), std::make_pair("g", 0),
                                           std::make_pair("-1", 0), std::make_pair("0-1", 0),
                                           std::make_pair("z", 0), std::make_pair("oops", 0),
                                           std::make_pair("-0", 0), std::make_pair("-f", 0),
                                           std::make_pair("@f", 0),
                                           std::make_pair("some message", 0)));

////////////////////////////////////////////////////////////////////////////////
// extractConfigTarGz
////////////////////////////////////////////////////////////////////////////////

std::string readFileContentsToString(const std::string &fileName) {
    std::ifstream file(fileName.c_str(), std::ifstream::in);
    std::stringstream fileBuffer;
    fileBuffer << file.rdbuf();
    return fileBuffer.str();
}

// Verify that the tar gz extraction function can extract a tar containing a single file.
TEST(extractConfigTarGz, can_extract_a_tar_gz_file) {
    // The file expected to be extracted from the .tar.gz.
    const std::string expectedExtractedFileName =
        std::string(TEST_INPUT_FILES_DIR) + "some-file-in-tar-gz.txt";

    // Try to delete the file in case it was not removed from a previous run. This should return
    // false, indicating that the file does not exist. If it returns true then log the issue.
    EXPECT_FALSE(fs::remove(expectedExtractedFileName))
        << "output file not properly cleaned up from previous test run";

    // TEST_INPUT_FILES_DIR in defined by cmake.
    const std::string inputFile = std::string(TEST_INPUT_FILES_DIR) + "tar-gz-for-testing.tar.gz";

    // Extract the .tar.gz.
    helper::extractConfigTarGz(inputFile, std::string(TEST_INPUT_FILES_DIR));

    // Verify that the file exists.
    ASSERT_TRUE(fs::exists(expectedExtractedFileName))
        << "extracted file \"" << expectedExtractedFileName << "\" does not exist";

    // Verify the file contents are as expected.
    const std::string expectedFileContents = "some sample text for testing tar gz extraction.";
    EXPECT_EQ(expectedFileContents, readFileContentsToString(expectedExtractedFileName));

    // Delete the extracted file to clean up for future tests.
    EXPECT_TRUE(fs::remove(expectedExtractedFileName));
}
