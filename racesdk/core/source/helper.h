
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

#ifndef _SOURCE_HELPER_H__
#define _SOURCE_HELPER_H__

// RACE includes
#include <ClrMsg.h>
#include <EncPkg.h>
#include <LinkStatus.h>
#include <PluginResponse.h>
#include <RaceLog.h>
#include <SdkResponse.h>
#include <StorageEncryption.h>

#include <memory>
#include <string>
#include <vector>

#include "OpenTracingForwardDeclarations.h"
#include "PersonaForwardDeclarations.h"
#include "filesystem.h"

namespace helper {

/**
 * @brief Check if a set of personas from a connection profile includes a set of arbitrary personas.
 *        This will commonly be used when network manager requests a connection that can reach a
 * list of personas. Effectively checking if givenPersonas is a subset of connectionProfilePersonas.
 *
 * @param connectionProfilePersonas A set of personas that a connection can reach.
 * @param givenPersonas A set of personas to check if included in the connection.
 * @return true If the connection can reach all given personas.
 * @return false Otherwise.
 */
bool doesConnectionIncludeGivenPersonas(const std::vector<std::string> &connectionProfilePersonas,
                                        const std::vector<std::string> &givenPersonas);

/**
 * @brief Get a short signature string representing the message contents of the ClrMsg.
 *        This uses a truncated SHA1 hash to minimize accidental collisions while
 *        keeping the signature convinient for manual checking.
 *
 * @param msg the ClrMsg to get a signature of
 * @return string with the signature of the message
 */
std::string getMessageSignature(const ClrMsg &msg);

/**
 * @brief Get a string representation of a persona list
 *
 * @param personas A list of persona strings
 * @return string representation of the list of personas
 */
std::string personasToString(const std::vector<std::string> &personas);

/**
 * @brief Convenience function for calling RaceLog::logDebug. Provides a common, default plugin name
 * so that logging is consistent throughout the code base.
 *
 * @param message The message to log.
 * @param stackTrace An optional stack trace to log. Defaults to empty string.
 */
void logDebug(const std::string &message, const std::string &stackTrace = "");

/**
 * @brief Convenience function for calling RaceLog::logInfo. Provides a common, default plugin name
 * so that logging is consistent throughout the code base.
 *
 * @param message The message to log.
 * @param stackTrace An optional stack trace to log. Defaults to empty string.
 */
void logInfo(const std::string &message, const std::string &stackTrace = "");

/**
 * @brief Convenience function for calling RaceLog::logWarning. Provides a common, default plugin
 * name so that logging is consistent throughout the code base.
 *
 * @param message The message to log.
 * @param stackTrace An optional stack trace to log. Defaults to empty string.
 */
void logWarning(const std::string &message, const std::string &stackTrace = "");

/**
 * @brief Convenience function for calling RaceLog::logError. Provides a common, default plugin name
 * so that logging is consistent throughout the code base.
 *
 * @param message The message to log.
 * @param stackTrace An optional stack trace to log. Defaults to empty string.
 */
void logError(const std::string &message, const std::string &stackTrace = "");

/**
 * @brief Convenience function for constructing a structured log update message about link changes
 *
 * @param linkId The LinkID to log
 * @param linkStatus The LinKStatus changed
 * @param personaSet The set of Personas associated with this link
 */
void logLinkChange(const std::string &linkId, LinkStatus linkStatus,
                   const personas::PersonaSet &personaSet);

/**
 * @brief Convenience function for extracting an int64_t from a byte array
 *
 * @param buf The byte array to extract the int from
 * @param offset A pointer to and integer containing the offset at which to read the int from. This
 *               offset is incremented by the number of bytes read.
 * @param ret A pointer to an integer that gets set to the extracted value
 * @return 0 on success, -1 otherwise
 */
int readInt(const std::vector<uint8_t> &buf, uint64_t *offset, int64_t *ret);

/**
 * @brief Convert an integer value to a hexadecimal string of length paddedLength (or longer if
 * padded length is too short).
 *
 * @param input A interger value to be encoded.
 * @param paddedLength The minumum length of the encoded string.
 * @return std::string Hexadecimal string containing input, containing at least paddedLength
 * characters.
 */
std::string convertToHexString(size_t input, size_t paddedLength = 0);

/**
 * @brief Convert a hexadecimal string into an integer value stored in size_t. If the hexadecimal
 * string is negative then zero will be returned.
 * @warning If an error occurs the function will return zero. This is valid for the current use
 * case, but may cause issues if used elswehere.
 *
 * @param hexString A hexadecimal string.
 * @return size_t The value of the hexadecimal string, or zero on error.
 */
size_t convertFromHexString(const std::string &hexString);

/**
 * @brief Convert a byte array into a hex string
 *
 * @param bytes A vector of bytes to be encoded.
 * @return std::string Hexadecimal string containing bytes
 */
std::string byteVectorToHexString(const std::vector<uint8_t> &bytes);

/**
 * @brief Convert hex string into a byte array. If the string contains invalid hex characters, an
 * empty vector will be returned.
 *
 * @param hex A string containing hexadecimal characters to be decoded
 * @return std::vector<uint8_t> A vector of bytes containing the decoded result, or an empty vector
 * if there was an error
 */
std::vector<uint8_t> hexStringToByteVector(const std::string &hex);

/**
 * @brief Convert an SdkStatus into a string
 *
 * @param enum the SdkStatus to convert
 * @return std::string string version of the sdk status code
 */
std::string sdkStatusToString(SdkStatus status);

/**
 * @brief Convert a PluginResponse into a string
 *
 * @param enum the PluginResponse to convert
 * @return std::string string version of the plugin response status code
 */
std::string pluginResponseToString(PluginResponse status);

/**
 * @brief Construct a filepath based on a filepath, pluginId, and data directory path. Helper
 * function used to create the path for a specific plugin's file.
 *
 * @param filepathStr The string name of the file
 * @param pluginName The string pluginId the file is associated with
 * @param configsPath the string path to the local data dircetory
 * @return fs::path Full path to the filepath
 */
fs::path makePluginFilepath(const std::string &filepathStr, const std::string &pluginName,
                            const std::string &configsPath);

/**
 * @brief Read the contents of a file in this plugin's storage.
 *
 * @param filepathStr The path of the file to be appended to (or written).
 * @param pluginId The ID of the plugin for which to read the file.
 * @param configsPath The path of the data directory.
 * @param pluginStorageEncryption The Storage Encryption object responsible for encrpting/decrypting
 * the file
 * @return std::vector<std::uint8_t> The contents of the file, or an empty vector on error.
 */
std::vector<std::uint8_t> readFile(const std::string &filepathStr, const std::string &pluginId,
                                   const std::string &configsPath,
                                   StorageEncryption &pluginStorageEncryption);

/**
 * @brief Append the contents of data to filepath in this plugin's storage.
 * @param filepathStr The string path of the file to be appended to (or written).
 * @param pluginId The ID of the plugin for which to append the file.
 * @param data The string of data to append to the file.
 * @param pluginStorageEncryption The Storage Encryption object responsible for encrpting/decrypting
 * the file
 *
 * @return SdkResponse indicator of success or failure of the append.
 */
bool appendFile(const std::string &filepathStr, const std::string &pluginId,
                const std::string &configsPath, const std::vector<std::uint8_t> &data,
                StorageEncryption &pluginStorageEncryption);

/**
 * @brief Create the directory of directoryPath, including any directories in the path that do
 * not yet exist
 * @param directoryPath the path of the directory to create.
 *
 * @return SdkResponse indicator of success or failure of the create
 */
bool makeDir(const std::string &directoryPath, const std::string &pluginId,
             const std::string &configsPath);

/**
 * @brief Recursively remove the directory of directoryPath
 * @param directoryPath the path of the directory to remove.
 * @param pluginId The ID of the plugin for which to remove the directory.
 *
 * @return SdkResponse indicator of success or failure of the removal
 */
bool removeDir(const std::string &directoryPath, const std::string &pluginId,
               const std::string &configsPath);

/**
 * @brief List the contents (directories and files) of the directory path
 * @param directoryPath the path of the directory to list.
 *
 * @return std::vector<std::string> list of directories and files
 */
std::vector<std::string> listDir(const std::string &directoryPath, const std::string &pluginId,
                                 const std::string &configsPath);

/**
 * @brief Recursively copy the contents of the source directory into the destination
 * @param srcPath Source directory path
 * @param destPath Destination directory path
 *
 * @return True if successful
 */
bool copyDir(const std::string &srcPath, const std::string &destPath);

/**
 * @brief Write the contents of data to filepath in this plugin's storage (overwriting if file
 * exists)
 * @param filepathStr The string path of the file to be written.
 * @param pluginId The ID of the plugin for which to write the file.
 * @param data The string of data to write to the file.
 * @param pluginStorageEncryption The Storage Encryption object responsible for encrpting/decrypting
 * the file
 *
 * @return SdkResponse indicator of success or failure of the write.
 */
bool writeFile(const std::string &filepathStr, const std::string &pluginId,
               const std::string &configsPath, const std::vector<std::uint8_t> &data,
               StorageEncryption &pluginStorageEncryption);

/**
 * @brief Encrypt the plaintext data using the key, return the ciphertext prepended with a random IV
 * @param plaintext Vector of data bytes to encrypt
 * @param key Vector of key bytes - should be 32 bytes long
 *
 * @return Vector of the IV followed by the ciphertext padded to fill blocksize
 */
std::vector<std::uint8_t> encrypt(const std::vector<std::uint8_t> &plaintext,
                                  const std::vector<std::uint8_t> &key);

/**
 * @brief Encrypt the plaintext data using the key and iv using AES-CBC
 * @param plaintext Vector of data bytes to encrypt
 * @param key Vector of key bytes - should be 32 bytes long
 * @param iv Vector of initialization vector bytes - should be 16 bytes long
 *
 * @return Vector of encrypted bytes of the ciphertext padded to fill blocksize
 */
std::vector<std::uint8_t> encrypt(const std::vector<std::uint8_t> &plaintext,
                                  const std::vector<std::uint8_t> &key,
                                  const std::vector<std::uint8_t> &iv);

/**
 * @brief Decrypt the ciphertext data using the key, reads the first 16 bytes as the IV for the rest
 * of the ciphertext
 * @param ciphertext Vector of data bytes to decrypt
 * @param key Vector of key bytes - should be 32 bytes long
 *
 * @return Vector of decrypted bytes with IV and padding removed
 */
std::vector<std::uint8_t> decrypt(const std::vector<std::uint8_t> &ciphertext,
                                  const std::vector<std::uint8_t> &key);

/**
 * @brief Decrypt the ciphertext data using the key and iv using AES-CBC
 * @param ciphertext Vector of data bytes to decrypt
 * @param key Vector of key bytes - should be 32 bytes long
 * @param iv Vector of initialization vector bytes - should be 16 bytes long
 *
 * @return Vector of decrypted bytes with padding removed
 */
std::vector<std::uint8_t> decrypt(const std::vector<std::uint8_t> &ciphertext,
                                  const std::vector<std::uint8_t> &key,
                                  const std::vector<std::uint8_t> &iv);

/**
 * @brief run a shell command and return the contents of stdout after the process terminates
 * @param cmd the command to run
 *
 * @return a string containg the contents of stdout of the executed command
 */
std::string shell(const std::string &cmd);

/**
 * @brief Convert a string to lowercase. Assumes the string is ascii.
 *
 * @param input The string to convert.
 * @return std::string The lower case version of the input string.
 */
std::string stringToLowerCase(std::string input);

/**
 * @brief Return the current time in seconds since epoch. The seconds may be fractional e.g.
 * xxxxxxxxx.xxx.
 *
 * @return double The current time
 */
double currentTime();

/**
 * @brief Set a name for the thread that can be retrieved via get_thread_name.
 *
 * @param name The name of the thread
 */
void set_thread_name(const std::string &name);

/**
 * @brief Get a name for the current thread that was previously set with set_thread_name. If the
 * name for the thread was not previously set, returns an empty string
 *
 * @return std::string the name of the thread
 */
std::string get_thread_name();

/**
 * @brief Extract TarGZ file to a destination location
 *
 * @param filename Full path to file including it's name
 * @param destDir destination directory to extract the tar to
 * @return
 */
void extractConfigTarGz(const std::string &filename, const std::string &destDir);

void createConfigTarGz(const std::string &filename, const std::string &dirToTar);

}  // namespace helper

#define TRACE_FUNCTION(...) TRACE_FUNCTION_BASE(RaceSdkCore, ##__VA_ARGS__)
#define TRACE_METHOD(...) TRACE_METHOD_BASE(RaceSdkCore, ##__VA_ARGS__)

#endif
