from __future__ import annotations

from dataclasses import dataclass
from typing import Dict, Set
import random


@dataclass
class AttackProfile:
    sybil_nodes: Set[int]
    sinkhole_nodes: Set[int]
    selective_nodes: Set[int]


class AttackEngine:
    """Injects malicious behavior for evaluation and provides trust observations."""

    def __init__(self, node_count: int, attack_fraction: float, seed: int = 0):
        random.seed(seed)
        candidates = list(range(1, node_count))
        random.shuffle(candidates)
        k = max(1, int(node_count * attack_fraction)) if attack_fraction > 0 else 0
        chosen = candidates[:k]
        split = max(1, k // 3) if k else 0
        sybil = set(chosen[:split])
        sinkhole = set(chosen[split:2 * split])
        selective = set(chosen[2 * split:])
        self.profile = AttackProfile(sybil_nodes=sybil, sinkhole_nodes=sinkhole, selective_nodes=selective)
        self.trust: Dict[int, float] = {i: 1.0 for i in range(node_count)}

    def advertised_progress_multiplier(self, node_id: int) -> float:
        # sinkhole exaggerates attractiveness
        return 1.8 if node_id in self.profile.sinkhole_nodes else 1.0

    def should_drop(self, node_id: int) -> bool:
        if node_id in self.profile.selective_nodes:
            return random.random() < 0.45
        return False

    def observe_forward(self, node_id: int, forwarded: bool) -> None:
        delta = 0.02 if forwarded else -0.08
        if node_id in self.profile.sybil_nodes:
            delta -= 0.02
        t = self.trust[node_id]
        self.trust[node_id] = max(0.0, min(1.0, t + delta))

    def trust_score(self, node_id: int) -> float:
        return self.trust[node_id]
