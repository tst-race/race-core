#!/usr/bin/env python3

#
# Copyright 2023 Two Six Technologies
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""
    Purpose:
        PluginCommsTwoSixPython is the python class for a RACE Comms Plugin. This is
        an exemplar of what a comms plugin would look like when using the RACE SDK
        Language bindings for comms
"""

# Python Library Imports
import json
import os
import requests
import socket
import threading
import time
from base64 import b64encode, b64decode
from typing import Any, Dict, List, Optional, Tuple

# RACE Libraries
from .Log import logDebug, logError, logWarning, logInfo
from .response_logger import ResponseLogger
from .channels import (
    DIRECT_CHANNEL_GID,
    INDIRECT_CHANNEL_GID,
    get_default_channel_properties_for_channel,
    get_default_link_properties_for_channel,
)

# Swig Libraries
from commsPluginBindings import (
    EncPkg,
    IRacePluginComms,
    IRaceSdkComms,
    LinkProperties,
    LinkPropertyPair,
    LinkPropertySet,
    PluginConfig,
    RACE_BLOCKING,
    CONNECTION_CLOSED,  # ConnectionStatus
    CONNECTION_OPEN,  # ConnectionStatus
    NULL_RACE_HANDLE,  # RaceHandle
    PACKAGE_FAILED_GENERIC,  # PackageStatus
    PACKAGE_SENT,  # PackageStatus
    PLUGIN_ERROR,  # PluginResponse
    PLUGIN_FATAL,  # PluginResponse
    PLUGIN_OK,  # PluginResponse
    SDK_OK,  # SdkStatus
    LT_SEND,  # LinkTypes
    LT_RECV,  # LinkTypes
    LT_BIDI,  # LinkTypes
    TT_MULTICAST,  # TransmissionTypes
    TT_UNICAST,  # TransmissionTypes
    CT_DIRECT,  # ConnectionTypes
    CT_INDIRECT,  # ConnectionTypes
    ST_STORED_ASYNC,  # SendTypes
    ST_EPHEM_SYNC,  # SendTypes
    StringVector,
    CHANNEL_AVAILABLE,
    CHANNEL_UNAVAILABLE,
    CHANNEL_FAILED,
    ChannelProperties,
    LINK_CREATED,
    LINK_LOADED,
    LINK_DESTROYED,
    UD_DIALOG,  # UserDisplayTypes
    UD_QR_CODE,  # UserDisplayTypes
    UD_TOAST,  # UserDisplayTypes
    BS_DOWNLOAD_BUNDLE,  # BootstrapActionTypes
    BS_NETWORK_CONNECT,  # BootstrapActionTypes
    BS_COMPLETE,  # BootstrapActionTypes
)


PluginResponse = int  # mypy type alias


class PluginCommsTwoSixPython(IRacePluginComms):

    """

    Attributes:
        connections (dict): dict of CommsConn objects, keyed by connection_id string
        link_profiles (dict): dict of string-form link profiles, keyed by link_id string
        sdk (RaceSdk): the sdk object
    """

    response_logger = ResponseLogger()

    class CommsConn:

        """

        Attributes:
            connection_ids (list of str): identifiers for connection, to allow
                referencing conn without passing around the object. one
                connection ha many conn ids.
            host (str): hostname to use with sockets
            port (str): port to use with sockets
            link_id (str): unique identifier for link used by this connection
            link_type (enum): does this conn send? receive? - LT_SEND / LT_RECV / LT_BIDI
            sock (socket.socket): socket object used by connection
            thread (threading.Thread): thread on which socket is being monitored
        """

        def __init__(self, connection_id: str):
            """
                Initialize connection object with valid unique connection_id

            Args:
                connection_id: The initial/only connection ID
            """

            # Connection Tracking
            self.connection_ids = [connection_id]

            # Link Metadata
            self.link_id = None
            self.link_type = None

            # Monitoring Values
            self.terminate = False
            self.thread = None
            self.sock = None

            # Link Profile Attributes
            self.host = None
            self.port = None
            self.multicast = False
            self.frequency = None
            self.hashtag = None

    def __init__(self, sdk: IRaceSdkComms = None):
        """
        Purpose:
            Initialize the plugin
        Args:
            sdk: SDK object
        """

        super().__init__()

        logInfo("__init__ called")

        if sdk is None:
            raise Exception("sdk was not passed to plugin")

        self.race_sdk = sdk
        self.connections = {}
        self.connections_lock = threading.Lock()
        self.link_profiles = {}
        self.link_properties = {}
        self.lt_str_map = {
            "send": LT_SEND,
            "receive": LT_RECV,
            "recv": LT_RECV,
            "bidirectional": LT_BIDI,
            "bidi": LT_BIDI,
        }
        self.active_persona = None

        # Current statuses of the available channels. Used to prevent network manager performing
        # operations on channels that aren't (yet) active.
        self.channel_status = {
            DIRECT_CHANNEL_GID: CHANNEL_UNAVAILABLE,
            INDIRECT_CHANNEL_GID: CHANNEL_UNAVAILABLE,
        }

        # Map of channel GIDs keys to a set of link IDs active in that channel.
        self.links_in_channels = {
            DIRECT_CHANNEL_GID: set(),
            INDIRECT_CHANNEL_GID: set(),
        }

        self._next_available_port = 10000
        self._next_available_hashtag = 0
        self._whiteboard_hostname = "twosix-whiteboard"
        self._whiteboard_port = 5000

        self._hostname = "no-hostname-provided-by-user"
        self._request_start_port_handle = None
        self._request_hostname_handle = None

        self.thread = threading.Thread(target=self._keep_alive, args=())
        self.thread.start()
        self.user_input_requests = set()

        try:
            import torch  # pylint: disable=import-outside-toplevel

            if torch.cuda.is_available():
                logInfo("pytorch GPU is available")
            else:
                logInfo("pytorch GPU is NOT available")
        except Exception as exc:  # pylint: disable=broad-except
            logWarning(f"Error checking for pytorch GPU availability: {exc}")

        try:
            import tensorflow  # pylint: disable=import-outside-toplevel

            if tensorflow.test.is_gpu_available():
                logInfo("tensorflow GPU is available")
            else:
                logInfo("tensorflow GPU is NOT available")
        except Exception as exc:  # pylint: disable=broad-except
            logWarning(f"Error checking for tensorflow GPU availability: {exc}")

        try:
            import cupy  # pylint: disable=import-outside-toplevel,unused-import

            logInfo("cupy is available")
        except Exception as exc:  # pylint: disable=broad-except
            logWarning(f"cupy is NOT available: {exc}")

    def __del__(self):
        """
        Purpose:
            Destructure for the plugin object.
            Shuts down the plugin to ensure a clean shutdown of all connections.
        """

        self.shutdown()

    def init(self, plugin_config: PluginConfig) -> int:
        """
        Purpose:
            Initializing the application. This is called before both plugins are
            initalized so we cannot make calls to the other plugin functionality
        Args:
            plugin_config (PluginConfig): Config object containing dynamic config variables (e.g. paths)
        Returns:
            PluginResponse: The status of the plugin in response to this call
        """
        logInfo("init called")
        logInfo(f"etcDirectory: {plugin_config.etcDirectory}")
        logInfo(f"loggingDirectory: {plugin_config.loggingDirectory}")
        logInfo(f"auxDataDirectory: {plugin_config.auxDataDirectory}")
        logInfo(f"tmpDirectory: {plugin_config.tmpDirectory}")
        logInfo(f"pluginDirectory: {plugin_config.pluginDirectory}")

        self.active_persona = self.race_sdk.getActivePersona()
        logDebug(f"init: I am {self.active_persona}")

        self.race_sdk.writeFile(
            "initialized.txt", "Comms Python Plugin Initialized\n".encode("utf-8")
        )
        tmp = bytes(self.race_sdk.readFile("initialized.txt")).decode("utf-8")
        logDebug("Type of readFile return: {}".format(type(tmp)))
        logDebug(f"Read Initialization File: {tmp}")

        logInfo("init returned")
        return PLUGIN_OK

    def activateChannel(self, handle, channel_gid, _role_name):
        """
        Purpose:
            Activate a specific channed
        Returns:
            PluginResponse: The status of the plugin in response to this call
        """
        logInfo("activateChannel called for " + channel_gid)

        if channel_gid == INDIRECT_CHANNEL_GID:
            self.channel_status[channel_gid] = CHANNEL_AVAILABLE
            self.race_sdk.onChannelStatusChanged(
                NULL_RACE_HANDLE,
                channel_gid,
                CHANNEL_AVAILABLE,
                get_default_channel_properties_for_channel(self.race_sdk, channel_gid),
                RACE_BLOCKING,
            )
            self.race_sdk.displayInfoToUser(f"{channel_gid} is available", UD_TOAST)
        elif channel_gid == DIRECT_CHANNEL_GID:
            response = self.race_sdk.requestPluginUserInput(
                "startPort", "What is the first available port?", True
            )
            if response.status != SDK_OK:
                logWarning("Failed to request start port from user")
            self._request_start_port_handle = response.handle
            self.user_input_requests.add(self._request_start_port_handle)

            response = self.race_sdk.requestCommonUserInput("hostname")
            if response.status != SDK_OK:
                logWarning("Failed to request hostname from user")
                self.channel_status[channel_gid] = CHANNEL_FAILED
                self.race_sdk.onChannelStatusChanged(
                    NULL_RACE_HANDLE,
                    channel_gid,
                    CHANNEL_FAILED,
                    get_default_channel_properties_for_channel(
                        self.race_sdk, channel_gid
                    ),
                    RACE_BLOCKING,
                )
            self._request_hostname_handle = response.handle
            self.user_input_requests.add(self._request_hostname_handle)
        else:
            logWarning("Unrecognized channel_gid: " + channel_gid)

        return PLUGIN_OK

    def shutdown(self) -> int:
        """
        Purpose:
            Iterate through all of the existing connections, close each.
        Returns:
            PluginResponse: The status of the plugin in response to this call
        """
        logInfo("shutdown called")

        connections = list(self.connections.keys())
        for conn_id in connections:
            self.closeConnection(NULL_RACE_HANDLE, conn_id)
        return PLUGIN_OK

    def parsePropertySet(self, prop_json: Dict[str, Any]) -> LinkPropertySet:
        """
        Purpose:
            Parse the link property set from the given generic dict of values
        Args:
            prop_json: dict of link property values
        Returns:
            LinkPropertySet object
        """
        propset = LinkPropertySet()
        if prop_json is not None:
            propset.bandwidth_bps = prop_json.get("bandwidth_bps", -1)
            propset.latency_ms = prop_json.get("latency_ms", -1)
            propset.loss = prop_json.get("loss", -1.0)

        return propset

    def parsePropertyPair(self, prop_json: Dict[str, Any]) -> LinkPropertyPair:
        """
        Purpose:
            Parse the send/receive link property pair from the given
            generic dict of values
        Args:
            prop_json: dict of link property pair values
        Returns:
            LinkPropertyPair object
        """
        pair = LinkPropertyPair()
        if prop_json is not None:
            pair.send = self.parsePropertySet(prop_json.get("send", None))
            pair.receive = self.parsePropertySet(prop_json.get("receive", None))

        return pair

    def sendPackage(
        self,
        handle: int,
        conn_id: str,
        enc_pkg: EncPkg,
        timeoutTimestamp: float,
        batchId: int,
    ) -> int:
        """
        Purpose:
            Sends the provided EncPkg object on the connection identified by conn_id

        Args:
            handle: The RaceHandle to use for updating package status in
                    onPackageStatusChanged
            conn_id: The ID of the connection to use to send the package
            enc_pkg: The encrypted package to send

        Returns:
            PluginResponse: The status of the plugin in response to this call
        """
        logInfo("sendPackage called")

        conn = self.connections[conn_id]

        # Fail if this is a receive-only connection
        if conn.link_type == LT_RECV:
            logDebug("Attempting to send on a receive-only connection")
            return PLUGIN_ERROR

        connect_type_string = "multicast" if conn.multicast else "direct"
        logInfo(f"sendPackage: sending over {connect_type_string} link")

        data = bytes(enc_pkg.getRawData())

        if conn.multicast:
            self.sendPackageMulticast(handle, conn, data)
        else:
            self.sendPackageDirectLink(handle, conn, data)

        logInfo("sendPackage returned")
        return PLUGIN_OK

    def sendPackageDirectLink(self, handle: int, conn: CommsConn, data: bytes):
        """
            Sends the provided raw data over the specified direct link connection

        Args:
            handle: The RaceHandle to use for updating package status in
                    onPackageStatusChanged
            conn: The connection to use to send the package
            data: The raw data to send

        Returns:
            None
        """

        # Create Socket
        with socket.socket() as s:
            try:
                s.connect((conn.host, conn.port))
            except Exception as err:
                logError(
                    f"sendPackageDirectLink: Failed to connect to {conn.host}:"
                    f"{conn.port} with error: {err}"
                )
                self.race_sdk.onPackageStatusChanged(
                    handle, PACKAGE_FAILED_GENERIC, RACE_BLOCKING
                )
                return

            logDebug(
                "sendPackageDirectLink: socket connected. sending data over socket..."
            )

            # Get Data to Send and send it
            try:
                s.sendall(data)
                s.shutdown(socket.SHUT_RDWR)
                logDebug("sendPackageDirectLink: data sent over socket")
                self.race_sdk.onPackageStatusChanged(
                    handle, PACKAGE_SENT, RACE_BLOCKING
                )
            except Exception as err:
                logError(
                    f"sendPackageDirectLink: Failed to send message (direct link): {err}"
                )
                self.race_sdk.onPackageStatusChanged(
                    handle, PACKAGE_FAILED_GENERIC, RACE_BLOCKING
                )

        # Close socket for now, Plugin has no reuse

    def sendPackageMulticast(self, handle: int, conn: CommsConn, data: bytes):
        """
            Sends the provided raw data over the specified multicast link connection

        Args:
            handle: The RaceHandle to use for updating package status in
                    onPackageStatusChanged
            conn: The connection to use to send the package
            data: The raw data to send

        Returns:
            None
        """

        url = f"http://{conn.host}:{conn.port}/post/{conn.hashtag}"
        body = {"data": b64encode(data)}
        response = requests.post(url, json=body)

        if (
            response.status_code not in (200, 201)
            or response.json().get("index", None) is None
        ):
            logError(f"Failed to Generate Message: {response.status_code}")
            self.response_logger.log_if_first(url, response)
            self.race_sdk.onPackageStatusChanged(
                handle, PACKAGE_FAILED_GENERIC, RACE_BLOCKING
            )
        else:
            self.race_sdk.onPackageStatusChanged(handle, PACKAGE_SENT, RACE_BLOCKING)

    def openConnection(
        self,
        handle: int,
        link_type: int,
        link_id: str,
        link_hints: Optional[str] = None,
        send_timeout: int = None,
    ) -> int:
        """
        Purpose:
            Open a connection of a specific link type
        Args:
            handle: The RaceHandle to use for onConnectionStatusChanged calls
            link_type: The type of link to open (LT_SEND / LT_RECV / LT_BIDI)
            link_id: The ID of the link that the connection should be opened on
            link_hints: Additional optional configuration information
                provided by network manager as a stringified JSON object
        Returns:
            PluginResponse: The status of the plugin in response to this call
        """
        logInfo("openConnection called")

        if not self.link_profiles.get(link_id):
            logError(f"openConnection: no link profile found for link ID {link_id}")
            return PLUGIN_ERROR

        logDebug(f"    cached link profile: {self.link_profiles[link_id]}")
        logDebug(f"    link_hints: {link_hints}")

        with self.connections_lock:
            for c in self.connections.values():
                if c.link_id == link_id:
                    logInfo("connection already exists.")
                    connection_id = self.race_sdk.generateConnectionId(link_id)
                    self.connections[connection_id] = c
                    c.connection_ids.append(connection_id)
                    self.race_sdk.onConnectionStatusChanged(
                        handle,
                        connection_id,
                        CONNECTION_OPEN,
                        self.link_properties[link_id],
                        RACE_BLOCKING,
                    )
                    return PLUGIN_OK

            connection_id = self.race_sdk.generateConnectionId(link_id)
            conn = self.CommsConn(connection_id)
            self.connections[connection_id] = conn

            conn.link_id = link_id
            conn.link_type = link_type

            link_profile = self.link_profiles[link_id]

            # Parse Link Profile
            conn.port = link_profile.get("port", None)
            conn.host = link_profile.get("hostname", "localhost")
            conn.multicast = link_profile.get("multicast", False)
            conn.frequency = link_profile.get("checkFrequency", 0)
            conn.hashtag = link_profile.get("hashtag", "")

            if link_type in [LT_RECV, LT_BIDI]:
                # Links that can receive messages require a thread to listen/poll for
                # messages depending on multicast/unicase
                if conn.multicast:
                    logDebug("Start New Multicast Thread")
                    conn.thread = threading.Thread(
                        target=self._multicast_connection_monitor,
                        args=(
                            handle,
                            connection_id,
                            conn,
                        ),
                    )
                    conn.thread.start()
                else:
                    logDebug("Start New Direct Thread")
                    conn.thread = threading.Thread(
                        target=self._direct_connection_monitor,
                        args=(
                            handle,
                            connection_id,
                            conn,
                        ),
                    )
                    conn.thread.start()
            else:
                # Sending does not require starting a thread to listent/poll link,
                # just return success to SDK
                self.race_sdk.onConnectionStatusChanged(
                    handle,
                    connection_id,
                    CONNECTION_OPEN,
                    self.link_properties[link_id],
                    RACE_BLOCKING,
                )

            return PLUGIN_OK
        # end of with self.connections_lock

    def closeConnection(self, handle: int, conn_id: str) -> int:
        """
        Purpose:
            Closes the socket being used by a connection, stops the thread,
            deletes the CommsConn object.
        Args:
            handle: The RaceHandle to use for onConnectionStatusChanged calls
            conn_id: the unique identifier string for the connection to be closed
        Returns:
            PluginResponse: The status of the plugin in response to this call
        """
        logInfo("closeConnection called")
        logDebug(f"    connection: {conn_id}")

        with self.connections_lock:
            if not conn_id in self.connections:
                logWarning(f"No connection found: {conn_id}")
                return PLUGIN_ERROR

            # Unlink the connection from the given connection ID--the connection may
            # still be in use with another connection ID
            conn = self.connections.pop(conn_id)
            conn.connection_ids.remove(conn_id)

            # If this was the last logical connection, shut down the actual link
            if len(conn.connection_ids) == 0:
                try:
                    conn.terminate = True
                    if conn.sock:
                        conn.sock.close()
                    # the monitor thread will die off eventually
                except Exception as err:
                    logError(f"Closing connection err: {err}")
                    return PLUGIN_ERROR

            self.race_sdk.onConnectionStatusChanged(
                handle,
                conn_id,
                CONNECTION_CLOSED,
                self.link_properties[conn.link_id],
                RACE_BLOCKING,
            )
            return PLUGIN_OK
        # end of with self.connections_lock

    def destroyLink(self, handle, link_id):
        logPrefix = f"destroyLink: (handle: {handle}, link ID: {link_id}): "
        logDebug(f"{logPrefix}called")

        channel_gid_for_link = None
        for channel_gid, link_ids_in_channel in self.links_in_channels.items():
            if link_id in link_ids_in_channel:
                channel_gid_for_link = channel_gid
                break
        if not channel_gid:
            logError(f"{logPrefix}failed to find link ID: {link_id}")
            return PLUGIN_ERROR

        # Notify the SDK that the link has been destroyed
        link_props = get_default_link_properties_for_channel(
            self.race_sdk, channel_gid_for_link
        )
        link_props.linkType = LT_BIDI
        self.race_sdk.onLinkStatusChanged(
            handle,
            link_id,
            LINK_DESTROYED,
            link_props,
            RACE_BLOCKING,
        )

        # Close all the connections in the given link.
        # Iterate over a copy because connections will be removed during iteration
        for conn_id, conn in list(self.connections.items()):
            if conn.link_id == link_id:
                self.closeConnection(handle, conn_id)

        # Remove the link ID reference.
        self.links_in_channels[channel_gid_for_link].remove(link_id)

        logDebug(f"{logPrefix}returned")
        return PLUGIN_OK

    def createLink(self, handle, channel_gid):
        log_prefix = f"createLink: (handle: {handle}, channel GID: {channel_gid}): "
        logDebug(f"{log_prefix}called")

        if self.channel_status.get(channel_gid) != CHANNEL_AVAILABLE:
            logError(f"{log_prefix}channel not available.")
            self.race_sdk.onLinkStatusChanged(
                handle,
                "",
                LINK_DESTROYED,
                LinkProperties(),
                RACE_BLOCKING,
            )
            return PLUGIN_ERROR

        link_id = self.race_sdk.generateLinkId(channel_gid)
        if not link_id:
            logDebug(f"{log_prefix}sdk failed to generate link ID")
            self.race_sdk.onLinkStatusChanged(
                handle,
                "",
                LINK_DESTROYED,
                LinkProperties(),
                RACE_BLOCKING,
            )
            return PLUGIN_ERROR
        link_props = get_default_link_properties_for_channel(self.race_sdk, channel_gid)
        if channel_gid == DIRECT_CHANNEL_GID:
            logDebug(f"{log_prefix}creating direct link with ID: {link_id}")
            link_props.linkType = LT_RECV
            self.link_profiles[link_id] = {
                "hostname": self._hostname,
                "port": self._get_next_available_port(),
                "unicast": True,
            }
            link_props.linkAddress = json.dumps(self.link_profiles[link_id])
            self.link_properties[link_id] = link_props

            self.links_in_channels[channel_gid].add(link_id)
            self.race_sdk.onLinkStatusChanged(
                handle, link_id, LINK_CREATED, link_props, RACE_BLOCKING
            )
            self.race_sdk.updateLinkProperties(link_id, link_props, RACE_BLOCKING)
        elif channel_gid == INDIRECT_CHANNEL_GID:
            logDebug(f"{log_prefix}creating indirect link with ID: {link_id}")
            link_props.linkType = LT_BIDI
            self.link_profiles[link_id] = {
                "hostname": self._whiteboard_hostname,
                "port": self._whiteboard_port,
                "checkFrequency": 1000,
                "hashtag": f"python_{self.active_persona}_{self._next_available_hashtag}",
                "multicast": True,
            }
            link_props.linkAddress = json.dumps(self.link_profiles[link_id])
            self._next_available_hashtag = self._next_available_hashtag + 1
            self.link_properties[link_id] = link_props

            self.links_in_channels[channel_gid].add(link_id)
            self.race_sdk.onLinkStatusChanged(
                handle, link_id, LINK_CREATED, link_props, RACE_BLOCKING
            )
            self.race_sdk.updateLinkProperties(link_id, link_props, RACE_BLOCKING)
        else:
            logError(f"{log_prefix}invalid channel GID")
            self.race_sdk.onLinkStatusChanged(
                handle,
                "",
                LINK_DESTROYED,
                LinkProperties(),
                RACE_BLOCKING,
            )
            return PLUGIN_ERROR

        logDebug(
            f"{log_prefix}created link with ID: {link_id} and address: {link_props.linkAddress}"
        )
        logDebug(f"{log_prefix}returned")
        return PLUGIN_OK

    def createLinkFromAddress(self, handle, channel_gid, link_address):
        log_prefix = (
            f"createLinkFromAddress: (handle: {handle}, channel GID: {channel_gid}): "
        )
        logDebug(f"{log_prefix}called")

        if self.channel_status.get(channel_gid) != CHANNEL_AVAILABLE:
            logError(f"{log_prefix}channel not available.")
            self.race_sdk.onLinkStatusChanged(
                handle,
                "",
                LINK_DESTROYED,
                LinkProperties(),
                RACE_BLOCKING,
            )
            return PLUGIN_ERROR

        link_id = self.race_sdk.generateLinkId(channel_gid)

        link_props = get_default_link_properties_for_channel(self.race_sdk, channel_gid)
        if not link_id:
            logDebug(f"{log_prefix}sdk failed to generate link ID")
            self.race_sdk.onLinkStatusChanged(
                handle,
                "",
                LINK_DESTROYED,
                LinkProperties(),
                RACE_BLOCKING,
            )
            return PLUGIN_ERROR
        if channel_gid == DIRECT_CHANNEL_GID:
            logDebug(f"{log_prefix}creating direct link with ID: {link_id}")
            link_props.linkType = LT_RECV
            link_props.linkAddress = link_address

            self.link_profiles[link_id] = json.loads(link_address)
            self.link_properties[link_id] = link_props

            self.links_in_channels[channel_gid].add(link_id)
            self.race_sdk.onLinkStatusChanged(
                handle, link_id, LINK_CREATED, link_props, RACE_BLOCKING
            )
            self.race_sdk.updateLinkProperties(link_id, link_props, RACE_BLOCKING)
        elif channel_gid == INDIRECT_CHANNEL_GID:
            logDebug(f"{log_prefix}creating indirect link with ID: {link_id}")
            link_props.linkType = LT_BIDI
            link_props.linkAddress = link_address

            self.link_profiles[link_id] = json.loads(link_address)
            self.link_properties[link_id] = link_props

            self.links_in_channels[channel_gid].add(link_id)
            self.race_sdk.onLinkStatusChanged(
                handle, link_id, LINK_CREATED, link_props, RACE_BLOCKING
            )
            self.race_sdk.updateLinkProperties(link_id, link_props, RACE_BLOCKING)
        else:
            logError(f"{log_prefix}invalid channel GID")
            self.race_sdk.onLinkStatusChanged(
                handle,
                "",
                LINK_DESTROYED,
                LinkProperties(),
                RACE_BLOCKING,
            )
            return PLUGIN_ERROR

        logDebug(f"{log_prefix}returned")
        return PLUGIN_OK

    def loadLinkAddress(self, handle, channel_gid, link_address):
        log_prefix = (
            f"loadLinkAddress: (handle: {handle}, channel GID: {channel_gid}): "
        )
        logDebug(f"{log_prefix}called")

        if self.channel_status.get(channel_gid) != CHANNEL_AVAILABLE:
            logError(f"{log_prefix}channel not available.")
            self.race_sdk.onLinkStatusChanged(
                handle,
                "",
                LINK_DESTROYED,
                LinkProperties(),
                RACE_BLOCKING,
            )
            return PLUGIN_ERROR

        link_id = self.race_sdk.generateLinkId(channel_gid)
        if not link_id:
            logDebug(f"{log_prefix}sdk failed to generate link ID")
            self.race_sdk.onLinkStatusChanged(
                handle,
                "",
                LINK_DESTROYED,
                LinkProperties(),
                RACE_BLOCKING,
            )
            return PLUGIN_ERROR
        if channel_gid == DIRECT_CHANNEL_GID:
            logDebug(f"{log_prefix}loading direct link with ID: {link_id}")
            link_props = get_default_link_properties_for_channel(
                self.race_sdk, channel_gid
            )
            link_props.linkType = LT_SEND

            self.link_profiles[link_id] = json.loads(link_address)
            self.link_properties[link_id] = link_props

            self.links_in_channels[channel_gid].add(link_id)
            self.race_sdk.onLinkStatusChanged(
                handle, link_id, LINK_LOADED, link_props, RACE_BLOCKING
            )
            self.race_sdk.updateLinkProperties(link_id, link_props, RACE_BLOCKING)
        elif channel_gid == INDIRECT_CHANNEL_GID:
            logDebug(f"{log_prefix}loading indirect link with ID: {link_id}")
            link_props = get_default_link_properties_for_channel(
                self.race_sdk, channel_gid
            )
            link_props.linkType = LT_BIDI

            self.link_profiles[link_id] = json.loads(link_address)
            self.link_properties[link_id] = link_props

            self.links_in_channels[channel_gid].add(link_id)
            self.race_sdk.onLinkStatusChanged(
                handle, link_id, LINK_LOADED, link_props, RACE_BLOCKING
            )
            self.race_sdk.updateLinkProperties(link_id, link_props, RACE_BLOCKING)
        else:
            logError(f"{log_prefix}invalid channel GID")
            self.race_sdk.onLinkStatusChanged(
                handle,
                "",
                LINK_DESTROYED,
                LinkProperties(),
                RACE_BLOCKING,
            )
            return PLUGIN_ERROR

        logDebug(f"{log_prefix}returned")
        return PLUGIN_OK

    def loadLinkAddresses(self, handle, channel_gid, link_addresses):
        log_prefix = (
            f"loadLinkAddresses: (handle: {handle}, channel GID: {channel_gid}): "
        )
        logDebug(f"{log_prefix}called")

        if self.channel_status.get(channel_gid) != CHANNEL_AVAILABLE:
            logError(f"{log_prefix}channel not available.")
            self.race_sdk.onLinkStatusChanged(
                handle,
                "",
                LINK_DESTROYED,
                LinkProperties(),
                RACE_BLOCKING,
            )
            return PLUGIN_ERROR

        if get_default_channel_properties_for_channel(
            self.race_sdk, channel_gid
        ).multiAddressable:
            logError(log_prefix + "API not supported for this channel")
            self.race_sdk.onLinkStatusChanged(
                handle,
                "",
                LINK_DESTROYED,
                LinkProperties(),
                RACE_BLOCKING,
            )
            return PLUGIN_ERROR

        logDebug(f"{log_prefix}returned")
        return PLUGIN_OK

    def deactivateChannel(self, handle, channel_gid):
        log_prefix = (
            f"deactivateChannel: (handle: {handle}, channel GID: {channel_gid}): "
        )
        logDebug(f"{log_prefix}called")

        if self.channel_status.get(channel_gid) != CHANNEL_AVAILABLE:
            logError(f"{log_prefix}channel not available.")
            return PLUGIN_ERROR

        self.channel_status[channel_gid] = CHANNEL_UNAVAILABLE

        self.race_sdk.onChannelStatusChanged(
            NULL_RACE_HANDLE,
            channel_gid,
            CHANNEL_UNAVAILABLE,
            get_default_channel_properties_for_channel(self.race_sdk, channel_gid),
            RACE_BLOCKING,
        )

        # Destroy all links in channel, and implicitly all the connections in each link.
        # Iterate over a copy because links will be removed during iteration
        for link_id in list(self.links_in_channels[channel_gid]):
            self.destroyLink(handle, link_id)

        # Remove all links IDs associated with the channel
        self.links_in_channels[channel_gid].clear()

        logDebug(f"{log_prefix}returned")
        return PLUGIN_OK

    def onUserInputReceived(
        self,
        handle: int,
        answered: bool,
        response: str,
    ) -> PluginResponse:
        """
        Purpose:
            Notify comms about received user input response
        Args:
            handle: The handle for this callback
            answered: True if the response contains an actual answer to the input prompt, otherwise
                the response is an empty string and not valid
            response: The user response answer to the input prompt
        Returns:
            PluginResponse: The status of the Plugin in response to this call
        """
        log_prefix = f"onUserInputReceived: (handle: {handle}): "
        logDebug(f"{log_prefix}called")

        if handle == self._request_hostname_handle:
            if answered:
                self._hostname = response
                logInfo(f"{log_prefix}using hostname {self._hostname}")
            else:
                logError(
                    f"{log_prefix}direct channel not available without the hostname"
                )
                self.channel_status[DIRECT_CHANNEL_GID] = CHANNEL_DISABLED
                self.race_sdk.onChannelStatusChanged(
                    NULL_RACE_HANDLE,
                    DIRECT_CHANNEL_GID,
                    CHANNEL_DISABLED,
                    get_default_channel_properties_for_channel(
                        self.race_sdk, DIRECT_CHANNEL_GID
                    ),
                    RACE_BLOCKING,
                )

        elif handle == self._request_start_port_handle:
            if answered:
                try:
                    self._next_available_port = int(response)
                    logInfo(f"{log_prefix}using start port {self._next_available_port}")
                except ValueError:
                    logWarning(
                        f"{log_prefix}unable to parse response, {response}, using default start port"
                    )
            else:
                logWarning(f"{log_prefix}no answer, using default start port")

        else:
            logWarning(f"{log_prefix}handle is not recognized")
            return PLUGIN_ERROR

        self.user_input_requests.discard(handle)
        if not self.user_input_requests:
            # User Input Requests are only sent out when activateChannel is called on
            # DIRECT_CHANNEL_GID so it's safe to set it to available now
            self.channel_status[DIRECT_CHANNEL_GID] = CHANNEL_AVAILABLE
            self.race_sdk.onChannelStatusChanged(
                NULL_RACE_HANDLE,
                DIRECT_CHANNEL_GID,
                CHANNEL_AVAILABLE,
                get_default_channel_properties_for_channel(
                    self.race_sdk, DIRECT_CHANNEL_GID
                ),
                RACE_BLOCKING,
            )
            self.race_sdk.displayInfoToUser(
                DIRECT_CHANNEL_GID + " is available", UD_TOAST
            )

        logDebug(f"{log_prefix}returned")
        return PLUGIN_OK

    def flushChannel(
        self, handle: int, channelGid: str, batchId: int
    ) -> PluginResponse:
        logDebug(f"flushChannel: plugin does not support flushing")
        return PLUGIN_ERROR

    def onUserAcknowledgementReceived(
        self,
        handle: int,
    ) -> int:
        """
        Purpose:
            Notify the plugin that the user acknowledged the displayed information

        Args:
            handle: The handle for asynchronously returning the status of this call.

        Returns:
            PluginResponse: The status of the plugin in response to this call
        """
        logDebug(f"onUserAcknowledgementReceived: called")
        return PLUGIN_OK

    def _keep_alive(self):
        logDebug(
            "Started keep alive thread: Need to determine best method to keep "
            "Python threads running..."
        )
        while True:
            time.sleep(60)

    def _direct_connection_monitor(
        self, handle: int, connection_id: str, conn: CommsConn
    ):
        """
        Purpose:
            Continuously monitors the socket in the passed CommsConn object for EncPkgs,
            calls sdk.receiveEncPkg() on each
        Args:
            handle: The RaceHandle to use for initial onConnectionStatusChanged calls
            connection_id: The initial connection ID assigned to the connection
            conn: the connection object
        """

        logDebug(f"Creating connection monitor {conn.link_id}")

        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.bind(("", conn.port))
                s.listen()
                conn.sock = s
                self.race_sdk.onConnectionStatusChanged(
                    handle,
                    connection_id,
                    CONNECTION_OPEN,
                    self.link_properties[conn.link_id],
                    RACE_BLOCKING,
                )
                buf = bytearray(1024)
                while not conn.terminate:
                    sock, _ = s.accept()
                    with sock:
                        data = bytearray()
                        read = 1
                        while read > 0:
                            read = sock.recv_into(buf)
                            data += buf[:read]
                        enc_pkg = EncPkg(data)
                        self.race_sdk.receiveEncPkg(
                            enc_pkg, conn.connection_ids, RACE_BLOCKING
                        )
        except Exception as err:
            with self.connections_lock:
                logError(f"Direct Connection Monitor err: {err}")
                # Remove this connection from the connections map
                for conn_id in conn.connection_ids:
                    self.connections.pop(conn_id)

        # If shutting down due to an error, there will still be connection IDs
        # associated with this connection--let the SDK know the connection has
        # closed for each of them
        for conn_id in conn.connection_ids:
            self.race_sdk.onConnectionStatusChanged(
                NULL_RACE_HANDLE,
                conn_id,
                CONNECTION_CLOSED,
                self.link_properties[conn.link_id],
                RACE_BLOCKING,
            )

    @staticmethod
    def _get_last_post_index(conn: CommsConn) -> int:
        """
        Purpose:
            Fetch the index of the last post from the whiteboard service.
        Args:
            conn: Connection object
        Returns:
            Index of last post, or 0 if unable to fetch it
        """
        url = f"http://{conn.host}:{conn.port}/latest/{conn.hashtag}"
        headers = {"Content-Type": "application/json"}
        response = requests.get(url, headers=headers)
        if response.status_code == 200:
            return int(response.json().get("latest", 0))
        logDebug(f"failed to get last post index, status = {response.status_code}")
        PluginCommsTwoSixPython.response_logger.log_if_first(url, response)
        return 0

    @staticmethod
    def _get_new_posts(conn: CommsConn, oldest: int) -> Tuple[List[str], int]:
        """
        Purpose:
            Fetch all new posts after the given index.
        Args:
            conn: Connection object
            oldest: Index of oldest message to retrieve (i.e., fetch everything
                    after this index)
        Returns:
            Tuple of posts and index of last post
        """
        url = f"http://{conn.host}:{conn.port}/get/{conn.hashtag}/{oldest}/-1"
        headers = {"Content-Type": "application/json"}
        response = requests.get(url, headers=headers)
        if response.status_code == 200:
            body = response.json()
            return (body.get("data", []), body.get("length", 0))
        if 200 < response.status_code < 300:
            logDebug(f"no new posts, status = {response.status_code}")
            return ([], oldest)
        PluginCommsTwoSixPython.response_logger.log_if_first(
            f"http://{conn.host}:{conn.port}/get/{conn.hashtag}", response
        )
        raise Exception(
            f"Got Unexpected Status Code fetching new posts: {response.status_code}"
        )

    def _multicast_connection_monitor(
        self, handle: int, connection_id: str, conn: CommsConn
    ):
        """
        Purpose:
            Continuously monitors the whiteboard in the passed CommsConn object for
            EncPkgs, calls sdk.receiveEncPkg() on each
        Args:
            handle: The RaceHandle to use for initial onConnectionStatusChanged calls
            connection_id: The initial connection ID assigned to the connection
            conn: the connection object
        """

        try:
            self.race_sdk.onConnectionStatusChanged(
                handle,
                connection_id,
                CONNECTION_OPEN,
                self.link_properties[conn.link_id],
                RACE_BLOCKING,
            )

            latest = PluginCommsTwoSixPython._get_last_post_index(conn)
            while not conn.terminate:
                (posts, new_latest) = PluginCommsTwoSixPython._get_new_posts(conn, latest)
                expected_num = new_latest - latest
                actual_num = len(posts)
                if actual_num < expected_num:
                    lost_num = expected_num - actual_num
                    logError(
                        f"Expected {expected_num} posts, but only got {actual_num}. {lost_num} posts may have been lost."
                    )

                latest = new_latest

                for post in posts:
                    enc_pkg = EncPkg(b64decode(post))
                    response = self.race_sdk.receiveEncPkg(
                        enc_pkg, conn.connection_ids, RACE_BLOCKING
                    )
                    if response.status != SDK_OK:
                        logError(f"SDK failed with status {response.status}")

                # sleep's argument is fractional seconds, conn.frequency is milliseconds
                time.sleep(conn.frequency / 1000.0)

        except Exception as err:
            with self.connections_lock:
                logError(f"Multicast err: {err}")
                # Remove this connection from the connections map
                for conn_id in conn.connection_ids:
                    self.connections.pop(conn_id)

        # If shutting down due to an error, there will still be connection IDs
        # associated with this connection--let the SDK know the connection has
        # closed for each of them
        for conn_id in conn.connection_ids:
            self.race_sdk.onConnectionStatusChanged(
                NULL_RACE_HANDLE,
                conn_id,
                CONNECTION_CLOSED,
                self.link_properties[conn.link_id],
                RACE_BLOCKING,
            )

    def _get_next_available_port(self) -> int:
        MAX_PORT = 65535
        if self._next_available_port == MAX_PORT:
            logError("no more ports available for dynamically created links")
            raise Exception("no more ports available for dynamically created links")
        ret_val = self._next_available_port
        self._next_available_port = self._next_available_port + 1
        return ret_val
