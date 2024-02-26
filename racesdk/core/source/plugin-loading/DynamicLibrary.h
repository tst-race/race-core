
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

#ifndef __DYNAMICLIBRARY_H__
#define __DYNAMICLIBRARY_H__

#include <stdexcept>
#include <string>
#include <utility>

#include "filesystem.h"
#include "helper.h"

class DynamicLibrary {
public:
    using path_char = fs::path::value_type;
    using path_string = fs::path::string_type;

    void open(const path_char *path, bool global = false);
    void open(const path_string &path, bool global = false) {
        open(path.c_str(), global);
    }
    void open(const fs::path &path, bool global = false) {
        open(path.c_str(), global);
    }
    void close() noexcept;
    bool is_loaded() const noexcept {
        return lib != nullptr;
    }

    template <typename T>
    T &get(const char *name) const;
    template <typename T>
    T &get(const std::string &name) const {
        return get<T>(name.c_str());
    }
    // overloads that allow for template type deduction
    template <typename T>
    T &get(T *&result, const char *name) const {
        result = &get<T>(name);
        return *result;
    }
    template <typename T>
    T &get(T *&result, const std::string &name) const {
        return get<T>(result, name.c_str());
    }

    DynamicLibrary() noexcept = default;
    explicit DynamicLibrary(const path_char *path, bool global = false) {
        open(path, global);
    }
    explicit DynamicLibrary(const path_string &path, bool global = false) {
        open(path.c_str(), global);
    }
    explicit DynamicLibrary(const fs::path &path, bool global = false) {
        open(path.c_str(), global);
    }
    ~DynamicLibrary() {
        close();
    }

    // Make class move-only to prevent using the handle after closing
    DynamicLibrary(const DynamicLibrary &) = delete;
    DynamicLibrary &operator=(const DynamicLibrary &) = delete;
    DynamicLibrary(DynamicLibrary &&o) noexcept {
        std::swap(lib, o.lib);
    }
    DynamicLibrary &operator=(DynamicLibrary &&o) noexcept {
        std::swap(lib, o.lib);
        return *this;
    }

private:
    void *lib = nullptr;

    // Platform-dependent functions
    static void *dlopen(const path_char *path, bool global) noexcept;
    static void dlclose(void *lib) noexcept;
    static void *dlsym(void *lib, const char *name) noexcept;
    static char *dlerror() noexcept;
};

// Implementation

inline void DynamicLibrary::open(const DynamicLibrary::path_char *path, bool global) {
    if (lib != nullptr) {
        dlclose(lib);
    }
    lib = dlopen(path, global);
    if (lib == nullptr) {
        const char *dlerrorMessage = dlerror();
        const std::string errorMessage =
            "DynamicLibrary::open: unable to load the library: " +
            std::string(dlerrorMessage != nullptr ? dlerrorMessage : "unknown error");
        helper::logError(errorMessage);
        throw std::runtime_error(errorMessage);
    }
}

inline void DynamicLibrary::close() noexcept {
    if (lib != nullptr) {
        dlclose(lib);
        lib = nullptr;
    }
}

template <typename T>
inline T &DynamicLibrary::get(const char *name) const {
    using namespace std::literals::string_literals;
    if (lib == nullptr) {
        const std::string errorMessage =
            "DynamicLibrary::get: can't call get on an unloaded library";
        helper::logError(errorMessage);
        throw std::runtime_error(errorMessage);
    }
    void *ptr = dlsym(lib, name);
    if (ptr == nullptr) {
        const std::string errorMessage = "DynamicLibrary::get: Symbol not found: "s + name;
        helper::logError(errorMessage);
        throw std::runtime_error(errorMessage);
    }
    return *reinterpret_cast<T *>(ptr);
}

#endif  // __DYNAMICLIBRARY_H__
