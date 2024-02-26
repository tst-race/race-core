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

package com.twosix.race.daemon.actions;

import android.content.Context;

import com.twosix.race.daemon.sdk.FileUtils;
import com.twosix.race.daemon.sdk.IRaceNodeDaemonSdk;

import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;

public class ClearConfigsAndEtcAction implements NodeAction {
    public static final String TYPE = "clear-configs-and-etc";

    private final Logger logger = LoggerFactory.getLogger(getClass());

    private final Context context;

    private final IRaceNodeDaemonSdk sdk;

    public ClearConfigsAndEtcAction(Context context, IRaceNodeDaemonSdk sdk) {
        this.context = context;
        this.sdk = sdk;
    }

    /**
     * Clear configs and etc from the node.
     *
     * @param jsonPayload Action payload
     */
    @Override
    public void execute(JSONObject jsonPayload) {
        try {
            logger.info("Clearing configs and etc from the node");
            String basePath = "/storage/self/primary/Android/data/com.twosix.race/files/race";
            // remove runtime configs if they exist
            File configsDir = new File(basePath + "/data/configs");
            if (configsDir.isDirectory()) {
                FileUtils.deleteFilesInDirectory(configsDir);
                logger.info("deleted data dir");
            }
            // remove configs tar if it exists
            File configsTarFile = new File("/storage/self/primary/Download/race/configs.tar.gz");
            if (configsTarFile.exists()) {
                configsTarFile.delete();
                logger.info("Deleted configs tar");
            }
            // remove etc files if they exist
            File etcDir = new File("/storage/self/primary/Download/race/etc/");
            if (etcDir.isDirectory()) {
                FileUtils.deleteFilesInDirectory(etcDir);
                logger.info("Deleted Etc dir");
            }
            // remove etc tar if it exists
            File etcTarFile = new File(FileUtils.getTempDirectory(), "etc.tar.gz");
            if (etcTarFile.exists()) {
                etcTarFile.delete();
                logger.info("Deleted etc tar");
            }

            logger.info("Cleared configs and etc from the node");
        } catch (Exception e) {
            logger.error("Error clearing configs and etc from node: {}", e.getMessage());
        }
    }
}
