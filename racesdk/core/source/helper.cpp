
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

#include "helper.h"

#include <LinkStatus.h>
#include <RaceLog.h>
#include <archive.h>        // extracting tar.gz
#include <archive_entry.h>  // archive_entry_size
#include <fcntl.h>
#include <jaegertracing/Tracer.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>  // memcpy
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

#include "nlohmann/json.hpp"

#define IV_LENGTH 16

using json = nlohmann::json;

bool helper::doesConnectionIncludeGivenPersonas(
    const std::vector<std::string> &connectionProfilePersonas,
    const std::vector<std::string> &givenPersonas) {
    for (const auto &givenPersona : givenPersonas) {
        if (std::find(connectionProfilePersonas.begin(), connectionProfilePersonas.end(),
                      givenPersona) == connectionProfilePersonas.end()) {
            return false;
        }
    }

    return true;
}

static void handleOpensslError() {
    unsigned long err;
    while ((err = ERR_get_error()) != 0) {
        helper::logError(std::string("Error with OpenSSL call: ") + ERR_error_string(err, NULL));
    }
    throw std::logic_error("Error with OpenSSL call");
}

std::string helper::getMessageSignature(const ClrMsg &msg) {
    constexpr int SIGNATURE_SIZE = 20;
    const EVP_MD *md = EVP_sha1();
    std::uint8_t m[SIGNATURE_SIZE];
    int res;

    // Verify hash size to ensure no writes ococur outside of MsgSignature's memory
    if (std::size_t(EVP_MD_size(md)) != SIGNATURE_SIZE) {
        throw std::logic_error(
            "Unexpected size for hash. Expected: " + std::to_string(SIGNATURE_SIZE) +
            ", Received: " + std::to_string(EVP_MD_size(md)));
    }

    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
    if (ctx == nullptr)
        handleOpensslError();

    res = EVP_DigestInit_ex(ctx.get(), md, NULL);
    if (res != 1)
        handleOpensslError();

    const std::string &message = msg.getMsg();
    res = EVP_DigestUpdate(ctx.get(), message.data(), message.size());
    if (res != 1)
        handleOpensslError();

    res = EVP_DigestFinal_ex(ctx.get(), m, NULL);
    if (res != 1)
        handleOpensslError();

    // Log hash for debugging
    std::stringstream s;
    s << std::hex << std::setfill('0');
    for (int i = 0; i < 6; ++i) {
        s << std::setw(2) << +m[i];
    }

    return s.str();
}

std::string helper::personasToString(const std::vector<std::string> &personas) {
    if (personas.size() == 0) {
        return "";
    }

    std::size_t resultLen = (personas.size() - 1) * 2;
    for (std::size_t index = 0; index < personas.size(); ++index) {
        resultLen += personas[index].size();
    }

    std::string result;
    result.reserve(resultLen);
    result += personas[0];
    for (std::size_t index = 1; index < personas.size(); ++index) {
        result += ", ";
        result += personas[index];
    }
    return result;
}

static const std::string pluginNameForLogging = "RaceSdkCore";

void helper::logDebug(const std::string &message, const std::string &stackTrace) {
    RaceLog::logDebug(pluginNameForLogging, message, stackTrace);
}

void helper::logInfo(const std::string &message, const std::string &stackTrace) {
    RaceLog::logInfo(pluginNameForLogging, message, stackTrace);
}

void helper::logWarning(const std::string &message, const std::string &stackTrace) {
    RaceLog::logWarning(pluginNameForLogging, message, stackTrace);
}

void helper::logError(const std::string &message, const std::string &stackTrace) {
    RaceLog::logError(pluginNameForLogging, message, stackTrace);
}

void helper::logLinkChange(const std::string &linkId, LinkStatus linkStatus,
                           const personas::PersonaSet &personaSet) {
    json update;
    update["linkId"] = linkId;
    if (linkStatus == LINK_DESTROYED) {
        update["status"] = "LINK_DESTROYED";
    } else if (linkStatus == LINK_CREATED) {
        update["status"] = "LINK_CREATED";
    } else if (linkStatus == LINK_LOADED) {
        update["status"] = "LINK_LOADED";
    }
    update["personas"] = personaSet;

    RaceLog::logInfo(pluginNameForLogging, "LinkChange:", update.dump());
}

