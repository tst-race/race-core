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

/** Status enumeration for channel statuses. */
public enum ChannelStatus {
    /** Default/undefined status. */
    CHANNEL_UNDEF(0),
    /** Channel is available. */
    CHANNEL_AVAILABLE(1),
    /** Channel is unavailable */
    CHANNEL_UNAVAILABLE(2),
    CHANNEL_ENABLED(3),
    CHANNEL_DISABLED(4),
    CHANNEL_STARTING(5),
    CHANNEL_FAILED(6),
    CHANNEL_UNSUPPORTED(7);

    private final int value;

    private ChannelStatus(int value) {
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
     * Returns the enum value corresponding to the given integer value, or CHANNEL_INVALID if no
     * match found.
     *
     * @param value Integer value
     * @return Enum value
     */
    public static ChannelStatus valueOf(int value) {
        for (ChannelStatus status : values()) {
            if (status.value == value) {
                return status;
            }
        }
        return ChannelStatus.CHANNEL_UNDEF;
    }
}
