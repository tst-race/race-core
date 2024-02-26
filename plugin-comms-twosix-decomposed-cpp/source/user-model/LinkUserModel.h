
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

#ifndef __COMMS_TWOSIX_USER_MODEL_LINK_USER_MODEL_H__
#define __COMMS_TWOSIX_USER_MODEL_LINK_USER_MODEL_H__

#include <ComponentTypes.h>

#include <atomic>

#include "MarkovModel.h"

class LinkUserModel {
public:
    explicit LinkUserModel(const LinkID &linkId, std::atomic<uint64_t> &nextActionId);
    virtual ~LinkUserModel() {}

    /**
     * @brief Get the action timeline for this link between the specified start and end timestamps.
     *
     * @param start Timestamp at which to start
     * @param end Timestamp at which to end
     * @return Action timeline
     */
    virtual ActionTimeline getTimeline(Timestamp start, Timestamp end);

protected:
    virtual MarkovModel::UserAction getNextUserAction();

private:
    MarkovModel model;
    LinkID linkId;
    std::atomic<uint64_t> &nextActionId;
    ActionTimeline cachedTimeline;
};

#endif  // __COMMS_TWOSIX_USER_MODEL_LINK_USER_MODEL_H__