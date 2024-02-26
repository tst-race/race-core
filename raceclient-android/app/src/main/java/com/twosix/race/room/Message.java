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
import java.util.UUID;

@Entity
public class Message {
    @PrimaryKey @NonNull public String id;

    public String convId;
    public String plainMsg;
    public String timeReceived;
    public String createTime;
    public String fromPersona;
    public List<String> recipients;
    public int nonce;
    public long traceId;
    public long spanId;

    public Message() {
        id = UUID.randomUUID().toString();
        recipients = new ArrayList<>();
    }
}
