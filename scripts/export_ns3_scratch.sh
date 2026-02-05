#!/usr/bin/env bash
set -euo pipefail

TARGET_NS3="${1:-$HOME/ns-3-dev}"
TARGET_DIR="$TARGET_NS3/scratch/secure_ml"

mkdir -p "$TARGET_DIR/network" "$TARGET_DIR/ml_model" "$TARGET_DIR/simulation_scripts"
cp /workspace/Major_Project/ns3/network/wsn_network.h "$TARGET_DIR/network/wsn_network.h"
cp /workspace/Major_Project/ns3/ml_model/secure_ml_policy.h "$TARGET_DIR/ml_model/secure_ml_policy.h"
cp /workspace/Major_Project/ns3/simulation_scripts/secure_ml_main.cc "$TARGET_DIR/simulation_scripts/secure_ml_main.cc"

echo "Copied modular NS-3 sources -> $TARGET_DIR"
