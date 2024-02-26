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

package com.twosix.race.ui;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;

import com.twosix.race.service.IRaceService;
import com.twosix.race.service.IRaceServiceProvider;

/** Fragment base that retrieves the IRaceService from the attached context. */
public abstract class RaceFragment extends Fragment {

    protected IRaceService raceService;

    @Override
    public void onAttach(@NonNull Context context) {
        super.onAttach(context);
        try {
            raceService = ((IRaceServiceProvider) context).getRaceService();
            if (raceService == null) {
                throw new RuntimeException("Bound RACE service instance is null");
            }
        } catch (ClassCastException err) {
            throw new ClassCastException(context + " must implement IRaceServiceProvider");
        }
    }
}