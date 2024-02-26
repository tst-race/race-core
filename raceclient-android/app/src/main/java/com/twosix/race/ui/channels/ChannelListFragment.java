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

import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.lifecycle.ViewModelProvider;
import androidx.recyclerview.widget.RecyclerView;

import ShimsJava.ChannelStatus;
import ShimsJava.JChannelProperties;

import com.twosix.race.R;
import com.twosix.race.data.model.Channel;
import com.twosix.race.ui.RaceFragment;

import java.util.ArrayList;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

/** View fragment to allow the user to enable or disable channels during runtime. */
public class ChannelListFragment extends RaceFragment {

    private static final String TAG = "ChannelListFragment";

    private ChannelListViewModel viewModel;
    // Use a scheduled background thread to periodically update the channels in the UI. We need
    // an onChannelStatusChanged callback in the app so it can react rather than poll, but that
    // would require a breaking API change.
    private ScheduledExecutorService executor;
    private ScheduledFuture<?> scheduledFuture;

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        viewModel = new ViewModelProvider(this).get(ChannelListViewModel.class);
        viewModel.setChannels(getChannelsFromSdk());
        executor = Executors.newSingleThreadScheduledExecutor();

        // Use fragment as recipient of selection change events
        ChannelListAdapter adapter = new ChannelListAdapter(this::setEnabled);
        // Update adapter when channels change via the view model
        viewModel.getChannels().observe(getViewLifecycleOwner(), adapter::updateChannels);

        // Inflate the layout for this fragment
        View view = inflater.inflate(R.layout.fragment_channel_list, container, false);

        RecyclerView recyclerView = view.findViewById(R.id.channels_list);
        recyclerView.setAdapter(adapter);

        return view;
    }

    @Override
    public void onResume() {
        super.onResume();
        getActivity().setTitle(R.string.channels_list_title);
        scheduledFuture =
                executor.scheduleAtFixedRate(this::updateChannels, 0, 1, TimeUnit.SECONDS);
    }

    @Override
    public void onPause() {
        super.onPause();
        if (scheduledFuture != null) {
            scheduledFuture.cancel(false);
            scheduledFuture = null;
        }
    }

    private ArrayList<Channel> getChannelsFromSdk() {
        ArrayList<Channel> channels = new ArrayList<>();
        for (JChannelProperties channelProps : raceService.getChannels(null)) {
            if (!channelProps.channelStatus.equals(ChannelStatus.CHANNEL_UNSUPPORTED)) {
                Channel channel = new Channel();
                channel.setChannelId(channelProps.channelGid);
                // Channel is enabled if status is _ENABLED, _STARTING, _AVAILABLE, etc.
                channel.setEnabled(
                        !channelProps.channelStatus.equals(ChannelStatus.CHANNEL_DISABLED));
                channels.add(channel);
            }
        }
        channels.sort((lhs, rhs) -> lhs.getChannelId().compareTo(rhs.getChannelId()));
        return channels;
    }

    private void updateChannels() {
        ArrayList<Channel> channels = getChannelsFromSdk();
        getActivity().runOnUiThread(() -> viewModel.setChannels(channels));
    }

    private void setEnabled(int position, boolean enabled) {
        String channelId = viewModel.getChannels().getValue().get(position).getChannelId();
        Log.d(TAG, "Setting " + channelId + " enabled=" + enabled);
        new Thread(() -> raceService.setChannelEnabled(channelId, enabled)).start();
    }
}
