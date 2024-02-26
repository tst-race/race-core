
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

#include "MimeTypes.h"

#include <stdexcept>
#include <string_view>

#include "helper.h"

bool mimeTypeMatches(const std::string &mimeType, const std::string &pattern) {
    std::string logPrefix = "mimeTypeMatches: ";
    std::string_view mimeTypeView = mimeType;
    std::string_view patternView = pattern;

    try {
        size_t mimeTypeEnd = mimeTypeView.find('/');
        size_t mimeSubTypeEnd = mimeTypeView.find(';');
        size_t patternTypeEnd = patternView.find('/');
        size_t patternSubTypeEnd = patternView.find(';');
        std::string_view mimeTypeType = mimeTypeView.substr(0, mimeTypeEnd);
        std::string_view mimeTypeSubType =
            mimeTypeView.substr(mimeTypeEnd + 1, mimeSubTypeEnd - mimeTypeEnd - 1);
        std::string_view patternType = patternView.substr(0, patternTypeEnd);
        std::string_view patternSubType =
            patternView.substr(patternTypeEnd + 1, patternSubTypeEnd - patternTypeEnd - 1);

        if ((patternType == "*" || patternType == mimeTypeType) &&
            (patternSubType == "*" || patternSubType == mimeTypeSubType)) {
            return true;
        }

        return false;
    } catch (std::out_of_range &e) {
        helper::logError(logPrefix + "Invalid mime type");
        return false;
    }
}
