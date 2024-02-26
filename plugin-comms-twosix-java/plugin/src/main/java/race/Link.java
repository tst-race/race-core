
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

package race;

import ShimsJava.JEncPkg;
import ShimsJava.JLinkProperties;
import ShimsJava.LinkType;

import java.util.HashMap;
import java.util.Map;
import java.util.Vector;

public abstract class Link {
    final PluginCommsTwoSixJava plugin;
    final String linkId;
    LinkType linkType;
    final String profile;
    final Vector<String> personas;
    final JLinkProperties linkProperties;
    protected Map<String, Connection> connections = new HashMap<>();

    Link(
            PluginCommsTwoSixJava plugin,
            String linkId,
            String profile,
            Vector<String> personas,
            JLinkProperties linkProperties) {
        this.plugin = plugin;
        this.linkId = linkId;
        this.profile = profile;
        this.personas = personas;
        this.linkProperties = linkProperties;
    }

    /** @return the LinkId */
    String getLinkId() {
        return linkId;
    }

    /** @return the profile */
    String getProfile() {
        return profile;
    }

    /** @return the personas */
    public Vector<String> getPersonas() {
        return personas;
    }

    /** @return the linkProperties */
    public JLinkProperties getLinkProperties() {
        return linkProperties;
    }

    /** @return the linkConnections */
    public Vector<Connection> getLinkConnections() {
        return new Vector<Connection>(connections.values());
    }

    abstract Connection openConnection(
            LinkType linkType, final String connectionId, final String linkHints);

    abstract void closeConnection(String connectionId);

    abstract void sendPackage(JEncPkg pkg);
}
