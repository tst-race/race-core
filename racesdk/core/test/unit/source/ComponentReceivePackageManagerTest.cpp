
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

#include "../../../source/decomposed-comms/ComponentReceivePackageManager.h"
#include "../../common/LogExpect.h"
#include "../../common/MockComponentManagerInternal.h"
#include "gtest/gtest.h"

using namespace CMTypes;

class TestableComponentReceivePackageManager : public ComponentReceivePackageManager {
public:
    using ComponentReceivePackageManager::ComponentReceivePackageManager;
    using ComponentReceivePackageManager::pendingDecodings;
};

class ComponentReceivePackageManagerTestFixture : public ::testing::Test {
public:
    ComponentReceivePackageManagerTestFixture() :
        test_info(testing::UnitTest::GetInstance()->current_test_info()),
        logger(test_info->test_suite_name(), test_info->name()),
        mockComponentManager(logger),
        receiveManager(mockComponentManager) {}

    virtual void TearDown() override {
        logger.check();
    }

    std::vector<uint8_t> createHeader(std::vector<uint8_t> producerId, uint32_t fragmentId,
                                      uint8_t flags) {
        auto bytes = producerId;
        uint8_t *fragmentIdPtr = reinterpret_cast<uint8_t *>(&fragmentId);
        bytes.insert(bytes.end(), fragmentIdPtr, fragmentIdPtr + sizeof(fragmentId));
        bytes.push_back(flags);
        return bytes;
    }

    std::vector<uint8_t> createFragment(uint32_t len, std::vector<uint8_t> contents) {
        std::vector<uint8_t> bytes;
        uint8_t *lenPtr = reinterpret_cast<uint8_t *>(&len);
        bytes.insert(bytes.end(), lenPtr, lenPtr + sizeof(len));
        bytes.insert(bytes.end(), contents.begin(), contents.end());
        return bytes;
    }

    std::vector<uint8_t> append(std::vector<std::vector<uint8_t>> vecs) {
        std::vector<uint8_t> bytes;
        for (auto &vec : vecs) {
            bytes.insert(bytes.end(), vec.begin(), vec.end());
        }
        return bytes;
    }

public:
    const testing::TestInfo *test_info;
    LogExpect logger;
    MockComponentManagerInternal mockComponentManager;
    TestableComponentReceivePackageManager receiveManager;
};

TEST_F(ComponentReceivePackageManagerTestFixture, test_constructor) {
    LOG_EXPECT(this->logger, __func__, receiveManager);
}

TEST_F(ComponentReceivePackageManagerTestFixture, test_on_receive) {
    mockComponentManager.mode = EncodingMode::SINGLE;
    LOG_EXPECT(this->logger, __func__, receiveManager);
    EncodingParameters params = {"mockLinkId", "text/plain", {}, {}};
    receiveManager.onReceive({1}, "mockLinkId", params,
                             {0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41});
    LOG_EXPECT(this->logger, __func__, receiveManager);
}

TEST_F(ComponentReceivePackageManagerTestFixture, test_on_receive_batch) {
    mockComponentManager.mode = EncodingMode::BATCH;
    LOG_EXPECT(this->logger, __func__, receiveManager);
    EncodingParameters params = {"mockLinkId", "text/plain", {}, {}};
    receiveManager.onReceive({1}, "mockLinkId", params,
                             {0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41});
    LOG_EXPECT(this->logger, __func__, receiveManager);
}

TEST_F(ComponentReceivePackageManagerTestFixture, test_onBytesDecoded) {
    mockComponentManager.mode = EncodingMode::SINGLE;
    mockComponentManager.mockLink.connections = {"connection1", "connection2"};
    LOG_EXPECT(this->logger, __func__, receiveManager);
    receiveManager.pendingDecodings[{1}] = "mockLinkId";
    receiveManager.onBytesDecoded({2}, {1},
                                  {0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41,
                                   0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51},
                                  EncodingStatus::ENCODE_OK);
    LOG_EXPECT(this->logger, __func__, receiveManager);
}

TEST_F(ComponentReceivePackageManagerTestFixture, test_onBytesDecoded_batch) {
    mockComponentManager.mode = EncodingMode::BATCH;
    mockComponentManager.mockLink.connections = {"connection1", "connection2"};
    auto bytes = createFragment(20, {0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41,
                                     0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51});
    LOG_EXPECT(this->logger, __func__, receiveManager);
    receiveManager.pendingDecodings[{1}] = "mockLinkId";
    receiveManager.onBytesDecoded({2}, {1}, std::move(bytes), EncodingStatus::ENCODE_OK);
    LOG_EXPECT(this->logger, __func__, receiveManager);
}

