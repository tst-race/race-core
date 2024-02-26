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

package ShimsJava;

import java.util.HashMap;
import java.util.Map;

public enum BootstrapActionType {
    BS_PREPARING_BOOTSTRAP(0),
    BS_PREPARING_CONFIGS(1),
    BS_ACQUIRING_ARTIFACT(2),
    BS_CREATING_BUNDLE(3),
    BS_PREPARING_TRANSFER(4),
    BS_DOWNLOAD_BUNDLE(5),
    BS_NETWORK_CONNECT(6),
    BS_COMPLETE(7),
    BS_FAILED(8),
    BS_UNDEF(9);

    private int value = 0;
    private static Map map = new HashMap<>();

    private BootstrapActionType(int value) {
        this.value = value;
    }

    static {
        for (BootstrapActionType bootstrapActionType : BootstrapActionType.values()) {
            map.put(bootstrapActionType.value, bootstrapActionType);
        }
    }

    public static BootstrapActionType valueOf(int bootstrapActionType) {
        return (BootstrapActionType) map.get(bootstrapActionType);
    }

    public int getValue() {
        return value;
    }
}
