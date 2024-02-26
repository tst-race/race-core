
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

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.*;

import ShimsJava.JRaceSdkComms;
import ShimsJava.PluginConfig;
import ShimsJava.PluginResponse;

import org.junit.Test;

public class PluginCommsTwoSixJavaTest {
    // Load the C++ so
    static {
        System.loadLibrary("RaceJavaShims");
    }

    @Test
    public void initShouldGetPersona() {
        JRaceSdkComms sdk = mock(JRaceSdkComms.class);
        when(sdk.getActivePersona()).thenReturn("race-server-1");

        PluginCommsTwoSixJava plugin = new PluginCommsTwoSixJava(sdk);

        PluginConfig pluginConfig = new PluginConfig();
        assertEquals(PluginResponse.PLUGIN_OK, plugin.init(pluginConfig));
    }
}
