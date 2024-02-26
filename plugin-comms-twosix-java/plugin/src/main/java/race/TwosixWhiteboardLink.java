
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

import ShimsJava.JEncPkg;
import ShimsJava.JLinkProperties;
import ShimsJava.JRaceSdkComms;
import ShimsJava.LinkType;
import ShimsJava.RaceLog;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.Base64;
import java.util.Vector;

public class TwosixWhiteboardLink extends Link {
    Thread monitorThread;
    final String hostname;
    final int port;
    final String hastag;
    int configPeriod;
    int checkPeriod;
    double timestamp;

    String logLabel = "race.TwosixWhiteboardLink";

    TwosixWhiteboardLink(
            PluginCommsTwoSixJava plugin,
            String linkId,
            String linkProfile,
            Vector<String> personas,
            JLinkProperties linkProperties,
            String hostname,
            int port,
            String hashtag,
            int configPeriod,
            int checkPeriod,
            double timestamp) {
        super(plugin, linkId, linkProfile, personas, linkProperties);
        this.hostname = hostname;
        this.port = port;
        this.hastag = hashtag;
        this.configPeriod = configPeriod;
        this.checkPeriod = 0;
        this.timestamp = timestamp;
    }

    /**
     * @param linkType
     * @param connectionId
     * @param linkHints
     * @return Connection
     */
    @Override
    Connection openConnection(
            LinkType linkType, final String connectionId, final String linkHints) {
        RaceLog.logDebug(logLabel, "openConnection called", "");

        RaceLog.logDebug(logLabel, "    type:         " + linkType.name(), "");
        RaceLog.logDebug(logLabel, "    ID:           " + linkId, "");
        RaceLog.logDebug(logLabel, "    linkProfile:  " + profile, "");

        double hintTimestamp = this.timestamp;
        try {
            JSONParser parser = new JSONParser();
            JSONObject hintsJson = (JSONObject) parser.parse(linkHints);
            if (timestamp < 0) {
                if (hintsJson.get("after") != null) {
                    hintTimestamp = ((Number) hintsJson.get("after")).doubleValue();
                }
            }
        } catch (Exception e) {
            RaceLog.logWarning(
                    logLabel, "Error parsing LinkHints JSON, ignoring for this connection", "");
        }

        final double connectionTimestamp = hintTimestamp;
        Connection connection = new Connection(connectionId, linkType, this);

        if (linkType == LinkType.LT_BIDI || linkType == LinkType.LT_RECV) {
            if (connections.isEmpty()) {
                RaceLog.logDebug(logLabel, "creating thread for receiving link ID: " + linkId, "");
                monitorThread =
                        new Thread() {
                            public void run() {
                                twosixWhiteboardMonitor(connectionTimestamp);
                            }
                        };
                monitorThread.start();
            }

            // We only keep track of recv links, do we want to keep track of send links as
            // well?
            // Currently, we shut down the monitor thread if there's no receiving links.
            connections.put(connection.connectionId, connection);
        }
        return connection;
    }

