# Network Manager Two Six Labs C++ Plugin

The Network Manager Two Six Labs C++ Plugin is an exemplar implementation of a RACE network manager

## Required Files for the Plugin

The following files are required for all Network Manager Plugins

### config-generator/generate_configs.sh

Generate the plugin configs based on a provided range config and save config files to the specified config directory.

Expected Arguments

    * --range - Range config of the physical network
    * --config-dir - Where should configs be stored. defaults to ./configs
    * --overwrite - Overwrite configs if they exist
    * --local - local indicates that the deployment will run locally and not on the range. depending on implementation, this can result in different configuration values for network manager/comms plugin.

Example:
```bash
/generate_configs.sh \
	--range=/configs/range-configs/2x4-example.json \
	--config-dir=./configs/2x4-s2s-multicast/ \
	--overwrite 
$ tree -v configs/
.
└── 2x4-s2s-multicast
    ├── fulfilled-network-manager-request.json
    ├── link-profiles.json
    ├── range-config.json
    └── network-manager-request.json
```

Notes: 

* Configs generated in the output path will vary on implementation. the shown files are for this exemplar channel. 
* We do recommend storing the initial range-config.json files with the configs for debugging and historical context. 
* network-manager-request.json is required to be generated for network manager plugins to request links from comms channels them what links were created (not all requsted links are expected to be filled by every request)
* Script needs to be executable from any directory
