
# Table of Contents
- [Table of Contents](#table-of-contents)
- [Plugin Comms Two Six Decomposed C++](#plugin-comms-two-six-decomposed-c)
- [Directories](#directories)
  - [Source](#source)
  - [Test](#test)
  - [Kit](#kit)
- [Building](#building)
- [Output](#output)
- [Running the plugin](#running-the-plugin)
- [Creating your own plugin](#creating-your-own-plugin)

# Plugin Comms Two Six Decomposed C++

**Plugin type:** Comms
**Performer:** TwoSix Labs
**Variation:** C++

This directory contains source code for a C++ implementation of several decomposed comms components. There are five components: two encodings, one transport and two user models. 

# Directories
## Source

The source directory contains the C++ source code and manifest file for the kit. There are five subdirectories. Four contain plugins and one is common code. The encoding plugin contains two components.

## Test

The test directory contains unit tests for the plugins. The tests can be run using the target `run_plugin_comms_twosix_decomposed_cpp_tests` with the top-level cmake.

## Kit

The kit directory contains the artifacts necessary for running the plugin. There are two subdirectories, `channels` and `artifacts`. See the Comms Plugin Developer Guide for more details about the general structure and contents for each one. `channels` contains config generation time artifacts. Once the plugins have been built, `artifacts` will contain the plugin shared libraries and manifest file.

# Building

The plugin is meant to be built inside the race-compile image using the top-levels cmake. To build just this plugin the target `PluginCommsTwoSixStub` can be used.

To spin up the container
```bash
./docker_run --pull -c bash
```

Inside the container
```bash
./build.sh -t PluginCommsTwoSixStub
```

Additional targets for formatting are also available. Use `./build.sh -l` to list all targets.

# Output

After building, the kit directory should contain a populated artifacts directory
```
kit
├── artifacts
│   ├── android-arm64-v8a-client
│   │   └── PluginCommsTwoSixStubDecomposed
│   │       ├── actions.json
│   │       ├── libPluginCommsTwoSixStubEncoding.so
│   │       ├── libPluginCommsTwoSixStubTransport.so
│   │       ├── libPluginCommsTwoSixStubUserModelFile.so
│   │       ├── libPluginCommsTwoSixStubUserModel.so
│   │       └── manifest.json
│   ├── android-x86_64-client
│   │   └── PluginCommsTwoSixStubDecomposed
│   │       ├── actions.json
│   │       ├── libPluginCommsTwoSixStubEncoding.so
│   │       ├── libPluginCommsTwoSixStubTransport.so
│   │       ├── libPluginCommsTwoSixStubUserModelFile.so
│   │       ├── libPluginCommsTwoSixStubUserModel.so
│   │       └── manifest.json
│   ├── linux-x86_64-client
│   │   └── PluginCommsTwoSixStubDecomposed
│   │       ├── actions.json
│   │       ├── libPluginCommsTwoSixStubEncoding.so
│   │       ├── libPluginCommsTwoSixStubTransport.so
│   │       ├── libPluginCommsTwoSixStubUserModelFile.so
│   │       ├── libPluginCommsTwoSixStubUserModel.so
│   │       └── manifest.json
│   └── linux-x86_64-server
│       └── PluginCommsTwoSixStubDecomposed
│           ├── actions.json
│           ├── libPluginCommsTwoSixStubEncoding.so
│           ├── libPluginCommsTwoSixStubTransport.so
│           ├── libPluginCommsTwoSixStubUserModelFile.so
│           ├── libPluginCommsTwoSixStubUserModel.so
│           └── manifest.json

```

# Running the plugin

See the RiB documentation for running a plugin. To use a locally built plugin, use `--comms-kit local=/code/race-core/plugin-comms-twosix-decomposed-cpp/kit     --comms-channel twoSixIndirectCompositionCpp`. Replace the path as appropriate based on volume mounts.

# Creating your own plugin

See the Comms Plugin Developer Guide for details about creating your own plugin.
