
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

#include "ConfigLogging.h"

#include <RaceLog.h>
#include <openssl/err.h>
#include <openssl/evp.h>

#include <boost/io/ios_state.hpp>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>

#include "filesystem.h"
#include "helper.h"

static std::set<std::string> logContentsExtensions{
    ".cgf", ".json", ".mdf", ".toml", ".xml",
};
static std::set<std::string> logSignatureExtensions{
    ".npy",
    ".pb",
};

static void recursivePrintTree(std::ostream &o, const fs::directory_entry &entry,
                               const std::string &prefix = "") {
    std::string new_prefix = prefix + "    ";
    for (const fs::directory_entry &d : fs::directory_iterator(entry)) {
        o << new_prefix << d.path().filename().string() << "\n";
        if (fs::is_directory(d.status())) {
            recursivePrintTree(o, d, new_prefix);
        }
    }
}

constexpr std::streamsize BUFFER_SIZE = 4096;

static void handleOpensslError() {
    unsigned long err;
    while ((err = ERR_get_error()) != 0) {
        helper::logError(std::string("Error with OpenSSL call: ") + ERR_error_string(err, NULL));
    }
    throw std::logic_error("Error with OpenSSL call");
}

static void printFileSignature(std::ostream &o, const fs::directory_entry &file) {
    char buffer[BUFFER_SIZE];
    std::streamsize count;
    std::filebuf in;

    constexpr int SIGNATURE_SIZE = 20;
    const EVP_MD *md = EVP_sha1();
    std::uint8_t m[SIGNATURE_SIZE];
    int res;

    // Verify hash size to ensure no writes ococur outside of buffer's memory
    if (std::size_t(EVP_MD_size(md)) != SIGNATURE_SIZE) {
        throw std::logic_error(
            "Unexpected size for hash. Expected: " + std::to_string(SIGNATURE_SIZE) +
            ", Received: " + std::to_string(EVP_MD_size(md)));
    }

    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
    if (ctx == nullptr) {
        handleOpensslError();
    }

    res = EVP_DigestInit_ex(ctx.get(), md, NULL);
    if (res != 1) {
        handleOpensslError();
    }

    if (in.open(file.path().string(), std::ios_base::in) == nullptr) {
        helper::logError("failed to open file: " + file.path().string());
        return;
    }
    do {
        count = in.sgetn(buffer, BUFFER_SIZE);
        if (count > 0) {
            res = EVP_DigestUpdate(ctx.get(), buffer, std::size_t(count));
            if (res != 1) {
                handleOpensslError();
            }
        }
    } while (count == BUFFER_SIZE);
    in.close();

    res = EVP_DigestFinal_ex(ctx.get(), m, NULL);
    if (res != 1) {
        handleOpensslError();
    }

    o << " --- " << file.path().string() << " --- SHA1: ";
    boost::io::ios_flags_saver flags(o, std::ios_base::hex | std::ios_base::uppercase);
    boost::io::ios_fill_saver fill(o, '0');
    for (int i = 0; i < 6; ++i) {
        o << std::setw(2) << +m[i];
    }
    o << "\n";
}

void logDirectoryTree(const std::string &dir, StorageEncryption &pluginStorageEncryption) {
    std::ostream &o = RaceLog::getLogStream(RaceLog::LL_INFO);
    try {
        fs::path dirpath(dir);
        o << " --- Begin logging directory tree --- \n";
        o << dirpath.string() << "\n";
        recursivePrintTree(o, fs::directory_entry(dirpath));
        o << " --- End logging directory tree --- "
          << "\n";

        for (const fs::directory_entry &f : fs::recursive_directory_iterator(dirpath)) {
            if (fs::is_regular_file(f.status())) {
                std::string ext = f.path().extension().string();
                if (logContentsExtensions.count(ext)) {
                    try {
                        o << " --- Contents of " << f.path().string() << " --- \n";
                        std::vector<std::uint8_t> configData =
                            pluginStorageEncryption.read(f.path().string());
                        o << configData.data();
                        o << "\n --- End File Contents --- \n";
                    } catch (const std::exception &error) {
                        helper::logWarning("readFile error: " + f.path().string() + ": " +
                                           std::string(error.what()));
                    }
                } else if (logSignatureExtensions.count(ext)) {
                    printFileSignature(o, f);
                }
            }
        }

        o << std::flush;
    } catch (fs::filesystem_error &e) {
        o << std::flush;
        helper::logError("Error logging directory tree for " + dir + " : " + e.what());
    }
}
