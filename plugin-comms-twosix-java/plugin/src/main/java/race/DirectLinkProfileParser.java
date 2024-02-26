
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
import ShimsJava.RaceLog;

import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;

public class DirectLinkProfileParser extends LinkProfileParser {

    static final String logLabel = "race.DirectLinkProfileParser";
    private String hostname = "";
    private int port = -1;
    private boolean multicast = false;

    DirectLinkProfileParser(String linkProfile) {
        JSONParser parser = new JSONParser();
        JSONObject linkProfileJson = null;
        try {
            linkProfileJson = (JSONObject) parser.parse(linkProfile);
        } catch (Exception e) {
            RaceLog.logError(logLabel, "Invalid link profile (malformed JSON): " + linkProfile, "");
        }

        // Required fields
        RaceLog.logDebug(logLabel, linkProfileJson.toJSONString(), "");
        if (linkProfileJson.get("hostname") != null) {
            hostname = (String) linkProfileJson.get("hostname");
        } else {
            RaceLog.logError(
                    logLabel, "Invalid link profile (missing hostname): " + linkProfile, "");
        }
        if (linkProfileJson.get("port") != null) {
            RaceLog.logDebug("LinkProfileParser", "" + linkProfileJson.get("port"), "");
            port = Integer.valueOf("" + linkProfileJson.get("port"));
        } else {
            RaceLog.logError(logLabel, "Invalid link profile (missing port): " + linkProfile, "");
        }

        // Optional requirements
        if (linkProfileJson.get("multicast") != null) {
            multicast = (boolean) linkProfileJson.get("multicast");
        } else {
            multicast = false;
        }
    }

    /**
     * @param plugin
     * @param linkConfig
     * @return Link
     */
    @Override
    Link createLink(PluginCommsTwoSixJava plugin, JLinkConfig linkConfig, String channelGid) {
        String linkId = plugin.getjSdk().generateLinkId(channelGid);
        if (linkId == null || linkId.trim().isEmpty()) {
            RaceLog.logError(logLabel, "received invalid link ID from the SDK.", "");
            return null;
        }

        RaceLog.logDebug(logLabel, "Creating Direct Link: " + linkId, "");
        return new DirectLink(
                plugin,
                linkId,
                linkConfig.linkProfile,
                linkConfig.personas,
                linkConfig.linkProps,
                hostname,
                port);
    }

    /** @return String */
    @Override
    public String toString() {
        String string = "";
        string += "hostname: " + hostname + "\n";
        string += "port: " + port + "\n";
        string += "multicast: " + multicast + "\n";
        return string;
    }
}
