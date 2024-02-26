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
        Helper functions for the plugin that will be used by client and server
"""

# Python Library Imports
import os

from networkManagerPluginBindings import IRaceSdkNM
from .Log import logDebug, logError, logWarning

try:  # Try to import simplejson, if it is not installed default to json
    import simplejson as json
except Exception as err:
    import json


###
# JSON Helpers
###


def read_json_file_into_memory(sdk: IRaceSdkNM, json_file: str):
    """
    Purpose:
        Read properly formatted JSON file into memory.
    Args:
        json_file (String): Filename for JSON file to load (including path)
    Returns:
        json_object (Dictonary): Dictonary representation JSON Object
    Examples:
        >>> json_file = 'some/path/to/file.json'
        >>> json_object = read_json_file_into_memory(json_file)
        >>> print(json_object)
        { 'key': 'value' }
    """
    logDebug(f"Reading JSON File Into Memory: {json_file}")

    try:
        tmp = bytes(sdk.readFile(json_file)).decode("utf-8")
        return json.loads(tmp)
    except Exception as err:
        logError(f"Cannot Read json into memory ({json_file}): {err}")
        raise err
