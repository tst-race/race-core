# Comms Two Six Labs C++ Plugin

The Comms Two Six Labs C++ Plugin is an exemplar implementation of Comms channels. There are two channels, one direct and one indirect, that provide communication for the RACE network.

## Channels

* twoSixDirectCpp
* twoSixIndirectCpp
* twoSixBootstrapCpp
* twoSixIndirectBootstrapCpp

## Required Files per Channel

The following files are required for all Comms Channels

### channel_properties.json

Define the properties for the channel as defined in the channel properties documentation.

Properties should be similar to what is expected when a network manager plugin goes to utilize the link.

### generate_configs.sh

Generate the plugin configs based on a provided range config and save config files to the specified config directory.

Expected Arguments

    * --range - Range config of the physical network
    * --network-manager-request - Requested links from the network manager. Configs should generate only these links and as many of the links as possible.
    * --config-dir - Where should configs be stored. defaults to ./configs
    * --overwrite - Overwrite configs if they exist
    * --local - Ignore range config service connectivity, utilize configs (e.g. local hostname/port vs range services fields). RiB local will not have the same network connectivty as the T&E range.

Example:
```bash
/generate_configs.sh \
	--range=/configs/range-configs/2x4-example.json \
	--network-manager-request=/configs/network-manager-requests/2x4-s2s-multicast.json \
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
* We do recommend storing the initial network-manager-request.json and range-config.json files with the configs for debugging and historical context. 
* fulfilled-network-manager-request.json will be required to respond to network manager plugins to tell them what links were created (not all requsted links are expected to be filled by every request)

### get_status_of_external_services.sh

Get status of external (not running in a RACE node) services required by channel.

Intent is to ensure that the channel will be functional if a RACE deployment were to be started and connections established

### start_external_services.sh

Start external (not running in a RACE node) services required by channel 

### stop_external_services.sh

Stop external (not running in a RACE node) services required by channel
