#
# Copyright 2023 Two Six Technologies
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

from typing import Tuple

from networkManagerPluginBindings import MS_UNDEF
from collections import defaultdict

RaceHandle = int
PackageStatus = int
MessageStatus = int
from networkManagerPluginBindings import MS_UNDEF, MS_SENT, MS_FAILED  # MessageStatus
from networkManagerPluginBindings import (
    PACKAGE_INVALID,
    PACKAGE_SENT,
    PACKAGE_FAILED_GENERIC,
    PACKAGE_FAILED_NETWORK_ERROR,
    PACKAGE_FAILED_TIMEOUT,
)  # PackageStatus
from networkManagerPluginBindings import NULL_RACE_HANDLE


class ClearMessagePackageTracker:
    def __init__(self):
        self.clear_message_package_statuses = defaultdict(
            lambda: defaultdict(PackageStatus)
        )

    def add_enc_pkg_handle_for_clr_msg(self, enc_pkg_handle, clr_msg_handle) -> None:
        self.clear_message_package_statuses[clr_msg_handle][
            enc_pkg_handle
        ] = PACKAGE_INVALID

    def update_package_status_for_enc_pkg_handle(
        self, status: PackageStatus, enc_pkg_handle: RaceHandle
    ) -> Tuple[RaceHandle, MessageStatus]:
        for (
            clr_msg_handle,
            package_status_map,
        ) in self.clear_message_package_statuses.items():
            if enc_pkg_handle in package_status_map:
                self.clear_message_package_statuses[clr_msg_handle][
                    enc_pkg_handle
                ] = status
                return clr_msg_handle, self._get_status_for_clr_msg(clr_msg_handle)

        return NULL_RACE_HANDLE, MS_UNDEF

    def remove_clr_msg_handle(self, clr_msg_handle: RaceHandle):
        self.clear_message_package_statuses.pop(clr_msg_handle, None)

    def _get_status_for_clr_msg(self, clr_msg_handle: RaceHandle) -> MessageStatus:
        did_it_fail = True
        for encPkgHandle, package_status in self.clear_message_package_statuses[
            clr_msg_handle
        ].items():
            # If ANY package was sent then consider the clear message as sent.
            if package_status == PACKAGE_SENT:
                return MS_SENT
            # If ANY package didn't fail then there's still a chance it could be sent.
            # Update the flag accordingly,
            elif (
                package_status != PACKAGE_FAILED_GENERIC
                and package_status != PACKAGE_FAILED_NETWORK_ERROR
                and package_status != PACKAGE_FAILED_TIMEOUT
            ):
                did_it_fail = False

            # If neither of these cases hit for all the packages then the clear message
            # has failed.

        return MS_FAILED if did_it_fail else MS_UNDEF
