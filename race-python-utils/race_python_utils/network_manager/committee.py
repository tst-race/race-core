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
        Committee Class. To represent committees of servers, optionally functioning as
            exit committees for one or more clients
"""

# Python Library Imports
import networkx as nx
from typing import List, Dict, Any, Set, Tuple, TypeVar

# Local Library Imports
# Fixes circular dependencies in MyPy typing
MYPY = False
if MYPY:
    from race_python_utils.network_manager import Client, Server  # NOQA


class Committee:
    """
    Committee Class. To represent committees of servers, optionally functioning as
        exit committees for one or more clients
    """

    def __init__(self, name: str = "", flooding_factor: int = 2) -> None:
        """
        Purpose:
            Initialize Committee object with its name and the flooding_factor
        Args:
            name (String): name of the committee
            flooding_factor (Int): number of committees to send to when forwarding out of
                this committee
        Returns:
            N/A
        """
        self.name = name
        self.servers: List["Server"] = []
        self.clients: List["Client"] = []
        self.rings: List[List[str]] = []
        self.flooding_factor = flooding_factor

    def __repr__(self) -> str:
        """
        Purpose:
            Repr for this committee to enable Set usage
        Args:
            N/A
        Returns:
            The name of the committee
        """
        return self.name

    def generate_rings(self, num_rings: int = 1) -> None:
        """
        Purpose:
           Attempts to generate rings consisting of cycles around the committee using
                disjoint sets of edges.
        Args:
            num_rings: the number of rings to try to generate
        Returns:
            N/A
        """
        required_length = len(self.servers)
        member_set = set(self.servers)
        graph = nx.DiGraph()
        for src in self.servers:
            for dst in src.reachable_servers:
                if dst not in member_set:
                    continue

                graph.add_edge(src.name, dst.name)

        ringset: List[List[str]] = []
        while len(ringset) < num_rings:
            added = False
            for cycle in nx.simple_cycles(graph):
                if len(cycle) == required_length:
                    cycle_edges = [
                        (cycle[idx], cycle[(idx + 1) % len(cycle)])
                        for idx in range(len(cycle))
                    ]
                    ringset.append([edge[0] for edge in cycle_edges])
                    added = True
                    graph.remove_edges_from(cycle_edges)
                    break
            if not added:
                break

        self.rings = ringset
        if len(self.rings) < num_rings:
            print(
                f"Warning building stub network manager configs: could only generate "
                f"{len(self.rings)} rings for committee {self.name}"
            )

    def gather_reachable_committees(self) -> Set["Committee"]:
        """
        Purpose:
            Compute the list of committees this committee can reach based on server
                reachability.
        Args:
            N/A
        Returns:
            reachable (Set): set of Committee objects that can be reached from this Committee
        """
        s: Set[Committee] = set()
        return s.union(
            *[
                {reachable.committee for reachable in server.reachable_servers}
                for server in self.servers
            ]
        )

    def get_rings_for_server(self, server: "Server") -> List[Dict[str, Any]]:
        """
        Purpose:
            Produce the ring hop entries for a specific server in this committee, used for
                producing configs.
        Args:
            server: the Server object to fetch entries for
        Returns:
            server_rings: the list of ring next-hops and lengths for this server
        """
        server_rings = []
        for ring in self.rings:
            new_ring: Dict[str, Any] = {}
            if server.name in ring:
                server_idx = ring.index(server.name)
                new_ring["next"] = ring[(server_idx + 1) % len(ring)]
                new_ring["length"] = len(ring)
            else:  # put blank ring in to keep indexing correct across committee
                new_ring["next"] = ""
                new_ring["length"] = 0

            server_rings.append(new_ring)

        return server_rings
