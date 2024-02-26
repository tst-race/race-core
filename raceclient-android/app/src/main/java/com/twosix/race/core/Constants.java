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

package com.twosix.race.core;

/** Constants required by the Race Android app */
public class Constants {

    // RACE Daemon & broadcast receiver actions
    public static final String RACE_NODE_DAEMON_PACKAGE = "com.twosix.race.daemon";
    public static final String UPDATE_APP_STATUS_ACTION =
            "com.twosix.race.daemon.UPDATE_APP_STATUS";
    public static final String FORWARD_BOOTSTRAP_INFO_ACTION =
            "com.twosix.race.daemon.FORWARD_BOODSTRAP_INFO";

    // PATHS
    public static final String PATH_TO_ETC = "/storage/self/primary/Download/race/etc";
    public static final String PATH_TO_TESTAPP_CONFIG = PATH_TO_ETC + "/testapp-config.json";
    public static final String PATH_TO_DOWNLOADS = "/storage/self/primary/Download";
    public static final String PATH_TO_USER_RESPONSES =
            PATH_TO_DOWNLOADS + "/race/etc/user-responses.json";
}
