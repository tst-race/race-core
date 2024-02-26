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

import androidx.lifecycle.LiveData;
import androidx.room.Dao;
import androidx.room.Insert;
import androidx.room.Query;

import java.util.List;

/** Conversation Dao is used to insert/query conversations and messages persisted in Room. */
@Dao
public interface ConversationDao {
    @Insert
    void insertMessage(Message message);

    /**
     * Retrieve messages for a specific conversation.
     *
     * @param conversationId id of conversation to retrieve messages for.
     * @return LiveData list of messages which trigger updates when the DB is updated
     */
    @Query("Select * from Message where convId = :conversationId")
    LiveData<List<Message>> getMessages(String conversationId);

    /**
     * Retrieve messages for a specific conversation.
     *
     * @param conversationId id of conversation to retrieve messages for.
     * @return LiveData list of messages which trigger updates when the DB is updated
     */
    @Query("Select * from Message where convId = :conversationId")
    List<Message> getMessagesSynchronous(String conversationId);

    @Insert
    void insertConversation(Conversation conversation);

    /**
     * Retrieve all conversation is DB
     *
     * @return LiveData list of conversations which trigger updates when the DB is updated
     */
    @Query("Select * from Conversation")
    LiveData<List<Conversation>> getConversations();

    /**
     * Retrieve all conversation is DB except for active persona. This is needed because we're
     * storing all clients in the database including ourselves.
     *
     * @return LiveData list of conversations which trigger updates when the DB is updated
     */
    @Query("Select * from Conversation WHERE id != :conversationId")
    LiveData<List<Conversation>> getAllConversationsExcept(String conversationId);

    /**
     * Retrieve all conversation is DB
     *
     * @return LiveData list of conversations which trigger updates when the DB is updated
     */
    @Query("Select * from Conversation")
    List<Conversation> getConversationsSyncronous();

    /**
     * Retieve a specific conversation. This is helpful to get the participants list.
     *
     * @param conversationId
     * @return Conversation object
     */
    @Query("Select * from Conversation where id = :conversationId")
    Conversation getConversation(String conversationId);

    @Query("DELETE FROM Conversation")
    void deleteAllConversations();

    @Query("DELETE FROM Message")
    void deleteAllMessages();
}
