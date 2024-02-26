
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

#include "VoaConfig.h"

#include "helper.h"

const std::string VoaConfig::VOA_CONF_ACTION = "action";
const std::string VoaConfig::VOA_CONF_PERSONA = "persona";
const std::string VoaConfig::VOA_CONF_TAG = "tag";
const std::string VoaConfig::VOA_CONF_STARTUP_DELAY = "startupdelay";
const std::string VoaConfig::VOA_CONF_TO = "to";
const std::string VoaConfig::VOA_CONF_PARAMS = "params";
const std::string VoaConfig::VOA_CONF_TRIGGER = "trigger";
const std::string VoaConfig::VOA_CONF_WINDOW = "window";

const std::string VoaConfig::VOA_PARAMS_JITTER = "jitter";
const std::string VoaConfig::VOA_PARAMS_HOLDTIME = "holdtime";
const std::string VoaConfig::VOA_PARAMS_REPLAYTIMES = "replaytimes";
const std::string VoaConfig::VOA_PARAMS_ITERATIONS = "iterations";

const std::string VoaConfig::VOA_TARGET_TYPE = "type";
const std::string VoaConfig::VOA_TARGET_MATCHID = "matchid";
const std::string VoaConfig::VOA_TARGET_TYPE_PERSONA = "persona";
const std::string VoaConfig::VOA_TARGET_TYPE_LINK = "link";
const std::string VoaConfig::VOA_TARGET_TYPE_CHANNEL = "channel";
const std::string VoaConfig::VOA_TARGET_MATCHID_ALL = "all";

const std::string VoaConfig::VOA_TRIGGER_PROB = "prob";
const std::string VoaConfig::VOA_TRIGGER_SKIPN = "skipN";

const std::string VoaConfig::VOA_WINDOW_COUNT = "count";
const std::string VoaConfig::VOA_WINDOW_DURATION = "duration";

const std::string VoaConfig::VOA_STATE_COUNT = "count_state";
const std::string VoaConfig::VOA_STATE_DURATION = "duration_state";
const std::string VoaConfig::VOA_STATE_SKIPN = "skipN_state";

const std::string VoaConfig::VOA_ACTION_DROP = "drop";
const std::string VoaConfig::VOA_ACTION_DELAY = "delay";
const std::string VoaConfig::VOA_ACTION_TAMPER = "tamper";
const std::string VoaConfig::VOA_ACTION_REPLAY = "replay";

VoaConfig::VoaConfig(const std::string &voaConfigPath) :
    JsonConfig(voaConfigPath), mRnd(std::random_device{}()) {
    helper::logDebug("VoaConfig::Constructor called");

    try {
        // Create a list of VoA rules from the Json config list
        auto configItems = configJson.items();
        std::transform(
            configItems.begin(), configItems.end(), std::back_inserter(voaRules),
            [](auto configItem) { return VoaRule(configItem.key(), configItem.value()); });
    } catch (std::exception &error) {
        helper::logError("VoaConfig: failed to parse VoA configuration: " +
                         std::string(error.what()));
        voaRules = {};
    }

    std::chrono::duration<double> now = std::chrono::system_clock::now().time_since_epoch();
    startupTimestamp = now.count();

    helper::logDebug("VoaConfig::Constructor returned");
}

bool VoaConfig::addRules(const nlohmann::json &payload) {
    std::lock_guard<std::mutex> lock{voa_config_mutex};

    try {
        // Create the VoA rules from the Json config list
        auto configItems = payload.items();
        std::transform(
            configItems.begin(), configItems.end(), std::back_inserter(voaRules),
            [](auto configItem) { return VoaRule(configItem.key(), configItem.value()); });
    } catch (std::exception &error) {
        helper::logError("VoaConfig: failed to parse VoA command configuration: " +
                         std::string(error.what()));
        return false;
    }
    return true;
}

bool VoaConfig::deleteRules(const nlohmann::json &payload) {
    std::lock_guard<std::mutex> lock{voa_config_mutex};

    try {
        std::list<std::string> rule_ids = payload.at("rule_ids");

        // Remove all rules if the list of rule Ids is empty
        if (rule_ids.empty()) {
            voaRules.clear();
            return true;
        }

        // Remove rules that are present in the list of ruleids
        auto removeIt = std::remove_if(voaRules.begin(), voaRules.end(), [&](auto r) -> bool {
            return (std::find(rule_ids.begin(), rule_ids.end(), r.ruleId) != rule_ids.end());
        });

        if (removeIt == voaRules.end()) {
            return false;
        }

        voaRules.erase(removeIt, voaRules.end());

    } catch (std::exception &error) {
        helper::logError("VoaConfig: failed to parse VoA command configuration: " +
                         std::string(error.what()));
        return false;
    }

    return true;
}

