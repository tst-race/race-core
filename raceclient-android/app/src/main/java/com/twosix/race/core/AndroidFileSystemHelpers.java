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

package com.twosix.race.core;

import android.content.ContentResolver;
import android.content.Context;
import android.content.res.AssetManager;
import android.os.FileUtils;
import android.util.Log;

import androidx.documentfile.provider.DocumentFile;

import org.apache.commons.compress.archivers.ArchiveEntry;
import org.apache.commons.compress.archivers.ArchiveInputStream;
import org.apache.commons.compress.archivers.tar.TarArchiveInputStream;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.List;

/**
 * AndroidFileSystemHelpers Class containing static helper functions to interact with the Android
 * filesystem
 */
public class AndroidFileSystemHelpers {

    /**
     * Purpose: Copy directory from specified src to specified destination
     *
     * @param src directory to copy
     * @param dest destination to copy to
     */
    public static void copyDir(String src, String dest) throws IOException {
        Log.d("com.twosix.race.core.AndroidFileSystemHelpers", "src: " + src);
        Log.d("com.twosix.race.core.AndroidFileSystemHelpers", "dest: " + dest);
        File srcLocation = new File(src);
        File destLocation = new File(dest);
        if (srcLocation.isDirectory()) {
            if (!destLocation.exists()) {
                destLocation.mkdirs();
            }

            String[] children = srcLocation.list();
            if (children != null) {
                for (int i = 0; i < children.length; i++) {
                    copyDir(src + "/" + children[i], dest + "/" + children[i]);
                }
            }
        } else {
            FileInputStream srcFileStream = new FileInputStream(srcLocation);
            FileOutputStream destFileStream = new FileOutputStream(destLocation);
            FileUtils.copy(srcFileStream, destFileStream);
            srcFileStream.close();
            destFileStream.close();
        }
    }

    public static void copyDir(ContentResolver contentResolver, DocumentFile srcFile, File destFile)
            throws IOException {
        Log.d(
                "com.twosix.race.core.AndroidFileSystemHelpers",
                "copying from " + srcFile.getUri() + " to " + destFile);
        if (srcFile.isDirectory()) {
            if (!destFile.exists()) {
                destFile.mkdirs();
            }

            for (DocumentFile child : srcFile.listFiles()) {
                copyDir(contentResolver, child, new File(destFile, child.getName()));
            }
        } else {
            try (FileOutputStream destFileStream = new FileOutputStream(destFile);
                    InputStream srcFileStream = contentResolver.openInputStream(srcFile.getUri())) {
                FileUtils.copy(srcFileStream, destFileStream);
            }
        }
    }

    /**
     * Purpose: Copy directory from assets
     *
     * @param dir directory to copy (relative to assets dir)
     * @param context application context to get app specific info (/data/data/com.twosix.race)
     */
    public static void copyAssetDir(String dir, Context context) {
        String dest = context.getApplicationInfo().dataDir + "/";
        File destDir = new File(dest);
        File topDir = new File(dest + "/" + dir);
        Log.d("com.twosix.race copyAssetDir", "destDir: " + destDir.toString());
        Log.d("com.twosix.race copyAssetDir", "topDir: " + topDir.toString());

        // create dest dir if it's not created already
        try {
            Files.createDirectories(destDir.toPath());
            Log.d("com.twosix.race copyAssetDir", "dest created..");

        } catch (Exception e) {
            Log.e("com.twosix.race copyAssetDir", e.getMessage());
            Log.e("com.twosix.race copyAssetDir", e.getClass().getName());
        }

        // create top level dir to copy into
        try {
            Files.createDirectories(topDir.toPath());
            Log.d("com.twosix.race copyAssetDir", "topDir created..");
        } catch (Exception e) {
            Log.e("com.twosix.race copyAssetDir", e.getMessage());
            Log.e("com.twosix.race copyAssetDir", e.getClass().getName());
        }

        // copy contents
        AssetManager assetManager = context.getAssets();
        try {
            List<String> dirContents = new ArrayList<>();
            listDirContents(assetManager, dir, dirContents);
            for (String item : dirContents) {
                try {
                    InputStream in = assetManager.open(item);
                    File outFile = new File(dest + item);
                    OutputStream out = new FileOutputStream(outFile);
                    copyFile(in, out);
                    in.close();
                    out.close();
                } catch (FileNotFoundException f) {
                    File outFile = new File(dest + item);
                    outFile.mkdir();
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    /**
     * Purpose: Copy a file in the android filesystem
     *
     * @param in InputStream of the src file
     * @param out OutputStream of the dest file
     */
    public static void copyFile(InputStream in, OutputStream out) {
        try {
            byte[] buffer = new byte[1024];
            int read;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    /**
     * Purpose: List contents (files and sub-dirs) of a directory. Can only be used for directories
     * within "assets" dir
     *
     * @param mgr AssetManager passed in so a new one isn't created on each recursive call
     * @param dir String listing the path (relative to assets directory)
     * @param list List of files and dirs relative to the assets dir
     */
    public static void listDirContents(AssetManager mgr, String dir, List<String> list) {
        try {
            String currentDirContents[] = mgr.list(dir);
            // files passed to AssetManager list will return null
            if (currentDirContents != null)
                for (int i = 0; i < currentDirContents.length; ++i) {
                    String fullFilePath = dir + "/" + currentDirContents[i];

                    File file = new File(fullFilePath);
                    // Add path/dir to list
                    list.add(fullFilePath);
                    // Call again for all listed contents. Files will not get into this for loop
                    listDirContents(mgr, fullFilePath, list);
                }
        } catch (IOException e) {
            Log.e("List error:", "can't list" + dir);
        }
    }

    /**
     * Purpose: extract .tar files located in assets dir. .tar.gz files are automatically
     * uncompressed when the apk is installed.
     *
     * @param tarFilePath path to .tar file relative to assets dir
     * @param dest path to extract to. Must be full path (/data/data/com.twosix.race/...)
     * @param context application context to get app specific info
     */
    public static void extractTar(String tarFilePath, String dest, Context context)
            throws IOException {
        AssetManager assetManager = context.getAssets();
        InputStream in = assetManager.open(tarFilePath);
        ArchiveInputStream i = new TarArchiveInputStream(in);
        ArchiveEntry entry = null;
        while ((entry = i.getNextEntry()) != null) {
            if (!i.canReadEntryData(entry)) {
                Log.w("extractTar", "unable to read tar entry");
                continue;
            }
            String name = dest + entry.getName();
            File f = new File(name);
            if (entry.isDirectory()) {
                if (!f.isDirectory() && !f.mkdirs()) {
                    throw new IOException("failed to create directory " + f);
                }
            } else {
                File parent = f.getParentFile();
                if (!parent.isDirectory() && !parent.mkdirs()) {
                    throw new IOException("failed to create directory " + parent);
                }
                try (OutputStream o = Files.newOutputStream(f.toPath())) {
                    copyFile(i, o);
                }
            }
        }
    }
}
