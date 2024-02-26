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
        PluginNMTwoSixClientPy Class. Is a python implementation of the RACE network manager. Will
        perform oblivious routing for the RACE system.
"""

# Python Library Imports
from ordered_set import OrderedSet
from typing import Dict, List, Set, Tuple
import traceback
import yaml

# RACE Libraries
from .ExtClrMsg import ExtClrMsg
from .Persona import PersonaType
from .PluginHelpers import read_json_file_into_memory
from .PluginNMTwoSix import PluginNMTwoSix
from .Log import logDebug, logError, logInfo
from .ClearMessagePackageTracker import ClearMessagePackageTracker

# Swig Libraries
from networkManagerPluginBindings import ClrMsg, EncPkg, LinkProperties, PluginConfig, IRaceSdkNM
from networkManagerPluginBindings import PLUGIN_ERROR, PLUGIN_FATAL, PLUGIN_OK  # PluginResponse
from networkManagerPluginBindings import CT_INDIRECT, CT_UNDEF  # ConnectionTypes
from networkManagerPluginBindings import PLUGIN_NOT_READY  # PluginStatus
from networkManagerPluginBindings import CHANNEL_ENABLED, CHANNEL_FAILED  # ChannelStatus
from networkManagerPluginBindings import MS_UNDEF, MS_SENT, MS_FAILED  # MessageStatus
from networkManagerPluginBindings import NULL_RACE_HANDLE
from networkManagerPluginBindings import RACE_BLOCKING

# Jaeger
from jaeger_client import Config, SpanContext
from jaeger_client.constants import SAMPLED_FLAG

LinkType = int  # mypy type alias
PluginResponse = int  # mypy type alias


class PluginNMTwoSixClientPy(PluginNMTwoSix):
    """
    PluginNMTwoSixClientPy Class
    """

    ###
    # Class Lifecycle Methods
    ###

    def __init__(self, sdk=None):
        """
        Purpose:
            Constructor for the PluginNMTwoSixClientPy Class.
        Args:
            sdk (RaceSdk): RACE SDK
        Returns:
            N/A
        """

        super().__init__(sdk, PersonaType.P_CLIENT)

        # Connected RACE Nodes
        self.entranceCommittee = []
        self.exitCommittee = []

        # Jaeger
        self.jaegerConfig = None
        self.tracer = None

        # Set of received messages
        self.seenMessages = OrderedSet()
        self.maxSeenMessages = 0

        self.message_status_tracker = ClearMessagePackageTracker()

    def __del__(self):
        """
        Purpose:
            Constructor for the PluginNMTwoSixClientPy Class.
        Args:
            N/A
        Returns:
            N/A
        """

        pass

    #####
    ## API Methods
    #####

    def init(self, plugin_config: PluginConfig) -> PluginResponse:
        """
        Purpose:
            Initilize the PluginNMTwoSixClientPy Class.
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
            self.configureCommittees(self._race_sdk)

            # load LinkProfiles
            self.loadStaticLinks(self._race_sdk)

            # Jaeger
            jaegerConfigPath = plugin_config.etcDirectory + "/jaeger-config.yml"
            try:
                with open(jaegerConfigPath, "r") as file:
                    self.config = Config(
                        yaml.safe_load(file), service_name="PluginNMTwoSixClientPy"
                    )
                logDebug(f"Read jaeger config from {jaegerConfigPath}")
            except OSError as e:
                logError("Failed to open Jaeger Config: " + str(e))
                self.config = Config(
                    config={"disabled": True}, service_name="PluginNMTwoSixClientPy"
                )

            self.tracer = self.config.initialize_tracer()

            logDebug(
                f"Finished initializing, _uuid_to_persona_map len = {len(self._uuid_to_persona_map)}, "
                f"entranceCommittee len = {len(self.entranceCommittee)}, "
                f"exitCommittee len = {len(self.exitCommittee)}"
            )
            self._race_sdk.writeFile(
                "initialized.txt",
                "NM Python Client Plugin Initialized\n".encode("utf-8"),
            )
            tmp = bytes(self._race_sdk.readFile("initialized.txt")).decode("utf-8")
            logDebug(f"Read Initialization File: {tmp}")

            logDebug("Activate channels")
            channels = self._race_sdk.getAllChannelProperties()
            for channel in channels:
                role = self._channel_roles.get(channel.channelGid)
                if channel.channelStatus == CHANNEL_ENABLED and role:
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
            processClrMsg will process a cleartext message and send the encrypted
            package along to the entrance committee.
        Args:
            handle (int): The RaceHandle for this call
            msg (ClrMsg Obj): Message object to process
        Returns:
            PluginResponse: The status of the plugin in response to this call
        """
        try:
            logInfo("processClrMsg called")
            logDebug(
                f"    message len: {len(msg.getMsg())}, "
                f"from: {msg.getFrom()}, to: {msg.getTo()}"
            )

            ctx = SpanContext(msg.getTraceId(), msg.getSpanId(), None, SAMPLED_FLAG)
            # references may be used to contain additional parent span contexts
            # e.g. references = [opentracing.child_of(ctx2), ...]
            with self.tracer.start_span(
                "processClrMsg", child_of=ctx, references=[]
            ) as span:
                if msg.getTo() not in self._uuid_to_persona_map:
                    logError(f"{msg.getTo()} not a valid persona to send message to")
                    return PLUGIN_ERROR
                elif msg.getTo() == self._race_persona.getRaceUuid():
                    logInfo(f"I am {msg.getTo()}, nothing to send")
                    self._race_sdk.presentCleartextMessage(msg)
                    return PLUGIN_OK

                # Set the baseEncodedMsg message (encoding message, to, from)
                baseEncodedMsg = self.encryptor.formatDelimitedMessage(msg)
                msgHash = self.encryptor.getMessageHash(msg)
                if msgHash in self.seenMessages:
                    logError("new ClrMsg is identical to previously sent message")
                    raise Exception("New ClrMsg identical to previous ClrMsg")
                else:
                    self.addSeenMessage(msgHash)

                anySent = False
                for persona in self.entranceCommittee:
                    # For each persona on this connection, create a unique
                    # package to send to it
                    ePkg = EncPkg(
                        span.context.trace_id,
                        span.context.span_id,
                        self.encryptor.encryptClrMsg(
                            baseEncodedMsg, persona.getAesKey()
                        ),
                    )
                    enc_pkg_handle = self.sendTo(persona, ePkg)
                    if enc_pkg_handle != NULL_RACE_HANDLE:
                        anySent = True
                        self.message_status_tracker.add_enc_pkg_handle_for_clr_msg(
                            enc_pkg_handle, handle
                        )

            logInfo(f"Done sending: anySent={anySent}")
            if not anySent:
                logError("No valid links to any entrance committee members found")
                raise Exception(
                    "No valid links to any entrance committee members found"
                )

            logInfo("processClrMsg returned")
            return PLUGIN_OK
        except Exception as e:
            logError("".join(traceback.TracebackException.from_exception(e).format()))
            return PLUGIN_FATAL

    def processEncPkg(
        self, handle: int, ePkg: EncPkg, conn_ids: List[str]
    ) -> PluginResponse:
        """
        Purpose:
            Process an encrypted package received by the client
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

            extClrMsg = self.encryptor.parseDelimitedExtMessage(decryptedPkg)
            if not extClrMsg:
                logDebug(f"ExtClrMsg not found (Not for Me)")
                logInfo("processEncPkg returned")
                return PLUGIN_OK

            extClrMsg.setTraceId(ePkg.getTraceId())
            extClrMsg.setSpanId(ePkg.getSpanId())

            if extClrMsg.msgType == ExtClrMsg.MSG_LINKS:
                persona = self._uuid_to_persona_map.get(extClrMsg.getFrom(), None)
                if not persona:
                    logError(f"{extClrMsg.getFrom()} is not a valid persona")
                    logInfo("processEncPkg returned")
                    return PLUGIN_OK
                self._link_wizard.process_link_msg(persona, extClrMsg.getMsg())

            elif extClrMsg.msgType == ExtClrMsg.MSG_CLIENT:
                msgHash = self.encryptor.getMessageHash(extClrMsg)
                if msgHash in self.seenMessages:
                    logInfo("Package duplicate to one already seen. Ignoring")
                    return PLUGIN_OK
                else:
                    self.addSeenMessage(msgHash)

                    self._race_sdk.presentCleartextMessage(extClrMsg)

            else:
                logError("Message has undefined message type")

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
            (
                clr_msg_handle,
                message_status,
            ) = self.message_status_tracker.update_package_status_for_enc_pkg_handle(
                status, handle
            )
            if message_status != MS_UNDEF:
                self._race_sdk.onMessageStatusChanged(clr_msg_handle, message_status)

            logInfo("onPackageStatusChanged: returned")
            return PLUGIN_OK
        except Exception as e:
            logError("".join(traceback.TracebackException.from_exception(e).format()))
            return PLUGIN_FATAL

    def configureCommittees(self, sdk: IRaceSdkNM) -> None:
        """
        Purpose:
            Map personas to their committees based on the configs
            passed in.
        Args:
            N/A
        Returns:
            N/A
        """

        committeeConfig = read_json_file_into_memory(sdk, "config.json")
        committeeKeys = ["entranceCommittee", "exitCommittee"]
        for committeeKey in committeeKeys:
            for personaUuid in committeeConfig[committeeKey]:
                persona = self._uuid_to_persona_map.get(personaUuid, None)
                if not persona:
                    logError(
                        f"{personaUuid} is not a valid persona to add to {committeeKey}"
                    )
                    return
                else:
                    logDebug(f"Adding {personaUuid} to {committeeKey}")

                if committeeKey == "entranceCommittee":
                    self.entranceCommittee.append(persona)
                elif committeeKey == "exitCommittee":
                    self.exitCommittee.append(persona)

        self.maxSeenMessages = committeeConfig.get("maxSeenMessages", 10000)
        self._expected_links = committeeConfig.get("expectedLinks", {})
        self._channel_roles = committeeConfig.get("channelRoles", {})

    def addSeenMessage(self, hash: bytes) -> None:
        """
        Purpose:
            Add the message hash to the set of already-seen messages,
            truncating the set if it is over the size limit
        Args:
            hash: the message hash to add
        Returns:
            N/A
        """
        logDebug("  addSeenMessage: called")
        if len(self.seenMessages) > self.maxSeenMessages:
            logDebug(f"    trimming seenMessages from {len(self.seenMessages)}")
            trimAmount = max(
                1, int(self.maxSeenMessages / 10)
            )  # trim by 10% or at least 1
            self.seenMessages = self.seenMessages[trimAmount:]
            logDebug(f"    trimmed seenMessages to {len(self.seenMessages)}")

        self.seenMessages.add(hash)
        logDebug("  addSeenMessage: returned")
