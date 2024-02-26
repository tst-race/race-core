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

public enum ConnectionType {
    CT_UNDEF(0), // undefined
    CT_DIRECT(1), // direct
    CT_INDIRECT(2), // indirect
    CT_MIXED(3), // mixed
    CT_LOCAL(4); // local

    private int value = 0;
    private static Map map = new HashMap<>();

    private ConnectionType(int value) {
        this.value = value;
    }

    static {
        for (ConnectionType connectionType : ConnectionType.values()) {
            map.put(connectionType.value, connectionType);
        }
    }

    public static ConnectionType valueOf(int connectionType) {
        return (ConnectionType) map.get(connectionType);
    }

    public int getValue() {
        return value;
    }
}
