# WSN Secure ML Routing (Implementation)

This repository now contains an executable prototype for a **Machine Learning–Based Energy-Efficient and Secure Routing Framework for WSNs**.

## What is implemented
- WSN simulator with node energy depletion, multi-hop forwarding, and packet generation.
- Attack engine for Sybil/sinkhole/selective-forwarding style behavior (trust degradation + drops).
- Baseline routing strategies: AODV-like, LEACH-like, PEGASIS-like.
- Proposed hybrid Secure-ML router:
  - supervised route utility predictor (RandomForest if available, otherwise linear model fallback),
  - online Q-learning for adaptive next-hop valuation,
  - trust/energy-constrained route selection.
- Benchmark runner to compare all protocols on common metrics.

## Run
```bash
python scripts/run_experiment.py --nodes 80 --rounds 200 --attack-fraction 0.1 --seed 7
```

## Tests
```bash
pytest -q
```
