#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src"))

from wsn import SimulationConfig, compare_all  # noqa: E402


def main() -> None:
    p = argparse.ArgumentParser(description="Run secure WSN ML routing benchmark")
    p.add_argument("--nodes", type=int, default=80)
    p.add_argument("--rounds", type=int, default=200)
    p.add_argument("--attack-fraction", type=float, default=0.1)
    p.add_argument("--seed", type=int, default=7)
    args = p.parse_args()

    cfg = SimulationConfig(nodes=args.nodes, rounds=args.rounds, attack_fraction=args.attack_fraction, seed=args.seed)
    results = compare_all(cfg)
    print(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()