TEST_F(ComponentReceivePackageManagerTestFixture, test_onBytesDecoded_batch_multiple_packages) {
    mockComponentManager.mode = EncodingMode::BATCH;
    mockComponentManager.mockLink.connections = {"connection1", "connection2"};
    auto bytes =
        append({createFragment(20, {0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41,
                                    0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51}),
                createFragment(
                    24, {0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41, 0x42, 0x43,
                         0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55})});
    LOG_EXPECT(this->logger, __func__, receiveManager);
    receiveManager.pendingDecodings[{1}] = "mockLinkId";
    receiveManager.onBytesDecoded({2}, {1}, std::move(bytes), EncodingStatus::ENCODE_OK);
    LOG_EXPECT(this->logger, __func__, receiveManager);
}

TEST_F(ComponentReceivePackageManagerTestFixture, test_onBytesDecoded_fragment_single_producer) {
    mockComponentManager.mode = EncodingMode::FRAGMENT_SINGLE_PRODUCER;
    mockComponentManager.mockLink.connections = {"connection1", "connection2"};
    auto bytes =
        append({createHeader({}, 0, 0),
                createFragment(20, {0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41,
                                    0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51})});

    LOG_EXPECT(this->logger, __func__, receiveManager);
    receiveManager.pendingDecodings[{1}] = "mockLinkId";
    receiveManager.onBytesDecoded({2}, {1}, std::move(bytes), EncodingStatus::ENCODE_OK);
    LOG_EXPECT(this->logger, __func__, receiveManager);
}

TEST_F(ComponentReceivePackageManagerTestFixture,
       test_onBytesDecoded_fragment_single_producer_multiple_packages) {
    mockComponentManager.mode = EncodingMode::FRAGMENT_SINGLE_PRODUCER;
    mockComponentManager.mockLink.connections = {"connection1", "connection2"};
    auto bytes =
        append({createHeader({}, 0, 0),
                createFragment(20, {0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41,
                                    0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51}),
                createFragment(
                    24, {0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41, 0x42, 0x43,
                         0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55})});

    LOG_EXPECT(this->logger, __func__, receiveManager);
    receiveManager.pendingDecodings[{1}] = "mockLinkId";
    receiveManager.onBytesDecoded({2}, {1}, std::move(bytes), EncodingStatus::ENCODE_OK);
    LOG_EXPECT(this->logger, __func__, receiveManager);
}

TEST_F(ComponentReceivePackageManagerTestFixture,
       test_onBytesDecoded_fragment_single_producer_two_fragments) {
    mockComponentManager.mode = EncodingMode::FRAGMENT_SINGLE_PRODUCER;
    mockComponentManager.mockLink.connections = {"connection1", "connection2"};
    auto bytes1 =
        append({createHeader({}, 1, CONTINUE_NEXT_PACKAGE),
                createFragment(10, {0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41})});

    auto bytes2 =
        append({createHeader({}, 2, CONTINUE_LAST_PACKAGE),
                createFragment(10, {0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51})});

    LOG_EXPECT(this->logger, __func__, receiveManager);
    receiveManager.pendingDecodings[{1}] = "mockLinkId";
    receiveManager.onBytesDecoded({12}, {1}, std::move(bytes1), EncodingStatus::ENCODE_OK);
    receiveManager.pendingDecodings[{2}] = "mockLinkId";
    receiveManager.onBytesDecoded({13}, {2}, std::move(bytes2), EncodingStatus::ENCODE_OK);
    LOG_EXPECT(this->logger, __func__, receiveManager);
}

TEST_F(ComponentReceivePackageManagerTestFixture,
       test_onBytesDecoded_fragment_single_producer_three_fragments) {
    mockComponentManager.mode = EncodingMode::FRAGMENT_SINGLE_PRODUCER;
    mockComponentManager.mockLink.connections = {"connection1", "connection2"};

    auto bytes1 =
        append({createHeader({}, 1, CONTINUE_NEXT_PACKAGE),
                createFragment(10, {0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41})});

    auto bytes2 =
        append({createHeader({}, 2, CONTINUE_NEXT_PACKAGE | CONTINUE_LAST_PACKAGE),
                createFragment(20, {0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51,
                                    0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51})});

    auto bytes3 =
        append({createHeader({}, 3, CONTINUE_LAST_PACKAGE),
                createFragment(10, {0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51})});

    LOG_EXPECT(this->logger, __func__, receiveManager);
    receiveManager.pendingDecodings[{1}] = "mockLinkId";
    receiveManager.onBytesDecoded({12}, {1}, std::move(bytes1), EncodingStatus::ENCODE_OK);
    receiveManager.pendingDecodings[{2}] = "mockLinkId";
    receiveManager.onBytesDecoded({13}, {2}, std::move(bytes2), EncodingStatus::ENCODE_OK);
    receiveManager.pendingDecodings[{3}] = "mockLinkId";
    receiveManager.onBytesDecoded({14}, {3}, std::move(bytes3), EncodingStatus::ENCODE_OK);
    LOG_EXPECT(this->logger, __func__, receiveManager);
}

