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

public enum SendType {
    ST_UNDEF(0), // undefined
    ST_STORED_ASYNC(
            1), // sent messages are stored but there is no way to know the other side is ready
    ST_EPHEM_SYNC(2); // the other side must be ready, but the link will know when that occurs

    private int value = 0;
    private static Map map = new HashMap<>();

    private SendType(int value) {
        this.value = value;
    }

    static {
        for (SendType sendType : SendType.values()) {
            map.put(sendType.value, sendType);
        }
    }

    public static SendType valueOf(int sendType) {
        return (SendType) map.get(sendType);
    }

    public int getValue() {
        return value;
    }
}
