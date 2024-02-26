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
        Module for maintaining RACEcomms channel properties.
"""

from commsPluginBindings import (
    IRaceSdkComms,
    LinkProperties,
    LinkPropertySet,
    ChannelProperties,
    LD_LOADER_TO_CREATOR,
    TT_UNICAST,
    CT_DIRECT,
    ST_EPHEM_SYNC,
    LD_BIDI,
    TT_MULTICAST,
    CT_INDIRECT,
    ST_STORED_ASYNC,
    LT_BIDI,
)

DIRECT_CHANNEL_GID = "twoSixDirectPython"
INDIRECT_CHANNEL_GID = "twoSixIndirectPython"

_INT_MAX = 2147483647


def get_default_channel_properties_for_channel(
    sdk: IRaceSdkComms, channel_gid: str
) -> ChannelProperties:
    return sdk.getChannelProperties(channel_gid)


def get_default_link_properties_for_channel(
    sdk: IRaceSdkComms, channel_gid: str
) -> LinkProperties:
    if channel_gid == DIRECT_CHANNEL_GID:
        props = LinkProperties()

        channel_props = get_default_channel_properties_for_channel(sdk, channel_gid)

        props.transmissionType = channel_props.transmissionType
        props.connectionType = channel_props.connectionType
        props.sendType = channel_props.sendType
        props.reliable = channel_props.reliable
        props.isFlushable = channel_props.isFlushable
        props.duration_s = channel_props.duration_s
        props.period_s = channel_props.period_s
        props.mtu = channel_props.mtu

        worst_link_prop_set = LinkPropertySet()
        worst_link_prop_set.bandwidth_bps = 23130000
        worst_link_prop_set.latency_ms = 17
        worst_link_prop_set.loss = -1.0
        props.worst.send = worst_link_prop_set
        props.worst.receive = worst_link_prop_set

        props.expected = channel_props.creatorExpected

        best_link_prop_set = LinkPropertySet()
        best_link_prop_set.bandwidth_bps = 28270000
        best_link_prop_set.latency_ms = 14
        best_link_prop_set.loss = -1.0
        props.best.send = best_link_prop_set
        props.best.receive = best_link_prop_set

        props.supported_hints = channel_props.supported_hints
        props.channelGid = channel_gid

        return props
    if channel_gid == INDIRECT_CHANNEL_GID:
        props = LinkProperties()

        channel_props = get_default_channel_properties_for_channel(sdk, channel_gid)

        props.linkType = LT_BIDI
        props.transmissionType = channel_props.transmissionType
        props.connectionType = channel_props.connectionType
        props.sendType = channel_props.sendType
        props.reliable = channel_props.reliable
        props.isFlushable = channel_props.isFlushable
        props.duration_s = channel_props.duration_s
        props.period_s = channel_props.period_s
        props.mtu = channel_props.mtu

        worst_link_prop_set = LinkPropertySet()
        worst_link_prop_set.bandwidth_bps = 277200
        worst_link_prop_set.latency_ms = 3190
        worst_link_prop_set.loss = 0.1
        props.worst.send = worst_link_prop_set
        props.worst.receive = worst_link_prop_set

        props.expected = channel_props.creatorExpected

        best_link_prop_set = LinkPropertySet()
        best_link_prop_set.bandwidth_bps = 338800
        best_link_prop_set.latency_ms = 2610
        best_link_prop_set.loss = 0.1
        props.best.send = best_link_prop_set
        props.best.receive = best_link_prop_set

        props.supported_hints = channel_props.supported_hints
        props.channelGid = channel_gid

        return props

    raise Exception(
        f"get_default_link_properties_for_channel: invalid channel GID: {channel_gid}"
    )