int helper::readInt(const std::vector<uint8_t> &buf, uint64_t *offset, int64_t *ret) {
    if (!offset || !ret || *offset > buf.size() || (*offset + sizeof(*ret)) > buf.size()) {
        return -1;
    }

    memcpy(ret, &buf[*offset], sizeof(*ret));
    *offset += sizeof(*ret);
    return 0;
}

std::string helper::convertToHexString(size_t input, size_t paddedLength) {
    std::stringstream stream;
    stream << std::setfill('0') << std::setw(paddedLength) << std::hex << input;
    return stream.str();
}

size_t helper::convertFromHexString(const std::string &hexString) {
    if (hexString.length() > 0 && hexString[0] == '-') {
        return 0;
    }

    try {
        // TODO: any valid concerns for casting 'unsigned long' to 'size_t'?
        return static_cast<size_t>(std::stoul(hexString, nullptr, 16));
    } catch (std::invalid_argument &err) {
    } catch (std::out_of_range &err) {
    }

    return 0;
}

std::vector<uint8_t> helper::hexStringToByteVector(const std::string &hex) {
    std::vector<uint8_t> out;

    if (hex.size() % 2 != 0) {
        helper::logError(
            "hexStringToByteVector error: hex string is ill-formed. Size must be even.");
        return {};
    }

    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        std::string hex_byte(hex, i, 2);
        try {
            out.push_back(std::stoi(hex_byte, nullptr, 16));
        } catch (std::invalid_argument &err) {
            helper::logError(
                "hexStringToByteVector error: got std::invalid_argument when decoding hex byte "
                "(non-hex characters in hex string)");
            return {};
        } catch (std::out_of_range &err) {
            helper::logError(
                "hexStringToByteVector error: got std::out_of_range when decoding hex byte");
            return {};
        }
    }

    return out;
}

std::string helper::byteVectorToHexString(const std::vector<uint8_t> &bytes) {
    std::vector<char> out(2 * bytes.size() + 1);

    for (size_t i = 0; i < bytes.size(); ++i) {
        snprintf(out.data() + (2 * i), 3, "%02hhx", bytes[i]);
    }

    return out.data();
}

std::string helper::sdkStatusToString(SdkStatus status) {
    static std::unordered_map<SdkStatus, std::string> map = {
        {SDK_INVALID, "SDK_INVALID"},
        {SDK_OK, "SDK_OK"},
        {SDK_SHUTTING_DOWN, "SDK_SHUTTING_DOWN"},
        {SDK_PLUGIN_MISSING, "SDK_PLUGIN_MISSING"},
        {SDK_INVALID_ARGUMENT, "SDK_INVALID_ARGUMENT"},
        {SDK_QUEUE_FULL, "SDK_QUEUE_FULL"}};

    try {
        return map.at(status);
    } catch (...) {
        return "UNKNOWN SDK STATUS CODE '" + std::to_string(static_cast<int>(status));
    }
}

std::string helper::pluginResponseToString(PluginResponse status) {
    static std::unordered_map<PluginResponse, std::string> map = {
        {PLUGIN_INVALID, "PLUGIN_INVALID"},
        {PLUGIN_OK, "PLUGIN_OK"},
        {PLUGIN_TEMP_ERROR, "PLUGIN_TEMP_ERROR"},
        {PLUGIN_ERROR, "PLUGIN_ERROR"},
        {PLUGIN_FATAL, "PLUGIN_FATAL"}};

    try {
        return map.at(status);
    } catch (...) {
        return "UNKNOWN PLUGIN STATUS CODE '" + std::to_string(static_cast<int>(status));
    }
}

