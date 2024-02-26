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

package ShimsJava;

public class ChannelRole {
    public String roleName;
    public String[] mechanicalTags;
    public String[] behavioralTags;
    public LinkSide linkSide;

    public ChannelRole() {
        roleName = new String();
        mechanicalTags = new String[0];
        behavioralTags = new String[0];
        linkSide = LinkSide.LS_UNDEF;
    }

    public ChannelRole(
            String roleName, String[] mechanicalTags, String[] behavioralTags, LinkSide linkSide) {
        this.roleName = roleName;
        this.mechanicalTags = mechanicalTags;
        this.behavioralTags = behavioralTags;
        this.linkSide = linkSide;
    }

    /** @return int */
    public int getLinkSideAsInt() {
        return linkSide.ordinal();
    }
}
