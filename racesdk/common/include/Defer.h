
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

#ifndef __RACE_DEFER_H__
#define __RACE_DEFER_H__

#include <utility>  // std::move

template <class F>
class Defer {
public:
    explicit Defer(const F &_callback) : callback(_callback) {}
    explicit Defer(F &&_callback) : callback(std::move(_callback)) {}
    ~Defer() {
        callback();
    }

    Defer(const Defer &) = delete;
    Defer(Defer &&) = delete;
    Defer &operator=(const Defer &) = delete;
    Defer &operator=(Defer &&) = delete;

private:
    F callback;
};

#endif
