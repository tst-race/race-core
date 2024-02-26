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

/** Status enumeration for plugin operations. */
public enum PluginResponse {
    /** Default/undefined status. */
    PLUGIN_INVALID(0),
    /** Operation successful. */
    PLUGIN_OK(1),
    /** Temporary error occured. */
    PLUGIN_TEMP_ERROR(2),
    /** Non-fatal error occurred. */
    PLUGIN_ERROR(3),
    /** Fatal error occurrred. */
    PLUGIN_FATAL(4);

    private final int value;

    private PluginResponse(int value) {
        this.value = value;
    }

    /**
     * Returns the enum value corresponding to the given integer value, or PLUGIN_INVALID if no
     * match found.
     *
     * @param value Integer value
     * @return Enum value
     */
    public static PluginResponse valueOf(int value) {
        for (PluginResponse status : values()) {
            if (status.value == value) {
                return status;
            }
        }
        return PluginResponse.PLUGIN_INVALID;
    }
}
