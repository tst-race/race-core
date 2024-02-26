
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

#include "StorageEncryption.h"

#include <openssl/err.h>  // ERR_get_error
#include <openssl/evp.h>  // EVP_DecryptInit_ex, EVP_DecryptUpdate, EVP_DecryptFinal_ex, EVP_EncryptInit_ex, EVP_EncryptUpdate, EVP_EncryptFinal_ex
#include <openssl/rand.h>  // RAND_bytes

#include <fstream>
#include <sstream>  // std::stringstream
#include <vector>   // std::vector

#include "PassphraseHash.h"
#include "Salt.h"

#define FILE_KEY_LENGTH 32  // 256 bits
#define IV_LENGTH 16        // 128 bits

/**
 * @brief
 *
 */
static void handleOpensslError() {
    unsigned long err;
    std::string errorMessage;
    while ((err = ERR_get_error()) != 0) {
        errorMessage += ERR_error_string(err, NULL);
        errorMessage += ", ";
    }
    throw std::logic_error("Error with OpenSSL call: " + errorMessage);
}

void StorageEncryption::init(RaceEnums::StorageEncryptionType encType,
                             const std::string &passphrase, const std::string &keyDir) {
    workingDirectory = keyDir;
    this->createKey(encType, passphrase);
}

void StorageEncryption::createKey(RaceEnums::StorageEncryptionType encType,
                                  const std::string &passphrase) {
    fs::create_directory(workingDirectory);

    PassphraseHash passphraseHash(workingDirectory.string());
    const std::vector<std::uint8_t> salt = Salt::get(workingDirectory.string());

    if (encType == RaceEnums::StorageEncryptionType::ENC_AES) {
        if (passphraseHash.exists()) {
            if (!passphraseHash.compare(passphrase, salt)) {
                throw InvalidPassphrase("invalid passphrase");
            }
        } else {
            passphraseHash.create(passphrase, salt);
        }

        fileKey = std::vector<std::uint8_t>(FILE_KEY_LENGTH);
        PKCS5_PBKDF2_HMAC(passphrase.c_str(), passphrase.size(), salt.data(), salt.size(), 10000,
                          EVP_sha256(), fileKey.size(), fileKey.data());

    } else if (encType == RaceEnums::StorageEncryptionType::ENC_NONE) {
        // Create an empty hash file to signal that encryption is disabled.
        passphraseHash.create("", {});
    } else {
        throw std::runtime_error("StorageEncryption::createKey: ERROR: invalid encryption type: " +
                                 RaceEnums::storageEncryptionTypeToString(encType));
    }
}

RaceEnums::StorageEncryptionType StorageEncryption::getEncryptionType() {
    // NOTE: if we support multiple encryption types then we will need to come up with a better
    // scheme than this.
    // If the passphrase hash file exists and is empty then encryption is disabled.
    PassphraseHash passphraseHash(workingDirectory.string());
    if (passphraseHash.get().empty()) {
        return RaceEnums::StorageEncryptionType::ENC_NONE;
    } else {
        return RaceEnums::StorageEncryptionType::ENC_AES;
    }
}

std::vector<std::uint8_t> StorageEncryption::read(const std::string &fullFilePath) {
    // Check the encryption type, and raise an exception if key does not exist.
    const auto encType = getEncryptionType();

    fs::path filepath(fullFilePath);
    if (!fs::exists(filepath)) {
        throw std::runtime_error("StorageEncryption: failed to read file, does not exist: " +
                                 fullFilePath);
    } else {
        size_t fileSize = fs::file_size(filepath);
        std::ifstream file(filepath.native());
        if (file.fail()) {
            throw std::runtime_error(
                "StorageEncryption: an error occurred while trying to open file to read: " +
                fullFilePath);
        }
        std::vector<std::uint8_t> result(fileSize);
        file.read(reinterpret_cast<char *>(result.data()),
                  static_cast<int64_t>(static_cast<std::int64_t>(fileSize)));
        if (file.fail()) {
            throw std::runtime_error(
                "StorageEncryption: an error occurred while trying read file: " + fullFilePath);
        }
        if (encType == RaceEnums::StorageEncryptionType::ENC_AES &&
            isFileEncryptable(fullFilePath)) {
            return decrypt(result);
        } else {
            return result;
        }
    }
}

