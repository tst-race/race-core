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
        PluginNMTwoSixServerPy Class. Is a python implementation of the RACE network manager. Will
        perform oblivious routing for the RACE system.
"""

# Python Library Imports
from ordered_set import OrderedSet
from typing import List
import traceback

# RACE Libraries
from .PluginHelpers import read_json_file_into_memory
from .ExtClrMsg import ExtClrMsg
from .Persona import PersonaType
from .PluginNMTwoSix import PluginNMTwoSix
from .Log import logDebug, logError, logWarning, logInfo

# Swig Libraries
from networkManagerPluginBindings import ClrMsg, EncPkg, LinkProperties, PluginConfig, IRaceSdkNM
from networkManagerPluginBindings import PLUGIN_ERROR, PLUGIN_FATAL, PLUGIN_OK  # PluginResponse
from networkManagerPluginBindings import CT_INDIRECT, CT_UNDEF  # ConnectionTypes
from networkManagerPluginBindings import PLUGIN_NOT_READY  # PluginStatus
from networkManagerPluginBindings import CHANNEL_ENABLED, CHANNEL_FAILED  # ChannelStatus
from networkManagerPluginBindings import RACE_BLOCKING

LinkType = int  # mypy type alias
PluginResponse = int  # mypy type alias


class PluginNMTwoSixServerPy(PluginNMTwoSix):
    """
    PluginNMTwoSixServerPy Class
    """

    FULL_FLOODING = 0

    def __init__(self, sdk=None):
        """
        Purpose:
            Constructor for the PluginNMTwoSixServerPy Class.
        Args:
            sdk (RaceSdk): RACE SDK
        Returns:
            N/A
        """

        super().__init__(sdk, PersonaType.P_SERVER)

        # Connected RACE Nodes
        self.exitClients = []
        self.committeeClients = []
        self.reachableCommittees = {}

        self.staleUuids = OrderedSet()
        self.floodedUuids = OrderedSet()

        self.rings = []

    #####
    # API Methods
    #####

    def init(self, plugin_config: PluginConfig) -> PluginResponse:
        """
        Purpose:
            Initilize the PluginNMTwoSixServerPy Class.
        Args:
            plugin_config (PluginConfig): Config object containing dynamic config variables (e.g. paths)
        Returns:
            PluginResponse: The status of the plugin in response to this call
        """
        try:
            logInfo("init called")
            logInfo(f"etcDirectory: {plugin_config.etcDirectory}")
            logInfo(f"loggingDirectory: {plugin_config.loggingDirectory}")
            logInfo(f"auxDataDirectory: {plugin_config.auxDataDirectory}")
            logInfo(f"tmpDirectory: {plugin_config.tmpDirectory}")
            logInfo(f"pluginDirectory: {plugin_config.pluginDirectory}")

            # Load personas and set persona for node
            self.loadPersonas(self._race_sdk)

            # Configure Committees
            self.loadCommittee(self._race_sdk)

            # load LinkProfiles
            self.loadStaticLinks(self._race_sdk)

            logDebug(
                f"Finished initializing, _uuid_to_persona_map len = {len(self._uuid_to_persona_map)}"
            )
            self._race_sdk.writeFile(
                "initialized.txt",
                "NM Python Server Plugin Initialized\n".encode("utf-8"),
            )
            tmp = bytes(self._race_sdk.readFile("initialized.txt")).decode("utf-8")
            logDebug(f"Read Initialization File: {tmp}")

            logDebug("Activate channels")
            channels = self._race_sdk.getAllChannelProperties()
            for channel in channels:
                role = self._channel_roles.get(channel.channelGid)
                if channel.channelStatus == CHANNEL_ENABLED and len(channel.roles) > 0:
                    self.channels_to_use.add(channel.channelGid)
                    logDebug(f"Activating channel {channel.channelGid}")
                    self._race_sdk.activateChannel(
                        channel.channelGid, role, RACE_BLOCKING
                    )
                elif len(channel.roles) == 0:
                    logWarning(f"No roles available for channel: {channel.channelGid}")

            logInfo("init returned")
            return PLUGIN_OK
        except Exception as e:
            logError("".join(traceback.TracebackException.from_exception(e).format()))
            return PLUGIN_FATAL

    def shutdown(self) -> PluginResponse:
        """
        Purpose:
            Shutdown the plugin. Will close all open receiving connections.
        Args:
            N/A
        Returns:
            PluginResponse: The status of the plugin in response to this call
        """
        try:
            logInfo("shutdown called")

            self.closeRecvConns()
            return PLUGIN_OK
        except Exception as e:
            logError("".join(traceback.TracebackException.from_exception(e).format()))
            return PLUGIN_FATAL

    def compareLinkProperties(
        self, props1: LinkProperties, props2: LinkProperties, persona_type: PersonaType
    ) -> int:
        """
        Purpose:
            Compare two sets of LinkProperties, in the context of using them to send to a particular PersonaType
        Args:
            pair1 (LinkProperties): LinkProperties of first link
            pair2 (LinkProperties): LinkProperties of second link
            persona_type (PersonaType): PersonaType to use the link to send to
        Returns:
            Int: -1 if pair1 is superior, 1 if pair2 is superior
        """
        if props2.connectionType == CT_UNDEF:
            return -1
        if props1.connectionType == CT_UNDEF:
            return 1
        if persona_type == PersonaType.P_CLIENT:
            if (
                props1.connectionType == CT_INDIRECT
                and props2.connectionType != CT_INDIRECT
            ):
                return -1
            if (
                props2.connectionType == CT_INDIRECT
                and props1.connectionType != CT_INDIRECT
            ):
                return 1

        return props2.expected.send.bandwidth_bps - props1.expected.send.bandwidth_bps

    def processClrMsg(self, handle: int, msg: ClrMsg) -> PluginResponse:
        """
        Purpose:
            processClrMsg will process a cleartext message. On Servers, this should
            fail, as they are not getting clear text messages ATM
        Args:
            handle (int): The RaceHandle for this call
            msg (ClrMsg Obj): Message object to process
        Returns:
            PluginResponse: The status of the plugin in response to this call
        """
        try:
            logError("processClrMsg not callable for servers")
            return PLUGIN_ERROR
        except Exception as e:
            logError("".join(traceback.TracebackException.from_exception(e).format()))
            return PLUGIN_FATAL

    def processEncPkg(
        self, handle: int, ePkg: EncPkg, conn_ids: List[str]
    ) -> PluginResponse:
        """
        Purpose:
            Process an encrypted package received by the server
        Args:
            handle (int): The RaceHandle for this call
            ePkg (EncPkg Obj): The received package to process
            conn_ids (List of Strings): The ConnIds that the EncPkg was received on
        Returns:
            PluginResponse: The status of the plugin in response to this call
        """
        try:
            logInfo("processEncPkg called")
            logDebug(f"    ciphertext len = {len(ePkg.getCipherText())}")

            try:
                decryptedPkg = self.encryptor.decryptEncPkg(
                    bytes(ePkg.getCipherText()), self._race_persona.getAesKey()
                )
            except ValueError:
                logDebug("processEncPkg: unable to decrypt (Not for Me)")
                logInfo("processEncPkg returned")
                return PLUGIN_OK
            except Exception as err:
                logError(
                    f"processEncPkg: failed to decrypt message. error: {type(err)}: {err}"
                )
                logInfo("processEncPkg returned")
                return PLUGIN_OK

            msg = self.encryptor.parseDelimitedExtMessage(decryptedPkg)
            if msg is not None:
                msg.setTraceId(ePkg.getTraceId())
                msg.setSpanId(ePkg.getSpanId())

                if msg.getTo() == self._race_sdk.getActivePersona():
                    if msg.msgType == ExtClrMsg.MSG_LINKS:
                        persona = self._uuid_to_persona_map.get(msg.getFrom(), None)
                        if not persona:
                            logError(f"{msg.getFrom()} is not a valid persona")
                            logInfo("processEncPkg returned")
                            return PLUGIN_OK
                        self._link_wizard.process_link_msg(persona, msg.getMsg())
                        logInfo("processEncPkg returned")
                        return PLUGIN_OK

                self.routeMsg(msg)
            else:
                logError(f"ClrMsg failed to parse")
                return PLUGIN_OK

            logInfo("processEncPkg returned")
            return PLUGIN_OK
        except Exception as e:
            logError("".join(traceback.TracebackException.from_exception(e).format()))
            return PLUGIN_FATAL

    def onPackageStatusChanged(self, handle: int, status: int) -> PluginResponse:
        """
        Purpose:
            Notify network manager about a change in package status. The handle will correspond to a
            handle returned inside an SdkResponse to a previous sendEncryptedPackage call.
        Args:
            handle (int): The RaceHandle of the previous sendEncryptedPackage call
            status (int): The PackageStatus of the package updated
        Returns:
            PluginResponse: The status of the plugin in response to this call
        """
        try:
            logInfo("onPackageStatusChanged: called")
            logDebug(f"onPackageStatusChanged: handle={handle} status={status}")
            logInfo("onPackageStatusChanged: returned")
            return PLUGIN_OK
        except Exception as e:
            logError("".join(traceback.TracebackException.from_exception(e).format()))
            return PLUGIN_FATAL

    #####
    # Other Methods
    #####

    def loadCommittee(self, sdk: IRaceSdkNM) -> None:
        """
        Purpose:
            Setup committee-based data from config file
        Args:
            N/A
        Returns:
            N/A
        """

        committeeConfig = read_json_file_into_memory(sdk, "config.json")
        self.committeeName = committeeConfig.get("committeeName", "")
        if self.committeeName == "":
            logWarning(
                "Committee name is empty-string, this is likely a configuration error but should not be fatal"
            )

        self.exitClients = set(committeeConfig.get("exitClients", []))
        self.committeeClients = set(committeeConfig.get("committeeClients", []))
        self.reachableCommittees = committeeConfig.get("reachableCommittees", {})

        # if a committee is reachable it should have an actual reachable member
        if not all([len(members) > 0 for members in self.reachableCommittees.values()]):
            logError("Not all reachableCommittees had an actual reachable member")

        self.rings = committeeConfig.get("rings", [])
        self.floodingFactor = committeeConfig.get("floodingFactor", 2)
        self.maxStaleUuids = committeeConfig.get("maxStaleUuids", 1000000)
        self._expected_links = committeeConfig.get("expectedLinks", {})
        self._channel_roles = committeeConfig.get("channelRoles", {})

        logDebug(f"    exitClients: {self.exitClients}")
        logDebug(f"    committeeClients: {self.committeeClients}")
        logDebug(f"    reachableCommittees: {self.reachableCommittees}")
        logDebug(f"    floodingFactor: {self.floodingFactor}")
        logDebug(f"    rings: {self.rings}")

    def addStaleUuid(self, uuid: int) -> None:
        """
        Purpose:
            Add the uuid to the set of already-seen UUIDs,
               truncate that set if it is over the size limit
        Args:
            uuid (Int): the uuid to add
        Returns:
            N/A
        """

        logDebug(f"  addStaleUuid: called with {uuid}")
        if len(self.staleUuids) > self.maxStaleUuids:
            logDebug(f"    trimming staleUuids from {len(self.staleUuids)}")
            trimAmount = max(
                1, int(self.maxStaleUuids / 10)
            )  # trim by 10% or at least 1
            self.staleUuids = self.staleUuids[trimAmount:]
            logDebug(f"    trimmed to {len(self.staleUuids)}")

        if uuid != ExtClrMsg.UNSET_UUID:
            self.staleUuids.add(uuid)

        logDebug("  addStaleUuid: returned")

    def addFloodedUuid(self, uuid: int) -> None:
        """
        Purpose:
            Add the uuid to the set of already-flooded UUIDs,
               truncate that set if it is over the size limit
        Args:
            uuid (Int): the uuid to add
        Returns:
            N/A
        """

        logDebug(f"  addFloodedUuid: called with {uuid}")
        if len(self.floodedUuids) > self.maxStaleUuids:
            logDebug(f"    trimming floodedUuids from {len(self.floodedUuids)}")
            trimAmount = max(
                1, int(self.maxStaleUuids / 10)
            )  # trim by 10% or at least 1
            self.floodedUuids = self.floodedUuids[trimAmount:]
            logDebug(f"    trimmed to {len(self.floodedUuids)}")

        if uuid != ExtClrMsg.UNSET_UUID:
            self.floodedUuids.add(uuid)

        logDebug("  addFloodedUuid: returned")

    #####
    # Routing Methods
    #####
    def routeMsg(self, msg: ExtClrMsg) -> None:
        """
        Purpose:
            Checks if the msg has already been seen by this node; if not,
                calls sendToRings, else drops
        Args:
            msg (ExtClrMsg): the ExtClrMsg to process
        Returns:
            N/A
        """

        logDebug("routeMsg: called")
        # does msg have a Ring-TTL?
        if (
            not msg.isRingTtlSet() and len(self.rings) > 0
        ):  # escape for size-1 committees
            self.startRingMsg(msg)
        else:
            self.handleRingMsg(msg)
        logDebug("routeMsg: returned")

    def startRingMsg(self, msg: ExtClrMsg) -> None:
        """
        Purpose:
             Sends the msg out on each ring this node knows about, setting the ringTtl
                and ringIdx variables appropriately for each.
        Args:
            msg (ExtClrMsg): the ExtClrMsg to process
        Returns:
            N/A
        """

        logDebug("  startRingMsg: called")
        if msg.uuid in self.staleUuids:
            # already saw this msg previously
            logDebug(f"Received additional copy of msg with uuid={msg.uuid}")
            return

        self.addStaleUuid(msg.uuid)

        self.sendToRings(msg)
        logDebug("  startRingMsg: returned")

    def sendToRings(self, msg: ExtClrMsg) -> None:
        """
        Purpose:
            Checks if the msg has already been seen by this node; if not, calls
                sendToRings, else drops
        Args:
            msg (ExtClrMsg): the ExtClrMsg to process
        Returns:
            N/A
        """

        logDebug("    sendToRings: called")
        if msg.isRingTtlSet():
            logError(
                "Attempted to append a second Ring-TTL message, bad logic, ignoring"
            )
            return

        # TODO fix opentracing; is a new parent/child span the right thing here?
        for idx, ring in enumerate(self.rings):
            logDebug(
                f"      sending along ring of length {ring['length']} to {ring['next']}"
            )
            ringMsg = msg.copy()
            ringMsg.ringTtl = ring["length"] - 1
            # so that it ttl=0 when it reaches me
            ringMsg.ringIdx = idx
            self.sendMsg(ring["next"], ringMsg)

        logDebug("    sendToRings: returned")

    def handleRingMsg(self, msg: ExtClrMsg) -> None:
        """
        Purpose:
            Handles a received ring msg to either forward it along the ring
                (if ringTtl > 0), forward to other committees, or forward to a client.
        Args:
            msg (ExtClrMsg): the ExtClrMsg to process
        Returns:
            N/A
        """

        logDebug(f"  handleRingMsg: called on uuid={msg.uuid} Ring-TTL={msg.ringTtl}")
        self.addStaleUuid(msg.uuid)
        # NOTE: we do not abort on a repeated uuid in a ring message to allow
        # multiple/redundant ring paths but we DO want to filter out the same msg arriving
        # from outside the committee so the uuid is added to stale
        if msg.ringTtl > 0:
            # continue the trip around the ring
            msg.decRingTtl()
            # get the nextNode entry for this right ring
            self.sendMsg(self.rings[msg.ringIdx]["next"], msg)

        elif msg.uuid not in self.floodedUuids:
            self.addFloodedUuid(msg.uuid)
            dstClient = msg.getTo()
            if dstClient in self.exitClients:
                logDebug(f"    client is in exitClients, forwarding to: {dstClient}")
                self.sendMsg(dstClient, msg.asClrMsg())

            if dstClient in self.committeeClients and len(self.rings) > 0:
                # someone in our committee can reach it, send it around the rings
                logDebug(
                    f"    client is in committeeClients, forwarding around the rings"
                )
                dstUuid = self.rings[msg.ringIdx].get("next", None)
                if dstUuid is not None:
                    self.sendMsg(self.rings[msg.ringIdx]["next"], msg)
                else:
                    logWarning(
                        f"Received a ring message for an empty ring index (idx={msg.ringIdx}), dropping this message (uuid={msg.uuid})"
                    )
            else:
                self.forwardToNewCommittees(msg)
        else:
            logInfo(
                "    received end-of-ring msg we have already dealt with, ignoring."
            )

        logDebug("  handleRingMsg: returned")

    def forwardToNewCommittees(self, msg: ExtClrMsg) -> None:
        """
        Purpose:
            Resets ringTtl and ringIdx, appends this committee to committeesVisited, and
                sends to some committees this node knows about. If this node knows about
                fewer than floodingFactor committees, it _also_ forwards the msg along
                its rings with ringTtl=0 to prompt them to forward.
        Args:
            msg (ExtClrMsg): the ExtClrMsg to process
        Returns:
            N/A
        """

        logDebug("      forwardToNewCommittees: called")
        intercomMsg = msg.copy()
        intercomMsg.unsetRingTtl()
        if self.committeeName not in intercomMsg.committeesVisited:
            intercomMsg.committeesVisited.append(self.committeeName)

        intercomMsg.committeesSent = []
        intercom_dsts = set()
        visited = intercomMsg.committeesVisited
        sent = intercomMsg.committeesSent
        for committee, reachableMembers in self.reachableCommittees.items():
            if committee in visited or committee in sent:
                continue

            intercom_dsts.add(
                reachableMembers[0]
            )  # TODO handle multiple reachable members
            msg.committeesSent.append(committee)

            # A floodingFactor <= PluginNMTwosixServerPy.FULL_FLOODING (aka 0) means "maximum flooding"
            if (
                self.floodingFactor > PluginNMTwoSixServerPy.FULL_FLOODING
                and len(intercom_dsts) >= self.floodingFactor
            ):
                break

        logDebug(f"        forwarding to {intercom_dsts}")
        for dst in intercom_dsts:
            self.sendMsg(dst, intercomMsg)

        # A floodingFactor <= PluginNMTwosixServerPy.FULL_FLOODING (aka 0) means "maximum flooding"
        if (
            self.floodingFactor <= PluginNMTwoSixServerPy.FULL_FLOODING
            or len(intercom_dsts) < self.floodingFactor
        ):
            # if _this_ node cannot reach enough other committees, forward on the ring
            # NOTE: this will cause EACH node to send to reachable committee nodes,
            # potentially exceeding the floodingFactor over the course of the ring
            logDebug(
                f"        sent to {len(intercom_dsts)} other committees but "
                f"floodingFactor set to {self.floodingFactor}, forwarding on ring for"
                f"additional sends"
            )
            for ring in self.rings:
                self.sendMsg(ring["next"], msg)

        logDebug("      forwardToNewCommittees: returned")

    def sendMsg(self, dstUuid: str, msg: ClrMsg) -> None:
        """
        Purpose:
            Formats, encrypts, and sends a ClrMsg to the dstUuid
        Args:
            dstUuid (String): name of the destination persona
            msg (ClrMsg): the ClrMsg to format, encrypt, and send
        Returns:
            N/A
        """

        logDebug(f"        sendMsg: called to {dstUuid}")
        dstPersona = self._uuid_to_persona_map.get(dstUuid, None)
        if dstPersona is None:
            logError(
                f"Failed to find destination UUID {dstUuid} in self._uuid_to_persona_map"
            )
            return

        # For each dstPersona on this connection, create a unique package to send to it
        formattedMsg = self.encryptor.formatDelimitedMessage(msg)
        ePkg = EncPkg(
            msg.getTraceId(),
            msg.getSpanId(),
            self.encryptor.encryptClrMsg(
                formattedMsg,
                dstPersona.getAesKey(),
            ),
        )

        self.sendTo(dstPersona, ePkg)
        logDebug("        sendMsg: returned")
