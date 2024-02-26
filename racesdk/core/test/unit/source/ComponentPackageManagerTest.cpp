
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

#include "../../../source/decomposed-comms/ComponentPackageManager.h"
#include "../../common/LogExpect.h"
#include "../../common/MockComponentManagerInternal.h"
#include "gtest/gtest.h"

class TestableComponentPackageManager : public ComponentPackageManager {
public:
    using ComponentPackageManager::ComponentPackageManager;
    using ComponentPackageManager::fragments;
    using ComponentPackageManager::nextFragmentHandle;
};

class ComponentPackageManagerTestFixtureBase :
    public ::testing::TestWithParam<CMTypes::EncodingMode> {
public:
    ComponentPackageManagerTestFixtureBase() :
        test_info(testing::UnitTest::GetInstance()->current_test_info()),
        logger(test_info->test_suite_name(), test_info->name()),
        mockComponentManager(logger),
        packageManager(mockComponentManager) {}

    virtual void TearDown() override {
        logger.check();
    }

    CMTypes::PackageInfo *pushPackageOntoQueue(EncPkg &&pkg, CMTypes::PackageSdkHandle sdkHandle,
                                               CMTypes::EncodingHandle encodingHandle,
                                               CMTypes::Link *link = nullptr) {
        link = (link != nullptr) ? link : &mockComponentManager.mockLink;
        link->packageQueue.push_back(std::make_unique<CMTypes::PackageInfo>(
            CMTypes::PackageInfo{link, std::move(pkg), sdkHandle, encodingHandle, {}}));
        return link->packageQueue.back().get();
    }

    CMTypes::PackageFragmentInfo *createPackageFragment(
        CMTypes::PackageInfo *packageInfo, CMTypes::ActionInfo *actionInfo,
        CMTypes::PackageFragmentState state = CMTypes::PackageFragmentState::UNENCODED) {
        auto fragment = std::make_unique<CMTypes::PackageFragmentInfo>(
            CMTypes::PackageFragmentInfo{{packageManager.nextFragmentHandle++},
                                         packageInfo,
                                         state,
                                         actionInfo,
                                         0,
                                         packageInfo->pkg.getSize(),
                                         false});

        packageManager.fragments[fragment->handle] = fragment.get();

        if (actionInfo != nullptr) {
            actionInfo->fragments.push_back(fragment.get());
        }
        packageInfo->packageFragments.push_back(std::move(fragment));
        return packageInfo->packageFragments.back().get();
    }

public:
    const testing::TestInfo *test_info;
    LogExpect logger;
    MockComponentManagerInternal mockComponentManager;
    TestableComponentPackageManager packageManager;
};

class ComponentPackageManagerTestFixtureNonParameterized :
    public ComponentPackageManagerTestFixtureBase {
public:
    ComponentPackageManagerTestFixtureNonParameterized() {
        mockComponentManager.mode = GetParam();
    }
};

class ComponentPackageManagerTestFixture : public ComponentPackageManagerTestFixtureBase {
public:
    ComponentPackageManagerTestFixture() {
        mockComponentManager.mode = GetParam();
    }
};

TEST_P(ComponentPackageManagerTestFixtureNonParameterized, test_constructor) {
    LOG_EXPECT(this->logger, __func__, packageManager);
}

