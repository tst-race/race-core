
# Table of Contents
- [Table of Contents](#table-of-contents)
- [Plugin Comms Two Six Java](#plugin-comms-two-six-java)
- [Directories](#directories)
  - [Loader](#loader)
  - [Plugin](#plugin)
  - [Test](#test)
  - [Kit](#kit)
- [Building](#building)
- [Output](#output)
- [Running the plugin](#running-the-plugin)
- [Creating your own plugin](#creating-your-own-plugin)

# Plugin Comms Two Six Java

**Plugin type:** Comms
**Performer:** TwoSix Labs
**Variation:** Java

This directory contains source code for a Java implementation of a comms plugin. This plugin contains two channels: direct, and indirect. For more general information about comms plugins, view the Comms Plugin Developer Guide (TODO: link).

# Directories

## Loader

The source directory contains a manifest file for the plugin and a brief cpp file that constructs the java plugin.

## Plugin

The source directory contains the Java source code.

## Test

The test directory contains unit tests for the plugin. The tests can be run using the target `run_plugin_comms_twosix_java_tests` with the top-level cmake.

## Kit

The kit directory contains the artifacts necessary for running the plugin. There are two subdirectories, `channels` and `artifacts`. See the Comms Plugin Developer Guide for more details about the general structure and contents for each one. `channels` contains config generation time artifacts. `artifacts` will contain the java plugin jar and manifest file after building.

# Building

The plugin is meant to be built inside the race-compile image using the top-levels cmake. To build just this plugin the target `PluginCommsTwoSixJava` can be used.

To spin up the container
```bash
./docker_run --pull -c bash
```

Inside the container
```bash
./build.sh -t PluginCommsTwoSixJava
```

Additional targets for formatting are also available. Use `./build.sh -l` to list all targets.

# Output

After building, the kit directory should contain a populated artifacts directory
```
kit/
├── artifacts
│   ├── android-arm64-v8a-client
│   │   └── PluginCommsTwoSixJava
│   │       ├── libPluginCommsTwoSixJava.so
│   │       ├── manifest.json
│   │       └── PluginCommsTwoSixJava.dex
│   ├── android-x86_64-client
│   │   └── PluginCommsTwoSixJava
│   │       ├── libPluginCommsTwoSixJava.so
│   │       ├── manifest.json
│   │       └── PluginCommsTwoSixJava.dex
│   ├── linux-x86_64-client
│   │   └── PluginCommsTwoSixJava
│   │       ├── libPluginCommsTwoSixJava.so
│   │       ├── manifest.json
│   │       └── plugin-comms-twosix-java-1.jar
│   └── linux-x86_64-server
│       └── PluginCommsTwoSixJava
│           ├── libPluginCommsTwoSixJava.so
│           ├── manifest.json
│           └── plugin-comms-twosix-java-1.jar

```

# Running the plugin

See the RiB documentation for running a plugin. To use a locally built plugin, use `--comms-kit local=/code/race-core/plugin-comms-twosix-java/kit     --comms-channel twoSixDirectJava     --comms-channel twoSixIndirectJava`. Replace the path as appropriate based on volume mounts.

# Creating your own plugin

See the Comms Plugin Developer Guide for details about creating your own plugin.
