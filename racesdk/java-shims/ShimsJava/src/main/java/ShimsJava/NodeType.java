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

public enum NodeType {
    NT_ALL(0),
    NT_CLIENT(1),
    NT_SERVER(2),
    NT_UNDEF(3);

    private int value = 0;
    private static Map map = new HashMap<>();

    private NodeType(int value) {
        this.value = value;
    }

    static {
        for (NodeType NodeType : NodeType.values()) {
            map.put(NodeType.value, NodeType);
        }
    }

    public static NodeType valueOf(int NodeType) {
        return (NodeType) map.get(NodeType);
    }

    public int getValue() {
        return value;
    }
}