/**
 * @brief Construct a filepath based on a filepath, pluginId, and configs directory path. Helper
 * function used to create the path for a specific plugin's file.
 * prevent directory traversals based on filepaths.
 *
 * @param filepathStr The string name of the file
 * @param pluginName The string pluginId the file is associated with
 * @param configsPath the string path to the local configs dircetory
 * @return fs::path Full path to the filepath, unless the filepath violated the directory traversal
 * restrictions in which case it returns fs::path()
 */
fs::path helper::makePluginFilepath(const std::string &filepathStr, const std::string &pluginName,
                                    const std::string &configsPath) {
    return fs::path(configsPath) / pluginName / filepathStr;
}

/**
 * @brief Read the contents of a file in this plugin's storage.
 * @param filepath The string path of the file to be read.
 *
 * @return std::vector<uint8_t> of the file contents
 */

/**
 * @brief Read the contents of a file in this plugin's storage.
 *
 * @param filepathStr The path of the file to be read.
 * @param pluginId The ID of the plugin for which the file is being read.
 * @param configsPath The path of the configsPath directory.
 * @param pluginStorageEncryption The plugin storage utility.
 * @return std::vector<std::uint8_t> The contents of the file, or empty vector on error.
 */
std::vector<std::uint8_t> helper::readFile(const std::string &filepathStr,
                                           const std::string &pluginId,
                                           const std::string &configsPath,
                                           StorageEncryption &pluginStorageEncryption) {
    fs::path filepath = helper::makePluginFilepath(filepathStr, pluginId, configsPath);
    TRACE_FUNCTION(filepath.string());

    try {
        return pluginStorageEncryption.read(filepath.string());
    } catch (const std::exception &error) {
        logWarning("readFile error: " + filepath.string() + ": " + std::string(error.what()));
        return std::vector<std::uint8_t>();
    }
}

bool helper::appendFile(const std::string &filepathStr, const std::string &pluginId,
                        const std::string &configsPath, const std::vector<std::uint8_t> &data,
                        StorageEncryption &pluginStorageEncryption) {
    fs::path filepath = helper::makePluginFilepath(filepathStr, pluginId, configsPath);
    TRACE_FUNCTION(filepath.string());

    try {
        pluginStorageEncryption.append(filepath.string(), data);
    } catch (const std::exception &error) {
        logWarning("appendFile error: " + filepath.string() + ": " + std::string(error.what()));
        return false;
    }

    return true;
}

/**
 * @brief Create the directory of directoryPath, including any directories in the path that do
 * not yet exist
 * @param directoryPath the path of the directory to create.
 *
 * @return SdkResponse indicator of success or failure of the create
 */
bool helper::makeDir(const std::string &directoryPath, const std::string &pluginId,
                     const std::string &configsPath) {
    fs::path dirpath = helper::makePluginFilepath(directoryPath, pluginId, configsPath);
    TRACE_FUNCTION(dirpath.string());
    try {
        fs::create_directories(dirpath);
    } catch (const fs::filesystem_error &error) {
        helper::logError(
            "makeDir:: create_directories: failed to create intermediate directories for path: " +
            dirpath.string());
        return false;
    }
    return true;
}

/**
 * @brief Recurively remove the directory of directoryPath
 * @param directoryPath the path of the directory to remove.
 *
 * @return SdkResponse indicator of success or failure of the removal
 */
bool helper::removeDir(const std::string &directoryPath, const std::string &pluginId,
                       const std::string &configsPath) {
    fs::path dirpath = helper::makePluginFilepath(directoryPath, pluginId, configsPath);
    TRACE_FUNCTION(dirpath.string());
    int files_removed = 0;
    try {
        files_removed = fs::remove_all(dirpath);
    } catch (const fs::filesystem_error &error) {
        helper::logError("removeDir:: remove_all: failed to remove path: " + dirpath.string());
    }
    return files_removed > 0;
}

/**
 * @brief List the contents (directories and files) of the directory path
 * @param directoryPath the path of the directory to list.
 *
 * @return std::vector<std::string> list of directories and files
 */
