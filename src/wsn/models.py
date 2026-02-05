from __future__ import annotations

from dataclasses import dataclass
from typing import Dict, List, Tuple
import math
import random


@dataclass
class Node:
    node_id: int
    x: float
    y: float
    energy: float
    alive: bool = True


@dataclass
class Packet:
    source: int
    ttl: int
    created_at: int


@dataclass
class SimulationConfig:
    width: float = 500.0
    height: float = 500.0
    nodes: int = 80
    comm_range: float = 120.0
    initial_energy: float = 2.0
    rounds: int = 200
    packets_per_round: int = 20
    sink_id: int = 0
    attack_fraction: float = 0.1
    seed: int = 7


@dataclass
class Metrics:
    generated: int = 0
    delivered: int = 0
    total_delay: float = 0.0
    total_energy_spent: float = 0.0
    control_packets: int = 0
    fnd_round: int | None = None

    def finalize(self) -> Dict[str, float]:
        pdr = self.delivered / self.generated if self.generated else 0.0
        avg_delay = self.total_delay / self.delivered if self.delivered else math.inf
        energy_per_packet = self.total_energy_spent / self.delivered if self.delivered else math.inf
        routing_overhead = self.control_packets / self.delivered if self.delivered else math.inf
        return {
            "pdr": pdr,
            "avg_delay": avg_delay,
            "energy_per_packet": energy_per_packet,
            "routing_overhead": routing_overhead,
            "fnd_round": float(self.fnd_round if self.fnd_round is not None else -1),
        }


def distance(a: Node, b: Node) -> float:
    return math.hypot(a.x - b.x, a.y - b.y)


def seeded_nodes(cfg: SimulationConfig) -> List[Node]:
    random.seed(cfg.seed)
    nodes: List[Node] = []
    for i in range(cfg.nodes):
        x = random.uniform(0, cfg.width)
        y = random.uniform(0, cfg.height)
        nodes.append(Node(node_id=i, x=x, y=y, energy=cfg.initial_energy))
    # sink near center
    nodes[cfg.sink_id].x = cfg.width / 2
    nodes[cfg.sink_id].y = cfg.height / 2
    return nodes


def adjacency(nodes: List[Node], comm_range: float) -> Dict[int, List[int]]:
    g: Dict[int, List[int]] = {n.node_id: [] for n in nodes}
    for i in nodes:
        for j in nodes:
            if i.node_id == j.node_id:
                continue
            if distance(i, j) <= comm_range:
                g[i.node_id].append(j.node_id)
    return g
