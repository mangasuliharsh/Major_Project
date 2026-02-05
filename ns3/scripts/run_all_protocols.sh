#!/usr/bin/env bash
set -euo pipefail

NS3_DIR="${1:-$HOME/ns-3-dev}"
NODES="${NODES:-200}"
ROUNDS="${ROUNDS:-1000}"
ATTACK="${ATTACK:-0.1}"
SEED="${SEED:-7}"
CSV_OUT="${CSV_OUT:-/workspace/Major_Project/ns3/csv_logs/wsn_results.csv}"
ANIM_DIR="${ANIM_DIR:-/workspace/Major_Project/ns3/netanim}"

mkdir -p "$(dirname "$CSV_OUT")" "$ANIM_DIR"
: > "$CSV_OUT"
echo "protocol,nodes,rounds,generated,delivered,pdr,avg_delay,energy_per_packet,routing_overhead,fnd_round" >> "$CSV_OUT"

for p in aodv_like leach_like pegasis_like secure_ml; do
  echo "Running $p"
  ANIM_OUT="$ANIM_DIR/${p}_anim.xml"
  (cd "$NS3_DIR" && ./ns3 run "scratch/secure_ml/simulation_scripts/secure_ml_main --protocol=$p --nodes=$NODES --rounds=$ROUNDS --attackFraction=$ATTACK --seed=$SEED --csvOut=$CSV_OUT --animOut=$ANIM_OUT")
done

echo "CSV saved to $CSV_OUT"
echo "NetAnim XMLs saved to $ANIM_DIR"
