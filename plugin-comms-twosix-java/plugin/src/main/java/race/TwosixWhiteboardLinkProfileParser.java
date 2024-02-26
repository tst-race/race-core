
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

public class TwosixWhiteboardLinkProfileParser extends LinkProfileParser {

    static final String logLabel = "race.TwosixWhiteboardLinkProfileParser";
    private String hostname = "";
    private int port = 0;
    private String hashtag = "";
    private int checkFrequency = 1000;
    private double timestamp = -1.0;

    /**
     * @param original
     * @return String
     */
    String fixHastag(String original) {
        String fixed = original;
        fixed.replaceAll("([^a-z0-9])", "");
        if (fixed.compareTo(original) != 0) {
            RaceLog.logWarning(
                    logLabel,
                    "Warning: the orignal hashtag contained invalid characters, fixed to " + fixed,
                    "");
        }
        return fixed;
    }

    TwosixWhiteboardLinkProfileParser(String linkProfile) {
        JSONParser parser = new JSONParser();
        JSONObject linkProfileJson = null;

        try {
            linkProfileJson = (JSONObject) parser.parse(linkProfile);
        } catch (Exception e) {
            RaceLog.logError(logLabel, "Invalid link profile (malformed JSON): " + linkProfile, "");
        }

        // required
        if (linkProfileJson.get("hostname") != null) {
            hostname = (String) linkProfileJson.get("hostname");
        } else {
            RaceLog.logError(
                    logLabel, "Invalid link profile (missing hostname): " + linkProfile, "");
        }
        // required
        if (linkProfileJson.get("port") != null) {
            try {
                port = Math.toIntExact((Long) linkProfileJson.get("port"));
            } catch (Exception e) {
                RaceLog.logError(logLabel, e.getMessage(), "");
            }

        } else {
            RaceLog.logError(logLabel, "Invalid link profile (missing port): " + linkProfile, "");
        }

        // required
        if (linkProfileJson.get("hashtag") != null) {
            hashtag = (String) linkProfileJson.get("hashtag");
            hashtag = fixHastag(hashtag);
        } else {
            RaceLog.logError(
                    logLabel, "Invalid link profile (missing hashtag): " + linkProfile, "");
        }

        // optional
        if (linkProfileJson.get("checkFrequency") != null) {
            checkFrequency = Math.toIntExact((Long) linkProfileJson.get("checkFrequency"));
        }

        // optional
        if (linkProfileJson.get("timestamp") != null) {
            timestamp = ((Number) linkProfileJson.get("timestamp")).doubleValue();
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

        RaceLog.logDebug(logLabel, "Creating Twosix Whiteboard Link: " + linkId, "");
        return new TwosixWhiteboardLink(
                plugin,
                linkId,
                linkConfig.linkProfile,
                linkConfig.personas,
                linkConfig.linkProps,
                hostname,
                port,
                hashtag,
                checkFrequency,
                checkFrequency,
                timestamp);
    }
}
