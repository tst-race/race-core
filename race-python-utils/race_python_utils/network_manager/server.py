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
        Server Class. Represents RACE server nodes that are members of committees
"""

# Python Library Imports
from typing import List, Dict, Any

# Local Python Library Imports
from race_python_utils.network_manager import Committee

# Fixes circular dependencies in MyPy typing
MYPY = False
if MYPY:
    from race_python_utils.network_manager import Client  # NOQA


class Server:
    """
    Server Class. Represents RACE server nodes that are members of committees
    """

    def __init__(self, name: str = ""):
        """
        Purpose:
            Initializer method for the server
        Args:
            name (String): name of the server
        Returns:
            N/A
        """
        self.name = name
        self.reachable_servers: List[Server] = []
        self.reachable_clients: List[Client] = []
        self.exit_clients: List[Client] = []
        self.committee = Committee()

    def __repr__(self) -> str:
        """
        Purpose:
            Repr for this server to enable Set usage
        Args:
            N/A
        Returns:
            The name of the server
        """
        return self.name

    def json_config(self) -> Dict[str, Any]:
        """
        Purpose:
            Produces the network manager committee config for this server in JSON format to be written
                to a deployment
        Args:
            N/A
        Returns:
            Dictionary ready to be JSON-stringified
        """
        reachable_committees: Dict[str, List[str]] = {}
        for server in self.reachable_servers:
            committee_name = server.committee.name
            if committee_name == self.committee.name:
                continue
            elif committee_name not in reachable_committees:
                reachable_committees[committee_name] = [server.name]
            else:
                continue
                # reachable_committees[committee_name].append(server.name)
                # NOTE: current network manager implementation only sends to the first server in the
                # list, so this restricts the config to make this obvious when looking at
                # the configuration

        return {
            "committeeName": self.committee.name,
            "exitClients": [client.name for client in self.exit_clients],
            "committeeClients": [client.name for client in self.committee.clients],
            "reachableCommittees": reachable_committees,
            "rings": self.committee.get_rings_for_server(self),
            "floodingFactor": self.committee.flooding_factor,
        }