    void twosixWhiteboardMonitor(double timestamp) {
        String loggingPrefix = "TwosixWhiteboardLink::runMonitor (" + linkId + "): ";
        RaceLog.logInfo(
                logLabel,
                loggingPrefix
                        + ": called. hostname: "
                        + hostname
                        + ", tag: "
                        + hastag
                        + ", checkPeriod: "
                        + checkPeriod
                        + ", timestamp: "
                        + timestamp,
                "");

        // Read last recorded timestamp - if there is none, use the value of the LinkAddress or Hint
        // If there is nothing, then just start from <now>
        double timestampToUseInQuery;
        double lastRecordedTimestamp = readValue("lastTimestamp", -1.0);

        if (lastRecordedTimestamp > 0) {
            // Prioritize last recorded timestamp
            timestampToUseInQuery = lastRecordedTimestamp;
        } else if (timestamp > 0) {
            // Secondary option is the timestamp hint
            timestampToUseInQuery = timestamp;
        } else {
            // fallback to now
            timestampToUseInQuery = Time.getTimestampSeconds();
        }
        int latest = getIndexFromTimestamp(timestampToUseInQuery);

        // TODO: Once there is a way to inform the network manager, limit retries on failure. break and
        // notify network manager if
        // the limit is reached
        while (true) {
            if (Thread.interrupted()) {
                RaceLog.logDebug(logLabel, "Thread.interrupted()", "");
                break;
            }

            PostResponse postResponse = getNewPosts(latest);

            int numPosts = postResponse.posts.size();
            if (numPosts < postResponse.newLatest - latest) {
                int expectedPosts = postResponse.newLatest - latest;
                int postsLost = expectedPosts - numPosts;
                RaceLog.logError(
                        logLabel,
                        "Expected "
                                + expectedPosts
                                + " posts, but only got "
                                + numPosts
                                + ". "
                                + postsLost
                                + " posts may have been lost.",
                        "");
            }
            latest = postResponse.newLatest;

            for (byte[] post : postResponse.posts) {
                plugin.getjSdk()
                        .receiveEncPkg(
                                new JEncPkg(post),
                                connections.keySet().toArray(new String[] {}),
                                JRaceSdkComms.getBlockingTimeout());
            }

            if (numPosts > 0) {
                saveValue("lastTimestamp", postResponse.timestamp);
            }

            try {
                Thread.sleep(configPeriod);
            } catch (Exception e) {
            }
        }
    }

    /** @return String */
    String getIdentifier() {
        return hostname + ":" + port + ":" + hastag;
    }

    /**
     * @param key
     * @param defaultValue
     * @return int
     */
    <T> T readValue(String key, T defaultValue) {
        String taggedKey = getIdentifier() + ":" + key;
        return PersistentStorageHelpers.readValue(plugin.getjSdk(), taggedKey, defaultValue);
    }

    /**
     * @param key
     * @param value
     * @return boolean
     */
    <T> boolean saveValue(String key, T value) {
        String taggedKey = getIdentifier() + ":" + key;
        return PersistentStorageHelpers.saveValue(plugin.getjSdk(), taggedKey, value);
    }

    int getIndexFromTimestamp(double timestamp) {
        String loggingPrefix = "TwosixWhiteboardLink::getIndexFromTimestamp (" + linkId + "): ";

        // return 0 if error
        int index = 0;

        try {
            URL postUrl =
                    new URL(
                            "http://"
                                    + hostname
                                    + ":"
                                    + port
                                    + "/after/"
                                    + hastag
                                    + "/"
                                    + Time.getTimestampString(timestamp));
            HttpURLConnection conn = (HttpURLConnection) postUrl.openConnection();
            conn.setRequestMethod("GET");
            conn.setRequestProperty("Content-Type", "application/json");

            BufferedReader input = new BufferedReader(new InputStreamReader(conn.getInputStream()));
            String response = "";
            String line;
            while ((line = input.readLine()) != null) {
                response = response + line;
            }

            JSONParser parser = new JSONParser();
            JSONObject responseJson = (JSONObject) parser.parse(response);
            index = ((Number) responseJson.get("index")).intValue();
            RaceLog.logDebug(logLabel, loggingPrefix + "Got index: " + index, "");
        } catch (Exception e) {
            RaceLog.logError(
                    logLabel,
                    loggingPrefix + "failed to get last post index " + e.getMessage(),
                    "");
        }
        return index;
    }

