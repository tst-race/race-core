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

package com.twosix.race.daemon.sdk;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.io.IOException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLConnection;
import java.nio.file.Files;

class FileServerUtils {

    private static final String LINE_FEED = "\r\n";

    private static final Logger logger = LoggerFactory.getLogger(FileServerUtils.class);

    /**
     * Uploads the given file to the RiB orchestration file server.
     *
     * @param file Local file to be uploaded
     * @param config Daemon configuration
     * @return True if successful in uploading the file
     * @throws IOException if an error occurs
     */
    static boolean upload(File file, RaceNodeDaemonConfig config) throws IOException {
        URL url =
                new URL(
                        "http://"
                                + config.getFileServerHost()
                                + ":"
                                + config.getFileServerPort()
                                + "/upload");
        HttpURLConnection connection = (HttpURLConnection) url.openConnection();
        connection.setRequestMethod("POST");
        connection.setDoOutput(true);

        String boundary = "===" + System.currentTimeMillis() + "===";

        connection.setRequestProperty("Content-Type", "multipart/form-data;boundary=" + boundary);

        OutputStream outputStream = connection.getOutputStream();
        PrintWriter writer = new PrintWriter(new OutputStreamWriter(outputStream, "UTF-8"));

        // Add file field to multi-part form data
        writer.append("--" + boundary);
        writer.append(LINE_FEED);
        writer.append(
                "Content-Disposition: form-data; name=\"file\"; filename=\""
                        + file.getName()
                        + "\"");
        writer.append(LINE_FEED);
        writer.append("Content-Type: " + URLConnection.guessContentTypeFromName(file.getName()));
        writer.append(LINE_FEED);
        writer.append("Content-Transfer-Encoding: binary");
        writer.append(LINE_FEED);
        writer.append(LINE_FEED);
        writer.flush();

        Files.copy(file.toPath(), outputStream);
        outputStream.flush();

        writer.append(LINE_FEED);
        writer.flush();

        // Close out multi-part form data
        writer.append(LINE_FEED);
        writer.append("--" + boundary + "--");
        writer.append(LINE_FEED);
        writer.close();

        int status = connection.getResponseCode();
        if (status == HttpURLConnection.HTTP_OK) {
            return true;
        } else {
            logger.warn("File server upload failed with HTTP status: " + status);
            return false;
        }
    }
}
