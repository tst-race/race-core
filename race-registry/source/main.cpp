
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

#include <OpenTracingHelpers.h>  // createTracer
#include <RaceLog.h>
#include <RaceSdk.h>
#include <limits.h>
#include <unistd.h>  // sleep

#include <chrono>
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>

#include "RaceRegistry.h"
#include "RaceRegistryApp.h"
#include "createPidFile.h"
#include "input/RaceTestAppInputFifo.h"
#include "output/NodeDaemonPublisher.h"
#include "racetestapp/RaceTestApp.h"
#include "racetestapp/RaceTestAppOutputLog.h"
#include "racetestapp/raceTestAppHelpers.h"

std::int32_t main() {
    RaceLog::setLogLevelFile(RaceLog::LL_DEBUG);

    // Initialize output first since createPidFile() could print to stdout/err
    RaceTestAppOutputLog output("/log/");
    RaceTestAppInputFifo input;

    if (rta::createPidFile() == -1) {
        return -1;
    }

    output.writeOutput("racetestapp starting...");

    std::string errorMessage;
    bool validConfigs = false;
    try {
        AppConfig config;
        config.persona = rtah::getPersona();
        config.etcDirectory = "/etc/race";
        // Config Files
        config.configTarPath = "/tmp/configs.tar.gz";
        config.baseConfigPath = "/data/configs";
        // Testing specific files (user-responses.json, jaeger-config.json, voa.json)
        config.jaegerConfigPath = config.etcDirectory + "/jaeger-config.yml";
        config.userResponsesFilePath = config.etcDirectory + "/user-responses.json";
        const std::string encryptionTypeEnvVarName = "RACE_ENCRYPTION_TYPE";
        const std::string encryptionType = rtah::getEnvironmentVariable(encryptionTypeEnvVarName);
        if (encryptionType == "ENC_AES") {
            config.encryptionType = RaceEnums::ENC_AES;
        } else if (encryptionType == "ENC_NONE") {
            config.encryptionType = RaceEnums::ENC_NONE;
        } else {
            rtah::logWarning("failed to read valid encryption type from environment variable " +
                             encryptionTypeEnvVarName + ". Read value \"" + encryptionType +
                             "\". Using default encryption type: " +
                             RaceEnums::storageEncryptionTypeToString(config.encryptionType));
        }

        std::string passphrase;
        {
            UserInputResponseParser parser(config.userResponsesFilePath);
            auto response = parser.getResponse("sdk", "passphrase");
            passphrase = response.response;
        }
        RaceSdk raceSdk(config, passphrase);
        validConfigs = true;
        std::shared_ptr<opentracing::Tracer> tracer =
            createTracer(config.jaegerConfigPath, raceSdk.getActivePersona());
        NodeDaemonPublisher publisher;

        RaceRegistry registry(raceSdk, tracer);
        RaceRegistryApp raceApp(output, raceSdk, tracer, registry);

        if (!raceSdk.initRaceSystem(&raceApp)) {
            throw std::logic_error("initRaceSystem failed");
        }

        RaceTestApp app(output, raceSdk, raceApp, tracer);

        output.writeOutput("raceregistry started. Running raceregistry...");

        std::atomic<bool> isRunning = true;
        std::thread([&isRunning, &publisher, &raceApp] {
            while (isRunning) {
                // Publish status every 3 seconds, with expiration of 9 seconds (3*3)
                int period = 3;
                int ttl_factor = 3;
                nlohmann::json raceStatus = raceApp.getSdkStatus();
                raceStatus["validConfigs"] = true;
                publisher.publishStatus(raceStatus, period * ttl_factor);
                std::this_thread::sleep_for(std::chrono::seconds(period));
            }
        }).detach();

        while (isRunning) {
            const std::string inputCommand = input.getInputBlocking();

            output.writeOutput("Received input:\n" + inputCommand);

            // Stop if the function returns true
            isRunning = !app.processRaceTestAppCommand(inputCommand);
        };

        output.writeOutput("raceregistry shutting down...");
        return 0;
    } catch (const std::exception &error) {
        errorMessage = "Exception thrown: TYPE: " + std::string(typeid(error).name()) +
                       " WHAT: " + std::string(error.what());
    } catch (...) {
        std::exception_ptr currentException = std::current_exception();
        errorMessage =
            "an unknown error occurred: " +
            std::string(currentException ? currentException.__cxa_exception_type()->name() :
                                           "null");
    }
    if (!validConfigs) {
        NodeDaemonPublisher publisher;
        nlohmann::json raceStatus = {};
        // Node will need to Down then Up in order to fix status
        raceStatus["validConfigs"] = false;
        publisher.publishStatus(raceStatus, INT_MAX);
    }

    output.writeOutput(errorMessage);
    rtah::logError(errorMessage);

    return 1;
}
