
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

#include "ConfigPersonas.h"

#include <fstream>

#include "filesystem.h"
#include "gtest/gtest.h"
#include "race/mocks/MockRaceSdkNM.h"

using ::testing::Return;

const std::vector<uint8_t> aes1Bytes{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};
const std::vector<uint8_t> aes2Bytes{
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F};
const std::vector<uint8_t> aes3Bytes{
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F};
const std::vector<uint8_t> aes4Bytes{
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F};

TEST(ConfigPersonas, init) {
    std::string personasStr = R"([
    {
        "displayName": "RACE Client 1",
        "personaType": "client",
        "raceUuid": "race-client-1",
        "aesKeyFile": "race-client-1.aes"
    },
    {
        "displayName": "RACE Client 2",
        "personaType": "client",
        "raceUuid": "race-client-2",
        "aesKeyFile": "race-client-2.aes"
    },
    {
        "displayName": "RACE Server 1",
        "personaType": "server",
        "raceUuid": "race-server-1",
        "aesKeyFile": "race-server-1.aes"
    },
    {
        "displayName": "RACE Server 2",
        "personaType": "server",
        "raceUuid": "race-server-2",
        "aesKeyFile": "race-server-2.aes"
    }
]
)";
    MockRaceSdkNM sdk;
    ConfigPersonas configLoader;

    EXPECT_CALL(sdk, readFile("personas/race-personas.json"))
        .WillOnce(Return(std::vector<uint8_t>{personasStr.begin(), personasStr.end()}));
    EXPECT_CALL(sdk, readFile("personas/race-client-1.aes")).WillOnce(Return(aes1Bytes));
    EXPECT_CALL(sdk, readFile("personas/race-client-2.aes")).WillOnce(Return(aes2Bytes));
    EXPECT_CALL(sdk, readFile("personas/race-server-1.aes")).WillOnce(Return(aes3Bytes));
    EXPECT_CALL(sdk, readFile("personas/race-server-2.aes")).WillOnce(Return(aes4Bytes));
    ASSERT_TRUE(configLoader.init(sdk, "personas"));

    ASSERT_EQ(configLoader.numPersonas(), 4);

    {
        Persona persona = configLoader.getPersona(0);
        EXPECT_EQ(persona.getDisplayName(), "RACE Client 1");
        EXPECT_EQ(persona.getPersonaType(), P_CLIENT);
        EXPECT_EQ(persona.getRaceUuid(), "race-client-1");
    }

    {
        Persona persona = configLoader.getPersona(1);
        EXPECT_EQ(persona.getDisplayName(), "RACE Client 2");
        EXPECT_EQ(persona.getPersonaType(), P_CLIENT);
        EXPECT_EQ(persona.getRaceUuid(), "race-client-2");
    }

    {
        Persona persona = configLoader.getPersona(2);
        EXPECT_EQ(persona.getDisplayName(), "RACE Server 1");
        EXPECT_EQ(persona.getPersonaType(), P_SERVER);
        EXPECT_EQ(persona.getRaceUuid(), "race-server-1");
    }

    {
        Persona persona = configLoader.getPersona(3);
        EXPECT_EQ(persona.getDisplayName(), "RACE Server 2");
        EXPECT_EQ(persona.getPersonaType(), P_SERVER);
        EXPECT_EQ(persona.getRaceUuid(), "race-server-2");
    }
}

TEST(ConfigPersonas, init_missing) {
    MockRaceSdkNM sdk;
    ConfigPersonas configLoader;

    EXPECT_CALL(sdk, readFile("personas/race-personas.json"))
        .WillOnce(Return(std::vector<uint8_t>{}));
    EXPECT_FALSE(configLoader.init(sdk, "personas"));
}

TEST(ConfigPersonas, write) {
    MockRaceSdkNM sdk;
    ConfigPersonas writeConfig;

    std::string personasStr = R"([
    {
        "aesKeyFile": "race-client-00001.aes",
        "displayName": "RACE Client 1",
        "personaType": "client",
        "raceUuid": "race-client-00001"
    },
    {
        "aesKeyFile": "race-server-00001.aes",
        "displayName": "RACE Server 1",
        "personaType": "server",
        "raceUuid": "race-server-00001"
    }
])";

    std::vector<uint8_t> bytes{personasStr.begin(), personasStr.end()};

    {
        Persona persona;
        persona.setRaceUuid("race-client-00001");
        persona.setDisplayName("RACE Client 1");
        persona.setPersonaType(P_CLIENT);
        persona.setAesKeyFile("race-client-00001.aes");
        writeConfig.addPersona(persona);
    }

    {
        Persona persona;
        persona.setRaceUuid("race-server-00001");
        persona.setDisplayName("RACE Server 1");
        persona.setPersonaType(P_SERVER);
        persona.setAesKeyFile("race-server-00001.aes");
        writeConfig.addPersona(persona);
    }

    ASSERT_EQ(writeConfig.numPersonas(), 2);

    auto testDir = "personas";
    EXPECT_CALL(sdk, writeFile("personas/race-personas.json", bytes)).WillOnce(Return(SDK_OK));
    EXPECT_TRUE(writeConfig.write(sdk, testDir));
}
