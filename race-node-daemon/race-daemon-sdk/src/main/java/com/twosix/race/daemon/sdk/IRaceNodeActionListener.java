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

package com.twosix.race.daemon.sdk;

/**
 * Interface for daemon apps to receive orchestration actions received from RiB by the daemon SDK.
 */
public interface IRaceNodeActionListener {
    /**
     * Notifies the listener of the received action.
     *
     * @param jsonAction Action JSON string
     */
    public void execute(String jsonAction);
}
