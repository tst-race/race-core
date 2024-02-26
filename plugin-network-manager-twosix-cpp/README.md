
# Table of Contents
- [Table of Contents](#table-of-contents)
- [Plugin Network Manager Two Six C++](#plugin-network-manager-two-six-c)
- [Directories](#directories)
  - [Source](#source)
  - [Test](#test)
  - [Scripts](#scripts)
  - [Kit](#kit)
- [Building](#building)
- [Output](#output)
- [Running the plugin](#running-the-plugin)

# Plugin Network Manager Two Six C++

**Plugin type:** Network Manager
**Performer:** Two Six Technologies
**Variation:** C++

This directory contains source code for a C++ implementation of a network manager plugin. For more general information about network manager plugins, view the RACE Developer Guide (TODO: link).

# Directories

## Source

The source directory contains the C++ source code and manifest file for the plugin.

## Test

The test directory contains unit tests for the plugin. The tests can be run using the target `run_plugin_network_manager_twosix_cpp_tests` with the top-level cmake.


## Scripts

The scripts directory contains various scripts relating to the plugin. Namely, a network visualizer exists that can take a saved deployment config and produce a visualization of the network links.

## Kit

The kit directory contains the artifacts necessary for running the plugin. There are two subdirectories, `config-generator` and `artifacts`. See the RACE developer guide for details about both sub-directories. `config-generator` contains config generation time artifacts. `artifacts` will contain the shared library and manifest file after building.

# Building

The plugin is meant to be built inside the race-compile image using the top-levels cmake. To build just this plugin, the targets `PluginNMClientTwoSixStub` and `PluginNMServerTwoSixStub` can be used.


To spin up the container
```bash
./docker_run --pull -c bash
```

Inside the container
```bash
./build.sh -t PluginNMTwoSixStub
```

Additional targets for formatting are also available. Use `./build.sh -l` to list all targets.

# Output

After building, the kit directory should contain a populated artifacts directory
```
kit/
├── artifacts
│   ├── android-arm64-v8a-client
│   │   └── PluginNMTwoSixStub
│   │       ├── libPluginNMClientTwoSixStub.so
│   │       └── manifest.json
│   ├── android-x86_64-client
│   │   └── PluginNMTwoSixStub
│   │       ├── libPluginNMClientTwoSixStub.so
│   │       └── manifest.json
│   ├── linux-x86_64-client
│   │   └── PluginNMTwoSixStub
│   │       ├── libPluginNMClientTwoSixStub.so
│   │       └── manifest.json
│   └── linux-x86_64-server
│       └── PluginNMTwoSixStub
│           ├── libPluginNMServerTwoSixStub.so
│           └── manifest.json
```

# Running the plugin

See the RiB documentation for running a plugin. To use a locally built plugin, use `--comms-kit local=/code/race-core/plugin-network-manager-twosix-cpp/kit     --network-manager PluginNMTwoSixStub`. Replace the path as appropriate based on volume mounts.
