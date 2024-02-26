# RACE Node Daemon

The RACE Node Daemon is a background application running on each node in a
RACE deployment to perform orchestration tasks as directed by race-in-the-box
(RIB) and to report status of the node and the RACE application on the node.

This repository is broken out into the following sub-projects:

* [race-daemon-sdk](/race-daemon-sdk/README.md) : Core daemon functionality
* [race-daemon-android](/race-daemon-android/README.md) : Android daemon implementation
* [race-daemon-linux](/race-daemon-linux/README.md) : Linux daemon implementation

## How To Build

It is expected that the RACE Node Daemon is built in a container running the
`race-images-base/race-compile` Docker image.

Although gradle is the primary build system for the daemon, it is wrapped by
CMake for compatibility with the RACE global build.

By default, neither the Android nor the Linux app will be built by CMake, so
one or both of `BUILD_ANDROID_DAEMON` or `BUILD_LINUX_DAEMON` must be set to
`ON`.

### A Note About App Signing

The gradle build is configured to use a custom signing key for the debug builds
rather than the ad-hoc debug key that gradle typically uses. This allows for
using the same signing key in Android Studio so the app can be re-installed
without requiring the app to be completely uninstalled (and thus losing any
configs or permissions that might have already been set up).

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
