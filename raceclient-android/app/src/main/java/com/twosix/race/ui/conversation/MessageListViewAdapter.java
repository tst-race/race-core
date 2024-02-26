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

package com.twosix.race.ui.conversation;

import android.content.res.Resources;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.TextView;

import androidx.cardview.widget.CardView;
import androidx.recyclerview.widget.RecyclerView;

import com.twosix.race.R;
import com.twosix.race.room.Message;

import java.util.ArrayList;
import java.util.List;

/**
 * {@link RecyclerView.Adapter} that can display a {@link Message}. This is used to display a list
 * of Messages in a particular conversation. This adapter is registered to the UI element in {@link
 * MessageListFragment}.onCreateView()
 */
public class MessageListViewAdapter
        extends RecyclerView.Adapter<MessageListViewAdapter.ViewHolder> {

    private List<Message> mValues;
    private String persona;

    public MessageListViewAdapter(String persona) {
        mValues = new ArrayList<>();
        this.persona = persona;
    }

    /**
     * Called when ConversationDatabase is updated with new/changed list of messages
     *
     * @param items updated list of {@link Message} objects
     */
    public void updateMessages(List<Message> items) {
        mValues = items;
        notifyDataSetChanged();
    }

    /**
     * Required method which defines what layout to use for each list item
     *
     * @param parent parent view to inflate from
     * @param viewType unused but required by interface
     * @return ViewHolder which contains the list item view
     */
    @Override
    public MessageListViewAdapter.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view =
                LayoutInflater.from(parent.getContext())
                        .inflate(R.layout.fragment_message_list_item, parent, false);
        return new MessageListViewAdapter.ViewHolder(view);
    }

    /**
     * Called for each item in message list
     *
     * @param holder
     * @param position item position in conversation list
     */
    @Override
    public void onBindViewHolder(final MessageListViewAdapter.ViewHolder holder, int position) {
        holder.mItem = mValues.get(position);
        holder.mContentView.setText(mValues.get(position).plainMsg);
        holder.mSentTimeLabel.setText(mValues.get(position).createTime);
        if (mValues.get(position).timeReceived != null) {
            holder.mReceivedTimeLabel.setText(mValues.get(position).timeReceived);
        } else {
            holder.mReceivedTimeLabel.setText("");
        }

        Resources res = holder.mContentView.getContext().getResources();

        if (mValues.get(position).fromPersona.compareTo(persona) == 0) {
            // If this was a message sent by this device, fake a read receipt and display the
            // message on the right side of the screen

            holder.mContentView.setGravity(Gravity.RIGHT);
            holder.mCard.setBackgroundColor(res.getColor(R.color.sentMessage, null));

            // set clickable so we can change the read receipt check mark
            holder.mReadCheck.setClickable(true);

            if (position == mValues.size() - 1) {
                // If this is the last message in the conversation fake a read receipt

                holder.mReadCheck.setVisibility(View.VISIBLE);
                holder.mReadCheck.setChecked(false);
                holder.mReadCheck.postDelayed(
                        new Runnable() {
                            @Override
                            public void run() {
                                // set clickable so we can change the read receipt check mark
                                holder.mReadCheck.setClickable(true);
                                holder.mReadCheck.setChecked(true);
                                // If this isn't the last message in the conversation show the read
                                // receipt checked
                                holder.mReadCheck.setClickable(false);
                            }
                        },
                        2000);
            } else {
                // If this isn't the last message in the conversation show the read receipt checked
                holder.mReadCheck.setChecked(true);
            }

            // unset clickable so the read receipt check mark can't be changed
            holder.mReadCheck.setClickable(false);

        } else {
            // If this was a message received by this device display the message to the left and
            // don't show read receipt check

            holder.mContentView.setGravity(Gravity.LEFT);
            holder.mCard.setBackgroundColor(res.getColor(R.color.receivedMessage, null));
            holder.mReadCheck.setVisibility(View.INVISIBLE);
        }
    }
    ;

    /**
     * Required method for ListViewAdapters
     *
     * @return size of message list
     */
    @Override
    public int getItemCount() {
        return mValues.size();
    }

    /**
     * Custom ViewHolder for message list items. This allows us to set fields we defined in the
     * layout.xml of the list item.
     */
    public class ViewHolder extends RecyclerView.ViewHolder {
        public final View mView;
        public final TextView mContentView;
        public final TextView mSentTimeLabel;
        public final TextView mReceivedTimeLabel;
        public final CheckBox mReadCheck;
        public Message mItem;
        public final CardView mCard;

        public ViewHolder(View view) {
            super(view);
            mView = view;
            mContentView = (TextView) view.findViewById(R.id.message_content);
            mSentTimeLabel = (TextView) view.findViewById(R.id.message_sent_time);
            mReceivedTimeLabel = (TextView) view.findViewById(R.id.message_received_time);
            mCard = (CardView) view.findViewById(R.id.message_card);
            mReadCheck = (CheckBox) view.findViewById(R.id.read_reciept);
        }

        @Override
        public String toString() {
            return super.toString() + " '" + mContentView.getText() + "'";
        }
    }
}
