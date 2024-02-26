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
 * Response object from SDK operations.
 *
 * <p>This class is immutable and thread-safe.
 */
public class SdkResponse {

    /** Status enumeration for SDK operations. */
    public enum SdkStatus {
        /** Default/undefined status. */
        SDK_INVALID(0),
        /** Operation successful. */
        SDK_OK(1),
        /** SDK is shutting down. */
        SDK_SHUTTING_DOWN(2),
        /** SDK is missing required plugin. */
        SDK_PLUGIN_MISSING(3),
        /** Argument is invalid. */
        SDK_INVALID_ARGUMENT(4),
        /** Plugin/connection queue is full. */
        SDK_QUEUE_FULL(5);

        private final int value;

        private SdkStatus(int value) {
            this.value = value;
        }

        /**
         * Returns the enum value corresponding to the given integer value, or SDK_INVALID if no
         * match found.
         *
         * @param value Integer value
         * @return Enum value
         */
        public static SdkStatus valueOf(int value) {
            for (SdkStatus status : values()) {
                if (status.value == value) {
                    return status;
                }
            }
            return SdkStatus.SDK_INVALID;
        }
    }

    private final SdkStatus status;
    private final double queueUtilization;
    private final RaceHandle handle;

    public SdkResponse(SdkStatus status, double queueUtilization, RaceHandle handle) {
        this.status = status;
        this.queueUtilization = queueUtilization;
        this.handle = handle;
    }

    /**
     * Returns the SDK operation status enumeration.
     *
     * @return SDK operation status
     */
    public SdkStatus getStatus() {
        return status;
    }

    /**
     * Returns the percentage of queue utilization as a fractional value from 0 to 1.
     *
     * @return Queue utilization percentage
     */
    public double getQueueUtilization() {
        return queueUtilization;
    }

    /**
     * Returns the RACE operation handle for status callbacks.
     *
     * @return RACE handle
     */
    public RaceHandle getHandle() {
        return handle;
    }

    @Override
    public String toString() {
        return "SdkStatus[status="
                + status
                + ", queueUtilization="
                + queueUtilization
                + ", handle="
                + handle
                + "]";
    }
}
