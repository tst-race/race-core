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

public enum LinkDirection {
    LD_UNDEF(0), // undefined
    LD_CREATOR_TO_LOADER(1), // creator sends to loader
    LD_LOADER_TO_CREATOR(2), // loader sends to creator
    LD_BIDI(3); // bi-directional

    private int value = 0;
    private static Map map = new HashMap<>();

    private LinkDirection(int value) {
        this.value = value;
    }

    static {
        for (LinkDirection linkType : LinkDirection.values()) {
            map.put(linkType.value, linkType);
        }
    }

    public static LinkDirection valueOf(int linkType) {
        return (LinkDirection) map.get(linkType);
    }

    public int getValue() {
        return value;
    }
}
