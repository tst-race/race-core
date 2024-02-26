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

/** Package type enumeration. */
public enum PackageType {
    /** Defualt/undefined type */
    PKG_TYPE_UNDEF(0),
    /** network manager package */
    PKG_TYPE_NM(1),
    /** Test harness package */
    PKG_TYPE_TEST_HARNESS(2),
    /** SDK (e.g., bootstrapping) package */
    PKG_TYPE_SDK(3);

    private final int value;

    private PackageType(int value) {
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
     * Returns the enum value corresponding to the given integer value, or PKG_TYPE_UNDEF if no
     * match found.
     *
     * @param value Integer value
     * @return Enum value
     */
    public static PackageType valueOf(int value) {
        for (PackageType type : values()) {
            if (type.value == value) {
                return type;
            }
        }
        return PackageType.PKG_TYPE_UNDEF;
    }
}