    /**
     * @param oldest
     * @return PostResponse
     */
    PostResponse getNewPosts(int oldest) {
        BufferedReader input = null;
        try {
            // get all posts after (and including) oldest
            URL postUrl =
                    new URL(
                            "http://" + hostname + ":" + port + "/get/" + hastag + "/" + oldest
                                    + "/-1");

            HttpURLConnection conn = (HttpURLConnection) postUrl.openConnection();
            conn.setRequestMethod("GET");
            conn.setRequestProperty("Content-Type", "application/json");

            input = new BufferedReader(new InputStreamReader(conn.getInputStream()));
            String response = "";
            String line = "";
            while ((line = input.readLine()) != null) {
                response = response + line;
            }

            JSONParser parser = new JSONParser();
            JSONObject responseJson = (JSONObject) parser.parse(response);
            JSONArray data = (JSONArray) responseJson.get("data");
            Vector<byte[]> posts = new Vector<>();
            for (Object obj : data) {
                try {
                    byte[] base64DecodedBytes = Base64.getDecoder().decode((String) obj);
                    posts.add(base64DecodedBytes);
                } catch (Exception e) {
                    RaceLog.logError(
                            logLabel, "failed to base64 decode pkg. " + e.getMessage(), "");
                }
            }
            int length = Integer.valueOf("" + responseJson.get("length"));
            double timestamp = Double.parseDouble((String) responseJson.get("timestamp"));
            return new PostResponse(posts, length, timestamp);

        } catch (Exception e) {
            RaceLog.logError(logLabel, e.getMessage(), "");
        } finally {
            if (input != null) {
                try {
                    input.close();
                } catch (Exception e) {
                    // do nothing
                }
            }
        }
        return new PostResponse(new Vector<>(), 0, 0);
    }

    class PostResponse {
        Vector<byte[]> posts;
        int newLatest;
        double timestamp;

        PostResponse(Vector<byte[]> posts, int newLatest, double timestamp) {
            this.posts = posts;
            this.newLatest = newLatest;
            this.timestamp = timestamp;
        }
    }

    /** @param connectionId */
    @Override
    void closeConnection(String connectionId) {
        connections.remove(connectionId);

        if (linkType == LinkType.LT_RECV || linkType == LinkType.LT_BIDI) {
            // if no more connections are using this link, close the link
            if (connections.isEmpty()) {
                monitorThread.interrupt();
            }
        }
    }

    /** @param pkg */
    @Override
    void sendPackage(JEncPkg pkg) {
        String base64encodedString;
        try {
            // TODO include trace ID & span ID in messages
            base64encodedString = Base64.getEncoder().encodeToString(pkg.getRawData());
        } catch (Exception e) {
            RaceLog.logError(logLabel, "failed to base64 encode pkg. " + e.getMessage(), "");
            return;
        }

        // 5 retries
        int tries = 0;
        for (; tries < 5; tries++) {
            BufferedReader input = null;
            try {
                URL postUrl = new URL("http://" + hostname + ":" + port + "/post/" + hastag);

                HttpURLConnection conn = (HttpURLConnection) postUrl.openConnection();
                conn.setRequestMethod("POST");
                conn.setRequestProperty("Content-Type", "application/json");

                conn.setDoOutput(true);
                OutputStream os = conn.getOutputStream();
                os.write("{\"data\":\"".getBytes());
                os.write(base64encodedString.getBytes());
                os.write("\"}".getBytes());
                os.flush();
                os.close();

                input = new BufferedReader(new InputStreamReader(conn.getInputStream()));
                String response = "";
                String line;
                while ((line = input.readLine()) != null) {
                    response = response + line;
                }
                RaceLog.logDebug(logLabel, "response from send: " + response, "");
                if (response.contains("index")) {
                    RaceLog.logDebug(logLabel, "post successful", "");
                    if (input != null) {
                        try {
                            input.close();
                        } catch (Exception e) {
                            // do nothing
                        }
                    }
                    break;
                }
                Thread.sleep(500);
            } catch (Exception e) {
                RaceLog.logDebug(logLabel, "Exception: " + e.getMessage(), "");
            } finally {
                if (input != null) {
                    try {
                        input.close();
                    } catch (Exception e) {
                        // do nothing
                    }
                }
            }
        }

        if (tries == 5) {
            RaceLog.logError(logLabel, "Retry limit exceeded: post failed", "");
        }
    }
}
