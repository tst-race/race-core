
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

#ifndef __COMMS_TWOSIX_USER_MODEL_MARKOV_MODEL_H__
#define __COMMS_TWOSIX_USER_MODEL_MARKOV_MODEL_H__

#include <boost/numeric/ublas/matrix.hpp>
#include <random>

class MarkovModel {
public:
    enum class UserAction {
        FETCH,
        POST,
        WAIT,
    };

    UserAction getNextUserAction();

protected:
    int currentState{0};

    virtual double getRandom();

private:
    std::random_device rd;
    std::mt19937 gen{rd()};
};

#endif  // __COMMS_TWOSIX_USER_MODEL_MARKOV_MODEL_H__