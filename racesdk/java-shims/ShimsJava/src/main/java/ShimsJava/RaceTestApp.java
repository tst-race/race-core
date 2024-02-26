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

public class RaceTestApp {

    long nativePtr;

    public RaceTestApp(long outputPtr, long sdkPtr, long appPtr, long tracerPtr) {
        _jni_initialize(outputPtr, sdkPtr, appPtr, tracerPtr);
    }

    private native void _jni_initialize(long outputPtr, long sdkPtr, long appPtr, long tracerPtr);

    /**
     * For sending messages from the UI only. DO NOT CALL FROM ADB COMMANDS Takes message from UI
     * and recipient, create C++ Message object and then calls C++ version of
     * RaceTestApp.sendMessage
     *
     * @param message Message to send
     */
    public native void sendMessage(String message, String recipient);

    /**
     * Forward all test commands to raceTestAppShared to process. This includes single/auto send
     * messages invoke through adb commands
     *
     * @param command command to be processed
     * @return True if the app should stop
     */
    public native boolean processRaceTestAppCommand(String command);
}