std::vector<VoaConfig::VoaRule> VoaConfig::findTargetedRules(
    std::string activePersona, LinkID linkId, std::string channelGid,
    std::vector<std::string> &personaList) {
    std::lock_guard<std::mutex> lock{voa_config_mutex};

    std::vector<VoaConfig::VoaRule> matchedRule;

    // Simply do a sequential search for now. This search should be optimized in
    // the future.
    for (auto voaRule : voaRules) {
        if (voaRule.matches(activePersona, linkId, channelGid, personaList)) {
            matchedRule.push_back(voaRule);
        }
    }
    return matchedRule;
}

std::optional<VoaConfig::VoaRule> VoaConfig::getRuleForId(const std::string &ruleId) {
    std::lock_guard<std::mutex> lock{voa_config_mutex};

    std::list<VoaRule>::iterator it =
        std::find_if(voaRules.begin(), voaRules.end(),
                     [&](const VoaConfig::VoaRule &r) { return r.ruleId == ruleId; });
    if (it == std::end(voaRules)) {
        return std::nullopt;
    }
    return *it;
}

template <typename KEY, typename VAL>
bool VoaConfig::keyInMap(const std::map<KEY, VAL> &mapName, const std::string &key) {
    if (mapName.empty() || (mapName.find(key) == mapName.end())) {
        return false;
    }
    return true;
}

bool VoaConfig::isActive(VoaConfig::VoaRule rule, double currentTimestamp) {
    double waitTime = rule.getRuleStartupDelay();

    // Ensure that we've waited long enough to startup
    if ((currentTimestamp - startupTimestamp) < waitTime) {
        helper::logDebug(
            "VoaConfig::isActive - skipping until startup "
            "time (cur/start/wait) " +
            std::to_string(currentTimestamp) + " " + std::to_string(startupTimestamp) + " " +
            std::to_string(waitTime));
        return false;
    }

    if (keyInMap(rule.window, VOA_WINDOW_COUNT)) {
        if (!keyInMap(ruleState, rule.ruleId) ||
            !keyInMap(ruleState[rule.ruleId], VOA_STATE_COUNT)) {
            ruleState[rule.ruleId][VOA_STATE_COUNT] = 0.0;
        }
        if (ruleState[rule.ruleId][VOA_STATE_COUNT] >= stof(rule.window[VOA_WINDOW_COUNT])) {
            helper::logDebug("VoaConfig::isActive reached count_state=" +
                             std::to_string(ruleState[rule.ruleId][VOA_STATE_COUNT]));
            return false;
        } else {
            ruleState[rule.ruleId][VOA_STATE_COUNT] += 1.0;
        }
    } else if (keyInMap(rule.window, VOA_WINDOW_DURATION)) {
        if (!keyInMap(ruleState, rule.ruleId) ||
            !keyInMap(ruleState[rule.ruleId], VOA_STATE_DURATION)) {
            double durationTimestamp = currentTimestamp + stof(rule.window[VOA_WINDOW_DURATION]);
            ruleState[rule.ruleId][VOA_STATE_DURATION] = durationTimestamp;
        }
        if (currentTimestamp >= ruleState[rule.ruleId][VOA_STATE_DURATION]) {
            helper::logDebug("VoaConfig::isActive reached duration_state=" +
                             std::to_string(ruleState[rule.ruleId][VOA_STATE_DURATION]));
            return false;
        }
    }
    return true;
}

bool VoaConfig::isTriggered(VoaConfig::VoaRule rule) {
    if (keyInMap(rule.trigger, VOA_TRIGGER_SKIPN)) {
        if (keyInMap(ruleState, rule.ruleId) && keyInMap(ruleState[rule.ruleId], VOA_STATE_SKIPN)) {
            ruleState[rule.ruleId][VOA_STATE_SKIPN]++;
        } else {
            ruleState[rule.ruleId][VOA_STATE_SKIPN] = 0;
        }
        // apply the rule after every N packets
        int skipNval = static_cast<int>(ruleState[rule.ruleId][VOA_STATE_SKIPN]);
        int skipNparam = stoi(rule.trigger[VOA_TRIGGER_SKIPN]);
        // Trigger every Nth package
        if ((skipNparam == 0) || (skipNval % skipNparam == 0)) {
            helper::logDebug("VoaConfig::isTriggered TRUE skipN=" +
                             std::to_string(ruleState[rule.ruleId][VOA_STATE_SKIPN]));
            ruleState[rule.ruleId][VOA_STATE_SKIPN] = 0;
            return true;
        }
        helper::logDebug("VoaConfig::isTriggered FALSE skipN=" +
                         std::to_string(ruleState[rule.ruleId][VOA_STATE_SKIPN]));
        return false;
    } else if (keyInMap(rule.trigger, VOA_TRIGGER_PROB)) {
        std::uniform_real_distribution<double> distribution(0.0, 1.0);
        float calcProb = distribution(mRnd);
        if (calcProb < stof(rule.trigger[VOA_TRIGGER_PROB])) {
            helper::logDebug("VoaConfig::isTriggered TRUE prob=" + std::to_string(calcProb));
            return true;
        }
        helper::logDebug("VoaConfig::isTriggered FALSE prob=" + std::to_string(calcProb));
        return false;
    }
    return true;
}
