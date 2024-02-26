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

public enum StorageEncryptionType {
    ENC_AES(0),
    ENC_NONE(1);

    private int value = 0;
    private static Map map = new HashMap<>();

    private StorageEncryptionType(int value) {
        this.value = value;
    }

    static {
        for (StorageEncryptionType encryptionType : StorageEncryptionType.values()) {
            map.put(encryptionType.value, encryptionType);
        }
    }

    public static StorageEncryptionType valueOf(int encryptionType) {
        return (StorageEncryptionType) map.get(encryptionType);
    }

    public int getValue() {
        return value;
    }
}
