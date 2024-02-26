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

import java.util.Vector;

public class JLinkConfig {

    public String linkProfile = "";
    public Vector<String> personas = new Vector<>();
    public JLinkProperties linkProps = new JLinkProperties();

    @Override
    public String toString() {
        String config = "";
        config += "linkProfile: " + linkProfile + "\n";
        for (String persona : personas) {
            config += "connectedTo: " + persona + "\n";
        }
        config += linkProps.toString();
        config += "bestReceive: " + linkProps.best.toString() + "\n";
        return config;
    }
}
