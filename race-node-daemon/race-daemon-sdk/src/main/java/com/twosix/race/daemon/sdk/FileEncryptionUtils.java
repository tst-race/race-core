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
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.security.InvalidAlgorithmParameterException;
import java.security.InvalidKeyException;
import java.security.Key;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;

import javax.crypto.Cipher;
import javax.crypto.NoSuchPaddingException;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;

public class FileEncryptionUtils {

    private static final Logger logger = LoggerFactory.getLogger(FileUtils.class);

    private static Key readFileKey(String pathToKey) throws Exception {
        final String logPrefix = "FileEncryptionUtils::readFileKey: ";
        // Read the key from file.
        Key key;

        InputStream keyInputStream;
        try {
            keyInputStream = new FileInputStream(pathToKey);
        } catch (FileNotFoundException e) {
            throw new Exception(
                    logPrefix + "failed to find key file " + pathToKey + ". " + e.getMessage());
        }

        byte[] keyBuffer = new byte[32];
        int keyBufferReadLength;
        try {
            keyBufferReadLength = keyInputStream.read(keyBuffer);
        } catch (IOException e) {
            throw new Exception(
                    logPrefix
                            + "failed to read key buffer from source file: "
                            + pathToKey
                            + ". "
                            + e.getMessage());
        }
        if (keyBufferReadLength != keyBuffer.length) {
            throw new Exception(
                    logPrefix
                            + "failed to read key buffer."
                            + " Incorrect size of "
                            + String.valueOf(keyBufferReadLength)
                            + ". Expected "
                            + String.valueOf(keyBuffer.length));
        }

        try {
            key = new SecretKeySpec(keyBuffer, 0, keyBuffer.length, "AES");
        } catch (Exception e) {
            throw new Exception(
                    logPrefix
                            + "failed to read file key input stream: "
                            + pathToKey
                            + ". "
                            + e.getMessage());
        }

        return key;
    }

