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

/** Status enumeration for link statuses. */
public enum LinkStatus {
    /** Default/undefined status. */
    LINK_UNDEF(0),
    /** Link has been created (via createLink()). */
    LINK_CREATED(1),
    /** Link has been loaded (via loadLinkAddress(es)(). */
    LINK_LOADED(2),
    /** Link is closed. */
    LINK_DESTROYED(3);

    private final int value;

    private LinkStatus(int value) {
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
     * Returns the enum value corresponding to the given integer value, or LINK_INVALID if no match
     * found.
     *
     * @param value Integer value
     * @return Enum value
     */
    public static LinkStatus valueOf(int value) {
        for (LinkStatus status : values()) {
            if (status.value == value) {
                return status;
            }
        }
        return LinkStatus.LINK_UNDEF;
    }
}
