
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

#include "racetestapp/RaceTestApp.h"

#include <OpenTracingHelpers.h>  // traceIdFromContext, spanIdFromContext, createTracer

#include <nlohmann/json.hpp>
#include <sstream>  // std::stringstream
#include <thread>   // std::thread

#include "racetestapp/Message.h"             // Message
#include "racetestapp/raceTestAppHelpers.h"  // rtah::logError, rtah::makeClrMsg

RaceTestApp::RaceTestApp(IRaceTestAppOutput &_output, IRaceSdkTestApp &_sdk, RaceApp &_app,
                         std::shared_ptr<opentracing::Tracer> _tracer) :
    sdkCore(_sdk), app(_app), output(_output), tracer(_tracer) {}

bool RaceTestApp::processRaceTestAppCommand(const std::string &command) {
    try {
        nlohmann::json commandJson = nlohmann::json::parse(command);
        std::string type = commandJson.at("type");
        if (type == "stop") {
            return true;
        } else if (type == "send-message") {
            parseAndSendMessage(commandJson);
        } else if (type == "open-network-manager-bypass-receive-connection") {
            parseAndOpenNMBypassRecvConnection(commandJson);
        } else if (type == "prepare-to-bootstrap") {
            parseAndPrepareToBootstrap(commandJson);
        } else if (type == "voa-action") {
            parseAndProcessVoaAction(commandJson);
        } else if (type == "rpc") {
            parseAndExecuteRpcAction(commandJson);
        } else {
            output.writeOutput("unknown command: " + type);
        }
    } catch (nlohmann::json::parse_error &err) {
        output.writeOutput("Error parsing command: " + std::string(err.what()));
    }

    return false;
}

void RaceTestApp::sendMessage(const rta::Message &message) {
    using namespace std::chrono;
    std::shared_ptr<opentracing::Span> span = tracer->StartSpan("sendMessage");

    auto start = steady_clock::now();
    std::string messageContent = message.messageContent;
    messageContent.append(message.generated.data(), message.generated.size());
    ClrMsg msg =
        rtah::makeClrMsg(messageContent, sdkCore.getActivePersona(), message.personaOfRecipient);

    auto middle = steady_clock::now();
    output.writeOutput("Creating the clear message took " +
                       std::to_string(duration_cast<milliseconds>(middle - start).count()) + " ms");

    span->SetTag("source", "racetestapp");
    span->SetTag("file", __FILE__);
    span->SetTag("messageSize", std::to_string(msg.getMsg().size()));
    span->SetTag("messageHash", rtah::getMessageSignature(msg));
    span->SetTag("messageFrom", msg.getFrom());
    span->SetTag("messageTo", msg.getTo());
    span->SetTag("messageTestId", rtah::testIdFromClrMsg(msg));

    msg.setTraceId(traceIdFromContext(span->context()));
    msg.setSpanId(spanIdFromContext(span->context()));

    try {
        output.writeOutput("sending message...");
        rtah::outputMessage(output, msg);
        if (message.isNMBypass) {
            sdkCore.sendNMBypassMessage(msg, message.networkManagerBypassRoute);
        } else {
            app.addMessageToUI(msg);
            auto handle = sdkCore.sendClientMessage(msg);
            output.writeOutput("message sent to SDK CORE with handle: " + std::to_string(handle));
            if (not sdkCore.isConnected()) {
                output.writeOutput(
                    "The client was not ready to send yet (expecting"
                    " onPluginStatusChanged(PLUGIN_READY) call from network manager), so the send "
                    "may not be "
                    "successful.");
            }
        }
    } catch (...) {
#ifndef __ANDROID__
        std::exception_ptr exception = std::current_exception();
        const std::string errorMessage =
            "Exception thrown while sending a message: " +
            std::string(exception ? exception.__cxa_exception_type()->name() :
                                    "exception was null");
#else
        const std::string errorMessage = "Exception thrown while sending a message: ";
#endif
        rtah::logError(errorMessage);
        output.writeOutput(errorMessage);
    }
    auto end = steady_clock::now();
    output.writeOutput("Sending the clear message took " +
                       std::to_string(duration_cast<milliseconds>(end - middle).count()) + " ms");
}

void RaceTestApp::parseAndSendMessage(const nlohmann::json &inputCommand) {
    try {
        const auto messages = rta::Message::createMessage(inputCommand);
        std::thread t1(&RaceTestApp::sendPeriodically, this, messages);
        t1.detach();
    } catch (std::invalid_argument &e) {
        output.writeOutput("ERROR: message: " + inputCommand.dump() + " what: " + e.what());
    }
}

