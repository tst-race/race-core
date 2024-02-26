
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

#include <RaceLog.h>

#include "IRacePluginComms.h"
#include "IRacePluginNM.h"
#include "IRaceSdkComms.h"
#include "IRaceSdkNM.h"
#include "gtest/gtest.h"

IRacePluginNM *createPluginNM(IRaceSdkNM *) {
    EXPECT_TRUE(false) << "createPluginNM should not be called from the unit tests";
    return nullptr;
}

void destroyPluginNM(IRacePluginNM *) {
    EXPECT_TRUE(false) << "destroyPluginNM should not be called from the unit tests";
}

IRacePluginComms *createPluginComms(IRaceSdkComms *) {
    EXPECT_TRUE(false) << "createPluginComms should not be called from the unit tests";
    return nullptr;
}

void destroyPluginComms(IRacePluginComms *) {
    EXPECT_TRUE(false) << "destroyPluginComms should not be called from the unit tests";
}

int main(int argc, char **argv) {
    RaceLog::setLogLevel(RaceLog::LL_DEBUG);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
