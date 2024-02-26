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
// racesdk/common/include/EncPkg.h

package ShimsJava;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;

public class JEncPkg {
    public long traceId;
    public long spanId;
    private byte packageTypeByte;
    public byte[] cipherText;

    public JEncPkg() {
        this.traceId = -1;
        this.spanId = -1;
        this.packageTypeByte = 0;
    }

    public JEncPkg(long traceId, long spanId, byte[] cipherText) {
        this.traceId = traceId;
        this.spanId = spanId;
        this.cipherText = cipherText;
        this.packageTypeByte = 0;
    }

    public JEncPkg(long traceId, long spanId, byte[] cipherText, byte packageTypeByte) {
        this.traceId = traceId;
        this.spanId = spanId;
        this.cipherText = cipherText;
        this.packageTypeByte = packageTypeByte;
    }

    /**
     * Construct an encrypted package from the raw data of another encrypted package.
     *
     * <p>Expected form of the incoming raw data should be an appended byte array of trace ID, span
     * ID, package type, and cipher text IN THAT ORDER. This constructor expects the raw data format
     * of the return value of EncPkg.getRawData(). So, for example, one could copy an encrypted
     * package by doing this: ``` JEncPkg newEncryptedPackage = new
     * JEncPkg(oldEncryptedPackage.getRawData()); ```
     *
     * @param rawData An appended byte array of trace ID, span ID, package type, and cipher text IN
     *     THAT ORDER.
     */
    public JEncPkg(byte[] rawData) {
        ByteBuffer buffer = ByteBuffer.wrap(rawData);
        buffer.order(ByteOrder.LITTLE_ENDIAN);
        this.traceId = buffer.getLong();
        this.spanId = buffer.getLong();
        this.packageTypeByte = buffer.get();
        this.cipherText = new byte[buffer.remaining()];
        buffer.get(this.cipherText, 0, this.cipherText.length);
    }

    /**
     * Get the encrypted package in the form of raw data.
     *
     * <p>The returned value will be the appended trace ID, span ID, package type, and cipher text
     * bytes.
     *
     * @return Appended trace ID, span ID, package type, and cipher text bytes.
     */
    public byte[] getRawData() {
        ByteBuffer buffer = ByteBuffer.allocate(getSize());
        buffer.order(ByteOrder.LITTLE_ENDIAN);
        buffer.putLong(traceId);
        buffer.putLong(spanId);
        buffer.put(packageTypeByte);
        buffer.put(cipherText);
        return buffer.array();
    }

    /**
     * Get the package type of the encrypted package.
     *
     * <p>The package type is set automatically by the SDK to differentiate between packages sent by
     * a network manager plugin and the test harness. This field is irrelevant to a network manager
     * plugin, and comms channels do not need to handle it so long as they are using the raw data of
     * the package.
     *
     * @return Package type
     */
    public PackageType getPackageType() {
        return PackageType.valueOf((int) packageTypeByte);
    }

    /**
     * Set the package type for the encrypted package.
     *
     * @param pkgType Package type
     */
    public void setPackageType(PackageType pkgType) {
        packageTypeByte = Integer.valueOf(pkgType.getValue()).byteValue();
    }

    /**
     * Get the size of the encrypted package's raw data, in bytes.
     *
     * @return Raw data size in bytes
     */
    public int getSize() {
        return cipherText.length + Long.BYTES + Long.BYTES + Byte.BYTES;
    }

    @Override
    public String toString() {
        return "JEncPkg[traceId="
                + traceId
                + ",spanId="
                + spanId
                + ",cipherText="
                + Arrays.toString(cipherText)
                + "]";
    }
}
