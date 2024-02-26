
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

#include "RaceCrypto.h"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <boost/io/ios_state.hpp>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <sstream>
#include <vector>

#include "ExtClrMsg.h"
#include "Log.h"
#include "RaceLog.h"

using json = nlohmann::json;

#define IV_LENGTH 12
#define TAG_LENGTH 16

RaceCrypto::RaceCrypto() : delimiter(":::") {
    TRACE_METHOD();
}

/**
 * @brief Log the error and throw a logic_error
 *
 * @throws std::logic_error
 */
static void handleOpensslError() {
    unsigned long err;
    while ((err = ERR_get_error()) != 0) {
        RaceLog::logError("PluginNMTwoSixCpp: RaceCrypto",
                          std::string("Error with OpenSSL call: ") + ERR_error_string(err, NULL),
                          "");
    }
    throw std::logic_error("Error with OpenSSL call");
}

/**
 * @brief Encrypt the input string using AES-GCM with a random 12-byte IV and a 32-byte key.
 * The key must be 32 bytes long.
 *
 * @throws std::logic_error if openssl encounters an error
 * @param input The input string to encrypt
 * @param seed The "key" used to encrypt the input
 * @return the ciphertext as RawData (vector<uint8>) prefixed with the 12-byte IV and 16-byte
 * authentication tag
 */
RawData RaceCrypto::encryptClrMsg(const std::string &input, const std::vector<uint8_t> &key) const {
    unsigned char iv[IV_LENGTH];
    RAND_bytes(iv, IV_LENGTH);

    // AES-GCM does not change length of ciphertext
    std::unique_ptr<unsigned char[]> ciphertext(new unsigned char[input.length()]);
    unsigned char tag[TAG_LENGTH];
    std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> ctx(EVP_CIPHER_CTX_new(),
                                                                        &EVP_CIPHER_CTX_free);

    if (ctx == nullptr) {
        handleOpensslError();
    }

    /* Initialise the encryption operation. */
    if (1 != EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), NULL, NULL, NULL)) {
        handleOpensslError();
    }

    /* Initialise key and IV */
    if (1 != EVP_EncryptInit_ex(ctx.get(), NULL, NULL, key.data(),
                                reinterpret_cast<unsigned char *>(iv))) {
        handleOpensslError();
    }

    int curLength;
    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if (1 != EVP_EncryptUpdate(ctx.get(), ciphertext.get(), &curLength,
                               reinterpret_cast<const unsigned char *>(input.data()),
                               input.length())) {
        handleOpensslError();
    }

    /*
     * Finalise the encryption. Normally ciphertext bytes may be written at
     * this stage, but this does not occur in GCM mode
     */
    if (1 != EVP_EncryptFinal_ex(ctx.get(), ciphertext.get() + curLength, &curLength)) {
        handleOpensslError();
    }

    /* Get the tag */
    if (1 != EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, TAG_LENGTH, tag)) {
        handleOpensslError();
    }

    RawData output = RawData();
    output.reserve(static_cast<size_t>(IV_LENGTH + TAG_LENGTH + input.length()));
    output.insert(output.end(), std::begin(iv), std::end(iv));
    output.insert(output.end(), std::begin(tag), std::end(tag));
    output.insert(output.end(), ciphertext.get(), ciphertext.get() + input.length());
    return output;
}

/**
 * @brief Decrypt the input using the provided key. Assumes input consists of a 12-byte IV, 16-byte
 * tag, ciphertext encrypted with AES-GCM using a 32-byte key. If the decryption fails due to the
 * key being wrong, the empty string is returned (other openssl errors trigger an exception).
 * The key must be 32 bytes long.
 *
 * @throws std::logic_error if openssl encounters an error
 * @param input The RawData input to decrypt
 * @param seed The int seed used as the basis for the key
 * @return The decrypted ciphertext as a string or the empty string if decryption verification fails
 */
