**This is a RACE Binary Repository**

**RACE binary type:** Android Client
**Performer:** TwoSix Labs
**Variant:** Android (Java)

## Overview

The RACE apk is built by TwoSix and installed in the race-android-runtime-base image provided to 
performers. If performers need additional dependencies or changes made to the RACE app, please reach
out to TwoSix.

## Building

The RACE apk can be built in either Android Studio or in the race-android-compile-plugin image. 
Building through Android Studio can be easier for rapid testing but building through compile base is
safer to ensure you have the most recent RACE libraries.

### A Note About App Signing

The gradle build is configured to use a custom signing key for the debug builds
rather than the ad-hoc debug key that gradle typically uses. This allows for
using the same signing key in Android Studio so the app can be re-installed
without requiring the app to be completely uninstalled (and thus losing any
configs that might have already been set up).

This repo contains the `debug.keystore` file that was created with the following
command (using "android" as the password):

```
# keytool -genkey -v -keystore debug.keystore -keyalg RSA -keysize 2048 -validity 1000 -alias androiddebugkey
Enter keystore password:
Re-enter new password:
What is your first and last name?
  [Android Debug]:  Android Debug
What is the name of your organizational unit?
  [Android]:  Android
What is the name of your organization?
  [US]:  Android
What is the name of your City or Locality?
  [US]:  US
What is the name of your State or Province?
  [US]:  US
What is the two-letter country code for this unit?
  [US]:  US
Is CN=Android Debug, OU=Android, O=Android, L=US, ST=US, C=US correct?
  [no]:  yes

Generating 2,048 bit RSA key pair and self-signed certificate (SHA256withRSA) with a validity of 10,000 days
        for: CN=Android Debug, OU=Android, O=Android, L=US, ST=US, C=US
Enter key password for <androiddebugkey>
        (RETURN if same as keystore password):
[Storing debug.keystore]
```

This ensures the keystore is compatible with Android Studio when it creates a
debug build of the app. Copy the `debug.keystore` file to
`~/.android/debug.keystore` to make it available to Android Studio.

### Building in docker container:
To build in a docker container mount the repo into a container based on race-android-compile-plugin. Copy all cross compiled libraries from `/android/x86_64/lib/` to `app/src/main/jniLibs/x86_64/`. Copy jars into `app/libs/`. Then run `./gradlew clean assembleDebug` to produce `app/build/outputs/apk/debug/app-debug.apk`. This apk should be renmamed to `race.apk` if you want to install it in the android runtime image.

There is a script that will do this all for you.
```
bash scripts/build.sh
```

### Building in Android Studio:
Import this repository into Android Studio with SDK 29 installed. Next, add all cross-compiled so's to `./app/src/main/jniLibs/x86_64`. Next add racesdk-java-shims.jar and json-simple-1.1.1.jar to `./app/libs/`. Lastly, hit `Build > Build Bundle(s) / APK(s) > Build APK(s)`. APK will be located in `./app/build/outputs/apk/debug`.

## Running

There are two ways to test the apk, in a RiB deployment or in a local Android Emulator.

### Running in RiB deployment:
The RACE apk is installed in the race-android-runtime-base image at /android/x86_64/. The emulator is automatically started and the apk is automatically installed when the container is started. To test in an actual deployment, it is easiest to:

```
docker run --name test-apk race-android-runtime-base
docker cp race.apk test-apk:/android/x86_64/
docker commit test-apk race-android-runtime-base:test-apk
docker kill test-apk
```

You can then run then create a rib deployment with that image
```
rib:1.0.0@code# rib deployment create --linux-client-count=2 --android-client-count=1 --linux-server-count=3 --android-client-image=race-android-runtime-base:test-apk --name=android-deployment-1.0.0
```
After this point you may interact with the deployment like any other local deployment.


### Running in Android Studio:

Create a emulator with API level 29 and CPU/ABI x86_64. Hit "Run 'app'" to start the emulator, install the app, and start the app. The first time you do this the app will fail because the configs and plugins aren't copied onto the device. With the emulator still running, open a terminal and copy the configs and plugins into the app's directory: 

```
# copy configs (see below for config_directory structure)
adb push {config_directory} /storage/self/primary
adb shell
run-as com.twosix.race
cp -r /storage/self/primary/{config_directory}/* ./
```

## Config Directory Structure

```
.
├── configs
│   ├─── global
|   ├─── network-manager
|   |   └── {network-manager-plugin-dir}
|   ├─── comms
|   |   └── {comms-plugin-dir}
|   ├─── whiteboard
|   └── race-config.json
├── race
│   ├── network-manager
|   |   └── {network-manager-plugin.so}
|   └── comms
|       └── {comms-plugin.so}
```

## ADB Test App Commands

```
# Start App
adb shell am start -n "com.twosix.race/.MainActivity"

# Send Manual Message
adb shell am broadcast -a com.twosix.intent.SEND --es sendType "manual" --es receiver "{receiver}" --es message "{message}"

# Send Auto Message
adb shell am broadcast -a com.twosix.intent.SEND --es sendType "auto" --es receiver "{receiver}" --ei period "{period}" --ei size "{size}" --ei quantity "{quantity}"

# Stop App
adb shell am broadcast -a com.twosix.intent.STOP

# Clear Messages
adb shell am broadcast -a com.twosix.intent.ClearMessages

```
