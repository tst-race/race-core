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
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;

import com.twosix.race.daemon.sdk.FileUtils;
import com.twosix.race.daemon.sdk.IRaceNodeDaemonSdk;

import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;

public class ClearArtifactsAction implements NodeAction {
    public static final String TYPE = "clear-artifacts";

    private final Logger logger = LoggerFactory.getLogger(getClass());

    private final Context context;

    private final IRaceNodeDaemonSdk sdk;

    public ClearArtifactsAction(Context context, IRaceNodeDaemonSdk sdk) {
        this.context = context;
        this.sdk = sdk;
    }

    /**
     * Clear RACE Artifacts from the node.
     *
     * @param jsonPayload Action payload
     */
    @Override
    public void execute(JSONObject jsonPayload) {
        try {
            logger.info("Clear Artifacts from the node");

            PackageManager packageManager = context.getPackageManager();

            String pkg = "package:com.twosix.race";
            Uri apkUri = Uri.parse(pkg);
            Intent intent = new Intent(Intent.ACTION_DELETE, apkUri);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(intent);

            File raceDir = new File("/storage/self/primary/Download/race");
            if (raceDir.isDirectory()) {
                FileUtils.deleteFilesInDirectory(raceDir);
            }

            logger.info("Cleared Artifacts from the node");
        } catch (Exception e) {
            logger.error("Error clearing artifacts from node: {}", e.getMessage());
        }
    }
}
