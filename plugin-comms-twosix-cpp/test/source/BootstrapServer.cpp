
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

// clang-format off
#include "gtest/gtest.h"    // If this file isn't included first then you'll get some weird build errors
// clang-format on

#include "../../source/bootstrap/BootstrapServer.h"
#include "../../source/filesystem.h"

#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

static std::string exec(std::string cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

class BootstrapServerTestFixture : public ::testing::Test {
public:
    BootstrapServerTestFixture() {
        std::chrono::duration<double> sinceEpoch =
            std::chrono::system_clock::now().time_since_epoch();
        testDir = "/tmp/bootstrapServerTest/" + std::to_string(sinceEpoch.count()) + "/";
        fs::create_directories(testDir);
    }
    virtual ~BootstrapServerTestFixture() {}
    virtual void SetUp() override {}
    virtual void TearDown() override {}

    std::string getTestFilePath() {
        static int counter = 0;
        return testDir + "/test_file_" + std::to_string(counter++);
    }

    std::string createTestFile(const std::string &data) {
        std::string path = getTestFilePath();
        std::ofstream file(path);
        file << data;
        return path;
    }

    std::string createTestDirectory(std::vector<std::string> contents) {
        static int counter = 0;
        std::string dir = testDir + "/test_dir_" + std::to_string(counter++);
        fs::create_directories(dir);

        int file_counter = 0;
        for (auto &content : contents) {
            std::string filepath = dir + "/test_file_" + std::to_string(file_counter++);
            std::ofstream file(filepath);
            file << content;
        }
        return dir;
    }

public:
    std::string testDir;
};

TEST_F(BootstrapServerTestFixture, bootstrap_server_missing_passphrase) {
    std::string passphrase = "pass1";
    std::string data = "Hello, World!";
    std::string path = createTestFile(data);

    BootstrapServer server(testDir);
    server.serveFile(passphrase, path);

    std::string output = exec("wget -qO- 127.0.0.1:2626/");

    EXPECT_EQ(output.empty(), true);
}

TEST_F(BootstrapServerTestFixture, bootstrap_server_incorrect_passphrase) {
    std::string passphrase = "pass1";
    std::string data = "Hello, World!";
    std::string path = createTestFile(data);

    BootstrapServer server(testDir);
    server.serveFile(passphrase, path);

    std::string output = exec("wget -qO- 127.0.0.1:2626/foobar");

    EXPECT_EQ(output.empty(), true);
}

TEST_F(BootstrapServerTestFixture, bootstrap_server_missing_file) {
    std::string passphrase = "pass1";
    std::string path = getTestFilePath();

    BootstrapServer server(testDir);
    server.serveFile(passphrase, path);

    std::string output = exec("wget -qO- 127.0.0.1:2626/" + passphrase);

    EXPECT_EQ(output.empty(), true);
}

TEST_F(BootstrapServerTestFixture, bootstrap_server_file_is_directory) {
    std::string passphrase = "pass1";
    std::string path = createTestDirectory({});

    BootstrapServer server(testDir);
    server.serveFile(passphrase, path);

    std::string output = exec("wget -qO- 127.0.0.1:2626/" + passphrase);

    EXPECT_EQ(output.empty(), true);
}

TEST_F(BootstrapServerTestFixture, bootstrap_server_serves_file) {
    std::string passphrase = "pass1";
    std::string data = "Hello, World!";
    std::string path = createTestFile(data);

    BootstrapServer server(testDir);
    server.serveFile(passphrase, path);

    std::string output = exec("wget -qO- 127.0.0.1:2626/" + passphrase);

    EXPECT_EQ(data, output);
}

TEST_F(BootstrapServerTestFixture, bootstrap_server_serves_multiple_files) {
    std::string passphrase1 = "pass1";
    std::string data1 = "Hello, World!";
    std::string path1 = createTestFile(data1);

    std::string passphrase2 = "pass2";
    std::string data2 = "Hello, Other World!";
    std::string path2 = createTestFile(data2);

    BootstrapServer server(testDir);
    server.serveFile(passphrase1, path1);
    server.serveFile(passphrase2, path2);

    std::string output1 = exec("wget -qO- 127.0.0.1:2626/" + passphrase1);
    std::string output2 = exec("wget -qO- 127.0.0.1:2626/" + passphrase2);
    EXPECT_EQ(data1, output1);
    EXPECT_EQ(data2, output2);
}

TEST_F(BootstrapServerTestFixture, bootstrap_server_stops_serving) {
    std::string passphrase = "pass1";
    std::string data = "Hello, World!";
    std::string path = createTestFile(data);

    BootstrapServer server(testDir);
    server.serveFile(passphrase, path);
    server.stopServing(passphrase);

    std::string output = exec("wget -qO- 127.0.0.1:2626/" + passphrase);
    EXPECT_EQ(output.empty(), true);
}

TEST_F(BootstrapServerTestFixture, bootstrap_server_stops_serving_some_but_not_others) {
    std::string passphrase1 = "pass1";
    std::string data1 = "Hello, World!";
    std::string path1 = createTestFile(data1);

    std::string passphrase2 = "pass2";
    std::string data2 = "Hello, Other World!";
    std::string path2 = createTestFile(data2);

    BootstrapServer server(testDir);
    server.serveFile(passphrase1, path1);
    server.serveFile(passphrase2, path2);

    server.stopServing(passphrase1);

    std::string output1 = exec("wget -qO- 127.0.0.1:2626/" + passphrase1);
    std::string output2 = exec("wget -qO- 127.0.0.1:2626/" + passphrase2);
    EXPECT_EQ(output1.empty(), true);
    EXPECT_EQ(data2, output2);
}

TEST_F(BootstrapServerTestFixture, bootstrap_server_serves_directory) {
    std::string passphrase = "pass1";
    std::string data = "Hello, World!";
    std::string path = createTestDirectory({data});
    std::string destpath = createTestDirectory({});

    BootstrapServer server(testDir);
    server.serveFiles(passphrase, path);

    exec("wget -O " + destpath + ".tar 127.0.0.1:2626/" + passphrase);
    exec("tar xvf " + destpath + ".tar -C " + destpath);

    std::string output = exec("cat " + destpath + "/test_file_0");
    EXPECT_EQ(data, output);
}
