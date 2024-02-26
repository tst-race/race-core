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

public enum TransmissionType {
    TT_UNDEF(0),
    TT_UNICAST(1),
    TT_MULTICAST(2);

    private int value = 0;
    private static Map map = new HashMap<>();

    private TransmissionType(int value) {
        this.value = value;
    }

    static {
        for (TransmissionType transmissionType : TransmissionType.values()) {
            map.put(transmissionType.value, transmissionType);
        }
    }

    public static TransmissionType valueOf(int transmissionType) {
        return (TransmissionType) map.get(transmissionType);
    }

    public int getValue() {
        return value;
    }
}