std::string RaceCrypto::decryptEncPkg(RawData input, const std::vector<uint8_t> &key) const {
    int curLength;
    int plaintextLength;
    int ret;
    std::unique_ptr<unsigned char[]> plaintext(new unsigned char[input.size()]);

    int ciphertextLength = static_cast<int>(input.size()) - IV_LENGTH - TAG_LENGTH;
    auto end = input.begin();
    std::advance(end, IV_LENGTH);
    RawData rawIv = RawData(input.begin(), end);

    auto start = input.begin();
    std::advance(start, IV_LENGTH);
    end = input.begin();
    std::advance(end, IV_LENGTH + TAG_LENGTH);
    RawData rawTag = RawData(start, end);

    start = input.begin();
    std::advance(start, IV_LENGTH + TAG_LENGTH);
    RawData rawCiphertext = RawData(start, input.end());

    std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> ctx(EVP_CIPHER_CTX_new(),
                                                                        &EVP_CIPHER_CTX_free);
    if (ctx == nullptr) {
        handleOpensslError();
    }

    /* Initialise the decryption operation. */
    if (!EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), NULL, NULL, NULL)) {
        handleOpensslError();
    }

    /* Initialise key and IV */
    if (!EVP_DecryptInit_ex(ctx.get(), NULL, NULL, key.data(), rawIv.data())) {
        handleOpensslError();
    }

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary
     */
    if (!EVP_DecryptUpdate(ctx.get(), plaintext.get(), &curLength, rawCiphertext.data(),
                           ciphertextLength)) {
        handleOpensslError();
    }
    plaintextLength = curLength;

    /* Set expected tag value. Works in OpenSSL 1.0.1d and later */
    if (!EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, TAG_LENGTH, rawTag.data())) {
        handleOpensslError();
    }

    /*
     * Finalise the decryption. A positive return value indicates success,
     * anything else is a failure - the plaintext is not trustworthy.
     */
    ret = EVP_DecryptFinal_ex(ctx.get(), plaintext.get() + curLength, &curLength);

    if (ret > 0) {
        /* Success */
        plaintextLength += curLength;
        std::string result = std::string(plaintext.get(), plaintext.get() + plaintextLength);
        return result;
    } else {
        /* Verify failed */
        return std::string();
    }
}

/**
 * @brief Stringify a ClrMsg into a series of string values separated by a delimiter string.
 *
 * @param msg The ClrMsg to format
 * @return the format delimited version of msg
 */
std::string RaceCrypto::formatDelimitedMessage(const ClrMsg &msg) const {
    return "clrMsg" + delimiter + msg.getMsg() + delimiter + msg.getFrom() + delimiter +
           msg.getTo() + delimiter + std::to_string(msg.getTime()) + delimiter +
           std::to_string(msg.getNonce()) + delimiter + std::to_string(msg.getAmpIndex());
}

/**
 * @brief Stringify an ExtClrMsg into a series of string values separated by a delimiter string.
 * Vectors are converted to JSON lists and then stringified.
 *
 * @param msg The ExtClrMsg to format
 * @return the format delimited version of msg
 */
std::string RaceCrypto::formatDelimitedMessage(const ExtClrMsg &msg) const {
    json jsonCommitteesVisited(msg.getCommitteesVisited());
    json jsonCommitteesSent(msg.getCommitteesSent());
    return "extClrMsg" + delimiter + msg.getMsg() + delimiter + msg.getFrom() + delimiter +
           msg.getTo() + delimiter + std::to_string(msg.getTime()) + delimiter +
           std::to_string(msg.getNonce()) + delimiter + std::to_string(msg.getAmpIndex()) +
           delimiter + std::to_string(msg.getUuid()) + delimiter +
           std::to_string(msg.getRingTtl()) + delimiter + std::to_string(msg.getRingIdx()) +
           delimiter + std::to_string(msg.getMsgType()) + delimiter + jsonCommitteesVisited.dump() +
           delimiter + jsonCommitteesSent.dump();
}

/**
 * @brief Get the size of the msg component of a formatted ClrMsg (or ExtClrMsg). This is the first
 * field after a delimiter
 *
 * @param formatted The string to parse the msg length from
 * @return the length of the msg
 */
std::size_t RaceCrypto::getMsgLength(std::string formatted) const {
    size_t msg_start = formatted.find(delimiter) + delimiter.length();
    size_t msg_end = formatted.find(delimiter, msg_start);
    return msg_end - msg_start;
}

/**
 * @brief Parse the passed string into a ClrMsg (if possible)
 *
 * @param msg The string to parse a ClrMsg from.
 * @return the parsed ClrMsg. On failure an invalid_argument exception is thrown.
 */
