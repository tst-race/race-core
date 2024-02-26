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

public enum UserDisplayType {
    UD_DIALOG(0),
    UD_QR_CODE(1),
    UD_TOAST(2),
    UD_NOTIFICATION(3),
    UD_UNDEF(4);

    private int value = 0;
    private static Map map = new HashMap<>();

    private UserDisplayType(int value) {
        this.value = value;
    }

    static {
        for (UserDisplayType userDisplayType : UserDisplayType.values()) {
            map.put(userDisplayType.value, userDisplayType);
        }
    }

    public static UserDisplayType valueOf(int userDisplayType) {
        return (UserDisplayType) map.get(userDisplayType);
    }

    public int getValue() {
        return value;
    }
}
