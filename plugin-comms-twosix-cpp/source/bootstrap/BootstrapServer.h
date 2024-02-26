
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

#ifndef _BOOTSTRAP_SERVER_H_
#define _BOOTSTRAP_SERVER_H_

#include <cpprest/http_listener.h>

#include <unordered_map>

class BootstrapServer {
    std::unordered_map<std::string, std::string> requestMap;
    std::string bootstrapDir;
    bool running;
    web::http::experimental::listener::http_listener listener;

public:
    explicit BootstrapServer(const std::string &bootstrapDir);
    ~BootstrapServer();

    void startServer();
    void stopServer();

    void serveFile(const std::string &passphrase, const std::string &path);
    void serveFiles(const std::string &passphrase, const std::string &path);
    void stopServing(const std::string &passphrase);
    void handleGet(web::http::http_request message);
};

#endif