ClrMsg RaceCrypto::parseDelimitedMessage(std::string msg) const {
    std::vector<std::string> tokens;
    size_t pos = 0;
    while ((pos = msg.find(delimiter)) != std::string::npos) {
        std::string token = msg.substr(0, pos);
        tokens.push_back(token);
        msg.erase(0, pos + delimiter.length());
    }
    tokens.push_back(msg);

    if (tokens.size() == 7 and tokens[0] == "clrMsg") {
        return ClrMsg(tokens[1], tokens[2], tokens[3], std::stol(tokens[4]), std::stoi(tokens[5]),
                      std::stoi(tokens[6]));
    } else {
        throw std::invalid_argument("Invalid message to parse");
    }
}

/**
 * @brief Parse the passed string into an ExtClrMsg (if possible). If msg is actually a valid format
 * delimited ClrMsg rather than ExtClrMsg, then the ClrMsg is parsed and then an ExtClrMsg is
 * constructed using the ClrMsg.
 *
 * @param msg The string to parse an ExtClrMsg from.
 * @return the parsed ExtClrMsg. On failure an invalid_argument exception is thrown.
 */
ExtClrMsg RaceCrypto::parseDelimitedExtMessage(std::string msg) const {
    std::vector<std::string> tokens;
    size_t pos = 0;
    while ((pos = msg.find(delimiter)) != std::string::npos) {
        std::string token = msg.substr(0, pos);
        tokens.push_back(token);
        msg.erase(0, pos + delimiter.length());
    }
    tokens.push_back(msg);

    if (tokens.size() == 7) {
        return ExtClrMsg(ClrMsg(tokens[1], tokens[2], tokens[3], std::stol(tokens[4]),
                                std::stoi(tokens[5]), std::stoi(tokens[6])));
    } else if (tokens.size() == 13) {
        json committeesVisited = json::parse(tokens[11]);
        json committeesSent = json::parse(tokens[12]);
        return ExtClrMsg(
            tokens[1], tokens[2], tokens[3], std::stol(tokens[4]), std::stoi(tokens[5]),
            std::stoi(tokens[6]), std::stol(tokens[7]), std::stoi(tokens[8]), std::stoi(tokens[9]),
            static_cast<MsgType>(std::stoi(tokens[10])), committeesVisited, committeesSent);
    } else {
        throw std::invalid_argument("Invalid message to parse");
    }
}

/**
 * @brief Update the hash digest with the string input
 *
 * @param ctx An OpenSSL hash context
 * @param str A string to add to the hash context
 * @throws std::logic_error for an openssl error
 */
static void hashString(const std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> &ctx,
                       const std::string &str) {
    int res = EVP_DigestUpdate(ctx.get(), str.data(), str.size());
    if (res != 1)
        handleOpensslError();
}

/**
 * @brief Get the SHA256 of a ClrMsg or throws a logic_error
 *
 * @throws std::logic_error for openssl errors
 * @param msg The ClrMsg to hash
 * @return the SHA256 hash of the msg
 */
MsgHash RaceCrypto::getMessageHash(const ClrMsg &msg) const {
    const EVP_MD *md = EVP_sha256();
    MsgHash m;
    int res;

    // Verify hash size to ensure no writes ococur outside of MsgHash's memory
    if (std::size_t(EVP_MD_size(md)) != MsgHashSize) {
        throw std::logic_error(
            "Unexpected size for hash. Expected: " + std::to_string(MsgHashSize) +
            ", Received: " + std::to_string(EVP_MD_size(md)));
    }

    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
    if (ctx == nullptr)
        handleOpensslError();

    res = EVP_DigestInit_ex(ctx.get(), md, NULL);
    if (res != 1)
        handleOpensslError();

    // Write the message to hash in a similar format as the delimited message
    hashString(ctx, formatDelimitedMessage(msg));

    res = EVP_DigestFinal_ex(ctx.get(), m.data(), NULL);
    if (res != 1)
        handleOpensslError();

    // Log hash for debugging
    std::stringstream s;
    s << "Message Hash: " << m;
    RaceLog::logDebug("PluginNMTwoSixCpp: RaceCrypto", s.str(), "");

    return m;
}

std::ostream &operator<<(std::ostream &o, const MsgHash &m) {
    boost::io::ios_flags_saver flags(o, std::ios_base::hex);
    boost::io::ios_fill_saver fill(o, '0');
    for (std::uint8_t b : m) {
        o << std::setw(2) << +b;
    }
    return o;
}
