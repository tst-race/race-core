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

import androidx.annotation.NonNull;
import androidx.room.Entity;
import androidx.room.PrimaryKey;

import java.util.ArrayList;
import java.util.List;

/** Conversation Entity is a table of messages between two clients. */
@Entity
public class Conversation {

    @PrimaryKey @NonNull public String id;

    // participants is a list to enable group messages in the future but it is currently limited to
    // two participants: this device and another client.
    public List<String> participants;

    /**
     * Overloaded constructor is used for testing purposes. This allows us to prepopulate the
     * database and overwrite existing conversations rather than add new random ids every time the
     * app starts
     *
     * @param id
     */
    public Conversation(String id) {
        participants = new ArrayList<>();
        this.id = id;
    }
}
