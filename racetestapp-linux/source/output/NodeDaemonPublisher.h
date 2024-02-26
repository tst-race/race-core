
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

#ifndef _NODE_DAEMON_PUBLISHER_H_
#define _NODE_DAEMON_PUBLISHER_H_

#include <string>

#include "RaceEnums.h"
#include "nlohmann/json.hpp"

/**
 * @brief Class for sending app status to the node daemon
 *
 */
class NodeDaemonPublisher {
public:
    /**
     * @brief Construct a new Race Test App Input Fifo object.
     *
     * @throw std::runtime_exception if construction fails.
     */
    NodeDaemonPublisher();

    /**
     * @brief publish app status to node daemon
     *
     * @throw std::runtime_exception if writing to fifo fails.
     */
    void publishStatus(const nlohmann::json &status, int ttl);

    void publishBootstrapAction(const std::string &message,
                                RaceEnums::BootstrapActionType actionType);

private:
    int fifoFd;
};

#endif
