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

/** Status enumeration for link connections. */
public enum ConnectionStatus {
    /** Default/undefined status. */
    CONNECTION_INVALID(0),
    /** Connection is open. */
    CONNECTION_OPEN(1),
    /** Connection is closed. */
    CONNECTION_CLOSED(2),
    /** Connection is awaiting contact from other side */
    CONNECTION_AWAITING_CONTACT(3),
    /** Connection failed to initialize */
    CONNECTION_INIT_FAILED(4),
    /** Connection is available */
    CONNECTION_AVAILABLE(5),
    /** Connection is unavailable */
    CONNECTION_UNAVAILABLE(6);

    private final int value;

    private ConnectionStatus(int value) {
        this.value = value;
    }

    /**
     * Returns the integer value of this enum value.
     *
     * <p>This is only used for portable communication of values. It may be equal to the ordinal
     * value of the numeration, but it is not guaranteed.
     *
     * @return Integer value
     */
    public int getValue() {
        return value;
    }

    /**
     * Returns the enum value corresponding to the given integer value, or CONNECTION_INVALID if no
     * match found.
     *
     * @param value Integer value
     * @return Enum value
     */
    public static ConnectionStatus valueOf(int value) {
        for (ConnectionStatus status : values()) {
            if (status.value == value) {
                return status;
            }
        }
        return ConnectionStatus.CONNECTION_INVALID;
    }
}
