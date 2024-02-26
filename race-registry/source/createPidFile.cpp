
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

#include "createPidFile.h"

#include <string.h>
#include <sys/file.h>
#include <unistd.h>

#include <iostream>

std::int32_t rta::createPidFile() {
    static const std::string pidFileName = "/var/run/racetestapp.pid";
    std::int32_t pidFile = open(pidFileName.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (pidFile == -1) {
        std::cerr << "failed to create PID file: " << pidFileName << std::endl;
        return -1;
    }

    // NOTE: initially tried using the command
    //      fcntl(fd, F_SETLK, &fl);
    // to lock the PID file. However, this method only works on a file system mounted with mandatory
    // locking support. Currently, the docker container running this code is not setup with support.
    // However, to add support use the following command:
    //      mount -o mand /dev/sda10 /testfs
    // The flock() call only supports advisory locking (as well as some other limitations). However,
    // this does not currently affect the use case of this application.
    if (flock(pidFile, LOCK_EX | LOCK_NB) == -1) {
        if (errno == EWOULDBLOCK) {
            std::cout << "PID file '" << pidFileName << "' is locked." << std::endl;
            const std::int32_t bufferSize = 128;
            char buffer[bufferSize];
            const ssize_t numRead = read(pidFile, buffer, bufferSize);
            std::cout << "To kill running daemon try running:" << std::endl;
            if (numRead > 0) {
                std::cout << "kill " << std::string(buffer, static_cast<size_t>(numRead))
                          << std::endl;
            } else {
                std::cout << "kill $(echo " << pidFileName << ")" << std::endl;
            }
        } else {
            std::cerr << "Failed to lock PID file: " << pidFileName << std::endl;
        }
        return -1;
    }

    if (ftruncate(pidFile, 0) == -1) {
        std::cerr << "Failed to truncate PID file: " << pidFileName << std::endl;
        return -1;
    }

    const std::int32_t bufferSize = 128;
    char buffer[bufferSize];
    snprintf(buffer, bufferSize, "%ld\n", static_cast<long>(getpid()));
    const ssize_t numWritten = write(pidFile, buffer, strlen(buffer));
    if (numWritten < 0 || static_cast<size_t>(numWritten) != strlen(buffer)) {
        std::cerr << "Failed to write PID to file: " << pidFileName << std::endl;
        return -1;
    }

    return pidFile;
}
