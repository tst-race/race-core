
# Table of Contents
- [Table of Contents](#table-of-contents)
- [Plugin Comms Two Six Golang](#plugin-comms-two-six-golang)
- [Directories](#directories)
  - [Source](#source)
  - [Test](#test)
  - [Kit](#kit)
- [Building](#building)
- [Output](#output)
- [Running the plugin](#running-the-plugin)
- [Creating your own plugin](#creating-your-own-plugin)

# Plugin Comms Two Six Golang

**Plugin type:** Comms
**Performer:** TwoSix Labs
**Variation:** Golang

This directory contains source code for a Golang implementation of a comms plugin. This plugin contains two channels: direct, and indirect. For more general information about comms plugins, view the Comms Plugin Developer Guide (TODO: link).

# Directories
## Source

The source directory contains the Golang source code and manifest file for the plugin, and a c++ delegator that calls the golang code. The c++ and golang code get compiled together to generate the plugin.

## Test

The test directory contains unit tests for the plugin. The tests can be run using the target `run_plugin_comms_twosix_golang_tests` with the top-level cmake.

## Kit

The kit directory contains the artifacts necessary for running the plugin. There are two subdirectories, `channels` and `artifacts`. See the Comms Plugin Developer Guide for more details about the general structure and contents for each one. `channels` contains config generation time artifacts. `artifacts` will contain the shared library and manifest file after building.

# Building

The plugin is meant to be built inside the race-compile image using the top-levels cmake. To build just this plugin the target `PluginCommsTwoSixGolang` can be used.

To spin up the container
```bash
./docker_run --pull -c bash
```

Inside the container
```bash
./build.sh -t PluginCommsTwoSixGolang
```

Additional targets for formatting are also available. Use `./build.sh -l` to list all targets.

# Output

After building, the kit directory should contain a populated artifacts directory
```
kit/
├── artifacts
│   ├── android-arm64-v8a-client
│   │   └── PluginCommsTwoSixGolang
│   │       ├── libPluginCommsTwoSixGolang.so
│   │       └── manifest.json
│   ├── android-x86_64-client
│   │   └── PluginCommsTwoSixGolang
│   │       ├── libPluginCommsTwoSixGolang.so
│   │       └── manifest.json
│   ├── linux-x86_64-client
│   │   └── PluginCommsTwoSixGolang
│   │       ├── libPluginCommsTwoSixGolang.so
│   │       └── manifest.json
│   └── linux-x86_64-server
│       └── PluginCommsTwoSixGolang
│           ├── libPluginCommsTwoSixGolang.so
│           └── manifest.json
```

# Running the plugin

See the RiB documentation for running a plugin. To use a locally built plugin, use `--comms-kit local=/code/race-core/plugin-comms-twosix-golang/kit     --comms-channel twoSixDirectGolang     --comms-channel twoSixIndirectGolang`. Replace the path as appropriate based on volume mounts.

# Creating your own plugin

See the Comms Plugin Developer Guide for details about creating your own plugin.
