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

/** Status enumeration for package transmission operations. */
public enum MessageStatus {
    MS_UNDEF(0),
    MS_SENT(1),
    MS_FAILED(2);

    private final int value;

    private MessageStatus(int value) {
        this.value = value;
    }

    /**
     * Returns the enum value corresponding to the given integer value, or MS_UNDEF if no match
     * found.
     *
     * @param value Integer value
     * @return Enum value
     */
    public static MessageStatus valueOf(int value) {
        for (MessageStatus status : values()) {
            if (status.value == value) {
                return status;
            }
        }
        return MessageStatus.MS_UNDEF;
    }
}
