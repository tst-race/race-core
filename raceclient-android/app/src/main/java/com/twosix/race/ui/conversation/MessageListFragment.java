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

import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;

import androidx.lifecycle.Observer;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.twosix.race.R;
import com.twosix.race.room.ConversationDatabase;
import com.twosix.race.room.Message;
import com.twosix.race.ui.RaceFragment;

import java.util.List;

/**
 * MessageListFragment is responsible for showing a list of messages in a single conversation
 * between the client the app is running on and one other client
 */
public class MessageListFragment extends RaceFragment {

    private static final String TAG = "MessageListFragment";

    private String conversationId;

    private Button buttonSend;
    private EditText editTextNewMessage;

    /**
     * Android Fragment Lifecycle method called when the view is created. In this case this is
     * called when a user clicks on a conversation to view messages.
     *
     * @param inflater the object used to inflate the conversation layout
     * @param container the parent container the conversation layout is inflated into
     * @param savedInstanceState not used because state data is saved in Room
     * @return View containing a list of messages
     */
    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_message_list, container, false);

        final RecyclerView recyclerView = view.findViewById(R.id.message_list);
        Context context = recyclerView.getContext();
        recyclerView.setAdapter(new MessageListViewAdapter(raceService.getPersona()));
        recyclerView.setLayoutManager(new GridLayoutManager(context, 1));

        conversationId = MessageListFragmentArgs.fromBundle(getArguments()).getConversationId();

        ConversationDatabase db = ConversationDatabase.getInstance(context);
        db.conversationDao()
                .getMessages(conversationId)
                .observe(
                        this.getActivity(),
                        new Observer<List<Message>>() {
                            @Override
                            public void onChanged(List<Message> messages) {
                                ((MessageListViewAdapter) recyclerView.getAdapter())
                                        .updateMessages(messages);
                            }
                        });

        buttonSend = (Button) view.findViewById(R.id.button_send);
        buttonSend.setOnClickListener(this::onClickSend);
        editTextNewMessage = (EditText) view.findViewById(R.id.edit_text_new_message);

        return view;
    }

    /**
     * Android Fragment Lifecycle method called whenever the fragment is started or resumed. This
     * can be called if the user hits the home button then goes back to the app. We need to ensure
     * messages received while the app was in the background are rendered.
     */
    @Override
    public void onResume() {
        super.onResume();
        Log.v(TAG, "onResume Called");
        getActivity().setTitle(conversationId);
        raceService.suspendMessageNotificationsFor(conversationId);
    }

    @Override
    public void onPause() {
        super.onPause();
        raceService.suspendMessageNotificationsFor(null);
    }

    public void onClickSend(View view) {
        // Get text from editText then clear the field
        String plainText = editTextNewMessage.getText().toString();
        editTextNewMessage.setText("");

        if (plainText.compareTo("") == 0) {
            // no-op if there's no message
            return;
        }

        raceService.sendMessage(plainText, conversationId);
    }
}
