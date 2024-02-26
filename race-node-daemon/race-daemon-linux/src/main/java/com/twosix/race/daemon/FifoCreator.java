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

import java.io.File;
import java.io.IOException;

class FifoCreator {
    /**
     * Creates a new FIFO file (named pipe).
     *
     * @param fifoName File name
     * @return FIFO file
     * @throws IOException if an error occurs creating the FIFO file
     * @throws InterruptedException if interrupted waiting for the FIFO file to be created
     */
    public static File createFifo(String fifoName) throws IOException, InterruptedException {
        Process process = null;
        process = new ProcessBuilder("mkfifo", fifoName).inheritIO().start();
        process.waitFor();

        // return regardless of whether the previous command succeeded (handles file already exists)
        return new File(fifoName);
    }
}
