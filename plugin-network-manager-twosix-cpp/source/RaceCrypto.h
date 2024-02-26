
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

#ifndef __PLUGIN_NETWORK_MANAGER_TWOSIX_CRYPTO_CPP_H__
#define __PLUGIN_NETWORK_MANAGER_TWOSIX_CRYPTO_CPP_H__

#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <utility>

#include "ClrMsg.h"
#include "EncPkg.h"
class ExtClrMsg;  // Forward declaration to fix circular dependency

constexpr std::size_t MsgHashSize = 32;  // 256 bits = 32 bytes

// Distinct type to store SHA-256 hash
class MsgHash : public std::array<std::uint8_t, MsgHashSize> {};
std::ostream &operator<<(std::ostream &, const MsgHash &);

// Specialize std::hash for MsgHash (yes, this is allowed by the standard)
namespace std {
template <>
class hash<MsgHash> {
public:
    std::size_t operator()(const MsgHash &m) const noexcept {
        // Since the value is already a cryptographic hash, we can
        // take a subset of the value as the hashtable hash value
        return *reinterpret_cast<const std::size_t *>(m.data());
    }
};
}  // namespace std

class RaceCrypto {
private:
    std::string delimiter;

public:
    RaceCrypto();

    /**
     * @brief Encrypt the input string using AES-GCM with a random 12-byte IV and a 32-byte key.
     * The key must be 32 bytes long.
     *
     * @throws std::logic_error if openssl encounters an error
     * @param input The input string to encrypt
     * @param key The string key used to encrypt the input
     * @return the ciphertext as RawData (vector<uint8>) prefixed with the 12-byte IV and 16-byte
     * authentication tag
     */
    RawData encryptClrMsg(const std::string &input, const std::vector<uint8_t> &key) const;

    /**
     * @brief Decrypt the input using the provided key. Assumes input consists of a 12-byte IV,
     * 16-byte tag, ciphertext encrypted with AES-GCM using a 32-byte key. If the decryption fails
     * due to the key being wrong, the empty string is returned (other openssl errors trigger an
     * exception). The key must be 32 bytes long.
     *
     * @throws std::logic_error if openssl encounters an error
     * @param input The RawData input to decrypt
     * @param key The string key
     * @return The decrypted ciphertext as a string or the empty string if decryption verification
     * fails
     */
    std::string decryptEncPkg(RawData input, const std::vector<uint8_t> &key) const;

    /**
     * @brief Stringify a ClrMsg into a series of string values separated by a delimiter string.
     *
     * @param msg The ClrMsg to format
     * @return the format delimited version of msg
     */
    std::string formatDelimitedMessage(const ClrMsg &msg) const;

    /**
     * @brief Stringify an ExtClrMsg into a series of string values separated by a delimiter string.
     * Vectors are converted to JSON lists and then stringified.
     *
     * @param msg The ExtClrMsg to format
     * @return the format delimited version of msg
     */
    std::string formatDelimitedMessage(const ExtClrMsg &msg) const;

    /**
     * @brief Get the size of the msg component of a formatted ClrMsg (or ExtClrMsg). This is the
     * first field after a delimiter
     *
     * @param formatted The string to parse the msg length from
     * @return the length of the msg
     */
    std::size_t getMsgLength(std::string formatted) const;

    /**
     * @brief Parse the passed string into a ClrMsg (if possible)
     *
     * @param msg The string to parse a ClrMsg from.
     * @return the parsed ClrMsg. On failure an invalid_argument exception is thrown.
     */
    ClrMsg parseDelimitedMessage(std::string msg) const;

    /**
     * @brief Parse the passed string into an ExtClrMsg (if possible). If msg is actually a valid
     * format delimited ClrMsg rather than ExtClrMsg, then the ClrMsg is parsed and then an
     * ExtClrMsg is constructed using the ClrMsg.
     *
     * @param msg The string to parse an ExtClrMsg from.
     * @return the parsed ExtClrMsg. On failure an invalid_argument exception is thrown.
     */
    ExtClrMsg parseDelimitedExtMessage(std::string msg) const;

    /**
     * @brief Get the SHA256 of a ClrMsg or throws a logic_error
     *
     * @throws std::logic_error for openssl errors
     * @param msg The ClrMsg to hash
     * @return the SHA256 hash of the msg
     */
    MsgHash getMessageHash(const ClrMsg &msg) const;

    /**
     * @brief Get the delimiter for stringified messages
     *
     * @return The string delimiter
     */
    std::string getDelimiter() const {
        return delimiter;
    }

    /**
     * @brief Set the delimiter to use when stringifying messages
     *
     * @param d the new delimiter to use
     */
    void setDelimiter(std::string d) {
        delimiter = std::move(d);
    }
};

#endif
