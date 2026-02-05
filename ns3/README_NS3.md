# NS-3 Only Implementation (Modular Structure)

This project now keeps only the NS-3 implementation for WSN secure-ML routing.

## Directory structure
- `ns3/network/`
  - `wsn_network.h` (topology, node state, adjacency, distance/progress utilities)
- `ns3/ml_model/`
  - `secure_ml_policy.h` (attack model, trust update, AODV/LEACH/Secure-ML next-hop, Q-update)
- `ns3/simulation_scripts/`
  - `secure_ml_main.cc` (main simulator loop, PEGASIS chain, metrics, NetAnim integration)
- `ns3/csv_logs/`
  - output location for CSV result logs
- `ns3/netanim/`
  - output location for NetAnim XML files
- `ns3/scripts/`
  - `run_all_protocols.sh` (batch run across protocols)

## Export to NS-3 scratch
```bash
bash scripts/export_ns3_scratch.sh ~/ns-3-dev
```

This copies sources to:
`~/ns-3-dev/scratch/secure_ml/{network,ml_model,simulation_scripts}`

## Run single simulation
```bash
cd ~/ns-3-dev
./ns3 run "scratch/secure_ml/simulation_scripts/secure_ml_main --protocol=secure_ml --nodes=200 --rounds=1000 --attackFraction=0.1 --seed=7 --csvOut=/workspace/Major_Project/ns3/csv_logs/wsn_results.csv --animOut=/workspace/Major_Project/ns3/netanim/secure_ml_anim.xml"
```

## Run all protocols
```bash
bash /workspace/Major_Project/ns3/scripts/run_all_protocols.sh ~/ns-3-dev
```

Protocols:
- `aodv_like`
- `leach_like`
- `pegasis_like`
- `secure_ml`

## Outputs
- CSV metrics at `ns3/csv_logs/wsn_results.csv`
- NetAnim XML per protocol at `ns3/netanim/*_anim.xml`
