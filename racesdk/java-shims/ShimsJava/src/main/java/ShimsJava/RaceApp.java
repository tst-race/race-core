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

public abstract class RaceApp {

    public final long raceAppWrapperPtr;

    protected RaceApp(long outputPtr, long sdkPtr, long tracerPtr) {
        raceAppWrapperPtr = _jni_initialize(outputPtr, sdkPtr, tracerPtr);
    }

    @Override
    public void finalize() {
        shutdown();
    }

    // This needs to create a RaceApp Wrapper and pass the pointer back
    // The pointer will be pass to RaceSdkApp
    protected native long _jni_initialize(long outputPtr, long sdkPtr, long tracerPtr);

    /**
     * Shut down the RACE app. The RaceApp instance is no longer valid after calling this method.
     */
    public native void shutdown();

    // Add a send message that was created/sent through RaceTestApp to the UI
    // required for messages created/sent directly through RaceTestApp-Shared
    // (auto messages)
    public abstract void addMessageToUI(JClrMsg clrMsg);

    // The native wrapper will have it's own handleReceivedMessage that it needs
    // to implement because it inherits from IRaceApp. The native version will
    // just look up this Java method and call it.
    // This needs to call android things the Android class should extend
    // RaceApp.java and the native code will still map ok
    public abstract void handleReceivedMessage(JClrMsg msg);

    /**
     * JNI Wrapper to convert String param to JSONObject prior to calling the app's method
     *
     * @param sdkStatus json object as String
     */
    public abstract void onSdkStatusChanged(String sdkStatus);

    public void onMessageStatusChanged(RaceHandle handle, MessageStatus status) {
        // Purposefully Empty, the JNI bindings call the C++ implementation prior to
        // calling the Java implementation. This method exists in Java incase we want to add
        // Android specific functionality
    }

    /**
     * @brief Requests input from the user
     *     <p>The task posted to the work queue will lookup a response for the user input prompt,
     *     wait an optional amount of time, then notify the SDK of the user response.
     * @param handle The RaceHandle to use for the onUserInputReceived callback
     * @param pluginId Plugin ID of the plugin requesting user input (or "Common" for common user
     *     input)
     * @param key User input key
     * @param prompt User input prompt
     * @param cache If true, the response will be cached
     * @return SdkResponse object that contains whether the post was successful
     */
    public abstract SdkResponse requestUserInput(
            RaceHandle handle, String pluginId, String key, String prompt, boolean cache);

    protected static class UserResponse {
        public final boolean answered;
        public final String response;

        UserResponse(boolean answered, String response) {
            this.answered = answered;
            this.response = response;
        }
    }

    protected native UserResponse getCachedResponse(String pluginId, String key);

    protected native UserResponse getAutoResponse(String pluginId, String key);

    protected native boolean setCachedResponse(String pluginId, String key, String response);

    /**
     * @brief Displays information to the User
     *     <p>The task posted to the work queue will display information to the user input prompt,
     *     wait an optional amount of time, then notify the SDK of the user acknowledgment.
     * @param handle The RaceHandle to use for the onUserInputReceived callback
     * @param data data to display
     * @param displayType type of user display to display data in
     * @return SdkResponse object that contains whether the post was successful
     */
    public abstract SdkResponse displayInfoToUser(
            RaceHandle handle, String data, UserDisplayType displayType);

    /**
     * @brief Displays information to the User and forward information to target node for automated
     *     testing
     * @param handle The RaceHandle to use for the onUserInputReceived callback
     * @param data data to display
     * @param displayType type of user display to display data in
     * @param actionType type of action the Daemon must take
     * @return SdkResponse
     */
    public abstract SdkResponse displayBootstrapInfoToUser(
            RaceHandle handle,
            String data,
            UserDisplayType displayType,
            BootstrapActionType actionType);
}