    /**
     * Decrypts the individual files and copies the contents of the given source directory into the
     * destination directory.
     *
     * @param sourcePath Source directory path.
     * @param destPath Destination directory path containing the decrypted files.
     * @param pathToKey Path to the key used for encryption, e.g. "/etc/race/file_key" on Linux.
     * @param overwrite Flag to signal if an existing directory should be overwritten.
     * @throws IOException if an error occurs.
     * @throws InterruptedException if the decryption process fails.
     */
    public static void decryptDirectory(
            Path sourcePath, Path destPath, String pathToKey, boolean overwrite)
            throws IOException {
        final String logPrefix = "decryptDirectory: ";

        if (destPath.toFile().exists()) {
            if (overwrite) {
                FileUtils.deleteFilesInDirectory(destPath.toFile());
            } else {
                throw new IOException("Directory already exists: " + destPath);
            }
        } else if (!destPath.toFile().mkdirs()) {
            throw new IOException("Unable to create destination path: " + destPath);
        }

        File keyFile = new File(pathToKey);
        if (!keyFile.exists()) {
            throw new IOException(logPrefix + "Key file \"" + pathToKey + "\" does not exist.");
        }

        Files.walk(sourcePath)
                .forEach(
                        srcFilePath -> {
                            if (srcFilePath.toFile().isFile()) {
                                Path relPath = sourcePath.relativize(srcFilePath);

                                // TODO: this is a temporary hack to avoid copying any
                                // potential bootstrap files. This logic is to be removed
                                // once the encrypted directory structure is fixed in
                                // RaceSdk.
                                if (relPath.toString().contains("/bootstrap-cache/")
                                        || relPath.toString().contains("/bootstrap-files/")) {
                                    logger.warn(
                                            logPrefix
                                                    + "skipping irrelevant file: \""
                                                    + relPath
                                                    + "\"");
                                    return;
                                }

                                Path destFilePath = destPath.resolve(relPath);
                                if (!destFilePath.getParent().toFile().exists()) {
                                    destFilePath.getParent().toFile().mkdirs();
                                }

                                if (keyFile.length() == 0) {
                                    // If the key file is empty then encryption is disabled. Simply
                                    // copy the file.
                                    try {
                                        Files.copy(
                                                srcFilePath,
                                                destFilePath,
                                                StandardCopyOption.REPLACE_EXISTING);
                                    } catch (IOException e) {
                                        logger.error(
                                                "Unable to copy "
                                                        + relPath
                                                        + ": "
                                                        + e.getMessage());
                                    }
                                } else {

                                    Cipher cipher;
                                    try {
                                        cipher = Cipher.getInstance("AES/CBC/PKCS5Padding");
                                    } catch (NoSuchAlgorithmException e) {
                                        logger.error(
                                                logPrefix
                                                        + "failed to get cipher for encryption"
                                                        + " type :"
                                                        + e.getMessage());
                                        return;
                                    } catch (NoSuchPaddingException e) {
                                        logger.error(
                                                logPrefix
                                                        + "failed to get cipher for padding type :"
                                                        + e.getMessage());
                                        return;
                                    }

                                    // Read the first 16 bytes (128 bits) to get the initialization
                                    // vector (iv) from the file.
                                    byte[] ivBuffer = new byte[16];
                                    InputStream sourceFileInputStream;
                                    try {
                                        sourceFileInputStream =
                                                new FileInputStream(srcFilePath.toString());
                                    } catch (FileNotFoundException e) {
                                        logger.error(
                                                logPrefix
                                                        + "failed to find "
                                                        + srcFilePath.toString()
                                                        + ". "
                                                        + e.getMessage());
                                        return;
                                    }
                                    int ivBufferReadLength;
                                    try {
                                        ivBufferReadLength = sourceFileInputStream.read(ivBuffer);
                                    } catch (IOException e) {
                                        logger.error(
                                                logPrefix
                                                        + "failed to read IV buffer from source"
                                                        + " file: "
                                                        + srcFilePath.toString()
                                                        + ". "
                                                        + e.getMessage());
                                        return;
                                    }
                                    if (ivBufferReadLength != ivBuffer.length) {
                                        logger.error(
                                                logPrefix
                                                        + "failed to read iv buffer."
                                                        + " Incorrect size of "
                                                        + String.valueOf(ivBufferReadLength)
                                                        + ". Expected "
                                                        + String.valueOf(ivBuffer.length));
                                        return;
                                    }
                                    try {
                                        sourceFileInputStream.close();
                                    } catch (IOException e) {
                                        logger.error(
                                                logPrefix
                                                        + "failed to close source file: "
                                                        + srcFilePath.toString()
                                                        + ". "
                                                        + e.getMessage());
                                        return;
                                    }

                                    IvParameterSpec iv = new IvParameterSpec(ivBuffer);

                                    // Read the key from file.
                                    Key key;
                                    try {
                                        key = readFileKey(pathToKey);
                                    } catch (Exception e) {
                                        logger.error(
                                                logPrefix
                                                        + "failed to read key file "
                                                        + pathToKey
                                                        + ". "
                                                        + e.getMessage());
                                        return;
                                    }

                                    try {
                                        cipher.init(Cipher.DECRYPT_MODE, key, iv);
                                    } catch (InvalidKeyException e) {
                                        logger.error(
                                                logPrefix
                                                        + "failed to create cipher. invalid key: "
                                                        + e.getMessage());
                                        return;
                                    } catch (InvalidAlgorithmParameterException e) {
                                        logger.error(
                                                logPrefix
                                                        + "failed to create cipher. invalid"
                                                        + " algorithm parameter: "
                                                        + e.getMessage());
                                        return;
                                    }

                                    byte[] cipherText;
                                    try {
                                        cipherText = Files.readAllBytes(srcFilePath);
                                    } catch (IOException e) {
                                        logger.error(
                                                logPrefix
                                                        + "failed to read bytes from cipher text"
                                                        + " file: "
                                                        + srcFilePath.toString()
                                                        + ". "
                                                        + e.getMessage());
                                        return;
                                    }

                                    byte[] plainText;
                                    try {
                                        // The iv is stored at the beginning of the file, so skip
                                        // that.
                                        plainText =
                                                cipher.doFinal(
                                                        Arrays.copyOfRange(
                                                                cipherText,
                                                                ivBufferReadLength,
                                                                cipherText.length));
                                    } catch (Exception e) {
                                        logger.error(
                                                logPrefix
                                                        + "failed to decode file \""
                                                        + srcFilePath.toString()
                                                        + "\". Reverting to plain copy. Error"
                                                        + " message: "
                                                        + e.getMessage());
                                        // TODO: If this fails it likely means the file is _not_
                                        // encrypted. This fallback to a plain copy is a hack since
                                        // out data dir is currently a mix of encrypted and
                                        // unecrypted files (e.g. jaeger.config). In RaceSdk we
                                        // whitelist certain files. Could do the same here, but
                                        // would prefer to just copy as it will make debugging
                                        // easier if a file gets missed from the whitelist. In any
                                        // case, this logic should be removed once the encrypted
                                        // directory structure is sorted out on the RaceSdk side.
                                        try {
                                            Files.copy(
                                                    srcFilePath,
                                                    destFilePath,
                                                    StandardCopyOption.REPLACE_EXISTING);
                                        } catch (IOException error) {
                                            logger.error(
                                                    "Fallback copy operation failed. Failed to"
                                                            + " copy \""
                                                            + relPath
                                                            + "\": "
                                                            + error.getMessage());
                                        }
                                        return;
                                    }

                                    // Write the decoded source file to the destination.
                                    try {
                                        FileOutputStream stream =
                                                new FileOutputStream(destFilePath.toString());
                                        stream.write(plainText);
                                    } catch (Exception e) {
                                        logger.error(
                                                logPrefix
                                                        + "failed to write decoded output to file: "
                                                        + destFilePath.toString()
                                                        + ". "
                                                        + e.getMessage());
                                        return;
                                    }
                                }
                            }
                        });
    }
}