TEST_F(ComponentReceivePackageManagerTestFixture,
       test_onBytesDecoded_fragment_single_producer_missing_fragment) {
    mockComponentManager.mode = EncodingMode::FRAGMENT_SINGLE_PRODUCER;
    mockComponentManager.mockLink.connections = {"connection1", "connection2"};

    auto bytes1 =
        append({createHeader({}, 1, CONTINUE_NEXT_PACKAGE),
                createFragment(10, {0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41})});
    auto bytes3 =
        append({createHeader({}, 3, CONTINUE_LAST_PACKAGE),
                createFragment(10, {0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51})});

    LOG_EXPECT(this->logger, __func__, receiveManager);
    receiveManager.pendingDecodings[{1}] = "mockLinkId";
    receiveManager.onBytesDecoded({12}, {1}, std::move(bytes1), EncodingStatus::ENCODE_OK);
    receiveManager.pendingDecodings[{2}] = "mockLinkId";
    receiveManager.onBytesDecoded({13}, {2}, std::move(bytes3), EncodingStatus::ENCODE_OK);
    LOG_EXPECT(this->logger, __func__, receiveManager);
}

TEST_F(ComponentReceivePackageManagerTestFixture,
       test_onBytesDecoded_fragment_single_producer_bad_continue) {
    mockComponentManager.mode = EncodingMode::FRAGMENT_SINGLE_PRODUCER;
    mockComponentManager.mockLink.connections = {"connection1", "connection2"};

    auto bytes1 =
        append({createHeader({}, 1, CONTINUE_NEXT_PACKAGE),
                createFragment(10, {0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51})});

    auto bytes2 =
        append({createHeader({}, 2, 0),
                createFragment(20, {0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51,
                                    0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51})});

    LOG_EXPECT(this->logger, __func__, receiveManager);
    receiveManager.pendingDecodings[{1}] = "mockLinkId";
    receiveManager.onBytesDecoded({12}, {1}, std::move(bytes1), EncodingStatus::ENCODE_OK);
    receiveManager.pendingDecodings[{2}] = "mockLinkId";
    receiveManager.onBytesDecoded({13}, {2}, std::move(bytes2), EncodingStatus::ENCODE_OK);
    LOG_EXPECT(this->logger, __func__, receiveManager);
}

TEST_F(ComponentReceivePackageManagerTestFixture,
       test_onBytesDecoded_fragment_single_producer_missing_fragment_multiple_fragments) {
    mockComponentManager.mode = EncodingMode::FRAGMENT_SINGLE_PRODUCER;
    mockComponentManager.mockLink.connections = {"connection1", "connection2"};

    auto bytes3 =
        append({createHeader({}, 3, CONTINUE_LAST_PACKAGE),
                createFragment(10, {0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51}),
                createFragment(20, {0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51,
                                    0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51})});

    LOG_EXPECT(this->logger, __func__, receiveManager);
    receiveManager.pendingDecodings[{2}] = "mockLinkId";
    receiveManager.onBytesDecoded({13}, {2}, std::move(bytes3), EncodingStatus::ENCODE_OK);
    LOG_EXPECT(this->logger, __func__, receiveManager);
}

TEST_F(ComponentReceivePackageManagerTestFixture, test_onBytesDecoded_fragment_multiple_producer) {
    mockComponentManager.mode = EncodingMode::FRAGMENT_MULTIPLE_PRODUCER;
    mockComponentManager.mockLink.connections = {"connection1", "connection2"};
    std::vector<uint8_t> producerId = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    auto bytes =
        append({createHeader(producerId, 0, 0),
                createFragment(20, {0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41,
                                    0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51})});

    LOG_EXPECT(this->logger, __func__, receiveManager);
    receiveManager.pendingDecodings[{1}] = "mockLinkId";
    receiveManager.onBytesDecoded({2}, {1}, std::move(bytes), EncodingStatus::ENCODE_OK);
    LOG_EXPECT(this->logger, __func__, receiveManager);
}

