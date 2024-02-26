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

public class DeviceInfo {
    private String platform;
    private String architecture;
    private String nodeType;

    public DeviceInfo(String platform, String architecture, String nodeType) {
        this.platform = platform;
        this.architecture = architecture;
        this.nodeType = nodeType;
    }

    public String getPlatform() {
        return platform;
    }

    public String getArchitecture() {
        return architecture;
    }

    public String getNodeType() {
        return nodeType;
    }
}
