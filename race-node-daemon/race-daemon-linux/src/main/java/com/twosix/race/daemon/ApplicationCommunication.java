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

package com.twosix.race.daemon;

import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

/** Bidirectional communication to the RACE application */
public class ApplicationCommunication {
    private final Logger logger = LoggerFactory.getLogger(getClass());

    private final String inputFifoPath = "/tmp/racetestapp-input";
    private final String outputFifoPath = "/tmp/racetestapp-output";

    private final DaemonState state;

    private File inputFifo; // input to the application, i.e. send from daemon
    private File outputFifo; // output of the application, i.e. send to daemon

    private final ExecutorService inputExecutorService;
    private final ExecutorService outputExecutorService;

    public ApplicationCommunication(DaemonState state) throws IOException, InterruptedException {
        logger.info("RACE node daemon initializing");
        this.state = state;

        inputFifo = FifoCreator.createFifo(inputFifoPath);
        outputFifo = FifoCreator.createFifo(outputFifoPath);

        inputExecutorService = Executors.newSingleThreadExecutor();
        outputExecutorService = Executors.newSingleThreadExecutor();

        listen();
    }

    /**
     * Sends the given message to the RACE application.
     *
     * <p>The actual send is executed on a separate thread because the FIFO write can block.
     *
     * @param message Message content
     */
    public void sendToApp(String message) {
        inputExecutorService.submit(
                () -> {
                    logger.debug("Sending action to application");
                    try {
                        FileOutputStream send = new FileOutputStream(inputFifo);
                        send.write(message.getBytes(StandardCharsets.UTF_8));
                        send.flush();
                    } catch (IOException e) {
                        logger.error("Error sending to application", e);
                    }
                    logger.debug("Finished sending action to application");
                });
    }

    /**
     * Starts a thread to continually read from the FIFO to receive application status from the RACE
     * application.
     *
     * <p>All received status updates are published via the daemon SDK.
     */
    public void listen() {
        outputExecutorService.execute(
                new Runnable() {
                    public void run() {
                        while (true) {
                            try (BufferedReader reader =
                                    new BufferedReader(new FileReader(outputFifo))) {
                                String readLine = reader.readLine();
                                logger.debug("Got line from RACE app: {}", readLine);
                                JSONObject received = new JSONObject(readLine);

                                if (received.has("status") && received.has("ttl")) {
                                    JSONObject status = received.getJSONObject("status");
                                    int ttl = received.getInt("ttl");
                                    state.sdk.updateAppStatus(status.toString(), ttl);
                                } else if (received.has("message") && received.has("actionType")) {
                                    if (state.currentBootstrapTarget.isEmpty()) {
                                        logger.error(
                                                "No current bootstrap target, unable to forward"
                                                        + " bootstrap info to target");
                                        return;
                                    }

                                    String message = received.getString("message");
                                    String actionType = received.getString("actionType");

                                    if (actionType.equals("BS_COMPLETE")) {
                                        state.currentBootstrapTarget = "";
                                    } else {
                                        state.sdk.sendBootstrapInfo(
                                                state.currentBootstrapTarget, message, actionType);
                                    }
                                } else {
                                    logger.error(
                                            "Invalid message received from RACE app: {}",
                                            received.toString());
                                }

                            } catch (Exception e) {
                                logger.error("Error processing status", e);
                                try {
                                    TimeUnit.SECONDS.sleep(1);
                                } catch (InterruptedException e2) {
                                }
                            }
                        }
                    }
                });
    }
}
