
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

import java.io.BufferedInputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.Vector;

public class DirectLink extends Link {

    final String hostname;
    final int port;
    private Socket socket;
    private Thread monitorThread;
    final String logLabel = "race.DirectLink";

    DirectLink(
            PluginCommsTwoSixJava plugin,
            String linkId,
            String profile,
            Vector<String> personas,
            JLinkProperties linkProperties,
            String hostname,
            int port) {
        super(plugin, linkId, profile, personas, linkProperties);
        this.hostname = hostname;
        this.port = port;
    }

    /**
     * @param linkType
     * @param connectionId
     * @param linkHints
     * @return Connection
     */
    Connection openConnection(
            LinkType linkType, final String connectionId, final String linkHints) {
        synchronized (this) {
            RaceLog.logDebug(logLabel, "openConnection called", "");

            RaceLog.logDebug(logLabel, "    type:         " + linkType.name(), "");
            RaceLog.logDebug(logLabel, "    ID:           " + linkId, "");
            RaceLog.logDebug(logLabel, "    linkProfile:  " + profile, "");

            Connection connection = new Connection(connectionId, linkType, this);

            if (linkType == LinkType.LT_BIDI || linkType == LinkType.LT_RECV) {
                if (connections.isEmpty()) {
                    RaceLog.logDebug(
                            logLabel, "creating thread for receiving link ID: " + linkId, "");
                    monitorThread =
                            new Thread() {
                                public void run() {
                                    directConnectionMonitor();
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
    }

    /** @param connectionId */
    void closeConnection(String connectionId) {
        connections.remove(connectionId);

        if (linkType == LinkType.LT_RECV || linkType == LinkType.LT_BIDI) {
            // if no more connections are using this link, close the link
            if (connections.isEmpty()) {
                try {
                    socket.close();
                } catch (IOException ioException) {
                    RaceLog.logWarning(
                            logLabel,
                            "Exception when closing socket: " + ioException.getMessage(),
                            "");
                }
                monitorThread.interrupt();
            }
        }
    }

    void directConnectionMonitor() {
        RaceLog.logDebug(logLabel, "directConnectionMonitor called", "");

        ServerSocket server = null;
        try {
            server = new ServerSocket(port);
        } catch (Exception e) {
            RaceLog.logWarning(logLabel, e.getMessage(), "");
        }
        while (true) {
            if (Thread.interrupted()) {
                // break out of infinite loop if thread is stopped
                break;
            }
            try {
                try {
                    socket = server.accept();
                    socket.setReuseAddress(true);
                } catch (Exception e) {
                    RaceLog.logDebug(
                            logLabel,
                            "Failed to listen on "
                                    + " hostname: "
                                    + hostname
                                    + " port: "
                                    + port
                                    + " because "
                                    + e.getMessage(),
                            "");
                    continue;
                }

                DataInputStream in =
                        new DataInputStream(new BufferedInputStream(socket.getInputStream()));
                byte[] resultBuff = new byte[0];
                byte[] buff = new byte[1024];
                int k = -1;
                while ((k = in.read(buff, 0, buff.length)) > -1) {
                    byte[] tbuff =
                            new byte
                                    [resultBuff.length
                                            + k]; // temp buffer size = bytes already read + bytes
                    // last read
                    System.arraycopy(
                            resultBuff, 0, tbuff, 0, resultBuff.length); // copy previous bytes
                    System.arraycopy(buff, 0, tbuff, resultBuff.length, k); // copy current lot
                    resultBuff = tbuff; // call the temp buffer as your result buff
                }
                if (resultBuff.length > 0) {
                    RaceLog.logDebug(
                            logLabel,
                            "directConnectionMonitor: Received encrypted package. "
                                    + linkId
                                    + " on "
                                    + hostname
                                    + ":"
                                    + port,
                            "");
                    // TODO include trace ID & span ID in messages
                    plugin.getjSdk()
                            .receiveEncPkg(
                                    new JEncPkg(resultBuff),
                                    connections.keySet().toArray(new String[] {}),
                                    JRaceSdkComms.getBlockingTimeout());
                }
            } catch (Exception e) {
                RaceLog.logDebug(logLabel, e.getMessage(), "");
            } finally {
                try {
                    socket.close();
                } catch (Exception e) {
                    RaceLog.logWarning(logLabel, e.getMessage(), "");
                }
            }
        }
        try {
            server.close();
        } catch (Exception e) {
            RaceLog.logWarning(logLabel, e.getMessage(), "");
        }
    }

    /** @param pkg */
    void sendPackage(JEncPkg pkg) {
        final String loggingPrefix = logLabel + "sendPackageDirectLink (" + linkId + "): ";
        RaceLog.logDebug(loggingPrefix, "sendPackageDirectLink: called", "");
        RaceLog.logDebug(loggingPrefix, "    Hostname: " + hostname, "");
        RaceLog.logDebug(loggingPrefix, "    Port: " + port, "");

        Socket socket = null;
        OutputStream out = null;
        int retries = 0;
        while (socket == null || !socket.isConnected()) {
            retries++;
            if (retries > 50) {
                RaceLog.logWarning(
                        logLabel,
                        "Connect still failing after 50 retries. Continuing to retry, but"
                                + " something may be wrong",
                        "");
            }
            // open connection
            try {
                socket = new Socket(hostname, port);
                socket.setReuseAddress(true);
                if (socket.isConnected()) {
                    RaceLog.logDebug(loggingPrefix, "Connected " + linkId, "");
                } else {
                    try {
                        Thread.sleep(10);
                    } catch (Exception e) {
                    }
                    continue;
                }
                // sends output to the socket
                out = new DataOutputStream(socket.getOutputStream());
            } catch (UnknownHostException u) {
                RaceLog.logError(
                        loggingPrefix, u.getMessage() + "unkown hostname: " + hostname, "");
            } catch (IOException i) {
                RaceLog.logError(
                        loggingPrefix,
                        i.getMessage() + " hostname: " + hostname + " port: " + port,
                        "");
            }
        }
        // send packet
        try {
            // sending data to server
            // TODO include trace ID & span ID in messages
            out.write(pkg.getRawData());
        } catch (Exception e) {
            RaceLog.logError(loggingPrefix, e.getMessage(), "");
        }
        // close connection
        try {
            socket.close();
            out.close();
        } catch (Exception e) {
            RaceLog.logWarning(logLabel, "Exception when closing socket: " + e.getMessage(), "");
        }
    }
}
