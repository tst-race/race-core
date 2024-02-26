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

package com.twosix.race.ui.channels;

import android.util.Log;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;

import com.twosix.race.data.model.Channel;

import java.util.List;

/** ViewModel to allow communication of channel selections between fragments. */
public class ChannelListViewModel extends ViewModel {

    private final MutableLiveData<List<Channel>> channels = new MutableLiveData<>();

    public ChannelListViewModel() {
        Log.d("ChannelListViewModel", "constructing");
    }

    public void setChannels(List<Channel> channels) {
        Log.d("ChannelListViewModel", "Setting channels to " + channels.toString());
        this.channels.setValue(channels);
    }

    public void setEnabled(int position, boolean enabled) {
        List<Channel> channels = this.channels.getValue();
        channels.get(position).setEnabled(enabled);
        this.channels.setValue(channels);
    }

    public LiveData<List<Channel>> getChannels() {
        return channels;
    }
}