TEST_F(ComponentReceivePackageManagerTestFixture,
       test_onBytesDecoded_fragment_multiple_producer_multiple_packages) {
    mockComponentManager.mode = EncodingMode::FRAGMENT_MULTIPLE_PRODUCER;
    mockComponentManager.mockLink.connections = {"connection1", "connection2"};
    std::vector<uint8_t> producerId = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    auto bytes =
        append({createHeader(producerId, 0, 0),
                createFragment(20, {0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41,
                                    0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51}),
                createFragment(
                    24, {0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41, 0x42, 0x43,
                         0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55})});

    LOG_EXPECT(this->logger, __func__, receiveManager);
    receiveManager.pendingDecodings[{1}] = "mockLinkId";
    receiveManager.onBytesDecoded({2}, {1}, std::move(bytes), EncodingStatus::ENCODE_OK);
    LOG_EXPECT(this->logger, __func__, receiveManager);
}

TEST_F(ComponentReceivePackageManagerTestFixture,
       test_onBytesDecoded_fragment_multiple_producer_two_fragments) {
    mockComponentManager.mode = EncodingMode::FRAGMENT_MULTIPLE_PRODUCER;
    mockComponentManager.mockLink.connections = {"connection1", "connection2"};
    std::vector<uint8_t> producerId = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    auto bytes1 =
        append({createHeader(producerId, 1, CONTINUE_NEXT_PACKAGE),
                createFragment(10, {0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41})});

    auto bytes2 =
        append({createHeader(producerId, 2, CONTINUE_LAST_PACKAGE),
                createFragment(10, {0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51})});

    LOG_EXPECT(this->logger, __func__, receiveManager);
    receiveManager.pendingDecodings[{1}] = "mockLinkId";
    receiveManager.onBytesDecoded({12}, {1}, std::move(bytes1), EncodingStatus::ENCODE_OK);
    receiveManager.pendingDecodings[{2}] = "mockLinkId";
    receiveManager.onBytesDecoded({13}, {2}, std::move(bytes2), EncodingStatus::ENCODE_OK);
    LOG_EXPECT(this->logger, __func__, receiveManager);
}

TEST_F(ComponentReceivePackageManagerTestFixture,
       test_onBytesDecoded_fragment_multiple_producer_three_fragments) {
    mockComponentManager.mode = EncodingMode::FRAGMENT_MULTIPLE_PRODUCER;
    mockComponentManager.mockLink.connections = {"connection1", "connection2"};
    std::vector<uint8_t> producerId = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    auto bytes1 =
        append({createHeader(producerId, 1, CONTINUE_NEXT_PACKAGE),
                createFragment(10, {0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41})});

    auto bytes2 =
        append({createHeader(producerId, 2, CONTINUE_NEXT_PACKAGE | CONTINUE_LAST_PACKAGE),
                createFragment(20, {0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51,
                                    0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51})});

    auto bytes3 =
        append({createHeader(producerId, 3, CONTINUE_LAST_PACKAGE),
                createFragment(10, {0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51})});

    LOG_EXPECT(this->logger, __func__, receiveManager);
    receiveManager.pendingDecodings[{1}] = "mockLinkId";
    receiveManager.onBytesDecoded({12}, {1}, std::move(bytes1), EncodingStatus::ENCODE_OK);
    receiveManager.pendingDecodings[{2}] = "mockLinkId";
    receiveManager.onBytesDecoded({13}, {2}, std::move(bytes2), EncodingStatus::ENCODE_OK);
    receiveManager.pendingDecodings[{3}] = "mockLinkId";
    receiveManager.onBytesDecoded({14}, {3}, std::move(bytes3), EncodingStatus::ENCODE_OK);
    LOG_EXPECT(this->logger, __func__, receiveManager);
}

