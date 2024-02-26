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

import android.app.Dialog;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.DialogFragment;
import androidx.lifecycle.ViewModelProvider;
import androidx.recyclerview.widget.RecyclerView;

import com.twosix.race.R;
import com.twosix.race.data.model.Channel;

import java.util.ArrayList;
import java.util.Arrays;

/** Dialog fragment to allow the user to select a set of enabled channels on app startup. */
public class SelectChannelsFragment extends DialogFragment {

    public static final String TAG = "SelectChannels";

    public static final String FRAGMENT_RESULT_KEY = "enabledChannels";

    public static final String RESULT_CHANNELS_KEY = "enabledChannels";

    private static final String ARGS_CHANNELS_KEY = "supportedChannels";

    public static SelectChannelsFragment newInstance(String[] supportedChannels) {
        Bundle args = new Bundle();
        args.putStringArray(ARGS_CHANNELS_KEY, supportedChannels);

        SelectChannelsFragment fragment = new SelectChannelsFragment();
        fragment.setArguments(args);
        return fragment;
    }

    private ChannelListViewModel viewModel;

    @NonNull
    @Override
    public Dialog onCreateDialog(@Nullable Bundle savedInstanceState) {
        viewModel = new ViewModelProvider(this).get(ChannelListViewModel.class);

        if (getArguments() != null) {
            String[] supportedChannels = getArguments().getStringArray(ARGS_CHANNELS_KEY);
            Arrays.sort(supportedChannels);

            ArrayList<Channel> channels = new ArrayList<>();
            for (String channelId : supportedChannels) {
                Channel channel = new Channel();
                channel.setChannelId(channelId);
                channel.setEnabled(false);
                channels.add(channel);
            }

            viewModel.setChannels(channels);
        }

        // Use view model as recipient of selection change events
        ChannelListAdapter adapter = new ChannelListAdapter(viewModel::setEnabled);
        // Update adapter when channels change via the view model
        viewModel.getChannels().observe(this, adapter::updateChannels);

        AlertDialog.Builder builder =
                new AlertDialog.Builder(getActivity()); // , R.style.FullscreenDialog);
        LayoutInflater inflater = requireActivity().getLayoutInflater();

        View view = inflater.inflate(R.layout.fragment_channel_list, null);

        RecyclerView recyclerView = view.findViewById(R.id.channels_list);
        recyclerView.setAdapter(adapter);

        builder.setView(view)
                .setTitle(R.string.select_channels_title)
                .setPositiveButton(
                        R.string.apply_channel_selection,
                        (dialog, id) -> setResultToEnabledChannels());
        AlertDialog alertDialog = builder.create();

        viewModel
                .getChannels()
                .observe(
                        this,
                        channels -> {
                            Button okButton = alertDialog.getButton(AlertDialog.BUTTON_POSITIVE);
                            if (okButton != null) {
                                boolean anyEnabled = channels.stream().anyMatch(Channel::isEnabled);
                                okButton.setEnabled(anyEnabled);
                            }
                        });

        return alertDialog;
    }

    private void setResultToEnabledChannels() {
        if (viewModel != null) {
            String[] enabledChannels =
                    viewModel.getChannels().getValue().stream()
                            .filter(Channel::isEnabled)
                            .map(Channel::getChannelId)
                            .toArray(String[]::new);

            Bundle result = new Bundle();
            result.putStringArray(RESULT_CHANNELS_KEY, enabledChannels);

            new Thread(
                            () ->
                                    getParentFragmentManager()
                                            .setFragmentResult(FRAGMENT_RESULT_KEY, result))
                    .start();
        }
    }
}
