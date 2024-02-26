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

/** Configuration for the daemon SDK. */
public class RaceNodeDaemonConfig {
    private String fileServerHost = "rib-file-server";
    private int fileServerPort = 8080;
    private String persona;
    private String redisHost = "rib-redis";
    private int redisPort = 6379;
    private boolean isGenesis = false;
    private String daemonStateInfoJsonPath;

    public String getFileServerHost() {
        return fileServerHost;
    }

    /**
     * Sets the hostname (or IP address) of the file server used for transferring of files with RiB.
     *
     * @param fileServerHost File server hostname
     */
    public void setFileServerHost(String fileServerHost) {
        this.fileServerHost = fileServerHost;
    }

    public int getFileServerPort() {
        return fileServerPort;
    }

    /**
     * Sets the port on the file server used for transferring of files with RiB.
     *
     * @param fileServerPort File server port
     */
    public void setFileServerPort(int fileServerPort) {
        this.fileServerPort = fileServerPort;
    }

    public String getPersona() {
        return persona;
    }

    /**
     * Sets the node persona corresponding to the node on which the daemon is running.
     *
     * @param persona RACE node persona of the current node
     */
    public void setPersona(String persona) {
        this.persona = persona;
    }

    public String getRedisHost() {
        return redisHost;
    }

    /**
     * Sets the hostname (or IP address) of the Redis server used for communication with RiB.
     *
     * @param redisHost Redis server hostname
     */
    public void setRedisHost(String redisHost) {
        this.redisHost = redisHost;
    }

    public int getRedisPort() {
        return redisPort;
    }

    /**
     * Sets the port on the Redis server used for communication with RiB.
     *
     * @param redisPort Redis server port
     */
    public void setRedisPort(int redisPort) {
        this.redisPort = redisPort;
    }

    /**
     * Sets whether or not this node is a genesis based on whether the RACE app was installed at the
     * time the daemon started
     *
     * @param isGenesis whether or not the node is a genesis node
     */
    public void setIsGenesis(boolean isGenesis) {
        this.isGenesis = isGenesis;
    }

    public boolean getIsGenesis() {
        return this.isGenesis;
    }

    /** Getter and Setter for daemonStateInfoJsonPath */
    public void setDaemonStateInfoJsonPath(String path) {
        this.daemonStateInfoJsonPath = path;
    }

    public String getDaemonStateInfoJsonPath() {
        return this.daemonStateInfoJsonPath;
    }
}