TEST_F(ComponentReceivePackageManagerTestFixture,
       test_onBytesDecoded_fragment_multiple_producer_multiple_producers) {
    mockComponentManager.mode = EncodingMode::FRAGMENT_MULTIPLE_PRODUCER;
    mockComponentManager.mockLink.connections = {"connection1", "connection2"};
    std::vector<uint8_t> producerId = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    std::vector<uint8_t> producerId2 = {16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

    auto bytes1 = append(
        {createHeader(producerId, 1, CONTINUE_NEXT_PACKAGE),
         createFragment(11, {0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41, 0x42})});

    auto bytes2 = append({createHeader(producerId2, 1, CONTINUE_NEXT_PACKAGE),
                          createFragment(12, {0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40,
                                              0x41, 0x42, 0x43})});

    auto bytes3 = append({createHeader(producerId, 2, CONTINUE_LAST_PACKAGE),
                          createFragment(13, {0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50,
                                              0x51, 0x52, 0x53, 0x54})});

    auto bytes4 = append({createHeader(producerId2, 2, CONTINUE_LAST_PACKAGE),
                          createFragment(14, {0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50,
                                              0x51, 0x52, 0x53, 0x54, 0x55})});

    LOG_EXPECT(this->logger, __func__, receiveManager);
    receiveManager.pendingDecodings[{1}] = "mockLinkId";
    receiveManager.onBytesDecoded({12}, {1}, std::move(bytes1), EncodingStatus::ENCODE_OK);
    receiveManager.pendingDecodings[{2}] = "mockLinkId";
    receiveManager.onBytesDecoded({13}, {2}, std::move(bytes2), EncodingStatus::ENCODE_OK);
    receiveManager.pendingDecodings[{3}] = "mockLinkId";
    receiveManager.onBytesDecoded({14}, {3}, std::move(bytes3), EncodingStatus::ENCODE_OK);
    receiveManager.pendingDecodings[{4}] = "mockLinkId";
    receiveManager.onBytesDecoded({15}, {4}, std::move(bytes4), EncodingStatus::ENCODE_OK);
    LOG_EXPECT(this->logger, __func__, receiveManager);
}

TEST_F(ComponentReceivePackageManagerTestFixture,
       test_onBytesDecoded_fragment_multiple_producer_missing_fragment) {
    mockComponentManager.mode = EncodingMode::FRAGMENT_MULTIPLE_PRODUCER;
    mockComponentManager.mockLink.connections = {"connection1", "connection2"};
    std::vector<uint8_t> producerId = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    auto bytes1 =
        append({createHeader(producerId, 1, CONTINUE_NEXT_PACKAGE),
                createFragment(10, {0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41})});
    auto bytes3 =
        append({createHeader(producerId, 3, CONTINUE_LAST_PACKAGE),
                createFragment(10, {0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51})});

    LOG_EXPECT(this->logger, __func__, receiveManager);
    receiveManager.pendingDecodings[{1}] = "mockLinkId";
    receiveManager.onBytesDecoded({12}, {1}, std::move(bytes1), EncodingStatus::ENCODE_OK);
    receiveManager.pendingDecodings[{2}] = "mockLinkId";
    receiveManager.onBytesDecoded({13}, {2}, std::move(bytes3), EncodingStatus::ENCODE_OK);
    LOG_EXPECT(this->logger, __func__, receiveManager);
}

TEST_F(ComponentReceivePackageManagerTestFixture,
       test_onBytesDecoded_fragment_multiple_producer_missing_fragment_multiple_fragments) {
    mockComponentManager.mode = EncodingMode::FRAGMENT_MULTIPLE_PRODUCER;
    mockComponentManager.mockLink.connections = {"connection1", "connection2"};
    std::vector<uint8_t> producerId = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    auto bytes3 =
        append({createHeader(producerId, 3, CONTINUE_LAST_PACKAGE),
                createFragment(10, {0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51}),
                createFragment(20, {0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51,
                                    0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51})});

    LOG_EXPECT(this->logger, __func__, receiveManager);
    receiveManager.pendingDecodings[{1}] = "mockLinkId";
    receiveManager.onBytesDecoded({13}, {1}, std::move(bytes3), EncodingStatus::ENCODE_OK);
    LOG_EXPECT(this->logger, __func__, receiveManager);
}

TEST_F(ComponentReceivePackageManagerTestFixture,
       test_onBytesDecoded_fragment_multiple_producer_bad_continue) {
    mockComponentManager.mode = EncodingMode::FRAGMENT_MULTIPLE_PRODUCER;
    mockComponentManager.mockLink.connections = {"connection1", "connection2"};
    std::vector<uint8_t> producerId = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    auto bytes1 =
        append({createHeader(producerId, 1, CONTINUE_NEXT_PACKAGE),
                createFragment(10, {0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51})});

    auto bytes2 =
        append({createHeader(producerId, 2, 0),
                createFragment(20, {0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51,
                                    0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51})});

    LOG_EXPECT(this->logger, __func__, receiveManager);
    receiveManager.pendingDecodings[{1}] = "mockLinkId";
    receiveManager.onBytesDecoded({12}, {1}, std::move(bytes1), EncodingStatus::ENCODE_OK);
    receiveManager.pendingDecodings[{2}] = "mockLinkId";
    receiveManager.onBytesDecoded({13}, {2}, std::move(bytes2), EncodingStatus::ENCODE_OK);
    LOG_EXPECT(this->logger, __func__, receiveManager);
}