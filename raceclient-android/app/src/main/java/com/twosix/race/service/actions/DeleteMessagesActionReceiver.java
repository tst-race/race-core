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

package com.twosix.race.service.actions;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import com.twosix.race.room.ConversationDao;
import com.twosix.race.room.ConversationDatabase;
import com.twosix.race.room.SerialExecutor;

import java.util.concurrent.Executor;

public class DeleteMessagesActionReceiver extends BroadcastReceiver {

    private static final String TAG = "DeleteMessagesActionReceiver";

    Executor executor = new SerialExecutor();

    @Override
    public void onReceive(Context context, Intent intent) {
        executor.execute(
                () -> {
                    Log.i(TAG, "Deleting messages from database");

                    try {
                        ConversationDatabase db = ConversationDatabase.getInstance(context);
                        ConversationDao dao = db.conversationDao();
                        dao.deleteAllConversations();
                        dao.deleteAllMessages();
                    } catch (Exception e) {
                        Log.e(TAG, "Error deleting messages: " + e.getMessage());
                    }
                });
    }
}
