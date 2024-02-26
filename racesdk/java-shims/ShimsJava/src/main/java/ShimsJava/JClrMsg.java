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

// For API documentation please see the equivalent C++ header:
// racesdk/common/include/ClrMsg.h

package ShimsJava;

public class JClrMsg {
    public String plainMsg;
    public String fromPersona;
    public String toPersona;
    public long createTime;
    public int nonce;
    public byte ampIndex;
    public long traceId;
    public long spanId;

    public JClrMsg() {
        createTime = -1;
        nonce = -1;
        traceId = -1;
        spanId = -1;
    }

    public JClrMsg(
            String _plainMsg,
            String _fromPersona,
            String _toPersona,
            long _createTime,
            int _nonce,
            byte _ampIndex,
            long _traceId,
            long _spanId) {
        plainMsg = _plainMsg;
        fromPersona = _fromPersona;
        toPersona = _toPersona;
        createTime = _createTime;
        nonce = _nonce;
        ampIndex = _ampIndex;
        traceId = _traceId;
        spanId = _spanId;
    }

    public JClrMsg(
            String _plainMsg,
            String _fromPersona,
            String _toPersona,
            long _createTime,
            int _nonce) {
        plainMsg = _plainMsg;
        fromPersona = _fromPersona;
        toPersona = _toPersona;
        createTime = _createTime;
        nonce = _nonce;
        ampIndex = 0;
        traceId = -1;
        spanId = -1;
    }

    public String toString() {
        return "["
                + plainMsg
                + "], ["
                + fromPersona
                + "], ["
                + toPersona
                + "], ["
                + Long.toString(createTime)
                + "], ["
                + Integer.toString(nonce)
                + "], ["
                + Integer.toString(ampIndex)
                + "], ["
                + Long.toString(traceId)
                + "], ["
                + Long.toString(spanId)
                + "]";
    }
}
