
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

#include "DynamicLibrary.h"

#include <type_traits>

#if defined(__unix__)

#include <dlfcn.h>

static_assert(std::is_same_v<DynamicLibrary::path_char, char>);
static_assert(std::is_same_v<DynamicLibrary::path_string, std::string>);

void *DynamicLibrary::dlopen(const char *path, bool global) noexcept {
#ifdef __ANDROID__
    return ::dlopen(path, RTLD_LAZY | (global ? RTLD_GLOBAL : RTLD_LOCAL));
#else
    return ::dlopen(path, RTLD_LAZY | (global ? RTLD_GLOBAL : RTLD_LOCAL | RTLD_DEEPBIND));
#endif
}

void DynamicLibrary::dlclose(void *lib) noexcept {
    ::dlclose(lib);
}

void *DynamicLibrary::dlsym(void *lib, const char *name) noexcept {
    return ::dlsym(lib, name);
}

char *DynamicLibrary::dlerror() noexcept {
    return ::dlerror();
}

#elif defined(_WIN32)

static_assert(std::is_same_v<DynamicLibrary::path_char, wchar_t>);
static_assert(std::is_same_v<DynamicLibrary::path_string, std::wstring>);

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void *DynamicLibrary::dlopen(const wchar_t *path, bool /*global*/) noexcept {
    return (void *)LoadLibraryW((LPCWSTR)path);
}

void DynamicLibrary::dlclose(void *lib) noexcept {
    FreeLibrary((HMODULE)lib);
}

void *DynamicLibrary::dlsym(void *lib, const char *name) noexcept {
    return (void *)GetProcAddress((HMODULE)lib, (LPCSTR)name);
}

char *DynamicLibrary::dlerror() noexcept {
    return GetLastError();
}

#else

#error "Runtime library loading not implemented for this platform"

#endif
