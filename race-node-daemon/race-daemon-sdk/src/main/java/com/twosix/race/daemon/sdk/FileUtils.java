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

import org.apache.commons.compress.archivers.ArchiveEntry;
import org.apache.commons.compress.archivers.tar.TarArchiveEntry;
import org.apache.commons.compress.archivers.tar.TarArchiveInputStream;
import org.apache.commons.compress.archivers.tar.TarArchiveOutputStream;
import org.apache.commons.compress.compressors.gzip.GzipCompressorInputStream;
import org.apache.commons.compress.compressors.gzip.GzipCompressorOutputStream;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.Enumeration;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

public class FileUtils {

    private static final Logger logger = LoggerFactory.getLogger(FileUtils.class);

    /**
     * Copies the contents of the given source directory into the destination directory.
     *
     * @param sourcePath Source directory path
     * @param destPath Destination directory path
     * @throws IOException if an error occurs
     */
    public static void copyDirectory(Path sourcePath, Path destPath) throws IOException {
        copyDirectory(sourcePath, destPath, false);
    }

    /**
     * Copies the contents of the given source directory into the destination directory.
     *
     * @param sourcePath Source directory path
     * @param destPath Destination directory path
     * @param overwrite Flag to signal if an existing directory should be overwritten.
     * @throws IOException if an error occurs
     */
    public static void copyDirectory(Path sourcePath, Path destPath, boolean overwrite)
            throws IOException {
        if (destPath.toFile().exists()) {
            if (overwrite) {
                deleteFilesInDirectory(destPath.toFile());
            } else {
                throw new IOException("Directory already exists: " + destPath);
            }
        } else if (!destPath.toFile().mkdirs()) {
            throw new IOException("Unable to create destination path: " + destPath);
        }

        Files.walk(sourcePath)
                .forEach(
                        srcFilePath -> {
                            if (srcFilePath.toFile().isFile()) {
                                Path relPath = sourcePath.relativize(srcFilePath);
                                Path destFilePath = destPath.resolve(relPath);

                                if (!destFilePath.getParent().toFile().exists()) {
                                    destFilePath.getParent().toFile().mkdirs();
                                }

                                try {
                                    Files.copy(
                                            srcFilePath,
                                            destFilePath,
                                            StandardCopyOption.REPLACE_EXISTING);
                                } catch (IOException e) {
                                    logger.error(
                                            "Unable to copy " + relPath + ": " + e.getMessage());
                                }
                            }
                        });
    }

    /**
     * Creates a gzip compressed tar archive of the given directory.
     *
     * @param sourcePath Path to the directory to be archived
     * @param tarFile Output tar file
     * @throws IOException if an error occurs
     */
    public static void createArchiveOfDirectory(Path sourcePath, Path tarFile) throws IOException {
        try (OutputStream fileOutStream = Files.newOutputStream(tarFile);
                GzipCompressorOutputStream gzipOutStream =
                        new GzipCompressorOutputStream(fileOutStream);
                TarArchiveOutputStream tarOutStream = new TarArchiveOutputStream(gzipOutStream); ) {
            tarOutStream.setLongFileMode(TarArchiveOutputStream.LONGFILE_POSIX);
            sourcePath.toFile().setReadable(true, false);
            Files.walk(sourcePath)
                    .forEach(
                            filePath -> {
                                if (filePath.toFile().isFile()) {
                                    filePath.toFile().setReadable(true, false);

                                    Path relPath = sourcePath.relativize(filePath);
                                    logger.trace("Adding " + relPath);

                                    try {
                                        ArchiveEntry entry =
                                                tarOutStream.createArchiveEntry(
                                                        filePath.toFile(), relPath.toString());
                                        tarOutStream.putArchiveEntry(entry);
                                        Files.copy(filePath, tarOutStream);
                                        tarOutStream.closeArchiveEntry();
                                    } catch (IOException e) {
                                        logger.error("Unable to add " + relPath);
                                    }
                                }
                            });

            tarOutStream.finish();
        }
    }

    /**
     * Deletes all files in the given directory, leaving the directory itself intact.
     *
     * <p>This function is recursive, and will delete all sub-directories found in the given
     * directory.
     *
     * @param dir Directory in which to delete files
     * @return True if successful
     */
    public static boolean deleteFilesInDirectory(File dir) {
        if (!dir.isDirectory()) {
            logger.warn("Not a directory: " + dir.getAbsolutePath());
            return false;
        }

        boolean success = true;
        for (File file : dir.listFiles()) {
            if (file.isDirectory()) {
                deleteFilesInDirectory(file);
            }

            // If this is a subdirectory and a file couldn't be deleted inside it,
            // the subdirectory will fail to delete as well since it isn't empty.
            if (!file.delete()) {
                logger.warn("Unable to delete: " + file.getAbsolutePath());
                success = false;
            }
        }

        return success;
    }

