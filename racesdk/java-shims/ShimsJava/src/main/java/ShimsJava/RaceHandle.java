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

/**
 * Identifier used to correlate updates between asynchronous operations and status change callbacks.
 *
 * <p>The underlying data type is a 64-bit unsigned integer. Since Java does not natively support
 * unsigned types, the value is stored in a signed long.
 *
 * <p>This class is immutable and thread-safe.
 */
public class RaceHandle {

    public static final RaceHandle NULL_RACE_HANDLE = new RaceHandle(0l);

    private final long value;

    public RaceHandle(long value) {
        this.value = value;
    }

    public long getValue() {
        return value;
    }

    @Override
    public int hashCode() {
        return Long.hashCode(value);
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == this) {
            return true;
        } else if (!(obj instanceof RaceHandle)) {
            return false;
        }

        RaceHandle that = (RaceHandle) obj;
        return this.value == that.value;
    }

    @Override
    public String toString() {
        return Long.toUnsignedString(value);
    }
}
