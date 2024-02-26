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

import ShimsJava.JChannelProperties;
import ShimsJava.JLinkProperties;
import ShimsJava.JRaceSdkComms;
import ShimsJava.LinkPropertySet;
import ShimsJava.LinkType;

public class Channels {
    public static final String directChannelGid = "twoSixDirectJava";
    public static final String indirectChannelGid = "twoSixIndirectJava";

    public static JChannelProperties getDefaultChannelPropertiesForChannel(
            JRaceSdkComms jSdk, String channelGid) {
        return jSdk.getChannelProperties(channelGid);
    }

    public static JLinkProperties getDefaultLinkPropertiesForChannel(
            JRaceSdkComms jSdk, String channelGid) {
        JLinkProperties defaultLinkProperties = new JLinkProperties();

        JChannelProperties defaultChannelProperties =
                getDefaultChannelPropertiesForChannel(jSdk, channelGid);

        if (channelGid.equals(directChannelGid)) {
            defaultLinkProperties.transmissionType = defaultChannelProperties.transmissionType;
            defaultLinkProperties.connectionType = defaultChannelProperties.connectionType;
            defaultLinkProperties.sendType = defaultChannelProperties.sendType;
            defaultLinkProperties.reliable = defaultChannelProperties.reliable;
            defaultLinkProperties.isFlushable = defaultChannelProperties.isFlushable;
            defaultLinkProperties.duration = defaultChannelProperties.duration;
            defaultLinkProperties.period = defaultChannelProperties.period;
            defaultLinkProperties.mtu = defaultChannelProperties.mtu;

            LinkPropertySet worstLinkPropertySet = new LinkPropertySet();
            worstLinkPropertySet.bandwidthBitsPS = 23130000;
            worstLinkPropertySet.latencyMs = 17;
            worstLinkPropertySet.loss = -1.0f;
            defaultLinkProperties.worst.send = worstLinkPropertySet;
            defaultLinkProperties.worst.receive = worstLinkPropertySet;

            defaultLinkProperties.expected = defaultChannelProperties.creatorExpected;

            LinkPropertySet bestLinkPropertySet = new LinkPropertySet();
            bestLinkPropertySet.bandwidthBitsPS = 28270000;
            bestLinkPropertySet.latencyMs = 14;
            bestLinkPropertySet.loss = -1.0f;
            defaultLinkProperties.best.send = bestLinkPropertySet;
            defaultLinkProperties.best.receive = bestLinkPropertySet;

            defaultLinkProperties.supportedHints = defaultChannelProperties.supportedHints;
            defaultLinkProperties.channelGid = channelGid;

            return defaultLinkProperties;
        } else if (channelGid.equals(indirectChannelGid)) {

            defaultLinkProperties.linkType = LinkType.LT_BIDI;
            defaultLinkProperties.transmissionType = defaultChannelProperties.transmissionType;
            defaultLinkProperties.connectionType = defaultChannelProperties.connectionType;
            defaultLinkProperties.sendType = defaultChannelProperties.sendType;
            defaultLinkProperties.reliable = defaultChannelProperties.reliable;
            defaultLinkProperties.isFlushable = defaultChannelProperties.isFlushable;
            defaultLinkProperties.duration = defaultChannelProperties.duration;
            defaultLinkProperties.period = defaultChannelProperties.period;
            defaultLinkProperties.mtu = defaultChannelProperties.mtu;

            LinkPropertySet worstLinkPropertySet = new LinkPropertySet();
            worstLinkPropertySet.bandwidthBitsPS = 277200;
            worstLinkPropertySet.latencyMs = 3190;
            worstLinkPropertySet.loss = 0.1f;
            defaultLinkProperties.worst.send = worstLinkPropertySet;
            defaultLinkProperties.worst.receive = worstLinkPropertySet;

            defaultLinkProperties.expected = defaultChannelProperties.creatorExpected;

            LinkPropertySet bestLinkPropertySet = new LinkPropertySet();
            bestLinkPropertySet.bandwidthBitsPS = 338800;
            bestLinkPropertySet.latencyMs = 2610;
            bestLinkPropertySet.loss = 0.1f;
            defaultLinkProperties.best.send = bestLinkPropertySet;
            defaultLinkProperties.best.receive = bestLinkPropertySet;

            defaultLinkProperties.supportedHints = defaultChannelProperties.supportedHints;
            defaultLinkProperties.channelGid = channelGid;

            return defaultLinkProperties;
        }

        throw new IllegalArgumentException("invalid channel GID: " + channelGid);
    }
}
