
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

#ifndef __SOURCE_RACE_TEST_APP_H__
#define __SOURCE_RACE_TEST_APP_H__

#include <IRaceSdkTestApp.h>
#include <opentracing/tracer.h>

#include <cstdint>
#include <nlohmann/json.hpp>

#include "RaceApp.h"
#include "RaceConfig.h"
#include "racetestapp/IRaceTestAppOutput.h"
#include "racetestapp/Message.h"
#include "racetestapp/RaceApp.h"

class RaceTestApp {
public:
    explicit RaceTestApp(IRaceTestAppOutput &_output, IRaceSdkTestApp &_sdk, RaceApp &_app,
                         std::shared_ptr<opentracing::Tracer> tracer);

    /**
     * @brief Parse a user input message and send the resulting message over the RACE network.
     * CORE.
     *
     * @param inputMessage The user input message to be parsed.
     */
    void parseAndSendMessage(const nlohmann::json &inputMessage);

    /**
     * @brief Send a message over the RACE network at a given rate for a set amount of time.
     * Expected to be called from a background thread.
     *
     * @param message The message to send periodically.
     */
    void sendPeriodically(std::vector<rta::Message> messages);

    /**
     * @brief Send a message over the RACE network.
     *
     * @param message The message to send.
     */
    void sendMessage(const rta::Message &message);

    /**
     * @brief Parse the user input message and open a network-manager-bypass receive connection.
     *
     * @param inputMessage The user input message to be parsed.
     */
    void parseAndOpenNMBypassRecvConnection(const nlohmann::json &inputMessage);

    /**
     * @brief Parse the user input message and prepare to bootstrap a new device.
     *
     * @param inputMessage The user input message to be parsed.
     */
    void parseAndPrepareToBootstrap(const nlohmann::json &inputMessage);

    /**
     * @brief Parse the user input message and add a VoA command
     *
     * @param inputMessage The VoA command to be parsed.
     */
    void parseAndProcessVoaAction(const nlohmann::json &voaCommand);

    /**
     * @brief Parse the given RPC command and execute it as described.
     *
     * @param rpcCommand The RPC command to be parsed.
     */
    void parseAndExecuteRpcAction(const nlohmann::json &rpcCommand);

    /**
     * @brief Process commands specific to racetestapp
     *
     * @param command The command to parse and process
     * @return True if the app should stop, else false if the app should keep running.
     */
    bool processRaceTestAppCommand(const std::string &command);

    nlohmann::json getSdkStatus();

private:
    // WARNING: RaceTestApp is uing the IRaceSdkTestApp interface which is pointing to an instance
    // of RaceSdk. RaceSdk can act as either a client or a server. At the current time, the server
    // interface does not provide any interesting interfaces needed by this application. HOWEVER,
    // this may change in the future. When necessary (if ever) it would probably be best to create
    // some wrapper interface that can handle both client and server interfaces. For now the client
    // interface is sufficent for everything this application needs to do for testing a race node,
    // be it client or server.
    /**
     * @brief The interface used to interact with the RACE SDK for test specific functionality
     *
     */
    IRaceSdkTestApp &sdkCore;

    /**
     * @brief The app which RaceTestApp exercises
     *
     */
    RaceApp &app;

    /**
     * @brief The interface used to send output to the client.
     *
     */
    IRaceTestAppOutput &output;

    /**
     * @brief The opentracing tracer used to do opentracing logging
     *
     */
    std::shared_ptr<opentracing::Tracer> tracer;
};

#endif
