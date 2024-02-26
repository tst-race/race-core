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

package com.twosix.race.service;

import android.content.Context;
import android.util.Log;

import ShimsJava.AppConfig;
import ShimsJava.BootstrapActionType;
import ShimsJava.Helpers;
import ShimsJava.JClrMsg;
import ShimsJava.PluginStatus;
import ShimsJava.RaceApp;
import ShimsJava.RaceHandle;
import ShimsJava.RaceSdkApp;
import ShimsJava.RaceTestApp;
import ShimsJava.SdkResponse;
import ShimsJava.UserDisplayType;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.HashMap;

public class AndroidRaceApp extends RaceApp {

    private static final String TAG = "AndroidRaceApp";

    public static AndroidRaceApp createInstance(
            Context context, RaceSdkApp raceSdk, IServiceListener serviceListener) {

        try {
            AppConfig appConfig = raceSdk.getAppConfig();

            long testAppOutputPtr = Helpers.createRaceTestAppOutputLog(appConfig.logDirectory);
            long tracerPtr = Helpers.createTracer(appConfig.jaegerConfigPath, appConfig.persona);

            return new AndroidRaceApp(
                    context, testAppOutputPtr, raceSdk, tracerPtr, serviceListener);
        } catch (Exception err) {
            Log.e(TAG, "Error creating RACE app", err);
            throw err;
        }
    }

    private static class UserInputRequest {
        public String pluginId;
        public String key;
        public boolean cache;
    }

    private final RaceSdkApp raceSdk;
    private final IServiceListener serviceListener;
    private final RaceTestApp raceTestApp;
    private final NodeDaemonActionPublisher nodeDaemonActionPublisher;
    private final HashMap<Long, UserInputRequest> userInputRequests = new HashMap<>();
    private boolean initialized = false;

    private AndroidRaceApp(
            Context context,
            long testAppOutputPtr,
            RaceSdkApp raceSdk,
            long tracerPtr,
            IServiceListener serviceListener) {
        super(testAppOutputPtr, raceSdk.getNativePtr(), tracerPtr);

        this.raceSdk = raceSdk;
        this.serviceListener = serviceListener;
        raceTestApp =
                new RaceTestApp(
                        testAppOutputPtr, raceSdk.getNativePtr(), raceAppWrapperPtr, tracerPtr);
        nodeDaemonActionPublisher = new NodeDaemonActionPublisher(context);
    }

    public void initialize() {
        Log.i(TAG, "Initializing RACE");
        initialized = raceSdk.initRaceSystem(raceAppWrapperPtr);
        if (!initialized) {
            Log.e(TAG, "Error initializing RACE SDK");
        } else {
            Log.d(TAG, "RACE SDK has been initialized");
        }
    }

    public boolean isInitialized() {
        return initialized;
    }

    public void sendMessage(String message, String recipient) {
        Log.v(TAG, "sendMessage");
        raceTestApp.sendMessage(message, recipient);
    }

    public void processRaceTestAppCommand(String command) {
        raceTestApp.processRaceTestAppCommand(command);
    }

    @Override
    public void addMessageToUI(JClrMsg jClrMsg) {
        Log.v(TAG, "addMessageToUI");
        serviceListener.onMessageAdded(jClrMsg);
    }

    @Override
    public void handleReceivedMessage(JClrMsg jClrMsg) {
        Log.v(TAG, "handleReceivedMessage");
        addMessageToUI(jClrMsg);
    }

    @Override
    public void onSdkStatusChanged(String jsonStr) {
        Log.d(TAG, "onSdkStatusChanged: " + jsonStr);
        try {
            JSONObject jsonObj = new JSONObject(jsonStr);
            PluginStatus networkManagerStatus =
                    PluginStatus.valueOf(jsonObj.getString("network-manager-status"));

            serviceListener.onRaceStatusChange(networkManagerStatus);
        } catch (JSONException err) {
            Log.e(TAG, "Error parsing SDK status json: " + jsonStr, err);
        }
    }

    @Override
    public SdkResponse requestUserInput(
            RaceHandle handle, String pluginId, String key, String prompt, boolean cache) {
        Log.v(TAG, "requestUserInput");

        UserResponse response = null;
        if (cache) {
            response = getCachedResponse(pluginId, key);
        }
        if (response == null || !response.answered) {
            response = getAutoResponse(pluginId, key);
        }

        if (response.answered) {
            return raceSdk.onUserInputReceived(handle, true, response.response);
        }

        // Save off request params so we can use them again once the user has responded
        UserInputRequest request = new UserInputRequest();
        request.pluginId = pluginId;
        request.key = key;
        request.cache = cache;
        synchronized (userInputRequests) {
            userInputRequests.put(handle.getValue(), request);
        }

        serviceListener.onUserInputRequested(handle, prompt);
        return new SdkResponse(SdkResponse.SdkStatus.SDK_OK, 0.0, new RaceHandle(0));
    }

    void processUserInputResponse(RaceHandle handle, boolean answered, String response) {
        Log.v(TAG, "processUserInputResponse");

        UserInputRequest request;
        synchronized (userInputRequests) {
            request = userInputRequests.remove(handle.getValue());
        }

        if (request != null && request.cache && answered) {
            setCachedResponse(request.pluginId, request.key, response);
        }

        raceSdk.onUserInputReceived(handle, answered, response);
    }

    @Override
    public SdkResponse displayInfoToUser(
            RaceHandle handle, String message, UserDisplayType displayType) {
        serviceListener.onDisplayAlert(handle, message, displayType);
        return new SdkResponse(SdkResponse.SdkStatus.SDK_OK, 0.0, new RaceHandle(0));
    }

    @Override
    public SdkResponse displayBootstrapInfoToUser(
            RaceHandle handle,
            String message,
            UserDisplayType displayType,
            BootstrapActionType bootstrapActionType) {
        nodeDaemonActionPublisher.publishBootstrapAction(message, bootstrapActionType);
        serviceListener.onDisplayBootstrapInfo(handle, message, displayType, bootstrapActionType);
        return new SdkResponse(SdkResponse.SdkStatus.SDK_OK, 0.0, new RaceHandle(0));
    }
}