void RaceTestApp::sendPeriodically(std::vector<rta::Message> messages) {
    std::stringstream threadId;
    threadId << std::this_thread::get_id();
    output.writeOutput("sendPeriodically called on thread: " + threadId.str());

    for (auto &message : messages) {
        if (message.messageContent.size() < rta::Message::sequenceStringLength) {
            output.writeOutput(
                "Warning: Message too short for sequence number. Resizing and continuing. "
                "message: " +
                message.messageContent);
            message.messageContent.resize(rta::Message::sequenceStringLength);
        }

        std::this_thread::sleep_until(message.sendTime);
        sendMessage(message);
    }

    output.writeOutput("sendPeriodically returned on thread: " + threadId.str());
}

void RaceTestApp::parseAndOpenNMBypassRecvConnection(const nlohmann::json &inputMessage) {
    try {
        nlohmann::json payload = inputMessage.at("payload");

        std::string persona = payload.at("persona");
        std::string route = payload.at("route");

        sdkCore.openNMBypassReceiveConnection(persona, route);
    } catch (nlohmann::json::exception &err) {
        output.writeOutput(
            "ERROR: invalid open-network-manager-bypass-receive-connection command: " +
            std::string(err.what()) + ", json: " + inputMessage.dump());
    }
}

void RaceTestApp::parseAndPrepareToBootstrap(const nlohmann::json &inputMessage) {
    try {
        nlohmann::json payload = inputMessage.at("payload");

        DeviceInfo deviceInfo;
        payload.at("platform").get_to(deviceInfo.platform);
        payload.at("architecture").get_to(deviceInfo.architecture);
        payload.at("nodeType").get_to(deviceInfo.nodeType);

        std::string passphrase = payload.at("passphrase");
        std::string bootstrapChannelId;
        if (payload.contains("bootstrapChannelId")) {
            bootstrapChannelId = payload.at("bootstrapChannelId");
        }

        sdkCore.prepareToBootstrap(deviceInfo, passphrase, bootstrapChannelId);
    } catch (nlohmann::json::exception &err) {
        output.writeOutput("ERROR: invalid prepare-to-bootstrap command: " +
                           std::string(err.what()) + ", json: " + inputMessage.dump());
    }
}

void RaceTestApp::parseAndProcessVoaAction(const nlohmann::json &voaCommand) {
    try {
        const nlohmann::json &payload = voaCommand.at("payload");
        const std::string &action = payload.at("action");
        const nlohmann::json &config = payload.at("config");

        if (action == "add-rules") {
            sdkCore.addVoaRules(config);
        } else if (action == "delete-rules") {
            sdkCore.deleteVoaRules(config);
        } else if (action == "set-active-state") {
            bool state = config.at("state");
            sdkCore.setVoaActiveState(state);
        } else {
            output.writeOutput("ERROR: unknown voa action " + std::string(action));
        }
    } catch (nlohmann::json::exception &err) {
        output.writeOutput("ERROR: invalid voa-action command: " + std::string(err.what()) +
                           ", json: " + voaCommand.dump());
    }
}

void RaceTestApp::parseAndExecuteRpcAction(const nlohmann::json &rpcCommand) {
    try {
        const nlohmann::json &payload = rpcCommand.at("payload");
        const std::string &action = payload.at("action");

        if (action == "enable-channel") {
            sdkCore.enableChannel(payload.at("channelGid"));
        } else if (action == "disable-channel") {
            sdkCore.disableChannel(payload.at("channelGid"));
        } else if (action == "deactivate-channel") {
            sdkCore.rpcDeactivateChannel(payload.at("channelGid"));
        } else if (action == "destroy-link") {
            sdkCore.rpcDestroyLink(payload.at("linkId"));
        } else if (action == "close-connection") {
            sdkCore.rpcCloseConnection(payload.at("connectionId"));
        } else if (action == "notify-epoch") {
            sdkCore.rpcNotifyEpoch(payload.at("data"));
        } else {
            output.writeOutput("ERROR: unknown RPC action " + action);
        }
    } catch (nlohmann::json::exception &err) {
        output.writeOutput("ERROR: invalid Comms RPC command: " + std::string(err.what()) +
                           ", json: " + rpcCommand.dump());
    }
}

nlohmann::json RaceTestApp::getSdkStatus() {
    return app.getSdkStatus();
}
