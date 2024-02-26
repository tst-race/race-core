
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

#include "ClearMessagePackageTracker.h"

void ClearMessagePackageTracker::addEncPkgHandleForClrMsg(RaceHandle encPkgHandle,
                                                          RaceHandle clrMsgHandle) {
    clearMessagePackageStatuses[clrMsgHandle][encPkgHandle] = PACKAGE_INVALID;
}

std::tuple<RaceHandle, MessageStatus>
ClearMessagePackageTracker::updatePackageStatusForEncPkgHandle(PackageStatus status,
                                                               RaceHandle encPkgHandle) {
    for (auto const &[clrMsgHandle, packageStatusMap] : clearMessagePackageStatuses) {
        if (packageStatusMap.count(encPkgHandle) != 0) {
            clearMessagePackageStatuses[clrMsgHandle][encPkgHandle] = status;
            return std::make_tuple(clrMsgHandle, getStatusForClrMsg(clrMsgHandle));
        }
    }

    return std::make_tuple(NULL_RACE_HANDLE, MS_UNDEF);
}

MessageStatus ClearMessagePackageTracker::getStatusForClrMsg(RaceHandle clrMsgHandle) {
    bool didItFail = true;
    for (auto const &[encPkgHandle, packageStatus] : clearMessagePackageStatuses[clrMsgHandle]) {
        // If ANY package was sent then consider the clear message as sent.
        if (packageStatus == PACKAGE_SENT) {
            return MS_SENT;
        }
        // If ANY package didn't fail then there's still a chance it could be sent. Update the flag
        // accordingly,
        else if (packageStatus != PACKAGE_FAILED_GENERIC &&
                 packageStatus != PACKAGE_FAILED_NETWORK_ERROR &&
                 packageStatus != PACKAGE_FAILED_TIMEOUT) {
            didItFail = false;
        }
        // If neither of these cases hit for all the packages then the clear message has failed.
    }

    return didItFail ? MS_FAILED : MS_UNDEF;
}

void ClearMessagePackageTracker::removeClrMsgHandle(RaceHandle clrMsgHandle) {
    clearMessagePackageStatuses.erase(clrMsgHandle);
}
