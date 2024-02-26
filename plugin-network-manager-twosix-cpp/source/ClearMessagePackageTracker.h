
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

#ifndef __SOURCE_CLEARMESSAGEPACKAGETRACKER_H__
#define __SOURCE_CLEARMESSAGEPACKAGETRACKER_H__

#include <MessageStatus.h>
#include <PackageStatus.h>
#include <SdkResponse.h>

#include <tuple>
#include <unordered_map>

/**
 * @brief Class to track the encrypted packages used to send a clear message over the RACE network.
 * A clear message can be sent to multiple entrance committee servers. This class will track each
 * of those encrypted packages and report the appropriate message status when available.
 *
 * Current limitations:
 *     * Only intended to be used for client nodes send to entrance committee servers.
 *     * Does not support the case where a clear message is split into multiple encrypted packages.
 *       Currently assumes a one-to-one relationship of ClrMsg to EncPkg. If this changes then this
 *       class will need to be updated.
 *
 */
class ClearMessagePackageTracker {
public:
    /**
     * @brief Associate an encrypted package with a clear message.
     *
     * @param encPkgHandle The handle for the encrypted package, i.e. the handle returned from the
     * SDK when calling the sendEncryptedPackage API.
     * @param clrMsgHandle The handle for the clear message, i.e. the handle provided from the SDK
     * via the processClrMsg API.
     */
    void addEncPkgHandleForClrMsg(RaceHandle encPkgHandle, RaceHandle clrMsgHandle);

    /**
     * @brief Update the status of a package with a given handle. The function will returned a tuple
     * containing the associated clear messagehandle as well as the message status of the message
     * associated with that handle. For more information on how the message status is determined see
     * the documentation for getStatusForClrMsg.
     *
     * @param status The new status of the encrypted package.
     * @param encPkgHandle The handle associated witht the encrypted package.
     * @return std::tuple<RaceHandle, MessageStatus> The handle of the clear message associated with
     * the specifed encrypted package and the current status of that clear message.
     */
    std::tuple<RaceHandle, MessageStatus> updatePackageStatusForEncPkgHandle(
        PackageStatus status, RaceHandle encPkgHandle);

    /**
     * @brief Remove a clear message handle that no longer needs to be tracked.
     *
     * @param clrMsgHandle The RaceHandle to remove.
     */
    void removeClrMsgHandle(RaceHandle clrMsgHandle);

protected:
    /**
     * @brief Get the status of a given clear message. The following describes each possible return
     * value:
     *     MS_UNDEF: can't determine message status based on current packages statuses, e.g. all
     * package statues are still PACKAGE_INVALID MS_SENT: at least one encrypted package has been
     * marked sent, meaning the clear message has made it into the RACE network via an entrance
     * committee node MS_FAILED: all of the associated encrypted packages have a failure status,
     * meaning the clear message has not made it into the RACE network.
     *
     * @param clrMsgHandle The RaceHandle associated wit the clear message for which to check
     * status.
     * @return MessageStatus The status of the clear message.
     */
    MessageStatus getStatusForClrMsg(RaceHandle clrMsgHandle);

private:
    /**
     * @brief Data structure for tracking encrypted packages and their status for a given clear
     * message.
     *
     */
    std::unordered_map<
        RaceHandle,                       // The RaceHandle passed into processClrMsg
        std::unordered_map<RaceHandle,    // The RaceHandle returned from sendEncryptedPackage
                           PackageStatus  // The PackageStatus of the encrypted package associated
                                          // with the RaceHandle key.
                           > >
        clearMessagePackageStatuses;
};

#endif
