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

public class AppConfig {

    public String persona;
    public String appDir;
    public String pluginArtifactsBaseDir;
    public String platform;
    public String architecture;
    // Type of environment the RACE node will be run on (phone, enterprise-server, desktop, etc.)
    public String environment;
    // Config Files
    public String configTarPath;
    public String baseConfigPath;
    // Testing specific files (user-responses.json, jaeger-config.json, voa.json)
    public String etcDirectory;
    public String jaegerConfigPath;
    public String userResponsesFilePath;
    public String voaConfigPath;
    // Bootstrap Directories
    public String bootstrapFilesDirectory;
    public String bootstrapCacheDirectory;
    // Others
    public String sdkFilePath;
    public String tmpDirectory;
    public String logDirectory;
    public String logFilePath;
    public String appPath;
    public NodeType nodeType;
    public StorageEncryptionType encryptionType;

    // default constructor is protected to force use of create which handles jni initialization
    protected AppConfig() {}

    // copy constructor
    public AppConfig(AppConfig other) {
        this.persona = other.persona;
        this.appDir = other.appDir;
        this.pluginArtifactsBaseDir = other.pluginArtifactsBaseDir;
        this.platform = other.platform;
        this.architecture = other.architecture;
        this.environment = other.environment;
        // Config Files
        this.configTarPath = other.configTarPath;
        this.baseConfigPath = other.baseConfigPath;
        // Testing specific files (user-responses.json, jaeger-config.json, voa.json)
        this.etcDirectory = other.etcDirectory;
        this.jaegerConfigPath = other.jaegerConfigPath;
        this.userResponsesFilePath = other.userResponsesFilePath;
        this.voaConfigPath = other.voaConfigPath;
        // Bootstrap Directories
        this.bootstrapFilesDirectory = other.bootstrapFilesDirectory;
        this.bootstrapCacheDirectory = other.bootstrapCacheDirectory;
        // Others
        this.sdkFilePath = other.sdkFilePath;

        this.tmpDirectory = other.tmpDirectory;
        this.logDirectory = other.logDirectory;
        this.logFilePath = other.logFilePath;
        this.appPath = other.appPath;

        this.nodeType = other.nodeType;
        this.encryptionType = other.encryptionType;
    }

    public void setNodeType(int type) {
        this.nodeType = NodeType.valueOf(type);
    }

    public void setEncryptionType(int type) {
        this.encryptionType = StorageEncryptionType.valueOf(type);
    }

    // initialize with the defaults from c++ so that logic isn't duplicated
    public static native AppConfig create();
}