std::vector<std::string> helper::listDir(const std::string &directoryPath,
                                         const std::string &pluginId,
                                         const std::string &configsPath) {
    fs::path dirpath = helper::makePluginFilepath(directoryPath, pluginId, configsPath);
    TRACE_FUNCTION(dirpath.string());
    std::vector<std::string> contents;
    try {
        auto dirIter = fs::directory_iterator(dirpath);
        for (auto &it : dirIter) {
            contents.push_back(it.path().filename().string());
        }
        return contents;
    } catch (const fs::filesystem_error &error) {
        helper::logError("listDir:: error creating directory_iterator for path: " +
                         dirpath.string());
        return std::vector<std::string>();
    }
}

bool helper::copyDir(const std::string &src, const std::string &dest) {
    TRACE_FUNCTION(src, dest);
    try {
        fs::path srcPath(src);
        fs::path destPath(dest);

        if (!fs::exists(srcPath)) {
            helper::logWarning(logPrefix + "source directory \"" + src + "\" does not exist.");
            return false;
        }

        if (!fs::is_directory(srcPath)) {
            helper::logWarning(logPrefix + "source \"" + src + "\" is not a directory.");
            return false;
        }

        // Create the destination directory if it does not exist.
        if (!fs::exists(destPath) && !fs::create_directory(destPath)) {
            helper::logWarning(logPrefix + "unable to create destination directory: " + dest);
            return false;
        }

        for (auto &iter : fs::recursive_directory_iterator(srcPath)) {
            auto relPath = iter.path().string();
            // Remove the src prefix, leaving a relative path under src/dest
            relPath.erase(0, srcPath.string().size());

            fs::path newPath = destPath / relPath;

            if (fs::is_directory(iter.path())) {
                helper::logDebug(logPrefix + "creating directory " + newPath.string());
                fs::create_directory(newPath);
            } else {
                helper::logDebug(logPrefix + "copying " + iter.path().string() + " to " +
                                 newPath.string());
                fs::copy(iter.path(), newPath);
            }
        }

        return true;
    } catch (const fs::filesystem_error &error) {
        helper::logError(logPrefix + "error: " + std::string(error.what()));
        return false;
    }
}

/**
 * @brief Write the contents of data to filepath in this plugin's storage (overwriting if file
 * exists)
 * @param filepath The string path of the file to be written.
 * @param data The string of data to write to the file.
 *
 * @return SdkResponse indicator of success or failure of the write.
 */
bool helper::writeFile(const std::string &filepathStr, const std::string &pluginId,
                       const std::string &configsPath, const std::vector<std::uint8_t> &data,
                       StorageEncryption &pluginStorageEncryption) {
    fs::path filepath = helper::makePluginFilepath(filepathStr, pluginId, configsPath);
    TRACE_FUNCTION(filepath.string());

    try {
        pluginStorageEncryption.write(filepath.string(), data);
    } catch (const std::exception &error) {
        logWarning("readFile error encountered during read of file: " + filepath.string() + ": " +
                   std::string(error.what()));
        return false;
    }

    return true;
}

