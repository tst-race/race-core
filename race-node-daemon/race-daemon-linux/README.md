# Linux RACE Node Daemon

The Linux RACE Node Daemon allows for remote control of the RACE application
on Linux client and server nodes.

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

The daemon requires the RACE persona to be set with the `RACE_PERSONA`
environment variable.

### App Status

The daemon will receive app status report updates from the RACE app through the
FIFO pipe. The received status update is forwarded to the daemon SDK to be
published.

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
are forwarded to the RACE application through the FIFO pipe.

#### Start

The daemon will run the `racetestapp` executable as a child process.

#### Kill

The daemon will forcibly kill the child RACE application process.

#### Log Rotation

The log rotation node action may result in a backup of the log files, deletion
of the log files, or both.

If instructed to perform a backup of log files, the daemon will produce a
gzip'd tar archive of the RACE application's log directory, which is in `/log`.
This backup archive is then uploaded to the RiB file server.

If instructed to delete the log files, all contents of the RACE application's
log directory are deleted.

#### Bootstrap

The bootstrap node action results in the installation of the RACE app onto the
node along with all configs necessary for it to run and join the RACE network.

The daemon will fetch the bootstrap bundle from the introducer node as
specified in the received bootstrap action properties. The bundle will be
extracted and the installation script contained in the bootstrap bundle will be
executed. This script will copy all configs, plugins, and executables into the
required locations.

Then, the daemon will run the `racetestapp` executable as a child process.

## Limitations

None identified at this time.

## Implementation Details

The daemon is built as an Java application and produces a `race-daemon-linux`
script to run the daemon application.

All communication to and from the RACE app is through FIFO files, or named
pipes. One FIFO pipe is created for communication from the daemon to the RACE
app (`/tmp/racetestapp-input`) and one for communication from the RACE app to
the daemon (`/tmp/racetestapp-output`). The daemon is responsible for creating
these FIFO files before starting the RACE application.
