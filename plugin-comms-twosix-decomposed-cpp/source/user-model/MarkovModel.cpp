
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

#include "MarkovModel.h"

#include <algorithm>
#include <array>

constexpr bool rowIsValid(const std::array<double, 3> &array) {
    int sum = 0;
    for (double val : array) {
        // cppcheck-suppress useStlAlgorithm
        sum += static_cast<int>(val * 100.0);
    }
    return sum == 100;
}

constexpr static std::array<std::array<double, 3>, 3> transitionWeights{{
    {0.0, 1.0, 0.0},  // fetch
    {0.0, 0.0, 1.0},  // post
    {1.0, 0.0, 0.0},  // wait
}};
static_assert(rowIsValid(transitionWeights.at(0)));
static_assert(rowIsValid(transitionWeights.at(1)));
static_assert(rowIsValid(transitionWeights.at(2)));

MarkovModel::UserAction MarkovModel::getNextUserAction() {
    int nextState = 0;
    auto weights = transitionWeights.at(static_cast<size_t>(currentState));
    double cumSum = 0.0;
    double random = getRandom();
    for (size_t i = 0; i < weights.size(); ++i) {
        cumSum += weights.at(i);
        if (cumSum > random) {
            nextState = i;
            break;
        }
    }
    currentState = nextState;
    return static_cast<UserAction>(currentState);
}

double MarkovModel::getRandom() {
    return std::generate_canonical<double, 10>(gen);
}