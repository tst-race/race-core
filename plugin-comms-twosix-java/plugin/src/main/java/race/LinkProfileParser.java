
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

package race;

import ShimsJava.JLinkConfig;
import ShimsJava.LinkPropertyPair;
import ShimsJava.LinkPropertySet;
import ShimsJava.RaceLog;

import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;

public abstract class LinkProfileParser {

    static final String logLabel = "race.LinkProfileParser";
    static final String twosixWhiteboardServiceName = "twosix-whiteboard";

    /**
     * Parses the given link configuration and creates a parser/factory according to the described
     * link.
     *
     * @param linkProfile Link profile from JSON
     * @param isDirect Expect direct links from link profile
     * @return LinkProfileParser or null if not a recognized link configuration
     */
    static LinkProfileParser parse(String linkProfile, boolean isDirect) {
        RaceLog.logDebug(logLabel, "parse called", "");
        JSONParser parser = new JSONParser();
        JSONObject linkProfileJson = null;
        try {
            linkProfileJson = (JSONObject) parser.parse(linkProfile);
        } catch (Exception e) {
            RaceLog.logError(logLabel, "Invalid link profile (malformed JSON): " + linkProfile, "");
            return null;
        }

        boolean multicast = false;
        if (linkProfileJson.get("multicast") != null) {
            multicast = (boolean) linkProfileJson.get("multicast");
        }
        String serviceName = "";
        if (linkProfileJson.get("service_name") != null) {
            serviceName = (String) linkProfileJson.get("service_name");
        }

        if (!multicast) {
            if (isDirect) {
                return new DirectLinkProfileParser(linkProfile);
            }
            RaceLog.logError(
                    logLabel, "Unicast link configuration found in indirect channel config", "");
            return null;
        } else if (serviceName.compareTo(twosixWhiteboardServiceName) == 0) {
            if (!isDirect) {
                return new TwosixWhiteboardLinkProfileParser(linkProfile);
            }
            RaceLog.logError(
                    logLabel, "Multicast link configuration found in direct channel config", "");
            return null;
        } else {
            RaceLog.logError(logLabel, "Unkown service: " + serviceName, "");
            return null;
        }
    }

    /**
     * Parse LinkPropery Pair from JsonObject read from link-profile.json. If fields are missing
     * deafault values will be used.
     *
     * @param jsonLinkPropertyPair json representation of LinkPropertyPair
     * @return LinkPropertyPair
     */
    public static LinkPropertyPair parseLinkPropertyPair(JSONObject jsonLinkPropertyPair) {
        LinkPropertyPair propPair = new LinkPropertyPair();
        if (jsonLinkPropertyPair != null) {
            // send
            LinkPropertySet propSetSend = new LinkPropertySet();
            if (jsonLinkPropertyPair.get("send") != null) {
                if (jsonLinkPropertyPair.get("bandwidth_bps") != null) {
                    propSetSend.bandwidthBitsPS = (int) jsonLinkPropertyPair.get("bandwidth_bps");
                }
                if (jsonLinkPropertyPair.get("latency_ms") != null) {
                    propSetSend.latencyMs = (int) jsonLinkPropertyPair.get("latency_ms");
                }
            }
            // receive
            LinkPropertySet propSetReceive = new LinkPropertySet();
            if (jsonLinkPropertyPair.get("receive") != null) {
                if (jsonLinkPropertyPair.get("bandwidth_bps") != null) {
                    propSetReceive.bandwidthBitsPS =
                            (int) jsonLinkPropertyPair.get("bandwidth_bps");
                }
                if (jsonLinkPropertyPair.get("latency_ms") != null) {
                    propSetReceive.latencyMs = (int) jsonLinkPropertyPair.get("latency_ms");
                }
            }
            propPair.send = propSetSend;
            propPair.receive = propSetReceive;
        }

        return propPair;
    }

    abstract Link createLink(PluginCommsTwoSixJava plugin, JLinkConfig linkConfig, String channelGid);
}
