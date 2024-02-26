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

public class JLinkProperties {

    public LinkType linkType;
    public TransmissionType transmissionType;
    public ConnectionType connectionType;
    public SendType sendType;
    public LinkPropertyPair worst = new LinkPropertyPair();
    public LinkPropertyPair expected = new LinkPropertyPair();
    public LinkPropertyPair best = new LinkPropertyPair();
    public boolean reliable = false;
    public boolean isFlushable = false;
    public int duration = 0;
    public int period = 0;
    public int mtu = 0;
    public ArrayList<String> supportedHints = new ArrayList<>();
    public String linkAddress = new String();
    public String channelGid = new String();

    public JLinkProperties() {
        connectionType = ConnectionType.CT_UNDEF;
        linkType = LinkType.LT_UNDEF;
        transmissionType = TransmissionType.TT_UNDEF;
        sendType = SendType.ST_UNDEF;
    }

    public JLinkProperties(
            LinkPropertyPair worst,
            LinkPropertyPair expected,
            LinkPropertyPair best,
            boolean reliable,
            String channelGid,
            String linkAddress,
            String[] hints) {
        linkType = LinkType.LT_UNDEF;
        transmissionType = TransmissionType.TT_UNDEF;
        connectionType = ConnectionType.CT_UNDEF;
        sendType = SendType.ST_UNDEF;

        this.worst = worst;
        this.expected = expected;
        this.best = best;
        this.reliable = reliable;
        this.channelGid = channelGid;
        this.linkAddress = linkAddress;
        Collections.addAll(this.supportedHints, hints);
    }

    public JLinkProperties(
            LinkPropertyPair worst,
            LinkPropertyPair expected,
            LinkPropertyPair best,
            boolean reliable,
            boolean isFlushable,
            String channelGid,
            String linkAddress,
            String[] hints,
            LinkType linkType,
            TransmissionType transmissionType,
            ConnectionType connectionType,
            SendType sendType,
            int duration,
            int period,
            int mtu) {
        this.worst = worst;
        this.expected = expected;
        this.best = best;
        this.reliable = reliable;
        this.isFlushable = isFlushable;
        this.channelGid = channelGid;
        this.linkAddress = linkAddress;
        Collections.addAll(this.supportedHints, hints);
        this.linkType = linkType;
        this.transmissionType = transmissionType;
        this.connectionType = connectionType;
        this.sendType = sendType;
        this.duration = duration;
        this.period = period;
        this.mtu = mtu;
    }

    /**
     * @param type
     * @return LinkType
     */
    public static LinkType linkTypeStringToEnum(String type) {
        if (type.compareTo("send") == 0) {
            return LinkType.LT_SEND;
        } else if (type.compareTo("receive") == 0) {
            return LinkType.LT_RECV;
        } else if (type.compareTo("bidirectional") == 0) {
            return LinkType.LT_BIDI;
        }
        return LinkType.LT_UNDEF;
    }

    /** @return String */
    @Override
    public String toString() {
        String prop = "";
        prop += "Link Type: " + linkType.name() + "\n";
        prop += "Transmission Type: " + transmissionType.name() + "\n";
        prop += "Connection Type: " + connectionType.name() + "\n";
        prop += "Send Type: " + sendType.name() + "\n";
        return prop;
    }

    /** @return int */
    public int getLinkTypeAsInt() {
        return linkType.ordinal();
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
    public LinkPropertyPair getBest() {
        return best;
    }

    /** @return LinkPropertyPair */
    public LinkPropertyPair getWorst() {
        return worst;
    }

    /** @return LinkPropertyPair */
    public LinkPropertyPair getExpected() {
        return expected;
    }

    /** @return Object[] */
    public Object[] getSupportedHints() {
        return supportedHints.toArray();
    }

    /** @return String */
    public String getChannelGid() {
        return channelGid;
    }

    /** @return String */
    public String getLinkAddress() {
        return linkAddress;
    }
}
