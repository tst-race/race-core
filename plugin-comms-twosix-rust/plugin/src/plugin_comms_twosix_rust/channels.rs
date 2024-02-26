//
// Copyright 2023 Two Six Technologies
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

extern crate shims;
use shims::race_common::channel_properties::ChannelProperties;

use shims::race_common::i_race_sdk_comms::IRaceSdkComms;
use shims::race_common::link_properties::LinkProperties;
use shims::race_common::link_properties::LinkPropertySet;
use shims::race_common::link_properties::LinkType;

pub const DIRECT_CHANNEL_GID: &str = "twoSixDirectRust";
pub const INDIRECT_CHANNEL_GID: &str = "twoSixIndirectRust";

pub fn get_default_channel_properties_for_channel(
    sdk: &dyn IRaceSdkComms,
    channel_gid: &str,
) -> Result<ChannelProperties, &'static str> {
    let props = sdk.get_channel_properties(channel_gid);
    return Ok(props);
}

pub fn get_default_link_properties_for_channel(
    sdk: &dyn IRaceSdkComms,
    channel_gid: &str,
) -> Result<LinkProperties, &'static str> {
    let mut props = LinkProperties::default();
    if channel_gid == DIRECT_CHANNEL_GID {
        let channel_props = match get_default_channel_properties_for_channel(sdk, channel_gid) {
            Ok(props) => props,
            Err(e) => return Err(e),
        };

        props.transmission_type = channel_props.transmission_type;
        props.connection_type = channel_props.connection_type;
        props.send_type = channel_props.send_type;
        props.reliable = channel_props.reliable;
        props.is_flushable = channel_props.is_flushable;
        props.duration_s = channel_props.duration_s;
        props.period_s = channel_props.period_s;
        props.mtu = channel_props.mtu;

        let mut worst_link_prop_set = LinkPropertySet::default();
        worst_link_prop_set.bandwidth_bps = 23130000;
        worst_link_prop_set.latency_ms = 17;
        worst_link_prop_set.loss = -1.0;
        props.worst.send = worst_link_prop_set;
        props.worst.receive = worst_link_prop_set;

        props.expected = channel_props.creator_expected;

        let mut best_link_prop_set = LinkPropertySet::default();
        best_link_prop_set.bandwidth_bps = 28270000;
        best_link_prop_set.latency_ms = 14;
        best_link_prop_set.loss = -1.0;
        props.best.send = best_link_prop_set;
        props.best.receive = best_link_prop_set;

        props.supported_hints = channel_props.supported_hints;
        props.channel_gid = channel_gid.to_string();

        return Ok(props);
    } else if channel_gid == INDIRECT_CHANNEL_GID {
        let channel_props = match get_default_channel_properties_for_channel(sdk, channel_gid) {
            Ok(props) => props,
            Err(e) => return Err(e),
        };

        props.link_type = LinkType::LtBidi;
        props.transmission_type = channel_props.transmission_type;
        props.connection_type = channel_props.connection_type;
        props.send_type = channel_props.send_type;
        props.reliable = channel_props.reliable;
        props.is_flushable = channel_props.is_flushable;
        props.duration_s = channel_props.duration_s;
        props.period_s = channel_props.period_s;
        props.mtu = channel_props.mtu;

        let mut worst_link_prop_set = LinkPropertySet::default();
        worst_link_prop_set.bandwidth_bps = 277200;
        worst_link_prop_set.latency_ms = 3190;
        worst_link_prop_set.loss = 0.1;
        props.worst.send = worst_link_prop_set;
        props.worst.receive = worst_link_prop_set;

        props.expected = channel_props.creator_expected;

        let mut best_link_prop_set = LinkPropertySet::default();
        best_link_prop_set.bandwidth_bps = 338800;
        best_link_prop_set.latency_ms = 2610;
        best_link_prop_set.loss = 0.1;
        props.best.send = best_link_prop_set;
        props.best.receive = best_link_prop_set;

        props.supported_hints = channel_props.supported_hints;
        props.channel_gid = channel_gid.to_string();

        return Ok(props);
    }

    return Err("invalid channel GID");
}
