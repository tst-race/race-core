
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

#ifndef __VOA_CONFIG_H_
#define __VOA_CONFIG_H_

#include <stdlib.h>

#include <list>
#include <nlohmann/json.hpp>
#include <optional>
#include <random>

#include "../include/RaceSdk.h"
#include "JsonConfig.h"

/**
 * VoaConfig: A class to manage the VoA configuration
 *
 * A VoA configuration has the following structure
 *
 *  {
 *    "rule-ID": {
 *
 *       "<key-string>": "<value-string>",
 *
 *       ...
 *
 *    }
 *    , ...
 *  }
 *
 * The rule-ID is an identifier string for the given rule. At this time it is primarily used for
 * easy identification of the rule that contributed to a particular VoA behavior, so duplicate rules
 * with the same identifier is strongly discouraged (although will not result in an error at this
 * time).
 *
 * The following key-string configuration parameters are defined (note that all key-strings and
 * value-strings must be quoted.
 *
 * persona
 * This configuration parameter specifies the active persona for which this rule is applicable. A
 * value of "any" matches all active personas.
 *
 * startupdelay
 * This configuration parameter specifies the duration in seconds that must elapse following node
 * start-up before the given rule can be processed. This is useful in cases where the system must be
 * given some time to stabilize prior to the application of any VoA rules.
 *
 * tag
 * This parameter can be used to specify a tag string associated with a particular VoA action.
 * Triggered VoA actions are logged to opentracing. The tag parameter provides a way to specify a
 * common identification string across multiple such log records in order to support subsequent VoA
 * analytics.
 *
 * to
 * This parameter is used to specify the target destination to which the VoA rule apples. The target
 * destination must have two options supplied, namely "type" and "matchid". Target types
 * can be one of the following, with matchId providing the value that needs to be matched.
 *
 *   persona
 *   Checks if one of the destination personas matches the given persona value in matchid.
 *
 *   linkId
 *   Checks if the link identifier associated with package transport matches the given value in
 *   matchid.
 *
 *   channel
 *   Checks if the channel identifier associated with package transport matches the given value in
 *   matchid.
 *
 * window
 * This parameter can be used to constrain the application of a rule to a given operating window.
 * The operating window can be provided through one of the following options
 *
 *   duration
 *   Specifying the time duration for application of the rule
 *
 *   count
 *   Specifying the number of packages processed
 *
 * trigger
 * This parameter enables one to trigger the application of a VoA rule under specific conditions.
 * Two such conditions can be specified as options
 *
 *   prob
 *   Specifies the probability (0-1) of rule application under a uniform random distribution.
 *
 *   skipN
 *   Specifies that the rule needs to be applied to every Nth package that is seen by this rule.
 *
 * action
 * This configuration parameter is used to specify the VoA action associated with the given rule.
 * Four VoA actions are defined currently, namely
 *
 *   delay
 *   This action introduces a delay prior to the package being sent. The delay is configurable
 *   through the params setting.
 *
 *   drop
 *   The drop action results in the package being dropped at the targeted node.
 *
 *   tamper
 *   This action results in the corruption of random bytes within the encrypted payload. The number
 *   of bytes that are corrupted is configurable through the params option.
 *
 *   replay
 *   This action results in multiple copies of the original package being sent out. The number of
 *   replayed packages is configurable through the params option.
 *
 * params
 * Additional options associated with a given VoA action are specified here. The following options
 * are defined, some of which are action-specific.
 *
 *   holdtime	(delay, replay): Specifies the duration (in seconds) that the package should be held
 *   prior to being sent.
 *
 *   jitter	(delay, replay): Specifies the maximum bounds (in seconds) for a random jitter that
 *   needs to be introduced prior to a package being sent out.
 *
 *   replaytimes (replay): Specifies the number of times a package needs to be replayed. A
 *   replaytimes value of 1 would result in two packages being sent out.
 *
 *   iterations	(tamper): Specifies the number of times a package is processed through the logic
 *   that results in a corruption of a random byte.
 *
 */
class VoaConfig : JsonConfig {
private:
    // Defaults for various action parameters
    static constexpr float DEFAULT_HOLDTIME = 0.0;
    static constexpr float DEFAULT_STARTUPDELAY = 0.0;
    static constexpr unsigned long DEFAULT_REPLAYTIMES = 0;
    static constexpr unsigned long DEFAULT_MANGLETIMES = 0;

public:
    static const std::string VOA_CONF_ACTION;
    static const std::string VOA_CONF_PERSONA;
    static const std::string VOA_CONF_TAG;
    static const std::string VOA_CONF_STARTUP_DELAY;
    static const std::string VOA_CONF_TO;
    static const std::string VOA_CONF_PARAMS;
    static const std::string VOA_CONF_TRIGGER;
    static const std::string VOA_CONF_WINDOW;

