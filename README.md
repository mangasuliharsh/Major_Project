# Machine Learning-Based Energy-Efficient and Secure Routing for WSN (NS-3)

This repository now contains an **NS-3-only** implementation with modular file organization:
- `ns3/network`
- `ns3/ml_model`
- `ns3/simulation_scripts`
- `ns3/csv_logs`
- `ns3/netanim`
- `ns3/scripts`

## Setup
1. Export source files into your NS-3 scratch tree:
```bash
bash scripts/export_ns3_scratch.sh ~/ns-3-dev
```

2. Run all protocols and generate CSV + NetAnim XML:
```bash
bash ns3/scripts/run_all_protocols.sh ~/ns-3-dev
```

3. Open NetAnim and load files from `ns3/netanim/`.

For detailed commands and parameters, see `ns3/README_NS3.md`.
