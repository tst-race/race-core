
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

#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

/*
// C++17 Filesystem was added with G++ 8.0, and race-linux-base
// only has G++ 7.4, so this isn't usable currently.
#include <filesystem>
namespace fs = std::filesystem;
*/

#ifdef __ANDROID__
// Boost Filesystem is the original implementation. It may have subtle
// incompatibilities, but it is a widely available implementation.
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#else
// Filesystem TS is more broadly available, and is largely the same.
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#endif  //__ANDROID__

#endif  // __FILESYSTEM_H__
