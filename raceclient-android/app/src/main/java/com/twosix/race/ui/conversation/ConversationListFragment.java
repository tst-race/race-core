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

import android.app.AlertDialog;
import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.Observer;
import androidx.navigation.fragment.NavHostFragment;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;
import com.twosix.race.R;
import com.twosix.race.room.Conversation;
import com.twosix.race.room.ConversationDatabase;
import com.twosix.race.ui.RaceFragment;

import java.util.ArrayList;
import java.util.List;

/**
 * ConversationListFragment is responsible for displaying the list of clients that are accessible.
 * Clicking on a client should open the conversation with that client.
 */
public class ConversationListFragment extends RaceFragment {

    private static final String TAG = "ConversationListFragment";

    private ConversationDatabase db;

    private LiveData<List<Conversation>> conversations;

    /**
     * Android Fragment Lifecycle method. onCreateView must inflate the fragment_conversation_list
     * layout and attached handler to populate it with all conversations and attach action listeners
     * for each list item
     *
     * @param inflater
     * @param container
     * @param savedInstanceState
     * @return
     */
    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        // If launched with a conversation ID, navigate immediately to that conversation
        ConversationListFragmentArgs args = ConversationListFragmentArgs.fromBundle(getArguments());
        if (args.getConversationId() != null) {
            getArguments().clear();
            NavHostFragment.findNavController(this)
                    .navigate(
                            ConversationListFragmentDirections.actionConversationListToMessageList(
                                    args.getConversationId()));
            return null;
        }

        setHasOptionsMenu(true);

        View view = inflater.inflate(R.layout.fragment_conversation_list, container, false);
        Log.v(TAG, "onCreateView called");

        // Set onClickListener for floating action button
        final FloatingActionButton newConversationButton =
                view.findViewById(R.id.newConversationButton);
        newConversationButton.setOnClickListener(this::onClickNewConversation);

        // Set action for clicking on a listed
        final RecyclerView recyclerView = view.findViewById(R.id.list);
        Context context = recyclerView.getContext();
        recyclerView.setAdapter(
                new ConversationListViewAdapter(
                        new ArrayList<Conversation>(), this::onClickConversation));
        recyclerView.setLayoutManager(new GridLayoutManager(context, 1));

        db = ConversationDatabase.getInstance(context);
        conversations = db.conversationDao().getAllConversationsExcept(raceService.getPersona());

        conversations.observe(
                this.getActivity(),
                new Observer<List<Conversation>>() {
                    @Override
                    public void onChanged(List<Conversation> conversations) {
                        ((ConversationListViewAdapter) recyclerView.getAdapter())
                                .updateConversation(conversations);
                    }
                });
        return view;
    }

    /**
     * Android Fragment Lifecycle method. This must update the title bar to RACE because the
     * conversationFragment changes it to match the conversation ID. When the user is is a
     * conversation and hits back, this is called.
     */
    @Override
    public void onResume() {
        super.onResume();
        Log.v(TAG, "onResume Called");
        getActivity().setTitle(getString(R.string.conversations_title, raceService.getPersona()));
    }

    @Override
    public void onCreateOptionsMenu(@NonNull Menu menu, @NonNull MenuInflater inflater) {
        inflater.inflate(R.menu.conversation_list_menu, menu);
    }

    @Override
    public boolean onOptionsItemSelected(@NonNull MenuItem item) {
        if (item.getItemId() == R.id.channels_menu_item) {
            NavHostFragment.findNavController(this)
                    .navigate(
                            ConversationListFragmentDirections
                                    .actionConversationListToChannelList());
        }
        if (item.getItemId() == R.id.bootstrap_menu_item) {
            NavHostFragment.findNavController(this)
                    .navigate(
                            ConversationListFragmentDirections
                                    .actionConversationListToNewBootstrap());
        }
        return super.onOptionsItemSelected(item);
    }

    private void onClickNewConversation(View source) {
        View view = LayoutInflater.from(getActivity()).inflate(R.layout.fragment_text_input, null);
        TextInputLayout textInputLayout = view.findViewById(R.id.text_input_layout);
        textInputLayout.setHint(R.string.conversations_new_convo_persona_hint);

        final TextInputEditText textInput = view.findViewById(R.id.text_input);

        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(getActivity());
        alertDialogBuilder
                .setTitle(R.string.conversations_new_convo_title)
                .setView(view)
                .setPositiveButton(
                        R.string.conversations_new_convo_add,
                        (dialog, which) -> {
                            String persona = textInput.getText().toString();
                            if (!persona.isEmpty()) {
                                addConversation(persona);
                            }
                            dialog.dismiss();
                        })
                .setNegativeButton(
                        android.R.string.cancel,
                        (dialog, which) -> {
                            dialog.cancel();
                        });
        alertDialogBuilder.show();
    }

    private void addConversation(String persona) {
        Log.d(TAG, "Adding conversation for " + persona);
        new Thread(
                        () -> {
                            try {
                                Conversation conversation = new Conversation(persona);
                                conversation.participants.add(persona);

                                db.conversationDao().insertConversation(conversation);

                                NavHostFragment.findNavController(ConversationListFragment.this)
                                        .navigate(
                                                ConversationListFragmentDirections
                                                        .actionConversationListToMessageList(
                                                                persona));
                            } catch (Exception err) {
                                Log.e(TAG, "Error adding new conversation to " + persona, err);
                                Toast.makeText(
                                                getActivity(),
                                                "Error adding conversation",
                                                Toast.LENGTH_LONG)
                                        .show();
                            }
                        })
                .start();
    }

    private void onClickConversation(View source) {
        Conversation conversation = (Conversation) source.getTag();
        Log.d(TAG, "onClickConversation: " + conversation.id);

        NavHostFragment.findNavController(this)
                .navigate(
                        ConversationListFragmentDirections.actionConversationListToMessageList(
                                conversation.id));
    }
}
