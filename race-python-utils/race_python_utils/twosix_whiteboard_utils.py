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
        Utilities for interacting with the Two Six Whiteboard
"""

# Python Library Imports
from typing import Any, Dict, List, Union


def generate_local_two_six_whiteboard_details() -> Dict[str, Union[str, int, bool]]:
    """
    Purpose:
        Generate Two Six Whiteboard details for link creation for a local
        deployment. This assumes that connectivity will use local docker networking
        instead of the range config information.
    Args:
        N/A
    Returns:
        two_six_whiteboard_details: Two Six Whiteboard connectivity using local
        docker networking instead of the range config information
    """

    return {
        "authenticated_service": False,
        "hostname": "twosix-whiteboard",
        "name": "twosix-whiteboard",
        "port": 5000,
        "protocol": "http",
        "url": "twosix-whiteboard:5000",
    }
