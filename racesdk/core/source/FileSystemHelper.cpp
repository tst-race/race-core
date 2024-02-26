
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

#include "FileSystemHelper.h"

#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <fstream>
#include <vector>

#include "filesystem.h"
#include "helper.h"

FileSystemHelper::FileSystemHelper() {}

bool FileSystemHelper::copyAndDecryptDir(const std::string &srcPath, const std::string &destPath,
                                         StorageEncryption &pluginStorageEncryption) {
    TRACE_FUNCTION(srcPath, destPath);
    try {
        if (!fs::exists(srcPath) || !fs::is_directory(srcPath)) {
            helper::logWarning(logPrefix +
                               "source directory does not exist or is not a directory: " + srcPath);
            return false;
        }

        if (!fs::exists(destPath)) {
            if (!fs::create_directory(destPath)) {
                helper::logWarning(logPrefix +
                                   "unable to create destination directory: " + destPath);
                return false;
            }
        }

        for (auto &iter : fs::recursive_directory_iterator(srcPath)) {
            auto relPath = iter.path().string();
            // Remove the src prefix, leaving a relative path under src/dest
            relPath.erase(0, srcPath.size());

            fs::path newPath = destPath + "/" + relPath;

            // TODO: probably a cleaner way to do this, but do NOT encrypt any of these files.
            // The jaegare config and the deployment name should be moved to another location.
            // for the file_key not sure how to handle this. maybe store elsewhere would make
            // sense as well.
            std::vector<std::string> filesToNotDecrypt = {"jaeger-config.yml", "deployment.txt"};

            // TODO: probably a cleaner way to do this...
            std::vector<std::string> filesToNotCopy = {"file_key", "user-input-response-cache.json",
                                                       "userEnabledChannels"};

            if (fs::is_directory(iter.path())) {
                helper::logDebug(logPrefix + "creating directory " + newPath.string());
                fs::create_directory(newPath);
            } else {
                // files to not copy
                bool shouldCopy = true;
                for (auto fileName : filesToNotCopy) {
                    if (iter.path().filename().string().find(fileName) != std::string::npos) {
                        shouldCopy = false;
                        break;
                    }
                }
                if (!shouldCopy) {
                    continue;
                }

                // check files to decrypt
                bool shouldEncryptThisFile = true;
                for (auto fileName : filesToNotDecrypt) {
                    if (iter.path().filename().string().find(fileName) != std::string::npos) {
                        shouldEncryptThisFile = false;
                        break;
                    }
                }
                if (!shouldEncryptThisFile) {
                    helper::logDebug(logPrefix + "copying " + iter.path().string() + " to " +
                                     newPath.string());
                    fs::copy(iter.path(), newPath);
                } else {
                    helper::logDebug(logPrefix + "copying and decrypting " + iter.path().string() +
                                     " to " + newPath.string());

                    try {
                        std::vector<std::uint8_t> decryptedFile =
                            pluginStorageEncryption.read(iter.path().string());

                        // Open the file in truncate mode. This will overwrite any existing file
                        // content.
                        std::ofstream file(newPath.string(), std::ofstream::trunc);
                        if (file.fail()) {
                            throw std::runtime_error(
                                "copyAndDecryptDir::write: failed to open output file: " +
                                newPath.string());
                        }
                        file.write(reinterpret_cast<const char *>(decryptedFile.data()),
                                   static_cast<std::int64_t>(decryptedFile.size()));
                        if (file.fail()) {
                            throw std::runtime_error(
                                "copyAndDecryptDir::write: failed to write file: " +
                                newPath.string());
                        }
                    } catch (...) {
                    }
                }
            }
        }

        return true;
    } catch (const fs::filesystem_error &error) {
        helper::logError(logPrefix + "error: " + std::string(error.what()));
        return false;
    }
}

// Wrap libarchive's structs so they are automatically closed/freed when destroyed
struct Archive {
    archive *obj;

    Archive() : obj(archive_write_new()) {}
    ~Archive() {
        if (obj != nullptr) {
            archive_write_close(obj);
            archive_write_free(obj);
        }
    }

    operator archive *() const {
        return obj;
    }
};

struct ArchiveEntry {
    archive_entry *obj;

    ArchiveEntry() : obj(archive_entry_new()) {}
    ~ArchiveEntry() {
        if (obj != nullptr) {
            archive_entry_free(obj);
        }
    }

    operator archive_entry *() const {
        return obj;
    }
};

bool FileSystemHelper::createZip(const std::string &zipFilePath,
                                 const std::string &sourceDirectoryPath) {
    TRACE_FUNCTION(zipFilePath, sourceDirectoryPath);
    if (!fs::exists(sourceDirectoryPath) || !fs::is_directory(sourceDirectoryPath)) {
        helper::logError(logPrefix + "source directory does not exist or is not a directory: " +
                         sourceDirectoryPath);
        return false;
    }

    Archive zipFile;
    if (zipFile.obj == nullptr) {
        helper::logError(logPrefix + "failed to create archive object");
        return false;
    }

    try {
        if (archive_write_set_format_zip(zipFile) != ARCHIVE_OK) {
            throw std::runtime_error("failed to set zip format");
        }
        if (archive_write_open_filename(zipFile, zipFilePath.c_str()) != ARCHIVE_OK) {
            throw std::runtime_error("failed to open " + zipFilePath);
        }

        ArchiveEntry entry;
        if (entry.obj == nullptr) {
            helper::logError(logPrefix + "failed to create archive entry object");
            return false;
        }

        struct stat st;

        for (auto &iter : fs::recursive_directory_iterator(
                 sourceDirectoryPath, fs::directory_options::follow_directory_symlink)) {
            auto absPath = iter.path().string();
            auto relPath = absPath.substr(sourceDirectoryPath.size() + 1);

            if (not fs::is_directory(iter.path())) {
                stat(absPath.c_str(), &st);
                archive_entry_set_pathname(entry, relPath.c_str());
                archive_entry_set_size(entry, st.st_size);
                archive_entry_set_filetype(entry, AE_IFREG);
                archive_entry_set_perm(entry, 0644);
                if (archive_write_header(zipFile, entry) != ARCHIVE_OK) {
                    throw std::runtime_error("failed to write header for " + relPath);
                }

                std::vector<char> buff(8192);
                int fd = open(absPath.c_str(), O_RDONLY);
                auto len = read(fd, buff.data(), buff.size());
                while (len > 0) {
                    if (archive_write_data(zipFile, buff.data(), static_cast<size_t>(len)) < 0) {
                        throw std::runtime_error("error writing data for " + relPath);
                    }
                    len = read(fd, buff.data(), buff.size());
                }
                close(fd);
            }

            archive_entry_clear(entry);
        }
    } catch (const std::exception &e) {
        helper::logError(logPrefix + std::string(e.what()) + ": " +
                         std::string(archive_error_string(zipFile)));
        return false;
    }

    return true;
}