    static const std::string VOA_ACTION_DROP;
    static const std::string VOA_ACTION_DELAY;
    static const std::string VOA_ACTION_TAMPER;
    static const std::string VOA_ACTION_REPLAY;

    static const std::string VOA_PARAMS_JITTER;
    static const std::string VOA_PARAMS_HOLDTIME;
    static const std::string VOA_PARAMS_REPLAYTIMES;
    static const std::string VOA_PARAMS_ITERATIONS;

    static const std::string VOA_TARGET_TYPE;
    static const std::string VOA_TARGET_MATCHID;
    static const std::string VOA_TARGET_TYPE_PERSONA;
    static const std::string VOA_TARGET_TYPE_LINK;
    static const std::string VOA_TARGET_TYPE_CHANNEL;
    static const std::string VOA_TARGET_MATCHID_ALL;

    static const std::string VOA_TRIGGER_PROB;
    static const std::string VOA_TRIGGER_SKIPN;

    static const std::string VOA_WINDOW_COUNT;
    static const std::string VOA_WINDOW_DURATION;

    static const std::string VOA_STATE_COUNT;
    static const std::string VOA_STATE_DURATION;
    static const std::string VOA_STATE_SKIPN;

public:
    /**
     * @brief Constructor.
     *
     * @param voaConfigPath Path to a configuration file
     */
    explicit VoaConfig(const std::string &voaConfigPath);

    /**
     * @brief Representation of a single VoA rule
     *
     */
    struct VoaRule {
        // The rule identifier
        std::string ruleId;
        // The active persona associated with a race node
        std::string racePersona;
        // A tag string to include in opentracing logs
        std::string tag;
        // The startup delay associated with this rule
        std::string startupDelay;
        // The VoA action associated with this rule
        std::string action;
        // a set of tuples specifying the package originator
        // std::map<std::string, std::string> from;
        // a set of tuples specifying the package destination
        std::map<std::string, std::string> to;
        // A set of tuples that supply parameters for the VoA action
        std::map<std::string, std::string> params;
        // A set of tuples that supply rule trigger parameters
        std::map<std::string, std::string> trigger;
        // A set of tuples that supply rule span  parameters
        std::map<std::string, std::string> window;

        /**
         * @brief Constructor to create a VoA rule object
         *
         * @param _ruleId An identifier for the rule
         * @param ruleItem A JSON representation of the rule
         */
        VoaRule(const std::string &_ruleId, nlohmann::json ruleItem) :
            ruleId(_ruleId),
            racePersona(ruleItem.value(VOA_CONF_PERSONA, VOA_TARGET_MATCHID_ALL)),
            tag(ruleItem.value(VOA_CONF_TAG, "")),
            startupDelay(ruleItem.value(VOA_CONF_STARTUP_DELAY, "")),
            action(ruleItem.value(VOA_CONF_ACTION, VOA_ACTION_DELAY)),
            to(ruleItem.value(VOA_CONF_TO, (std::map<std::string, std::string>()))),
            params(ruleItem.value(VOA_CONF_PARAMS, (std::map<std::string, std::string>()))),
            trigger(ruleItem.value(VOA_CONF_TRIGGER, (std::map<std::string, std::string>()))),
            window(ruleItem.value(VOA_CONF_WINDOW, (std::map<std::string, std::string>()))) {}

        /**
         * @brief Get the configured startup delay time
         *
         */
        double getRuleStartupDelay() {
            if (startupDelay == "") {
                return DEFAULT_STARTUPDELAY;
            }
            return stof(startupDelay);
        }

        /**
         * @brief Get the configured hold time
         *
         * @param randWeight A random value between 0 and 1 used to weight the jitter value
         */
        double getHoldTimeParam(float randWeight) {
            if (keyInMap(params, VOA_PARAMS_HOLDTIME)) {
                return stof(params[VOA_PARAMS_HOLDTIME]);
            } else if (keyInMap(params, VOA_PARAMS_JITTER)) {
                float jitter = stof(params[VOA_PARAMS_JITTER]) * randWeight;
                return jitter;
            }
            return DEFAULT_HOLDTIME;
        }

