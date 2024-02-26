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

package com.twosix.race.room;

import android.content.Context;
import android.util.Log;

import androidx.room.Database;
import androidx.room.Room;
import androidx.room.RoomDatabase;
import androidx.room.TypeConverters;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.parser.*;

import java.io.FileReader;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Executor;

/**
 * Conversation Database holds a table of conversations. Each conversation is another table of
 * messages.
 */
@Database(
        entities = {Message.class, Conversation.class},
        version = 1,
        exportSchema = false)
@TypeConverters({Converters.class})
public abstract class ConversationDatabase extends RoomDatabase {

    public static final String DB_NAME = "conversation_db";
    public static ConversationDatabase instance;
    public static final List<String> clients = new ArrayList<>();

    public abstract ConversationDao conversationDao();

    public static synchronized ConversationDatabase getInstance(Context context) {
        if (instance == null && context != null) {
            instance =
                    Room.databaseBuilder(
                                    context.getApplicationContext(),
                                    ConversationDatabase.class,
                                    DB_NAME)
                            .fallbackToDestructiveMigration()
                            .build();
            populateDB();
        }
        return instance;
    }

    /**
     * Parse the race-config.json file for the number RACE Clients listed in the current deployment.
     *
     * @return count The number of RACE Clients found listed in the race-config.json.
     */
    public static Integer parseConfigJson() {
        int count = 0;
        try {
            // parsing file "race-config.json"
            JSONObject obj =
                    (JSONObject)
                            new JSONParser()
                                    .parse(
                                            new FileReader(
                                                    "/data/data/com.twosix.race/race/config/race-config.json"));

            JSONObject range = (JSONObject) obj.get("range");
            JSONArray race_nodes = (JSONArray) range.get("RACE_nodes");
            for (int i = 0; i < race_nodes.size(); i++) {
                String name = (String) ((JSONObject) race_nodes.get(i)).get("name");
                String search = "race-client-";

                if (name.toLowerCase().contains(search.toLowerCase())) {
                    count++;
                }
            }
        } catch (Exception e) {
            Log.e("No RACE Clients found, conversation list will be empty: ", e.getMessage());
        }
        return count;
    }

    private static void populateDB() {
        final int numReachableClients = parseConfigJson();
        createClientList(numReachableClients);

        Executor executor = new SerialExecutor();
        executor.execute(
                new Runnable() {
                    @Override
                    public void run() {
                        Log.d(this.getClass().getName(), "com.twosix.race populateDB Run");

                        for (String client : clients) {
                            // specifying an id so that a new one isn't added on restart
                            Conversation sampleConversation = new Conversation(client);
                            sampleConversation.participants.add(client);
                            try {
                                instance.conversationDao().insertConversation(sampleConversation);
                            } catch (Exception e) {
                                // Instead of clearing the SQL database before every test run, I
                                // just populate it in a try-catch
                            }
                        }
                    }
                });
    }

    /**
     * Create client names with name padding
     *
     * @param numOfClients
     */
    private static void createClientList(int numOfClients) {
        for (int i = 1; i <= numOfClients; i++) {
            String client = "race-client-" + String.format("%05d", i);
            clients.add(client);
        }
    }
}