TEST_P(ComponentPackageManagerTestFixture, test_sendPackage_no_timeline) {
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    EXPECT_EQ(PLUGIN_TEMP_ERROR, packageManager.sendPackage({0}, 1.0, {7}, "mockConnectionId",
                                                            EncPkg(1, 2, {0x12, 0x34}), 0.0, 0));
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixture, test_sendPackage_no_available_actions) {
    double now = helper::currentTime();

    // In-progress action
    CMTypes::ActionInfo mockAction1;
    mockAction1.linkId = "mockLinkId";
    mockAction1.action.actionId = 3;
    mockAction1.action.timestamp = now + 100;
    mockAction1.encoding.push_back({{}, {1000}, {0}, CMTypes::EncodingState::ENCODING, nullptr});
    mockAction1.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction1);
    // To-be-removed action
    CMTypes::ActionInfo mockAction2;
    mockAction2.linkId = "mockLinkId";
    mockAction2.action.actionId = 4;
    mockAction2.action.timestamp = now + 100;
    mockAction2.encoding.push_back({{}, {1000}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction2.toBeRemoved = true;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction2);
    // action with no encodings
    CMTypes::ActionInfo mockAction3;
    mockAction3.linkId = "mockLinkId";
    mockAction3.action.actionId = 5;
    mockAction3.action.timestamp = now + 100;
    mockAction3.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction3);
    // encoding with different link id
    CMTypes::ActionInfo mockAction4;
    mockAction4.linkId = "mockLinkId2";
    mockAction4.action.actionId = 6;
    mockAction4.action.timestamp = now + 100;
    mockAction4.encoding.push_back({{"mockLinkId2", "*/*", true, ""},
                                    {1000},
                                    {0},
                                    CMTypes::EncodingState::UNENCODED,
                                    nullptr});
    mockAction4.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction4);

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    EXPECT_EQ(PLUGIN_TEMP_ERROR, packageManager.sendPackage({0}, now, {7}, "mockConnectionId",
                                                            EncPkg(1, 2, {0x12, 0x34}), 0.0, 0));
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixture, test_sendPackage_available_action) {
    double now = helper::currentTime();
    CMTypes::ActionInfo mockAction;
    mockAction.wildcardLink = true;
    mockAction.action.actionId = 3;
    mockAction.action.timestamp = now + 100;
    mockAction.encoding.push_back({{}, {1000}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction);

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    EXPECT_EQ(PLUGIN_OK, packageManager.sendPackage({0}, now, {7}, "mockConnectionId",
                                                    EncPkg(1, 2, {0x12, 0x34}), 0.0, 0));
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixture, test_sendPackage_available_action_explicit_link_id) {
    double now = helper::currentTime();
    CMTypes::ActionInfo mockAction;
    mockAction.linkId = "mockLinkId";
    mockAction.action.actionId = 3;
    mockAction.action.timestamp = now + 100;
    mockAction.encoding.push_back({{}, {1000}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction);

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    EXPECT_EQ(PLUGIN_OK, packageManager.sendPackage({0}, now, {7}, "mockConnectionId",
                                                    EncPkg(1, 2, {0x12, 0x34}), 0.0, 0));
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixture, test_sendPackage_available_action_existing_fragment) {
    double now = helper::currentTime();
    auto package = pushPackageOntoQueue(EncPkg(1, 2, {0x12, 0x34}), {6}, {3});

    CMTypes::ActionInfo mockAction;
    mockAction.linkId = "mockLinkId";
    mockAction.action.actionId = 3;
    mockAction.action.timestamp = now + 100;
    mockAction.encoding.push_back({{}, {1000}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    createPackageFragment(package, &mockAction, CMTypes::PackageFragmentState::UNENCODED);
    mockAction.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction);

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    auto resp = packageManager.sendPackage({0}, now, {7}, "mockConnectionId",
                                           EncPkg(1, 2, {0x12, 0x34}), 0.0, 0);
    LOG_EXPECT(this->logger, __func__, resp);
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixture,
       test_sendPackage_available_action_fragment_across_actions) {
    double now = helper::currentTime();
    CMTypes::ActionInfo mockAction1;
    mockAction1.linkId = "mockLinkId";
    mockAction1.action.actionId = 3;
    mockAction1.action.timestamp = now + 100;
    mockAction1.encoding.push_back({{}, {100}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction1.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction1);

    CMTypes::ActionInfo mockAction2;
    mockAction2.linkId = "mockLinkId";
    mockAction2.action.actionId = 4;
    mockAction2.action.timestamp = now + 100;
    mockAction2.encoding.push_back({{}, {100}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction2.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction2);

    CMTypes::ActionInfo mockAction3;
    mockAction3.linkId = "mockLinkId";
    mockAction3.action.actionId = 5;
    mockAction3.action.timestamp = now + 100;
    mockAction3.encoding.push_back({{}, {100}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction3.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction3);

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    auto resp = packageManager.sendPackage({0}, now, {7}, "mockConnectionId",
                                           EncPkg(std::vector<uint8_t>(220, 'a')), 0.0, 0);
    LOG_EXPECT(this->logger, __func__, resp);
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixture, test_encodeCoverTrafficForAction_no_encodings) {
    double now = helper::currentTime();
    CMTypes::ActionInfo mockAction;
    mockAction.action.actionId = 3;
    mockAction.action.timestamp = now + 100;
    mockAction.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction);

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    packageManager.encodeForAction(&mockAction);
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixture, test_encodeCoverTrafficForAction_no_packages) {
    double now = helper::currentTime();
    CMTypes::ActionInfo mockAction;
    mockAction.action.actionId = 3;
    mockAction.action.timestamp = now + 100;
    mockAction.encoding.push_back({{}, {1000}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction);

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    packageManager.encodeForAction(&mockAction);
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixture, test_encodeCoverTrafficForAction_single_packages) {
    double now = helper::currentTime();
    auto package = pushPackageOntoQueue(EncPkg(1, 2, {0x12, 0x34}), {6}, {3});

    CMTypes::ActionInfo mockAction;
    mockAction.action.actionId = 3;
    mockAction.action.timestamp = now + 100;
    mockAction.linkId = "mockLinkId";
    mockAction.encoding.push_back({{}, {1000}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    createPackageFragment(package, &mockAction, CMTypes::PackageFragmentState::UNENCODED);
    mockAction.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction);

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    packageManager.encodeForAction(&mockAction);
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixture, test_encodeCoverTrafficForAction_multiple_packages) {
    double now = helper::currentTime();
    auto package1 = pushPackageOntoQueue(EncPkg(1, 2, {0x12, 0x34}), {6}, {3});
    auto package2 = pushPackageOntoQueue(EncPkg(1, 2, {0x12, 0x34}), {7}, {3});

    CMTypes::ActionInfo mockAction;
    mockAction.action.actionId = 3;
    mockAction.action.timestamp = now + 100;
    mockAction.linkId = "mockLinkId";
    mockAction.encoding.push_back({{}, {1000}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    createPackageFragment(package1, &mockAction, CMTypes::PackageFragmentState::UNENCODED);
    createPackageFragment(package2, &mockAction, CMTypes::PackageFragmentState::UNENCODED);
    mockAction.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction);

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    if (mockComponentManager.mode == CMTypes::EncodingMode::SINGLE) {
        EXPECT_THROW(packageManager.encodeForAction(&mockAction), std::runtime_error);
    } else {
        packageManager.encodeForAction(&mockAction);
    }
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixture, test_encodeCoverTrafficForAction_fragmented_packages) {
    double now = helper::currentTime();
    auto package1 = pushPackageOntoQueue(EncPkg(1, 2, std::vector<uint8_t>(300, 'a')), {6}, {3});
    CMTypes::ActionInfo mockAction;
    mockAction.action.actionId = 3;
    mockAction.action.timestamp = now + 100;
    mockAction.linkId = "mockLinkId";
    mockAction.encoding.push_back({{}, {100}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    createPackageFragment(package1, &mockAction, CMTypes::PackageFragmentState::UNENCODED);
    mockAction.fragments.front()->len = 50;
    mockAction.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction);

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    packageManager.encodeForAction(&mockAction);
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixtureNonParameterized,
       test_onLinkStatusChanged_destroyed_link_resets_all_packages) {
    double now = helper::currentTime();
    auto package1 = pushPackageOntoQueue(EncPkg(1, 2, {0x12, 0x34}), {7}, {3});
    auto package2 = pushPackageOntoQueue(EncPkg(2, 3, {0x31, 0x41, 0x59}), {8}, {4});

    CMTypes::ActionInfo mockAction1;
    mockAction1.action.actionId = 42;
    mockAction1.action.timestamp = now + 100;
    mockAction1.encoding.push_back({{}, {1000}, {3}, CMTypes::EncodingState::ENQUEUED, nullptr});
    createPackageFragment(package1, &mockAction1, CMTypes::PackageFragmentState::ENQUEUED);
    mockAction1.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction1);

    CMTypes::ActionInfo mockAction2;
    mockAction2.action.actionId = 43;
    mockAction2.action.timestamp = now + 100;
    mockAction2.encoding.push_back({{}, {1000}, {4}, CMTypes::EncodingState::ENCODING, nullptr});
    createPackageFragment(package2, &mockAction2, CMTypes::PackageFragmentState::ENCODING);
    mockAction2.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction2);

    packageManager.pendingEncodings[{4}] = &mockAction2.encoding.front();

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);

    packageManager.onLinkStatusChanged({3}, {8}, mockComponentManager.mockLink.linkId,
                                       LINK_DESTROYED, {});

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixtureNonParameterized,
       test_onBytesEncoded_no_pending_encoding) {
    LOG_EXPECT(this->logger, __func__, packageManager);
    EXPECT_EQ(CMTypes::CmInternalStatus::OK,
              packageManager.onBytesEncoded({1}, {42}, {0x12, 0x34}, EncodingStatus::ENCODE_OK));
    LOG_EXPECT(this->logger, __func__, packageManager);
}

TEST_P(ComponentPackageManagerTestFixtureNonParameterized, test_onBytesEncoded_failed_encoding) {
    CMTypes::EncodingInfo encodingInfo{{}, {1000}, {42}, CMTypes::EncodingState::ENCODING, nullptr};
    packageManager.pendingEncodings[{42}] = &encodingInfo;
    LOG_EXPECT(this->logger, __func__, packageManager);
    EXPECT_EQ(CMTypes::CmInternalStatus::OK,
              packageManager.onBytesEncoded({1}, {42}, {}, EncodingStatus::ENCODE_FAILED));
    LOG_EXPECT(this->logger, __func__, packageManager);
}

TEST_P(ComponentPackageManagerTestFixtureNonParameterized,
       test_onBytesEncoded_successful_encoding) {
    double now = helper::currentTime();
    CMTypes::ActionInfo actionInfo;
    actionInfo.action.actionId = 3;
    actionInfo.action.timestamp = now + 100;
    CMTypes::EncodingInfo encodingInfo{
        {}, {1000}, {42}, CMTypes::EncodingState::ENCODING, &actionInfo};
    packageManager.pendingEncodings[{42}] = &encodingInfo;
    LOG_EXPECT(this->logger, __func__, packageManager);
    EXPECT_EQ(CMTypes::CmInternalStatus::OK,
              packageManager.onBytesEncoded({1}, {42}, {0x12, 0x34}, EncodingStatus::ENCODE_OK));
    LOG_EXPECT(this->logger, __func__, packageManager);
}

TEST_P(ComponentPackageManagerTestFixture, test_onPackageStatusChanged_no_package_found) {
    EXPECT_EQ(CMTypes::CmInternalStatus::OK,
              packageManager.onPackageStatusChanged({9}, {7}, PackageStatus::PACKAGE_SENT));
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixture, test_onPackageStatusChanged_valid_package) {
    auto package1 = pushPackageOntoQueue(EncPkg(1, 2, {0x12, 0x34}), {7}, {3});

    auto frag1 = createPackageFragment(package1, nullptr, CMTypes::PackageFragmentState::DONE);

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    EXPECT_EQ(CMTypes::CmInternalStatus::OK, packageManager.onPackageStatusChanged(
                                                 {9}, frag1->handle, PackageStatus::PACKAGE_SENT));
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixture, test_onPackageStatusChanged_package_fragment_not_done) {
    auto package1 = pushPackageOntoQueue(EncPkg(1, 2, {0x12, 0x34}), {7}, {3});

    auto frag1 = createPackageFragment(package1, nullptr, CMTypes::PackageFragmentState::DONE);
    auto frag2 = createPackageFragment(package1, nullptr, CMTypes::PackageFragmentState::ENQUEUED);
    frag1->len /= 2;
    frag2->offset = frag1->len;
    frag2->len = frag2->len - frag2->offset;

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    EXPECT_EQ(CMTypes::CmInternalStatus::OK, packageManager.onPackageStatusChanged(
                                                 {9}, frag1->handle, PackageStatus::PACKAGE_SENT));
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixture,
       test_onPackageStatusChanged_package_fragment_done_but_not_succeeded) {
    auto package1 = pushPackageOntoQueue(EncPkg(1, 2, {0x12, 0x34}), {7}, {3});

    auto frag1 = createPackageFragment(package1, nullptr, CMTypes::PackageFragmentState::DONE);
    auto frag2 = createPackageFragment(package1, nullptr, CMTypes::PackageFragmentState::DONE);
    frag1->len /= 2;
    frag2->offset = frag1->len;
    frag2->len = frag2->len - frag2->offset;

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    EXPECT_EQ(CMTypes::CmInternalStatus::OK,
              packageManager.onPackageStatusChanged({9}, package1->packageFragments.front()->handle,
                                                    PackageStatus::PACKAGE_SENT));
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixture,
       test_onPackageStatusChanged_package_fragment_done_but_not_all_created) {
    auto package1 = pushPackageOntoQueue(EncPkg(1, 2, {0x12, 0x34}), {7}, {3});

    auto frag1 = createPackageFragment(package1, nullptr, CMTypes::PackageFragmentState::DONE);
    frag1->len /= 2;

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    EXPECT_EQ(CMTypes::CmInternalStatus::OK,
              packageManager.onPackageStatusChanged({9}, package1->packageFragments.front()->handle,
                                                    PackageStatus::PACKAGE_SENT));
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixture,
       test_onPackageStatusChanged_package_fragment_all_succeeded) {
    auto package1 = pushPackageOntoQueue(EncPkg(1, 2, {0x12, 0x34}), {7}, {3});

    auto frag1 = createPackageFragment(package1, nullptr, CMTypes::PackageFragmentState::SENT);
    auto frag2 = createPackageFragment(package1, nullptr, CMTypes::PackageFragmentState::DONE);
    packageManager.fragments.erase(frag1->handle);
    frag1->len /= 2;
    frag2->offset = frag1->len;
    frag2->len = frag2->len - frag2->offset;

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    EXPECT_EQ(CMTypes::CmInternalStatus::OK, packageManager.onPackageStatusChanged(
                                                 {9}, frag2->handle, PackageStatus::PACKAGE_SENT));
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixture,
       test_onPackageStatusChanged_package_fragment_first_fail) {
    auto package1 = pushPackageOntoQueue(EncPkg(1, 2, {0x12, 0x34}), {7}, {3});

    auto frag1 = createPackageFragment(package1, nullptr, CMTypes::PackageFragmentState::DONE);
    auto frag2 = createPackageFragment(package1, nullptr, CMTypes::PackageFragmentState::DONE);
    frag1->len /= 2;
    frag2->offset = frag1->len;
    frag2->len = frag2->len - frag2->offset;

    // the fragments shuold be destroyed after the first call, so get the handles now
    auto fragHandle1 = frag1->handle;
    auto fragHandle2 = frag2->handle;

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    EXPECT_EQ(CMTypes::CmInternalStatus::OK,
              packageManager.onPackageStatusChanged({9}, fragHandle1,
                                                    PackageStatus::PACKAGE_FAILED_GENERIC));
    EXPECT_EQ(CMTypes::CmInternalStatus::OK,
              packageManager.onPackageStatusChanged({9}, fragHandle2, PackageStatus::PACKAGE_SENT));
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixture,
       test_onPackageStatusChanged_package_fragment_last_failed) {
    auto package1 = pushPackageOntoQueue(EncPkg(1, 2, {0x12, 0x34}), {7}, {3});

    auto frag1 = createPackageFragment(package1, nullptr, CMTypes::PackageFragmentState::SENT);
    auto frag2 = createPackageFragment(package1, nullptr, CMTypes::PackageFragmentState::DONE);
    frag1->len /= 2;
    frag2->offset = frag1->len;
    frag2->len = frag2->len - frag2->offset;
    packageManager.fragments.erase(frag1->handle);

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    EXPECT_EQ(CMTypes::CmInternalStatus::OK,
              packageManager.onPackageStatusChanged({9}, frag2->handle,
                                                    PackageStatus::PACKAGE_FAILED_GENERIC));
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixture,
       test_onPackageStatusChanged_package_fragment_failed_remove_future_fragments) {
    double now = helper::currentTime();
    auto package1 = pushPackageOntoQueue(EncPkg(1, 2, {0x12, 0x34}), {7}, {3});

    CMTypes::ActionInfo mockAction1;
    mockAction1.action.actionId = 42;
    mockAction1.action.timestamp = now + 100;
    mockAction1.encoding.push_back({{}, {1000}, {3}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction1.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction1);

    auto frag1 = createPackageFragment(package1, nullptr, CMTypes::PackageFragmentState::DONE);
    auto frag2 =
        createPackageFragment(package1, &mockAction1, CMTypes::PackageFragmentState::UNENCODED);
    frag1->len /= 2;
    frag2->offset = frag1->len;
    frag2->len = frag2->len - frag2->offset;

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    EXPECT_EQ(CMTypes::CmInternalStatus::OK,
              packageManager.onPackageStatusChanged({9}, frag1->handle,
                                                    PackageStatus::PACKAGE_FAILED_GENERIC));
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixture,
       test_onPackageStatusChanged_package_fragment_failed_remove_future_fragments_other_packages) {
    double now = helper::currentTime();
    auto package1 = pushPackageOntoQueue(EncPkg(1, 2, {0x12, 0x34}), {7}, {0});
    auto package2 = pushPackageOntoQueue(EncPkg(3, 4, {0x12, 0x34}), {8}, {0});

    CMTypes::ActionInfo mockAction1;
    mockAction1.action.actionId = 42;
    mockAction1.action.timestamp = now + 100;
    mockAction1.encoding.push_back({{}, {1000}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction1.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction1);

    CMTypes::ActionInfo mockAction2;
    mockAction2.action.actionId = 43;
    mockAction2.action.timestamp = now + 100;
    mockAction2.encoding.push_back({{}, {1000}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction2.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction2);

    auto frag1 = createPackageFragment(package1, nullptr, CMTypes::PackageFragmentState::DONE);
    auto frag2 =
        createPackageFragment(package1, &mockAction1, CMTypes::PackageFragmentState::UNENCODED);
    frag1->len /= 2;
    frag2->offset = frag1->len;
    frag2->len = frag2->len - frag2->offset;
    auto frag3 =
        createPackageFragment(package2, &mockAction1, CMTypes::PackageFragmentState::UNENCODED);
    auto frag4 =
        createPackageFragment(package2, &mockAction2, CMTypes::PackageFragmentState::UNENCODED);
    frag3->len /= 2;
    frag4->offset = frag3->len;
    frag4->len = frag4->len - frag4->offset;

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    EXPECT_EQ(CMTypes::CmInternalStatus::OK,
              packageManager.onPackageStatusChanged({9}, frag1->handle,
                                                    PackageStatus::PACKAGE_FAILED_GENERIC));
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(
    ComponentPackageManagerTestFixture,
    test_onPackageStatusChanged_package_fragment_failed_remove_future_fragments_wildcard_action) {
    double now = helper::currentTime();
    auto package1 = pushPackageOntoQueue(EncPkg(1, 2, {0x12, 0x34}), {7}, {0});
    auto package2 = pushPackageOntoQueue(EncPkg(3, 4, {0x12, 0x34}), {8}, {0});
    pushPackageOntoQueue(EncPkg(5, 6, {0x12, 0x34}), {9}, {0}, &mockComponentManager.mockLink2);

    CMTypes::ActionInfo mockAction1;
    mockAction1.action.actionId = 42;
    mockAction1.action.timestamp = now + 100;
    mockAction1.encoding.push_back(
        {{"mockLinkId", "*/*", true, ""}, {1000}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction1.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction1);

    CMTypes::ActionInfo mockAction2;
    mockAction2.wildcardLink = true;
    mockAction2.action.actionId = 43;
    mockAction2.action.timestamp = now + 100;
    mockAction2.encoding.push_back(
        {{"mockLinkId", "*/*", true, ""}, {1000}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction2.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction2);
    mockComponentManager.mockLink2.actionQueue.push_back(&mockAction2);

    auto frag1 = createPackageFragment(package1, nullptr, CMTypes::PackageFragmentState::DONE);
    auto frag2 =
        createPackageFragment(package1, &mockAction1, CMTypes::PackageFragmentState::UNENCODED);
    frag1->len /= 2;
    frag2->offset = frag1->len;
    frag2->len = frag2->len - frag2->offset;
    auto frag3 =
        createPackageFragment(package2, &mockAction1, CMTypes::PackageFragmentState::UNENCODED);
    auto frag4 =
        createPackageFragment(package2, &mockAction2, CMTypes::PackageFragmentState::UNENCODED);
    frag3->len /= 2;
    frag4->offset = frag3->len;
    frag4->len = frag4->len - frag4->offset;

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink2);
    EXPECT_EQ(CMTypes::CmInternalStatus::OK,
              packageManager.onPackageStatusChanged({9}, frag1->handle,
                                                    PackageStatus::PACKAGE_FAILED_GENERIC));
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink2);
}

TEST_P(ComponentPackageManagerTestFixture,
       test_updatedActionsForLink_new_actions_no_pending_packages) {
    double now = helper::currentTime();
    CMTypes::ActionInfo mockAction1;
    mockAction1.action.actionId = 3;
    mockAction1.action.timestamp = now + 100;
    mockAction1.encoding.push_back({{}, {1000}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction1.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction1);

    CMTypes::ActionInfo mockAction2;
    mockAction2.action.actionId = 4;
    mockAction2.action.timestamp = now + 100;
    mockAction2.encoding.push_back({{}, {1000}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction2.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction2);

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);

    packageManager.updatedActions();

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixture,
       test_updatedActionsForLink_new_actions_pending_packages) {
    double now = helper::currentTime();
    CMTypes::ActionInfo mockAction1;
    mockAction1.action.actionId = 3;
    mockAction1.action.timestamp = now + 100;
    mockAction1.encoding.push_back({{}, {1000}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction1.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction1);

    CMTypes::ActionInfo mockAction2;
    mockAction2.action.actionId = 4;
    mockAction2.action.timestamp = now + 100;
    mockAction2.encoding.push_back({{}, {1000}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction2.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction2);

    pushPackageOntoQueue(EncPkg(1, 2, {0x12, 0x34}), {7}, {0});
    pushPackageOntoQueue(EncPkg(2, 3, {0x31, 0x41, 0x59}), {8}, {0});

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);

    packageManager.updatedActions();

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixture,
       test_updatedActionsForLink_removed_actions_no_pending_packages) {
    double now = helper::currentTime();
    CMTypes::ActionInfo mockAction1;
    mockAction1.action.actionId = 3;
    mockAction1.action.timestamp = now + 100;
    mockAction1.encoding.push_back({{}, {1000}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction1.toBeRemoved = true;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction1);

    CMTypes::ActionInfo mockAction2;
    mockAction2.action.actionId = 4;
    mockAction2.action.timestamp = now + 100;
    mockAction2.encoding.push_back({{}, {1000}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction2.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction2);

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);

    packageManager.updatedActions();

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixture,
       test_updatedActionsForLink_removed_actions_refragments_after_removed) {
    double now = helper::currentTime();
    auto package1 = pushPackageOntoQueue(EncPkg(1, 2, std::vector<uint8_t>(100, 'a')), {7}, {3});
    auto package2 = pushPackageOntoQueue(EncPkg(3, 4, std::vector<uint8_t>(100, 'b')), {8}, {4});
    auto package3 = pushPackageOntoQueue(EncPkg(5, 6, std::vector<uint8_t>(100, 'c')), {9}, {5});
    auto package4 = pushPackageOntoQueue(EncPkg(7, 8, std::vector<uint8_t>(100, 'd')), {10}, {6});
    auto package5 = pushPackageOntoQueue(EncPkg(9, 10, std::vector<uint8_t>(100, 'e')), {11}, {7});

    CMTypes::ActionInfo mockAction1;
    mockAction1.action.actionId = 42;
    mockAction1.action.timestamp = now + 100;
    mockAction1.encoding.push_back({{}, {1000}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction1.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction1);

    CMTypes::ActionInfo mockAction2;
    mockAction2.action.actionId = 43;
    mockAction2.action.timestamp = now + 100;
    mockAction2.encoding.push_back({{}, {1000}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction2.toBeRemoved = true;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction2);

    CMTypes::ActionInfo mockAction3;
    mockAction3.action.actionId = 44;
    mockAction3.action.timestamp = now + 100;
    mockAction3.encoding.push_back({{}, {1000}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction3.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction3);

    createPackageFragment(package1, &mockAction1);
    auto frag2 = createPackageFragment(package2, &mockAction1);
    auto frag3 = createPackageFragment(package2, &mockAction2);
    createPackageFragment(package3, &mockAction2);
    auto frag5 = createPackageFragment(package4, &mockAction2);
    auto frag6 = createPackageFragment(package4, &mockAction3);
    createPackageFragment(package5, &mockAction3);

    frag2->len /= 2;
    frag3->offset = frag2->len;
    frag3->len = frag3->len - frag3->offset;

    frag5->len /= 2;
    frag6->offset = frag5->len;
    frag6->len = frag6->len - frag6->offset;

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);

    packageManager.updatedActions();

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixtureNonParameterized, test_actionDone) {
    double now = helper::currentTime();
    auto package1 = pushPackageOntoQueue(EncPkg(1, 2, std::vector<uint8_t>(100, 'a')), {7}, {3});
    auto package2 = pushPackageOntoQueue(EncPkg(3, 4, std::vector<uint8_t>(100, 'b')), {8}, {4});
    auto package3 = pushPackageOntoQueue(EncPkg(5, 6, std::vector<uint8_t>(100, 'c')), {9}, {5});

    CMTypes::ActionInfo mockAction1;
    mockAction1.action.actionId = 42;
    mockAction1.action.timestamp = now + 100;
    mockAction1.encoding.push_back({{}, {1000}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction1.toBeRemoved = false;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction1);

    CMTypes::ActionInfo mockAction2;
    mockAction2.action.actionId = 43;
    mockAction2.action.timestamp = now + 100;
    mockAction2.encoding.push_back({{}, {1000}, {0}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction2.toBeRemoved = true;
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction2);

    createPackageFragment(package1, &mockAction1, CMTypes::PackageFragmentState::ENQUEUED);
    auto frag1 =
        createPackageFragment(package2, &mockAction1, CMTypes::PackageFragmentState::ENQUEUED);
    auto frag2 =
        createPackageFragment(package2, &mockAction2, CMTypes::PackageFragmentState::ENQUEUED);
    frag1->len /= 2;
    frag2->offset = frag1->len;
    frag2->len = frag2->len - frag2->offset;
    createPackageFragment(package3, &mockAction2, CMTypes::PackageFragmentState::ENQUEUED);

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);

    packageManager.actionDone(&mockAction1);

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

TEST_P(ComponentPackageManagerTestFixtureNonParameterized, test_getPackageHandlesForAction) {
    double now = helper::currentTime();
    CMTypes::PackageInfo mockPackage1{nullptr, EncPkg(1, 2, {0x12, 0x34}), {5}, {2}, {}};
    CMTypes::PackageInfo mockPackage2{nullptr, EncPkg(1, 2, {0x12, 0x34}), {7}, {3}, {}};

    CMTypes::ActionInfo mockAction;
    mockAction.action.actionId = 42;
    mockAction.action.timestamp = now + 100;
    createPackageFragment(&mockPackage1, &mockAction);
    createPackageFragment(&mockPackage2, &mockAction);

    std::vector<CMTypes::PackageFragmentHandle> expected{{0}, {1}};
    EXPECT_EQ(expected, packageManager.getPackageHandlesForAction(&mockAction));
}

TEST_P(ComponentPackageManagerTestFixtureNonParameterized, test_teardown) {
    double now = helper::currentTime();
    CMTypes::PackageInfo mockPackage1{
        &mockComponentManager.mockLink, EncPkg(1, 2, {0x12, 0x34}), {5}, {2}, {}};
    CMTypes::PackageInfo mockPackage2{
        &mockComponentManager.mockLink, EncPkg(2, 3, {0x31, 0x41, 0x59}), {7}, {3}, {}};

    CMTypes::ActionInfo mockAction;
    mockAction.action.actionId = 42;
    mockAction.action.timestamp = now + 100;
    mockAction.encoding.push_back({{}, {1000}, {3}, CMTypes::EncodingState::UNENCODED, nullptr});
    mockAction.toBeRemoved = true;
    createPackageFragment(&mockPackage1, &mockAction);
    createPackageFragment(&mockPackage2, &mockAction);
    mockComponentManager.mockLink.actionQueue.push_back(&mockAction);

    CMTypes::EncodingInfo encodingInfo{{}, {1000}, {3}, CMTypes::EncodingState::ENCODING, nullptr};
    packageManager.pendingEncodings[{3}] = &encodingInfo;

    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
    packageManager.teardown();
    LOG_EXPECT(this->logger, __func__, packageManager);
    LOG_EXPECT(this->logger, __func__, mockComponentManager.mockLink);
}

INSTANTIATE_TEST_SUITE_P(ComponentPackageManagerTestSuite,
                         ComponentPackageManagerTestFixtureNonParameterized,
                         testing::Values(CMTypes::EncodingMode::BATCH),
                         ::testing::PrintToStringParamName());

INSTANTIATE_TEST_SUITE_P(ComponentPackageManagerTestSuite, ComponentPackageManagerTestFixture,
                         testing::Values(CMTypes::EncodingMode::SINGLE,
                                         CMTypes::EncodingMode::BATCH,
                                         CMTypes::EncodingMode::FRAGMENT_SINGLE_PRODUCER,
                                         CMTypes::EncodingMode::FRAGMENT_MULTIPLE_PRODUCER),
                         ::testing::PrintToStringParamName());