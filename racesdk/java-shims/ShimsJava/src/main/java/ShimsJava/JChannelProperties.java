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

import java.util.ArrayList;
import java.util.Collections;

public class JChannelProperties {

    public ChannelStatus channelStatus;
    public LinkDirection linkDirection;
    public TransmissionType transmissionType;
    public ConnectionType connectionType;
    public SendType sendType;
    public boolean multiAddressable = false;
    public boolean reliable = false;
    public boolean bootstrap = false;
    public boolean isFlushable = false;
    public int duration = 0;
    public int period = 0;
    public int mtu = 0;
    public int maxLinks = -1;
    public int creatorsPerLoader = -1;
    public int loadersPerCreator = -1;
    public ChannelRole[] roles = new ChannelRole[0];
    public ChannelRole currentRole = new ChannelRole();
    public int maxSendsPerInterval = -1;
    public int secondsPerInterval = -1;
    public long intervalEndTime = 0;
    public int sendsRemainingInInterval = -1;
    public LinkPropertyPair creatorExpected = new LinkPropertyPair();
    public LinkPropertyPair loaderExpected = new LinkPropertyPair();
    public ArrayList<String> supportedHints = new ArrayList<>();
    public String channelGid = new String();

    public JChannelProperties() {
        channelStatus = ChannelStatus.CHANNEL_UNDEF;
        connectionType = ConnectionType.CT_UNDEF;
        sendType = SendType.ST_UNDEF;
        linkDirection = LinkDirection.LD_UNDEF;
        transmissionType = TransmissionType.TT_UNDEF;
    }

    // TODO: where is this constructor used?
    public JChannelProperties(
            LinkPropertyPair creatorExpected,
            LinkPropertyPair loaderExpected,
            boolean reliable,
            boolean multiAddressable,
            String[] hints,
            String channelGid) {
        channelStatus = ChannelStatus.CHANNEL_UNDEF;
        linkDirection = LinkDirection.LD_UNDEF;
        transmissionType = TransmissionType.TT_UNDEF;
        connectionType = ConnectionType.CT_UNDEF;
        sendType = SendType.ST_UNDEF;

        this.creatorExpected = creatorExpected;
        this.loaderExpected = loaderExpected;
        this.reliable = reliable;
        this.multiAddressable = multiAddressable;
        this.channelGid = channelGid;
        Collections.addAll(this.supportedHints, hints);
    }

    public JChannelProperties(
            ChannelStatus channelStatus,
            LinkPropertyPair creatorExpected,
            LinkPropertyPair loaderExpected,
            boolean reliable,
            boolean bootstrap,
            boolean isFlushable,
            boolean multiAddressable,
            String[] hints,
            String channelGid,
            LinkDirection linkDirection,
            TransmissionType transmissionType,
            ConnectionType connectionType,
            SendType sendType,
            int duration,
            int period,
            int mtu,
            int maxLinks,
            int creatorsPerLoader,
            int loadersPerCreator,
            ChannelRole[] roles,
            ChannelRole currentRole,
            int maxSendsPerInterval,
            int secondsPerInterval,
            long intervalEndTime,
            int sendsRemainingInInterval) {
        this.channelStatus = channelStatus;
        this.creatorExpected = creatorExpected;
        this.loaderExpected = loaderExpected;
        this.reliable = reliable;
        this.bootstrap = bootstrap;
        this.isFlushable = isFlushable;
        this.multiAddressable = multiAddressable;
        this.channelGid = channelGid;
        Collections.addAll(this.supportedHints, hints);
        this.linkDirection = linkDirection;
        this.transmissionType = transmissionType;
        this.connectionType = connectionType;
        this.sendType = sendType;
        this.duration = duration;
        this.period = period;
        this.mtu = mtu;
        this.maxLinks = maxLinks;
        this.creatorsPerLoader = creatorsPerLoader;
        this.loadersPerCreator = loadersPerCreator;
        this.roles = roles;
        this.currentRole = currentRole;
        this.maxSendsPerInterval = maxSendsPerInterval;
        this.secondsPerInterval = secondsPerInterval;
        this.intervalEndTime = intervalEndTime;
        this.sendsRemainingInInterval = sendsRemainingInInterval;
    }

    /**
     * @param type
     * @return LinkDirection
     */
    public static LinkDirection linkDirectionStringToEnum(String type) {
        if (type.compareTo("creator_to_loader") == 0) {
            return LinkDirection.LD_CREATOR_TO_LOADER;
        } else if (type.compareTo("loader_to_creator") == 0) {
            return LinkDirection.LD_LOADER_TO_CREATOR;
        } else if (type.compareTo("bidirectional") == 0) {
            return LinkDirection.LD_BIDI;
        }
        return LinkDirection.LD_UNDEF;
    }

    /** @return String */
    @Override
    public String toString() {
        String prop = "";
        prop += "Link Direction: " + linkDirection.name() + "\n";
        prop += "Transmission Type: " + transmissionType.name() + "\n";
        prop += "Connection Type: " + connectionType.name() + "\n";
        return prop;
    }

    /** @return int */
    public int getChannelStatusAsInt() {
        return channelStatus.ordinal();
    }

    /** @return int */
    public int getLinkDirectionAsInt() {
        return linkDirection.ordinal();
    }

    /** @return int */
    public int getTransmissionTypeAsInt() {
        return transmissionType.ordinal();
    }

    /** @return int */
    public int getConnectionTypeAsInt() {
        return connectionType.ordinal();
    }

    /** @return int */
    public int getSendTypeAsInt() {
        return sendType.ordinal();
    }

    /** @return LinkPropertyPair */
    public LinkPropertyPair getCreatorExpected() {
        return creatorExpected;
    }

    /** @return LinkPropertyPair */
    public LinkPropertyPair getLoaderExpected() {
        return loaderExpected;
    }

    /** @return Object[] */
    public Object[] getSupportedHints() {
        return supportedHints.toArray();
    }

    /** @return String */
    public String getChannelGid() {
        return channelGid;
    }
}
