from __future__ import annotations

from dataclasses import dataclass
from typing import Dict, List, Sequence, Tuple
import math

from .attacks import AttackEngine
from .ml import QLearner, UtilityRegressor
from .models import Node, distance


def _progress(nodes: Sequence[Node], sink_id: int, a: int, b: int) -> float:
    da = distance(nodes[a], nodes[sink_id])
    db = distance(nodes[b], nodes[sink_id])
    return da - db


def aodv_like_next(nodes: Sequence[Node], adj: Dict[int, List[int]], sink_id: int, current: int) -> int | None:
    cands = [n for n in adj[current] if nodes[n].alive and nodes[n].energy > 0]
    if not cands:
        return None
    return max(cands, key=lambda n: _progress(nodes, sink_id, current, n))


def leach_like_next(nodes: Sequence[Node], adj: Dict[int, List[int]], sink_id: int, current: int) -> int | None:
    cands = [n for n in adj[current] if nodes[n].alive and nodes[n].energy > 0]
    if not cands:
        return None
    return max(cands, key=lambda n: 0.6 * nodes[n].energy + 0.4 * _progress(nodes, sink_id, current, n))


def pegasis_like_next(chain_next: Dict[int, int], current: int) -> int | None:
    return chain_next.get(current)


def make_chain(nodes: Sequence[Node], sink_id: int) -> Dict[int, int]:
    # simple nearest-neighbor chain ending at sink
    uns = {n.node_id for n in nodes if n.node_id != sink_id}
    chain: Dict[int, int] = {}
    cur = min(uns, key=lambda x: distance(nodes[x], nodes[sink_id])) if uns else sink_id
    while uns:
        uns.remove(cur)
        if not uns:
            chain[cur] = sink_id
            break
        nxt = min(uns, key=lambda x: distance(nodes[x], nodes[cur]))
        chain[cur] = nxt
        cur = nxt
    return chain


@dataclass
class SecureMLRouter:
    sink_id: int
    energy_min: float = 0.15
    trust_min: float = 0.3
    alpha: float = 0.6
    beta: float = 0.6
    gamma: float = 0.3

    def __post_init__(self) -> None:
        self.model = UtilityRegressor()
        self.q = QLearner()

    def features(self, nodes: Sequence[Node], adj: Dict[int, List[int]], current: int, j: int, attack: AttackEngine) -> List[float]:
        deg = len(adj[j])
        prog = _progress(nodes, self.sink_id, current, j)
        dist = distance(nodes[current], nodes[j])
        trust = attack.trust_score(j)
        return [
            nodes[j].energy,
            deg,
            prog,
            1.0 / (1.0 + dist),
            trust,
        ]

    def choose(self, nodes: Sequence[Node], adj: Dict[int, List[int]], current: int, attack: AttackEngine) -> int | None:
        candidates = []
        for j in adj[current]:
            if not nodes[j].alive or nodes[j].energy <= self.energy_min:
                continue
            trust = attack.trust_score(j)
            if trust < self.trust_min:
                continue
            x = self.features(nodes, adj, current, j, attack)
            u = self.model.predict_one(x)
            qv = self.q.value(current, j)
            risk = 1.0 - trust
            lb = nodes[j].energy
            score = self.alpha * u + (1.0 - self.alpha) * qv - self.beta * risk + self.gamma * lb
            candidates.append((j, score))
        if not candidates:
            return None
        return max(candidates, key=lambda t: t[1])[0]
