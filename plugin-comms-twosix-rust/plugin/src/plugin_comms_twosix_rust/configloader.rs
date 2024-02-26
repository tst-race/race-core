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

extern crate serde_json;
use shims::race_common::link_properties::LinkProperties;
use shims::race_common::link_properties::LinkPropertyPair;
use shims::race_common::link_properties::LinkPropertySet;
use shims::race_common::link_properties::{ConnectionType, LinkType, TransmissionType};
use shims::race_common::send_type::SendType;
use std::convert::TryFrom;

fn int_or(json: &serde_json::Map<String, serde_json::Value>, field: &str, default: i32) -> i32 {
    return json
        .get(field)
        .and_then(|v| v.as_i64())
        .and_then(|v| i32::try_from(v).ok())
        .unwrap_or(default);
}

fn float_or(json: &serde_json::Map<String, serde_json::Value>, field: &str, default: f32) -> f32 {
    return json
        .get(field)
        .and_then(|v| v.as_i64())
        .and_then(|v| Some(v as f32))
        .unwrap_or(default);
}

fn bool_or(json: &serde_json::Map<String, serde_json::Value>, field: &str, default: bool) -> bool {
    return json.get(field).and_then(|v| v.as_bool()).unwrap_or(default);
}

fn array_str_or(json: &serde_json::Map<String, serde_json::Value>, field: &str) -> Vec<String> {
    let hints_json = json.get(field).and_then(|v| v.as_array());
    return match hints_json {
        None => Vec::new(),
        Some(vec_json) => vec_json
            .iter()
            .map(|json| json.as_str().unwrap().to_string())
            .collect(),
    };
}

///
/// ConfigLoader ...
///
pub struct ConfigLoader {}

impl ConfigLoader {
    pub fn parse_link_properties(
        json: &serde_json::Map<String, serde_json::Value>,
    ) -> LinkProperties {
        let link_type = match json["type"].as_str().unwrap() {
            "send" => LinkType::LtSend,
            "receive" => LinkType::LtRecv,
            "bidirectional" => LinkType::LtBidi,
            _ => LinkType::LtUndef,
        };

        let props = LinkProperties::new(
            link_type,
            TransmissionType::TtUndef,
            ConnectionType::CtUndef,
            SendType::StUndef,
            bool_or(json, "reliable", false),
            bool_or(json, "isFlushable", false),
            int_or(json, "duration_s", -1),
            int_or(json, "period_s", -1),
            int_or(json, "mtu", -1),
            ConfigLoader::parse_link_property_pair(json, "worst"),
            ConfigLoader::parse_link_property_pair(json, "best"),
            ConfigLoader::parse_link_property_pair(json, "expected"),
            array_str_or(json, "supported_hints"),
            String::new(),
            String::new(),
        );
        return props;
    }

    fn parse_link_property_pair(
        json: &serde_json::Map<String, serde_json::Value>,
        name: &str,
    ) -> LinkPropertyPair {
        return match json.get(name).and_then(|subjson| subjson.as_object()) {
            Some(obj) => LinkPropertyPair {
                send: ConfigLoader::parse_link_property_set(obj, "send"),
                receive: ConfigLoader::parse_link_property_set(obj, "receive"),
            },
            None => LinkPropertyPair::default(),
        };
    }

    fn parse_link_property_set(
        json: &serde_json::Map<String, serde_json::Value>,
        name: &str,
    ) -> LinkPropertySet {
        return match json.get(name).and_then(|subjson| subjson.as_object()) {
            Some(obj) => LinkPropertySet {
                bandwidth_bps: int_or(obj, "bandwidth_bps", -1),
                latency_ms: int_or(obj, "latency_ms", -1),
                loss: float_or(obj, "loss", -1.0),
            },
            None => LinkPropertySet::default(),
        };
    }
}
