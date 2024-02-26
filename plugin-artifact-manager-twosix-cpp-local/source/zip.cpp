
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

#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <fstream>
#include <vector>

#include "log.h"

#ifdef __ANDROID__
// Boost Filesystem is the original implementation. It may have subtle
// incompatibilities, but it is a widely available implementation.
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#else
// Filesystem TS is more broadly available, and is largely the same.
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#endif  //__ANDROID__

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

bool createZip(const std::string &zipFilePath, const std::string &sourceDirectoryPath) {
    TRACE_FUNCTION(zipFilePath, sourceDirectoryPath);
    if (!fs::exists(sourceDirectoryPath) || !fs::is_directory(sourceDirectoryPath)) {
        logError(logPrefix +
                 "source directory does not exist or is not a directory: " + sourceDirectoryPath);
        return false;
    }

    Archive zipFile;
    if (zipFile.obj == nullptr) {
        logError(logPrefix + "failed to create archive object");
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
            logError(logPrefix + "failed to create archive entry object");
            return false;
        }

        struct stat st;

        for (auto &iter : fs::recursive_directory_iterator(
                 sourceDirectoryPath, fs::directory_options::follow_directory_symlink)) {
            auto absPath = iter.path().string();
            fs::path sourceDirectoryPath2 = sourceDirectoryPath;
            sourceDirectoryPath2 = sourceDirectoryPath2.parent_path();
            auto relPath = absPath.substr(sourceDirectoryPath2.string().size() + 1);

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
        logError(logPrefix + std::string(e.what()) + ": " +
                 std::string(archive_error_string(zipFile)));
        return false;
    }

    return true;
}

#ifdef __ANDROID__
bool createApkZip(const std::string &zipFilePath, const std::string &apkPath) {
    TRACE_FUNCTION(zipFilePath, apkPath);

    std::string relPath = "race/race.apk";

    Archive zipFile;
    if (zipFile.obj == nullptr) {
        logError(logPrefix + "failed to create archive object");
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
            logError(logPrefix + "failed to create archive entry object");
            return false;
        }

        // can't use stat on the android apk for some reason
        int fd = open(apkPath.c_str(), O_RDONLY);
        if (!fd) {
            logError(logPrefix + "failed to open " + apkPath);
            return false;
        }

        long fileSize = lseek(fd, 0L, SEEK_END);
        if (fileSize == -1) {
            logError(logPrefix + "failed to seek");
            return false;
        }
        logDebug(logPrefix + "fileSize: " + std::to_string(fileSize));

        if (lseek(fd, 0L, SEEK_SET)) {
            logError(logPrefix + "failed to seek to beginning");
            return false;
        }

        archive_entry_set_pathname(entry, relPath.c_str());
        archive_entry_set_size(entry, fileSize);
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);
        if (archive_write_header(zipFile, entry) != ARCHIVE_OK) {
            throw std::runtime_error("failed to write header for " + relPath);
        }

        std::vector<char> buff(8192);
        long totalLen = 0;
        auto len = read(fd, buff.data(), buff.size());
        while (len > 0) {
            totalLen += len;
            if (archive_write_data(zipFile, buff.data(), static_cast<size_t>(len)) < 0) {
                throw std::runtime_error("error writing data for " + relPath);
            }
            len = read(fd, buff.data(), buff.size());
        }
        close(fd);

        if (fileSize != totalLen) {
            logError(logPrefix + "Amount read does not match file size " + apkPath);
            return false;
        }

        archive_entry_clear(entry);
    } catch (const std::exception &e) {
        logError(logPrefix + std::string(e.what()) + ": " +
                 std::string(archive_error_string(zipFile)));
        return false;
    }
    return true;
}
#endif
