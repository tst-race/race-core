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

/*
 * Java shim for the IRaceSdkApp interface defined in common.
 */
public class RaceSdkApp {

    protected final long nativePtr;

    public RaceSdkApp(AppConfig config, String passphrase) {
        nativePtr = _jni_initialize(config, passphrase);
    }

    @Override
    public void finalize() {
        shutdown();
    }

    protected native long _jni_initialize(AppConfig config, String passphrase);

    public long getNativePtr() {
        return nativePtr;
    }

    /**
     * Shut down the RACE SDK. The RaceSdkApp instance is no longer valid after calling this method.
     */
    public native void shutdown();

    /*
     * RaceSdkApp functions
     */

    public native AppConfig getAppConfig();

    public native String[] getContacts();

    public native boolean isConnected();

    public native boolean initRaceSystem(long appPtr);

    public native RaceHandle prepareToBootstrap(
            String platform,
            String architecture,
            String nodeType,
            String passphrase,
            String bootstrapChannelId);

    public native void cancelBootstrap(RaceHandle bootstrapHandle);

    public native SdkResponse onUserAcknowledgementReceived(RaceHandle handle);

    public native SdkResponse onUserInputReceived(
            RaceHandle handle, boolean answered, String response);

    public native String[] getInitialEnabledChannels();

    public native boolean setEnabledChannels(String[] channelGids);

    public native boolean enableChannel(String channelGid);

    public native boolean disableChannel(String channelGid);

    /*
     * RaceSdkCommon functions
     */

    public native String getActivePersona();

    public native JChannelProperties[] getAllChannelProperties();
}
