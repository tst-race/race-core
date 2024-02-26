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

public enum LinkSide {
    LS_UNDEF(0),
    LS_CREATOR(1),
    LS_LOADER(2),
    LS_BOTH(3);

    private int value = 0;
    private static Map map = new HashMap<>();

    private LinkSide(int value) {
        this.value = value;
    }

    static {
        for (LinkSide LinkSide : LinkSide.values()) {
            map.put(LinkSide.value, LinkSide);
        }
    }

    public static LinkSide valueOf(int linkSide) {
        return (LinkSide) map.get(linkSide);
    }

    public int getValue() {
        return value;
    }
}
