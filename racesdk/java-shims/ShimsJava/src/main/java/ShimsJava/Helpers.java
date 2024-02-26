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

/**
 * Helper Functions that allow Java client apps (specifically Android apps) to use C++ code to
 * create inputs required for racetestapp
 */
public class Helpers {

    /**
     * Create RaceTestAppOutputLog and return pointer
     *
     * @param dir Directory to place RaceTestApp output logs
     * @return long RaceTestAppOutputLog ptr
     */
    public static native long createRaceTestAppOutputLog(String dir);

    /**
     * initialize tracer from file at specified path.
     *
     * @param jaegerConfigPath The path to load the file from
     * @param activePersona The persona of the calling app
     * @return long native tracer ptr
     */
    public static native long createTracer(String jaegerConfigPath, String activePersona);
}
