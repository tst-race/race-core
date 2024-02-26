# RACE SDK CORE

**Plugin type:** Core SDK  
**Performer:** TwoSix Labs  
**Variation:** Core

## Build

It is expected that the RACE SDK is built using the top-level cmake in a container running the
`race-images/race-compile` Docker image.

To spin up the container
```bash
./docker_run --pull -c bash
```

Inside the container
```bash
./build.sh -t raceSdkCore
```

## Tests

Inside the container
```bash
./build.sh -t run_racesdk_tests
```

## Global Configuration

`racesdk-core` has various golbal configuration options that can be set by a config file. In a RiB deployment this file will be located at `.race/rib/deployments/local/${DEPLOYMENT_NAME}/config/global/race.json`. A sample configuration may look like this:

```json
{
    "bandwidth": "-1",
    "debug": "false",
    "plugins": [
        {
            "file_path": "libPluginNMClientTwoSixStub.so",
            "plugin_type": "network-manager",
            "file_type": "shared_library",
            "node_type": "client",
            "config_path": "PluginNMTwoSixStub/"
        },
        {
            "file_path": "libPluginNMServerTwoSixStub.so",
            "plugin_type": "network-manager",
            "file_type": "shared_library",
            "node_type": "server",
            "config_path": "PluginNMTwoSixStub/"
        },
        {
            "file_path": "libPluginCommsTwoSixStub.so",
            "plugin_type": "comms",
            "file_type": "shared_library",
            "node_type": "any",
            "config_path": "PluginCommsTwoSixStub/"
        },
        {
            "file_path": "PluginCommsTwoSixPython",
            "plugin_type": "comms",
            "file_type": "python",
            "node_type": "any",
            "python_module": "PluginCommsTwoSixPython.PluginCommsTwoSixPython",
            "python_class": "PluginCommsTwoSixPython",
            "config_path": "PluginCommsTwoSixPython/"
        }
    ]
}
```

### `plugins` Section

The `plugins` section is used to specify the plugins to load. Note that this section is optional. If the section is omitted then the default plugin location will be searched and the first network manager plugin found and all the comms plugins found will be loaded.

The `plugins` section is a json array of objects. Each object represents one plugin to be loaded. The plugin object keys are described here.

|key|value||
|:-|:-|:-|
|`file_path`|The path to a plugin shared object or the Python plugin package name. Note that this path is relative to the default plugin path (e.g. <code>/usr/local/lib/race/ta[1\|2]/</code> in a linux container). An absolute path cannot be used.|Required|
|`plugin_type`|Either `"network-manager"` or `"comms"`.|Required|
|`file_type`|The file type of the plugin, currently either `"shared_library"` or `"python"`.|Required|
|`node_type`|The type of RACE node, one of `"client"`, `"server"`, or `"any"`.|Required|
|`python_module`|The name of your Python plugin module in the format `<PACKAGE_NAME>.<MODULE_NAME>`.|Required if `file_type` is `"python"`|
|`python_class`|The name of your Python plugin class with your Python module.|Required if `file_type` is `"python"`||
|`config_path`|The path to your plugin's configuration files. Note that this path is relative to the default configuration path (e.g. `/config/` in a linux container). If this key is omitted it will default to use your plugin ID.|Optional|
