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
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.recyclerview.widget.RecyclerView;

import com.twosix.race.R;
import com.twosix.race.room.Conversation;

import java.util.List;

/**
 * {@link RecyclerView.Adapter} that can display a {@link Conversation}. This is used to display a
 * list of Conversations on the main screen of the race app. This adapter is registered to the UI
 * element in {@link ConversationListFragment}.onCreateView()
 */
public class ConversationListViewAdapter
        extends RecyclerView.Adapter<ConversationListViewAdapter.ViewHolder> {

    private List<Conversation> mValues;
    private final View.OnClickListener itemOnClickListener;

    /**
     * @param items list of {@link Conversation} objects
     * @param itemOnClickListener listener that handles transitioning from current fragment to
     *     individual conversation fragment
     */
    public ConversationListViewAdapter(
            List<Conversation> items, View.OnClickListener itemOnClickListener) {
        mValues = items;
        this.itemOnClickListener = itemOnClickListener;
    }

    /**
     * Called when ConversationDatabase is updated with new/changed list of conversations
     *
     * @param items updated list of {@link Conversation} objects
     */
    public void updateConversation(List<Conversation> items) {
        mValues = items;
        // notifyDataSetChanged tells the UI to refresh
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
    public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view =
                LayoutInflater.from(parent.getContext())
                        .inflate(R.layout.fragment_conversation_list_item, parent, false);
        return new ViewHolder(view);
    }

    /**
     * Called for each item in conversation list
     *
     * @param holder
     * @param position item position in conversation list
     */
    @Override
    public void onBindViewHolder(final ViewHolder holder, int position) {
        holder.mItem = mValues.get(position);
        if (mValues.get(position).participants != null) {
            holder.mContentView.setText(participantsToString(mValues.get(position)));
        }
        holder.itemView.setTag(mValues.get(position));
        holder.itemView.setOnClickListener(itemOnClickListener);

        Resources res = holder.mContentView.getContext().getResources();
        int[] iconColors = res.getIntArray(R.array.icon_colors);

        int iconColor = position % iconColors.length;
        holder.mConversationIcon.setColorFilter(iconColors[iconColor]);
    }

    /**
     * Required method for ListViewAdapters
     *
     * @return size of conversation list
     */
    @Override
    public int getItemCount() {
        return mValues.size();
    }

    /**
     * Custom ViewHolder for conversation list items. This allows us to set fields we defined in the
     * layout.xml of the list item.
     */
    public class ViewHolder extends RecyclerView.ViewHolder {
        public final View mView;
        public final TextView mContentView;
        public Conversation mItem;
        public ImageView mConversationIcon;

        public ViewHolder(View view) {
            super(view);
            mView = view;
            mContentView = (TextView) view.findViewById(R.id.content);
            mConversationIcon = (ImageView) view.findViewById(R.id.conversation_icon);
        }

        @Override
        public String toString() {
            return super.toString() + " '" + mContentView.getText() + "'";
        }
    }

    /**
     * User to create a label for the conversation list item. This may be more complicated when/if
     * group messaging functionality is added
     *
     * @param conversation tied to a specific list item
     * @return string that should be displayed on the list item
     */
    String participantsToString(Conversation conversation) {
        return conversation.id;
    }
}