    private static TarArchiveInputStream openTarArchiveInputStream(
            InputStream inputStream, boolean gzip) throws IOException {
        if (gzip) {
            GzipCompressorInputStream gzipInputStream = new GzipCompressorInputStream(inputStream);
            return new TarArchiveInputStream(gzipInputStream);
        }
        return new TarArchiveInputStream(inputStream);
    }

    /**
     * Extracts the contents of the specified tar archive into the specified output directory.
     *
     * @param archiveFile Input archive file
     * @param outputDirectory Directory in which to place extracted contents
     * @param gzip Whether archive file has been gzip compressed
     * @throws IOException if an error occurs
     */
    public static void extractArchive(File archiveFile, File outputDirectory, boolean gzip)
            throws IOException {
        if (!archiveFile.exists() || !archiveFile.isFile()) {
            throw new IllegalArgumentException("Archive file does not exist or is not a file");
        }

        if (outputDirectory.exists() && !outputDirectory.isDirectory()) {
            throw new IllegalArgumentException("Output directory exists but is not a directory");
        }

        logger.trace(
                "Extracting "
                        + archiveFile.getAbsolutePath()
                        + " to "
                        + outputDirectory.getAbsolutePath());

        try (FileInputStream inputStream = new FileInputStream(archiveFile);
                TarArchiveInputStream archiveInputStream =
                        openTarArchiveInputStream(inputStream, gzip); ) {
            TarArchiveEntry entry = null;
            while ((entry = archiveInputStream.getNextTarEntry()) != null) {
                File dest = new File(outputDirectory, entry.getName());
                extractEntryFromArchive(entry.isDirectory(), dest, archiveInputStream);
            }
        }
    }

    /**
     * Extracts the contents of the specified zip archive into the specified output directory.
     *
     * @param archiveFile Input archive file
     * @param outputDirectory Directory in which to place extracted contents
     * @throws IOException if an error occurs
     */
    public static void extractZipFile(File archiveFile, File outputDirectory) throws IOException {
        if (!archiveFile.exists() || !archiveFile.isFile()) {
            throw new IllegalArgumentException("Zip file does not exist or is not a file");
        }

        if (outputDirectory.exists() && !outputDirectory.isDirectory()) {
            throw new IllegalArgumentException("Output directory exists but is not a directory");
        }

        logger.trace(
                "Extracting "
                        + archiveFile.getAbsolutePath()
                        + " to "
                        + outputDirectory.getAbsolutePath());

        try (ZipFile zipFile = new ZipFile(archiveFile)) {
            Enumeration<? extends ZipEntry> entries = zipFile.entries();
            while (entries.hasMoreElements()) {
                ZipEntry entry = entries.nextElement();
                File dest = new File(outputDirectory, entry.getName());
                extractEntryFromArchive(entry.isDirectory(), dest, zipFile.getInputStream(entry));
            }
        }
    }

    /**
     * Extracts the next entry (file or directory) from the archive input stream.
     *
     * @param isDirectory Next entry is a directory
     * @param destFile Destination location for the entry
     * @param inputStream Archive input stream
     * @throws IOException if an error occurs
     */
    private static void extractEntryFromArchive(
            boolean isDirectory, File destFile, InputStream inputStream) throws IOException {
        if (isDirectory) {
            logger.trace("Creating directory {}", destFile.getAbsolutePath());
            if (!destFile.exists()) {
                if (!destFile.mkdirs()) {
                    throw new IOException(
                            "Unable to create directory entry " + destFile.getAbsolutePath());
                }
            }
        } else {
            File parentDir = destFile.getParentFile();
            if (!parentDir.isDirectory() && !parentDir.mkdirs()) {
                throw new IOException(
                        "Unable to create parent directory " + parentDir.getAbsolutePath());
            }

            logger.trace("Extracting file {}", destFile.getAbsolutePath());
            Files.copy(inputStream, destFile.toPath(), StandardCopyOption.REPLACE_EXISTING);
        }
    }

    /**
     * Returns the temporary directory applicable for the current platform.
     *
     * @return Temp directory
     */
    public static String getTempDirectory() {
        return System.getProperty("java.io.tmpdir");
    }
}
