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
import android.content.pm.PackageManager.NameNotFoundException;
import android.util.Log;

import ShimsJava.AppConfig;
import ShimsJava.NodeType;
import ShimsJava.RaceSdkApp;
import ShimsJava.StorageEncryptionInvalidPassphraseException;
import ShimsJava.StorageEncryptionType;

import com.twosix.race.core.Constants;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;

/** Factory for creating RACE SDK instances */
public abstract class RaceSdkAppFactory {

    private static final String TAG = "RaceSdkAppFactory";

    // "lib" is automatically prepended and ".so" is automatically appended
    // Order is important here! Lib dependencies must be loaded before the lib
    // that depends on them (e.g., crypto must be loaded before ssl).
    private static final String[] NATIVE_LIBS = {
        "yaml-cpp",
        "stdc++",
        "c++_shared",
        "boost_system",
        "boost_filesystem",
        "ffi",
        "python3.7m",
        "crypto",
        "ssl",
        "thrift",
        "zip",
        "archive",
        "raceSdkCommon",
        "raceSdkStorageEncryption",
        "raceSdkCore",
        "RaceJavaShims",
        "raceTestAppShared"
    };

    public static RaceSdkApp createInstance(Context context, String persona, String passphrase) {
        try {
            loadNativeLibs();
            AppConfig appConfig = createAppConfig(context, persona);
            return new RaceSdkApp(appConfig, passphrase);
        } catch (StorageEncryptionInvalidPassphraseException err) {
            Log.e(TAG, "invalid passphrase given to RACE SDK", err);
            throw err;
        } catch (Exception err) {
            Log.e(TAG, "Error creating RACE SDK: " + err.getClass().getName(), err);
        }

        return null;
    }

    private static void loadNativeLibs() {
        for (String lib : NATIVE_LIBS) {
            try {
                Log.v(TAG, "Loading " + lib);
                System.loadLibrary(lib);
                Log.d(TAG, "Loaded " + lib);
            } catch (Error err) {
                Log.e(TAG, "Error loading native lib " + lib, err);
                throw err;
            }
        }
    }

    private static AppConfig createAppConfig(Context context, String persona)
            throws NameNotFoundException {
        AppConfig appConfig = AppConfig.create();

        appConfig.nodeType = NodeType.NT_CLIENT;
        appConfig.persona = persona;
        appConfig.environment = "phone"; // This is the only valid value for Android clients
        appConfig.appDir = context.getDataDir().getAbsolutePath();
        appConfig.sdkFilePath = "sdk";
        appConfig.baseConfigPath =
                context.getExternalFilesDir("race").getAbsolutePath() + "/data/configs";
        appConfig.pluginArtifactsBaseDir = appConfig.appDir + "/race/artifacts";
        appConfig.userResponsesFilePath = Constants.PATH_TO_USER_RESPONSES;
        appConfig.tmpDirectory = appConfig.appDir + "/cache";
        appConfig.logDirectory = context.getExternalFilesDir("logs").getAbsolutePath();
        appConfig.logFilePath = appConfig.logDirectory + "/race.log";
        appConfig.configTarPath =
                context.getExternalFilesDir("race").getAbsolutePath() + "/configs.tar.gz";
        appConfig.encryptionType = getEncryptionType();
        appConfig.appPath =
                context.getPackageManager().getApplicationInfo("com.twosix.race", 0).sourceDir;

        return appConfig;
    }

    private static StorageEncryptionType getEncryptionType() {
        final String encTypeEnvVarName = "debug.RACE_ENCRYPTION_TYPE";
        try {
            Process process = Runtime.getRuntime().exec("/system/bin/getprop " + encTypeEnvVarName);
            InputStream stdin = process.getInputStream();
            InputStreamReader isr = new InputStreamReader(stdin);
            BufferedReader br = new BufferedReader(isr);
            String encryptionType = br.readLine();
            if (encryptionType.equals("ENC_NONE")) {
                return StorageEncryptionType.ENC_NONE;
            } else if (encryptionType.equals("ENC_AES")) {
                return StorageEncryptionType.ENC_AES;
            } else {
                throw new Exception("Invalid encryption type: " + encryptionType);
            }
        } catch (Exception err) {
            Log.w(
                    TAG,
                    "Failed to read environment variable "
                            + encTypeEnvVarName
                            + ". Using default encryption type: "
                            + StorageEncryptionType.ENC_AES.name()
                            + ". "
                            + err.getMessage());
            return StorageEncryptionType.ENC_AES;
        }
    }
}
