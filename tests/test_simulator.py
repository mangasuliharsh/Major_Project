from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src"))

from wsn import SimulationConfig, compare_all


def test_compare_all_keys_and_ranges():
    cfg = SimulationConfig(nodes=35, rounds=40, packets_per_round=8, attack_fraction=0.1, seed=3)
    results = compare_all(cfg)
    assert set(results.keys()) == {"aodv", "leach", "pegasis", "secure_ml"}
    for metrics in results.values():
        assert 0.0 <= metrics["pdr"] <= 1.0
        assert metrics["avg_delay"] >= 0
        assert metrics["energy_per_packet"] >= 0
