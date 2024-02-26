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

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.appcompat.widget.SwitchCompat;
import androidx.recyclerview.widget.RecyclerView;

import com.twosix.race.R;
import com.twosix.race.data.model.Channel;

import java.util.ArrayList;
import java.util.List;

/** RecyclerView adapter to render items in a list as toggleable channels. */
public class ChannelListAdapter extends RecyclerView.Adapter<ChannelListAdapter.ViewHolder> {

    public static interface IChannelSelectionListener {
        public void setEnabled(int position, boolean enabled);
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {

        private final SwitchCompat switchView;

        public ViewHolder(@NonNull View view) {
            super(view);
            switchView = (SwitchCompat) view.findViewById(R.id.enabled_switch);
        }

        public SwitchCompat getSwitchView() {
            return switchView;
        }
    }

    private final IChannelSelectionListener selectionListener;

    private List<Channel> channels = new ArrayList<>();

    public ChannelListAdapter(@NonNull IChannelSelectionListener selectionListener) {
        this.selectionListener = selectionListener;
    }

    public void updateChannels(final List<Channel> channels) {
        this.channels = channels;
        notifyDataSetChanged();
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view =
                LayoutInflater.from(parent.getContext())
                        .inflate(R.layout.fragment_channel_list_item, parent, false);
        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        Channel channel = channels.get(position);
        SwitchCompat switchView = holder.getSwitchView();
        switchView.setOnCheckedChangeListener(null);
        switchView.setText(channel.getChannelId());
        switchView.setChecked(channel.isEnabled());
        switchView.setOnCheckedChangeListener(
                (view, checked) -> {
                    if (view.isPressed()) {
                        selectionListener.setEnabled(position, checked);
                    }
                });
    }

    @Override
    public int getItemCount() {
        return channels.size();
    }
}
