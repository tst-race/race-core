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

public class LinkPropertySet {
    public int bandwidthBitsPS = -1;
    public int latencyMs = -1;
    public float loss = -1.0f;

    public LinkPropertySet() {}

    public LinkPropertySet(int bandwidthBitsPS, int latencyMs, float loss) {
        this.bandwidthBitsPS = bandwidthBitsPS;
        this.latencyMs = latencyMs;
        this.loss = loss;
    }

    /** @return String */
    @Override
    public String toString() {
        String string = "";
        string += "bandwidthBitsPS: " + bandwidthBitsPS + "\n";
        string += "latencyMs: " + latencyMs + "\n";
        string += "loss: " + loss + "\n";
        return string;
    }
}