        /**
         * @brief Get the configured replay count
         *
         */
        unsigned long getReplayTimesParam() {
            if (keyInMap(params, VOA_PARAMS_REPLAYTIMES)) {
                return std::stoul(params[VOA_PARAMS_REPLAYTIMES]);
            }
            return DEFAULT_REPLAYTIMES;
        }

        /**
         * @brief Get the configured iterations value
         *
         */
        unsigned long getIterationsParam() {
            if (keyInMap(params, VOA_PARAMS_ITERATIONS)) {
                return std::stoul(params[VOA_PARAMS_ITERATIONS]);
            }
            return DEFAULT_MANGLETIMES;
        }

        /**
         * @brief Logic for matching rules against provided parameters
         *
         * @param activePersona The active persona of the race node
         * @param linkId The link used by sendEncryptedPackage()
         * @param personaList the personas associated with this link
         */
        bool matches(const std::string &activePersona, const LinkID &linkId,
                     const std::string &channelGid, std::vector<std::string> &personaList) {
            // The persona must match
            if (racePersona != VOA_TARGET_MATCHID_ALL && (racePersona != activePersona)) {
                return false;
            }

            // check if we have necessary config to process the destination
            if (keyInMap(to, VOA_TARGET_TYPE) && keyInMap(to, VOA_TARGET_MATCHID)) {
                // "all" matches any destination
                if (to[VOA_TARGET_MATCHID] == VOA_TARGET_MATCHID_ALL) {
                    return true;
                }
                // find a matching persona in the list
                if (to[VOA_TARGET_TYPE] == VOA_TARGET_TYPE_PERSONA) {
                    std::vector<std::string>::iterator it =
                        std::find(personaList.begin(), personaList.end(), to[VOA_TARGET_MATCHID]);
                    if (it != personaList.end()) {
                        return true;
                    }
                }
                // check if the linkId matches
                if ((to[VOA_TARGET_TYPE] == VOA_TARGET_TYPE_LINK) &&
                    (linkId == to[VOA_TARGET_MATCHID])) {
                    return true;
                }
                // check if the channelId matches
                if ((to[VOA_TARGET_TYPE] == VOA_TARGET_TYPE_CHANNEL) &&
                    (channelGid == to[VOA_TARGET_MATCHID])) {
                    return true;
                }
            }

            // We need a positive match
            return false;
        }
    };

    /**
     * @brief Add a new rule from the given configuration
     *
     * @param payload The payload for a VoA add rule action
     */
    bool addRules(const nlohmann::json &payload);

    /**
     * @brief Delete rules with identifiers specified
     *
     * @param payload The payload for a VoA delete rule action
     */
    bool deleteRules(const nlohmann::json &payload);

private:
    // Random engine for probablistic actions
    std::default_random_engine mRnd;

    // List of rules extracted from given configuration
    std::list<VoaRule> voaRules;

    std::map<std::string, std::map<std::string, float>> ruleState;

    // A mutex object that mediates access to the list of configuration rules
    std::mutex voa_config_mutex;

    // Timestamp associated when config started up
    double startupTimestamp;

public:
    /**
     * @brief Find matching rules for given selection criteria
     *
     * @param activePersona The active persona of the race node
     * @param linkId The link used by sendEncryptedPackage()
     * @param personaList the personas associated with this link
     */
    std::vector<VoaRule> findTargetedRules(std::string activePersona, LinkID linkId,
                                           std::string channelGid,
                                           std::vector<std::string> &personaList);

    /**
     * @brief Return the rule corresponding to the given ruleId
     *
     */
    std::optional<VoaRule> getRuleForId(const std::string &ruleId);

    /**
     * @brief Convenience function to check if key exists in map
     *
     * @param mapName the map within which to check
     * @param key the key to look for
     */
    template <typename KEY, typename VAL>
    static bool keyInMap(const std::map<KEY, VAL> &mapName, const std::string &key);

    /**
     * @brief Check if VoA is active
     *
     * Check if VoA processing should be applied based on number of packages
     * processed (if there is a limit), and whether there is a
     * configured duration.
     *
     * @param rule The rule to check against
     * @param currentTimestamp The current timestamp
     */
    bool isActive(VoaRule rule, double currentTimestamp);

    /**
     * @brief Check if VoA is triggered for the current package.
     *
     * @param rule The rule to check against
     */
    bool isTriggered(VoaRule rule);
};

#endif
