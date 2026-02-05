from __future__ import annotations

from dataclasses import dataclass
from typing import Callable, Dict, List, Tuple
import random

from .attacks import AttackEngine
from .models import Metrics, Packet, SimulationConfig, adjacency, seeded_nodes
from .routing import SecureMLRouter, aodv_like_next, leach_like_next, make_chain, pegasis_like_next

ENERGY_TX = 0.004
ENERGY_RX = 0.002
MAX_TTL = 15


def _apply_energy(nodes, src: int, dst: int, metrics: Metrics) -> None:
    nodes[src].energy -= ENERGY_TX
    nodes[dst].energy -= ENERGY_RX
    metrics.total_energy_spent += ENERGY_TX + ENERGY_RX
    if nodes[src].energy <= 0:
        nodes[src].alive = False
    if nodes[dst].energy <= 0:
        nodes[dst].alive = False


def _first_dead(metrics: Metrics, nodes, round_idx: int) -> None:
    if metrics.fnd_round is None and any(not n.alive for n in nodes[1:]):
        metrics.fnd_round = round_idx


def simulate_protocol(cfg: SimulationConfig, protocol: str) -> Dict[str, float]:
    random.seed(cfg.seed)
    nodes = seeded_nodes(cfg)
    adj = adjacency(nodes, cfg.comm_range)
    attack = AttackEngine(cfg.nodes, cfg.attack_fraction, seed=cfg.seed)
    metrics = Metrics()

    secure = SecureMLRouter(sink_id=cfg.sink_id)
    chain = make_chain(nodes, cfg.sink_id)

    # warm-up dataset for supervised utility
    if protocol == "secure_ml":
        X, y = [], []
        for i in range(1, cfg.nodes):
            for j in adj[i]:
                x = secure.features(nodes, adj, i, j, attack)
                utility = 0.5 * x[0] + 0.2 * x[2] + 0.3 * x[4]
                X.append(x)
                y.append(utility)
        secure.model.fit(X, y)

    for r in range(cfg.rounds):
        sources = [n.node_id for n in nodes[1:] if n.alive]
        random.shuffle(sources)
        sources = sources[: min(len(sources), cfg.packets_per_round)]
        for src in sources:
            metrics.generated += 1
            pkt = Packet(source=src, ttl=MAX_TTL, created_at=r)
            current = src
            hops = 0
            while pkt.ttl > 0 and current != cfg.sink_id and nodes[current].alive:
                pkt.ttl -= 1
                hops += 1
                if protocol == "aodv":
                    nxt = aodv_like_next(nodes, adj, cfg.sink_id, current)
                    metrics.control_packets += 1
                elif protocol == "leach":
                    nxt = leach_like_next(nodes, adj, cfg.sink_id, current)
                    metrics.control_packets += 1
                elif protocol == "pegasis":
                    nxt = pegasis_like_next(chain, current)
                else:
                    nxt = secure.choose(nodes, adj, current, attack)

                if nxt is None:
                    break

                # sinkhole can inflate advertised quality but still forward path physically
                if attack.should_drop(nxt):
                    _apply_energy(nodes, current, nxt, metrics)
                    attack.observe_forward(nxt, False)
                    break

                _apply_energy(nodes, current, nxt, metrics)
                attack.observe_forward(nxt, True)

                if protocol == "secure_ml":
                    reward = 1.0 + (nodes[nxt].energy * 0.5) + (attack.trust_score(nxt) * 0.8)
                    if nxt == cfg.sink_id:
                        reward += 2.0
                    secure.q.update(current, nxt, reward, nxt, adj.get(nxt, []))

                current = nxt

            if current == cfg.sink_id:
                metrics.delivered += 1
                metrics.total_delay += hops

        _first_dead(metrics, nodes, r)

    return metrics.finalize()


def compare_all(cfg: SimulationConfig) -> Dict[str, Dict[str, float]]:
    results = {}
    for protocol in ["aodv", "leach", "pegasis", "secure_ml"]:
        results[protocol] = simulate_protocol(cfg, protocol)
    return results