std::string helper::shell(const std::string &cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::string helper::stringToLowerCase(std::string input) {
    for (char &c : input) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    return input;
}

double helper::currentTime() {
    std::chrono::duration<double> sinceEpoch =
        std::chrono::high_resolution_clock::now().time_since_epoch();
    return sinceEpoch.count();
}

thread_local std::string thread_name;
void helper::set_thread_name(const std::string &name) {
    thread_name = name;
}

std::string helper::get_thread_name() {
    return thread_name;
}

static int copy_data(struct archive *ar, struct archive *aw) {
    const std::string logPrefix = "helper::extractConfigTarGz: copy_data: ";
    const void *buff;

    // TODO: update these god awful variable names throughout function.

    size_t size;
    la_int64_t offset;

    for (;;) {
        int r = archive_read_data_block(ar, &buff, &size, &offset);
        if (r == ARCHIVE_EOF) {
            return (ARCHIVE_OK);
        }
        if (r < ARCHIVE_OK) {
            return (r);
        }
        r = archive_write_data_block(aw, buff, size, offset);
        if (r < ARCHIVE_OK) {
            helper::logWarning(logPrefix + std::string(archive_error_string(aw)));
            return (r);
        }
    }
}

void helper::extractConfigTarGz(const std::string &filename, const std::string &destDir) {
    const std::string logPrefix = "helper::extractConfigTarGz: ";

    // TODO: update these god awful variable names.
    struct archive *a;
    struct archive *ext;
    struct archive_entry *entry;
    int flags;

    /* Select which attributes we want to restore. */
    flags = ARCHIVE_EXTRACT_TIME;
    flags |= ARCHIVE_EXTRACT_PERM;
    flags |= ARCHIVE_EXTRACT_ACL;
    flags |= ARCHIVE_EXTRACT_FFLAGS;

    a = archive_read_new();
    archive_read_support_format_all(a);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    archive_read_support_compression_all(a);
#pragma clang diagnostic pop
    ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, flags);
    archive_write_disk_set_standard_lookup(ext);
    if (archive_read_open_filename(a, filename.c_str(), 10240)) {
        throw std::runtime_error("helper::extractConfigTarGz: failed to open file: " + filename);
    }

    for (;;) {
        int r = archive_read_next_header(a, &entry);
        if (r == ARCHIVE_EOF) {
            break;
        }
        if (r < ARCHIVE_OK) {
            helper::logWarning(logPrefix + std::string(archive_error_string(a)));
        }
        if (r < ARCHIVE_WARN) {
            throw std::runtime_error(
                "helper::extractConfigTarGz: archive_write_finish_entry failed with error code: " +
                std::to_string(r));
        }
        const char *currentFile = archive_entry_pathname(entry);
        const std::string fullOutputPath = destDir + currentFile;
        archive_entry_set_pathname(entry, fullOutputPath.c_str());
        r = archive_write_header(ext, entry);
        if (r < ARCHIVE_OK) {
            helper::logWarning(logPrefix + std::string(archive_error_string(ext)));
        } else if (archive_entry_size(entry) > 0) {
            r = copy_data(a, ext);
            if (r < ARCHIVE_OK) {
                helper::logWarning(logPrefix + std::string(archive_error_string(ext)));
            }
            if (r < ARCHIVE_WARN) {
                throw std::runtime_error(
                    "helper::extractConfigTarGz: archive_write_finish_entry failed with error "
                    "code: " +
                    std::to_string(r));
            }
        }
        r = archive_write_finish_entry(ext);
        if (r < ARCHIVE_OK) {
            helper::logWarning(logPrefix + std::string(archive_error_string(ext)));
        }
        if (r < ARCHIVE_WARN) {
            throw std::runtime_error(
                "helper::extractConfigTarGz: archive_write_finish_entry failed with error code: " +
                std::to_string(r));
        }
    }
    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);
}

void helper::createConfigTarGz(const std::string &outname, const std::string &dirToTar) {
    // const std::string logPrefix = "helper::createConfigTarGz: ";

    struct archive *a;
    struct stat st;
    char buff[8192];

    a = archive_write_new();
    archive_write_add_filter_gzip(a);
    archive_write_set_format_pax_restricted(a);  // Note 1
    archive_write_open_filename(a, outname.c_str());

    if (!fs::exists(fs::path(dirToTar))) {
        helper::logError("Dir to tar does not exist");
        return;
    }
    for (const fs::directory_entry &d : fs::directory_iterator(dirToTar)) {
        std::string filename = d.path().filename().string();
        stat(filename.c_str(), &st);
        struct archive_entry *entry;
        entry = archive_entry_new();  // Note 2
        archive_entry_set_pathname(entry, filename.c_str());
        archive_entry_set_size(entry, st.st_size);  // Note 3
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);
        archive_write_header(a, entry);
        int fd = open(filename.c_str(), O_RDONLY);
        size_t len = static_cast<size_t>(read(fd, buff, sizeof(buff)));
        while (len > 0) {
            archive_write_data(a, buff, len);
            len = static_cast<size_t>(read(fd, buff, sizeof(buff)));
        }
        close(fd);
        archive_entry_free(entry);
    }
    archive_write_close(a);  // Note 4
    archive_write_free(a);   // Note 5
}