void StorageEncryption::write(const std::string &fullFilePath,
                              const std::vector<std::uint8_t> &data) {
    // Check the encryption type, and raise an exception if key does not exist.
    const auto encType = getEncryptionType();

    fs::path filepath(fullFilePath);
    fs::create_directories(filepath.parent_path());

    // Open the file in truncate mode. This will overwrite any existing file content.
    std::ofstream file(filepath.native(), std::ofstream::trunc);
    if (file.fail()) {
        throw std::runtime_error("StorageEncryption::write: failed to open output file: " +
                                 fullFilePath);
    }

    std::vector<std::uint8_t> writeData;
    if (encType == RaceEnums::StorageEncryptionType::ENC_AES && isFileEncryptable(fullFilePath)) {
        writeData = encrypt(data);
    } else {
        writeData = data;
    }

    file.write(reinterpret_cast<const char *>(writeData.data()),
               static_cast<std::int64_t>(writeData.size()));
    if (file.fail()) {
        throw std::runtime_error("StorageEncryption::write: failed to write file: " + fullFilePath);
    }
}

void StorageEncryption::append(const std::string &fullFilePath,
                               const std::vector<std::uint8_t> &data) {
    // Check the encryption type, and raise an exception if key does not exist.
    const auto encType = getEncryptionType();

    fs::path filepath(fullFilePath);
    std::vector<std::uint8_t> writeData;
    std::fstream file;
    if (!fs::exists(filepath)) {  // no existing file, just call writeFile
        return this->write(fullFilePath, data);
    }

    // existing file
    if (encType == RaceEnums::StorageEncryptionType::ENC_AES) {
        // deal with decryption end of file for appending
        size_t fileSize = fs::file_size(filepath);
        std::ifstream ifile(filepath.native());
        if (ifile.fail()) {
            throw std::runtime_error("appendFile could not open file: " + filepath.string());
        }
        // Seek to last two blocks
        long seekPos = static_cast<long>(fileSize) - (2 * IV_LENGTH);
        if (seekPos < 0) {
            throw std::runtime_error(
                "appendFile called with encryption key on an existing file of insufficient "
                "length " +
                std::to_string(fileSize) + ". Aborting append operation");
        }
        ifile.seekg(static_cast<std::streamoff>(seekPos));

        // Read penultimate block as the IV. Since we are using cipher-block-chaining, the previous
        // block is the IV of the current. We are rewriting the last block because it may have been
        // padded, so the IV is the second-to-last.
        std::vector<std::uint8_t> iv;
        iv.resize(IV_LENGTH);
        ifile.read(reinterpret_cast<char *>(iv.data()), IV_LENGTH);

        // read last block to remove padding before the new data
        std::vector<std::uint8_t> lastBlock;
        lastBlock.resize(IV_LENGTH);
        ifile.read(reinterpret_cast<char *>(lastBlock.data()), IV_LENGTH);
        ifile.close();
        try {
            lastBlock = decrypt(lastBlock, iv);
        } catch (const std::logic_error &error) {
            throw std::runtime_error("appendFile could not decrypt last block from file: " +
                                     filepath.string() + " : " + std::string(error.what()));
        }

        // combine last block of data with data to be appended
        std::vector<std::uint8_t> combinedData;
        combinedData.insert(combinedData.end(), lastBlock.begin(), lastBlock.end());
        combinedData.insert(combinedData.end(), data.begin(), data.end());
        try {
            writeData = encrypt(combinedData, iv);
        } catch (const std::logic_error &error) {
            throw std::runtime_error("appendFile failed to encrypt combined data: " +
                                     filepath.string() + " : " + std::string(error.what()));
        }

        // re-open as io to seek to last block
        file.open(filepath.native(), std::fstream::in | std::fstream::out);

        // Seek to last block to rewrite it to avoid padding in between new and old blocks
        file.seekg(static_cast<std::streamoff>(fileSize - IV_LENGTH));
    } else {
        // existing file but no encryption, just set to "append" mode.
        file.open(filepath.native(), std::fstream::app);
        writeData = data;
    }
    if (file.fail()) {
        throw std::runtime_error("appendFile could not open file: " + filepath.string());
    }

    file.write(reinterpret_cast<const char *>(writeData.data()),
               static_cast<std::int64_t>(writeData.size()));
    if (file.fail()) {
        throw std::runtime_error("appendFile error appending to file: " + filepath.string());
    }
}

std::vector<std::uint8_t> StorageEncryption::decrypt(const std::vector<std::uint8_t> &ciphertext) {
    auto iv_end = ciphertext.begin();
    if (ciphertext.size() < IV_LENGTH) {
        throw std::runtime_error("Attempted to decrypt a malformed ciphertext");
    }
    std::advance(iv_end, IV_LENGTH);
    std::vector<std::uint8_t> iv = std::vector<std::uint8_t>(ciphertext.begin(), iv_end);

    return decrypt({iv_end, ciphertext.end()}, iv);
}

