# RACE Node Daemon SDK

The RACE Node Daemon SDK provides core cross-platform functionality for the
RACE Node Daemon applications.

## Table of Contents

* [Capabilities](#capabilities)
* [APIs](apis)
* [Example Usage](#example-usage)
* [Data Structures](#data-structures)
    * [Actions](#actions)
    * [Node Status](#node-status)
    * [App Status](#app-status)
* [Implementation Details](#implementation-details)

## Capabilities

The SDK provides the following functional capabilities:

* Abstraction of communication with RiB
  * Currently uses Redis, but could change in future releases
* APIs to report node and application status
* APIs to respond to orchestration actions
* APIs to fetch remote files (via HTTP)
* APIs for file operations

## APIs

The following classes contain all the public APIs of the RACE Node
Daemon SDK:

* [IRaceNodeDaemonSdk](src/main/java/com/twosix/race/daemon/sdk/IRaceNodeDaemonSdk.java) : The interface for the SDK
* [IRaceNodeActionListener](src/main/java/com/twosix/race/daemon/sdk/IRaceNodeActionListener.java) : The interface for action listeners
* [RaceNodeDaemonConfig](src/main/java/com/twosix/race/daemon/sdk/IRaceNodeActionListener.java) : The SDK configuration object
* [RaceNodeDaemonSdk](src/main/java/com/twosix/race/daemon/sdk/IRaceNodeActionListener.java) : The implementation of the SDK
* [FileUtils](src/main/java/com/twosix/race/daemon/sdk/FileUtils.java) : File operation utilities

## Example Usage

```java
package com.twosix.race.daemon;

import com.twosix.race.daemon.sdk.IRaceNodeDaemonSdk;
import com.twosix.race.daemon.sdk.RaceNodeDaemonConfig;
import com.twosix.race.daemon.sdk.RaceNodeDaemonSdk;

public class DaemonExample {
    public static int main(String[] args) {
        // 1. Configure the SDK
        RaceNodeDaemonConfig config = new RaceNodeDaemonConfig();
        config.setPersona("race-client-00001");

        // 2. Create the SDK
        IRaceNodeDaemonSdk sdk = new RaceNodeDaemonSdk(config);

        // 3. Register an action listener
        sdk.registerActionListener((String jsonAction) -> {
            System.out.println("Received action: " + jsonAction);
        });

        // 4. Report node status (TTL of 15 seconds)
        sdk.updateNodeStatus(
            "{\"timestamp\": \"2021-11-24T14:03:01\"}", 15);

        // 5. Report app status (TTL of 10 seconds)
        sdk.updateAppStatus(
            "{\"timestamp\": \"2021-11-24T14:03:01\"}", 10);
    }
}
```

## Data Structures

All data sent and received from RiB is in a JSON format, in order to be
the most flexible and allow for opaque data processing by the daemon. That
is, most changes to the payloads will only affect the RACE apps and RiB,
without any change to the daemon.

***Note:*** All JSON data structures must be coordinated between RiB and
the other party (daemon, RACE app, RACE SDK).

### Actions

The basic JSON structure for orchestration actions originating from RiB will
follow the format:

```json
{
    "type": "action-type",
    "payload": {}
}
```

Where `type` is the name of the action to be performed. Some actions will
be performed by the daemon, while others will be forwarded to the RACE app.

The `payload` is specific to the action, and contains any parameters required
to perform the action. For example:

```json
{
    "type": "send-message",
    "payload": {
        "send-type": "manual",
        "recipient": "race-client-00002",
        "message": "Hello there!",
        "test-id": "",
        "network-manager-bypass-route": ""
    }
}
```

### Node Status

The basic JSON structure for RACE node status includes:

* The timestamp indicating the time at which the status report was created
* The persona of the node to which the status belongs
* Whether the RACE app is installed & configured on the node

The properties in the node status report may be expanded to include other
node information as needed in the future.

Example:

```json
{
    "timestamp": "2021-11-24T14:03:01",
    "persona": "race-client-00001",
    "installed": true
}
```

### App Status

The basic JSON structure for RACE app status includes:

* The timestamp indicating the time at which the status report was created

The properties in the app status report may be expanded to include other
app information as needed in the future.

Example:

```json
{
    "timestamp": "2021-11-24T14:03:01"
}
```

## Implementation Details

The current implementation of communication between the daemon and RiB is
through Redis.

Actions are published by RiB to a node-specific channel, where the daemon SDK
has registered a listener. Upon receiving a message from Redis, the SDK forwards
the received message to the `IRaceNodeActionListener` that was registered with
the SDK.

The node-specific channel is named `race.node.actions:<persona>`.

When the daemon uses an SDK method to report status (node and app), two entries
are set in Redis. One is the unaltered JSON status report, set to a node-specific
key. The other is a node-specific is-alive set to a boolean `true` with an
expiration. If the app or node dies, the is-alive key will be deleted by Redis
after the expiration time. This way, RiB can always retrieve the last-reported
status from the status key, but also know that a node or app has died by
checking for the is-alive key.

The node-specific keys are:

* `race.node.status:<persona>`
* `race.node.is.alive:<persona>`
* `race.app.status:<persona>`
* `race.app.is.alive:<persona>`
