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

package com.twosix.race.daemon;

public abstract class Constants {
    public static final String PREFERENCES = "race_node_daemon";
    public static final String CURRENT_BOOTSTRAP_TARGET = "currentBootstrapTarget";

    // RACE app & broadcast receiver actions
    public static final String RACE_APP_PACKAGE = "com.twosix.race";
    public static final String RACE_APP_ACTION = "com.twosix.race.APP_ACTION";

    // Our broadcast receiver actions
    public static final String UPDATE_APP_STATUS_ACTION =
            "com.twosix.race.daemon.UPDATE_APP_STATUS";
    public static final String FORWARD_BOOTSTRAP_INFO_ACTION =
            "com.twosix.race.daemon.FORWARD_BOODSTRAP_INFO";
    public static final String APP_INSTALLED_ACTION = "com.twosix.race.daemon.APP_INSTALLED_ACTION";
}