std::vector<std::uint8_t> StorageEncryption::decrypt(const std::vector<std::uint8_t> &rawCiphertext,
                                                     const std::vector<std::uint8_t> &iv) {
    std::unique_ptr<std::uint8_t[]> plaintext(new std::uint8_t[rawCiphertext.size()]);
    std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> ctx(EVP_CIPHER_CTX_new(),
                                                                        &EVP_CIPHER_CTX_free);

    int len;
    int plaintext_len;
    if (ctx == nullptr) {
        handleOpensslError();
    }

    /*
     * Initialise the decryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if (1 != EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_cbc(), NULL, fileKey.data(), iv.data())) {
        handleOpensslError();
    }

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary.
     */
    if (1 != EVP_DecryptUpdate(ctx.get(), plaintext.get(), &len, rawCiphertext.data(),
                               rawCiphertext.size())) {
        handleOpensslError();
    }
    plaintext_len = len;

    /*
     * Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
    if (1 != EVP_DecryptFinal_ex(ctx.get(), plaintext.get() + len, &len)) {
        handleOpensslError();
    }
    plaintext_len += len;
    return std::vector<std::uint8_t>(plaintext.get(), plaintext.get() + plaintext_len);
}

std::vector<std::uint8_t> StorageEncryption::encrypt(const std::vector<std::uint8_t> &plaintext) {
    std::vector<std::uint8_t> iv(IV_LENGTH);
    RAND_bytes(iv.data(), IV_LENGTH);

    std::vector<std::uint8_t> output;
    auto ciphertext = encrypt(plaintext, iv);
    output.reserve(static_cast<size_t>(ciphertext.size() + IV_LENGTH));
    output.insert(output.end(), iv.begin(), iv.end());
    output.insert(output.end(), ciphertext.begin(), ciphertext.end());
    return output;
}

/**
 * @brief Encrypt the plaintext data using the key and iv using AES-CBC
 * @param plaintext Vector of data bytes to encrypt
 * @param key Vector of key bytes - should be 32 bytes long
 * @param iv Vector of initialization vector bytes - should be 16 bytes long
 *
 * @return Vector of encrypted bytes of the ciphertext padded to fill blocksize
 */
std::vector<std::uint8_t> StorageEncryption::encrypt(const std::vector<std::uint8_t> &plaintext,
                                                     const std::vector<std::uint8_t> &iv) {
    std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> ctx(EVP_CIPHER_CTX_new(),
                                                                        &EVP_CIPHER_CTX_free);

    if (!ctx) {
        handleOpensslError();
    }

    int len = 0;
    /*
     * Initialise the encryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher.
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits.
     */
    if (1 != EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_cbc(), NULL, fileKey.data(), iv.data())) {
        handleOpensslError();
    }
    size_t blocksize = IV_LENGTH;
    std::unique_ptr<std::uint8_t[]> ciphertext =
        std::make_unique<std::uint8_t[]>(plaintext.size() + blocksize);
    if (!ciphertext) {
        throw std::runtime_error(
            "StorageEncryption::encrypt: failed to allocate space for cipher text");
    }

    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if (1 !=
        EVP_EncryptUpdate(ctx.get(), ciphertext.get(), &len, plaintext.data(), plaintext.size())) {
        handleOpensslError();
    }
    int ciphertext_len = len;

    /*
     * Finalize the encryption. Further ciphertext bytes may be written at
     * this stage.
     */
    if (1 != EVP_EncryptFinal_ex(ctx.get(), ciphertext.get() + len, &len)) {
        handleOpensslError();
    }
    ciphertext_len += len;
    return std::vector<std::uint8_t>(ciphertext.get(), ciphertext.get() + ciphertext_len);
}

/**
 * @brief Helper function that determines if a file is encryptable. Files are not encrytable if
 * they exist for testing purposes only
 *
 * @param filename file to check
 * @return true if file can be encrypted
 */
bool StorageEncryption::isFileEncryptable(std::string filename) {
    // hardcoded list of testing files to not encrypt.
    // TODO move these to a special dir (etc?) where they can be read directly
    std::vector<std::string> filesToNotEncrypt = {"jaeger-config.yml", "deployment.txt"};

    bool shouldEncryptThisFile = true;
    for (auto file : filesToNotEncrypt) {
        if (filename.find(file) != std::string::npos) {
            shouldEncryptThisFile = false;
            break;
        }
    }
    return shouldEncryptThisFile;
}

StorageEncryption::InvalidPassphrase::InvalidPassphrase(const char *what_arg) :
    std::runtime_error(what_arg) {}
