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

package com.twosix.race.daemon.util;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.Optional;

public abstract class Subprocess {

    public static class CompletedProcess {
        public final int returnCode;
        public final Optional<String> stdout;
        public final Optional<String> stderr;

        CompletedProcess() {
            returnCode = -1;
            stdout = Optional.empty();
            stderr = Optional.empty();
        }

        CompletedProcess(int returnCode, String stdout, String stderr) {
            this.returnCode = returnCode;
            this.stdout = Optional.of(stdout);
            this.stderr = Optional.of(stderr);
        }
    }

    private static final Logger logger = LoggerFactory.getLogger(Subprocess.class);

    public static CompletedProcess run(String command) {
        try {
            logger.debug("Executing command: {}", command);
            Process process = Runtime.getRuntime().exec(command);

            StringBuilder stdoutBuffer = new StringBuilder();
            try (BufferedReader reader =
                    new BufferedReader(new InputStreamReader(process.getInputStream()))) {
                for (String line = reader.readLine(); line != null; line = reader.readLine()) {
                    stdoutBuffer.append(line).append("\n");
                }
            }

            StringBuilder stderrBuffer = new StringBuilder();
            try (BufferedReader reader =
                    new BufferedReader(new InputStreamReader(process.getErrorStream()))) {
                for (String line = reader.readLine(); line != null; line = reader.readLine()) {
                    stderrBuffer.append(line).append("\n");
                }
            }

            int returnCode = process.waitFor();
            logger.debug("Exit value from command: {}, return code: {}", command, returnCode);

            return new CompletedProcess(
                    returnCode, stdoutBuffer.toString(), stderrBuffer.toString());
        } catch (Exception e) {
            logger.warn("Error executing command: {}, what: {}", command, e.getMessage());
            return new CompletedProcess();
        }
    }
}
