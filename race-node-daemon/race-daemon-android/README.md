# Android RACE Node Daemon

The Android RACE Node Daemon allows for remote control of the RACE application
on emulated and physical Android devices.

## Table of Contents

* [Capabilities](#capabilities)
    * [Persona](#persona)
    * [App Status](#app-status)
    * [Node Status](#node-status)
    * [Node Actions](#node-actions)
* [Limitations](#limitations)
* [Implementation Details](#implementation-details)

## Capabilities

### Persona

The daemon has several methods of determining the appropriate RACE persona for
the node.

When the daemon's main activity is launched, if the intent contains an extra
`persona` property then it will use the provided persona. Otherwise it will
look for the device property `debug.RACE_PERSONA`. Lastly, it will prompt the
user to enter the persona.

### App Status

The daemon will register a broadcast intent receiver in order to receive app
status report updates from the RACE client app. The received status update is
forwarded to the daemon SDK to be published.

### Node Status

The daemon will publish a node status report containing the following
information:

* The timestamp indicating the time at which the status report was created
* The persona of the node to which the status belongs
* Whether the RACE app is installed & configured on the node

This status report will be published every 10 seconds with a expiration time of
30 seconds.

### Node Actions

The following node actions are handled by the daemon. All other received actions
are forwarded to the RACE application by sending a broadcast intent.

#### Start

The daemon app will fetch configs for the RACE application from the RiB file
server and place them in the external shared `Download` folder where the RACE
client application will be looking for them.

After setting up the configs, a launch intent is sent to start the RACE
application.

#### Kill

A broadcast intent is sent to the RACE app to instruct it to forcibly kill
itself. We do this because the daemon app will not have the correct permission
to kill another app directly.

#### Log Rotation

The log rotation node action may result in a backup of the log files, deletion
of the log files, or both.

If instructed to perform a backup of log files, the daemon will produce a
gzip'd tar archive of the RACE application's log directory, which is in an
app-specific external storage location. This backup archive is then uploaded
to the RiB file server.

If instructed to delete the log files, all contents of the RACE application's
log directory are deleted. Additionally, a broadcast intent is sent to the RACE
app to instruct it to clear the message database.

#### Bootstrap

The bootstrap node action results in the installation of the RACE app onto the
node along with all configs necessary for it to run and join the RACE network.

The daemon will first fetch the configs from the RiB file server in order to
obtain the `user-responses.json` file for simulated user input response. This
file is placed in the external shared `Download` folder where the RACE client
application will be looking for it.

Then the daemon will fetch the bootstrap bundle from the introducer node as
specified in the received bootstrap action properties. The bundle will be
extracted and the configs and plugins will be placed in the correct locations
in the external shared `Download` folder where the RACE client application will
be looking for them.

Lastly, an activity intent is sent to prompt the user to install the RACE
application. The user must manually go through the installation procedure and
start the RACE app (see limitations).

## Limitations

While the daemon has several mechanisms for specifying the node persona, the
RACE client application currently only supports being set by device property.
Thus it is possible for the daemon and the client app to use different personas.
Eventually, the client app should use user input for the persona, and the daemon
can ensure the simulated user responses file contains the correct persona.

The daemon application must be explicitly granted read/write permission to
external storage. This can be done automatically when installing the app with
adb by using `adb install -g -t race-daemon-android-debug.apk`. If the daemon
is installed manually, then the permissions must be granted when prompted by the
app.

The daemon is only able to silently install the RACE client application if the
daemon is set as the device owner using
`adb shell dpm set-device-admin com.twosix.race.daemon/.AdminReceiver`.
Additionally, Play Protect must be disabled with
`adb shell settings put global package_verifier_user_consent -1`. If not set
as the device admin, the user will be prompted to confirm the install, bypass
Play Protect, start the app, and grant permissions to read/write external
storage.

## Implementation Details

The daemon is built as an Android application and produces an `apk`.

The daemon runs as a foreground service in order to keep the daemon running
while the RACE client app is in the foreground.

All communication to and from the RACE client app is through intents.

The daemon registers a broadcast receiver in order to receive application status
updates from the RACE client app.
