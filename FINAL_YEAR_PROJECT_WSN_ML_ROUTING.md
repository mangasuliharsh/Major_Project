# Machine Learning–Based Energy-Efficient and Secure Routing Framework for Wireless Sensor Networks (WSNs)

## 1) Problem Statement

### Real-world context
Wireless Sensor Networks (WSNs) are used in precision agriculture, industrial safety, forest fire detection, smart grids, and military surveillance. In these deployments, hundreds of battery-powered nodes collect and forward data to a sink/base station over multi-hop paths. Replacing batteries is expensive, sometimes impossible (hazardous zones, remote terrain), and network failures can lead to safety or economic losses.

### Core problem
Classical routing protocols are not jointly optimized for **energy efficiency**, **security**, and **adaptivity** under dynamic conditions. A route that is shortest in hop count may drain critical relay nodes quickly, may be routed through malicious nodes, and may fail when topology or traffic changes.

### Why traditional routing protocols fail
1. **Static or semi-static decision rules**: Protocols like AODV/LEACH use predefined heuristics (hop count, cluster-head rotation thresholds). They do not continuously learn from runtime outcomes.
2. **Weak security awareness**: Most baseline routing protocols assume benign nodes. They may not detect Sybil, sinkhole, or selective forwarding attacks in time.
3. **Energy blind spots**: Frequent route discoveries and control packets increase overhead; repeated use of central nodes creates energy holes.
4. **Poor adaptation to non-stationary environments**: Link quality, traffic intensity, interference, and node failures vary over time.
5. **Scalability bottlenecks**: Large-scale networks with heterogeneous nodes require intelligent route scoring under multiple constraints, which fixed rules cannot capture effectively.

### Research objective
Design a **hybrid ML-based routing framework** that:
- predicts secure and energy-efficient next-hop choices,
- detects malicious routing behavior,
- adapts online to changing network dynamics,
- and outperforms AODV, LEACH, and PEGASIS across delivery, delay, energy, and lifetime metrics.

---

## 2) System Architecture

### Textual architecture diagram
`[Sensor Layer] -> [Routing & Feature Collector] -> [ML Route Scorer] -> [Attack Detection Engine] -> [Policy/Decision Layer] -> [Forwarding Plane] -> [Sink/Cloud Analytics]`

### Components
1. **Sensor Nodes (N nodes)**
   - Sense environment and generate packets.
   - Maintain neighbor table: residual energy, ETX/link quality, queue load, trust score.
   - Run lightweight inference model (or query edge gateway model).

2. **Sink Node / Base Station**
   - Aggregates data and routing logs.
   - Performs heavier model training and global policy updates.
   - Disseminates updated model parameters to nodes (periodically or event-triggered).

3. **ML Module**
   - **Offline supervised learner** for route quality prediction.
   - **Online reinforcement learner** for adaptation to dynamic conditions.
   - Produces route score/Q-value for each candidate next hop.

4. **Attack Detection Module**
   - Monitors anomalies in forwarding ratio, identity behavior, route attraction patterns, and path consistency.
   - Outputs trust penalties or blacklist actions.

5. **Routing Policy Engine**
   - Combines ML route score + trust + energy thresholds.
   - Selects next hop with constrained optimization.

### Data flow
1. Nodes collect link and node statistics every time window (e.g., 5 s).
2. Features are sent to local routing engine; periodic logs sent to sink.
3. ML module computes route utility for candidate neighbors.
4. Attack module flags suspicious nodes and updates trust table.
5. Policy engine filters malicious/low-energy candidates.
6. Best next hop selected; packet forwarded.
7. End-to-end feedback (delivery success, delay, retransmissions, energy drop) updates model.

---

## 3) Dataset & Features

### Data generation strategy
Use a **simulation-driven dataset pipeline** with NS-3:
- Run many episodes under varied topology, traffic, mobility/noise, and attack scenarios.
- Export packet traces, flow stats, per-node energy, neighbor events, drop reasons.
- Build tabular training dataset from route decision snapshots.

Optional extension: small real testbed (ESP32/CC2538 + Contiki-NG) to validate transferability.

### Candidate feature set
Per candidate neighbor `j` from node `i`:
1. `E_res_j` = residual energy ratio of neighbor.
2. `ΔE_rate_j` = recent energy depletion rate.
3. `hop_to_sink_j` = estimated hops from `j` to sink.
4. `ETX_ij` / `PRR_ij` = link reliability.
5. `queue_len_j` = queue occupancy (congestion proxy).
6. `delay_ij_hist` = historical one-hop delay.
7. `drop_rate_j` = packet drop ratio.
8. `ctrl_overhead_j` = recent routing control load.
9. `trust_j` = trust score from security module.
10. `neighbor_degree_j` = redundancy/connectivity.
11. `distance_progress_j` = geometric progress toward sink.
12. `stability_j` = neighbor link persistence metric.

### Label definition
For supervised learning, define label per routing decision window:
- **Binary classification label**: `good_next_hop = 1` if path via `j` achieves success threshold (e.g., delivered within delay bound and no suspicious behavior).
- **Regression label**: route utility score
  \[
  U = w_1\cdot PDR - w_2\cdot Delay - w_3\cdot EnergyCost - w_4\cdot Overhead - w_5\cdot Risk
  \]
- **RL reward** uses similar utility with immediate + delayed components.

---

## 4) Machine Learning Design

### ML paradigm
**Hybrid approach (Supervised + Reinforcement Learning)**:
- Supervised model gives strong initial policy from historical simulations.
- RL fine-tunes decisions online in non-stationary network conditions.

### Model selection rationale
1. **Random Forest (RF)** for supervised stage:
   - Handles nonlinear interactions and mixed-scale features.
   - Robust to noise and feature collinearity.
   - Fast inference suitable for constrained nodes (or edge proxy).

2. **Q-learning / DQN** for online adaptation:
   - State = local network snapshot.
   - Action = choose next-hop neighbor.
   - Reward = weighted utility including security penalty.

Recommended final-year baseline:
- Start with tabular Q-learning for simplicity and explainability.
- Upgrade to DQN when state/action space becomes large.

### Training process
1. Generate multi-scenario traces in NS-3.
2. Preprocess: clean missing values, normalize continuous features, encode categorical events.
3. Split by scenario (not random packets) to avoid leakage.
4. Train RF to predict route utility or class.
5. Evaluate with ROC-AUC/F1 (if classification) or RMSE (if regression).
6. Deploy RF model for warm-start route scoring.
7. Run online RL episodes; update Q-values with exploration decay.
8. Periodically compare policy performance and checkpoint best model.

### Route prediction mechanism
For each packet at node `i`:
1. Build feature vector for each candidate neighbor `j`.
2. RF predicts utility `U_hat(i,j)`.
3. RL estimates long-term value `Q(i,j)`.
4. Combined score:
   \[
   Score(i,j) = \alpha U_{hat}(i,j) + (1-\alpha)Q(i,j) - \beta\cdot Risk(j)
   \]
5. Choose argmax subject to energy/trust constraints.

### Adaptation strategy
- Sliding window feature refresh.
- Online trust updates.
- Concept-drift trigger: if PDR drops or delays spike, temporarily increase exploration.
- Sink retrains RF weekly/periodically and disseminates updated model.

---

## 5) Security & Attack Detection

### Attacks handled
1. **Sybil Attack**: one physical node claims multiple identities.
2. **Sinkhole Attack**: malicious node advertises attractive route and draws traffic.
3. **Selective Forwarding**: node drops specific packets while behaving normally otherwise.

### Detection logic
Use a hybrid trust + anomaly ML detector:
1. **Identity consistency checks** (RSSI/location/time correlation) for Sybil.
2. **Route-attraction anomaly**: sudden excessive route requests/advertisements with unrealistic quality for sinkhole.
3. **Forwarding behavior monitoring**:
   - overhearing/watchdog or ACK-based inference,
   - compare packets received vs forwarded,
   - detect class-specific drops for selective forwarding.
4. Train lightweight classifier (e.g., One-Class SVM / Isolation Forest / RF classifier) on behavior features:
   - forwarding ratio,
   - control-message frequency,
   - route-change rate,
   - neighbor disagreement index,
   - trust decay slope.

### Mitigation strategy
1. Reduce trust score immediately on anomaly.
2. Add suspicious node to quarantine list.
3. Exclude node from routing candidates.
4. Trigger local route repair avoiding quarantined nodes.
5. Share signed alert summary with nearby nodes/sink to avoid false positives via consensus threshold.

---

## 6) Routing Algorithm (Core Logic)

### Step-by-step process
1. Node receives packet (origin or relay).
2. Retrieve live neighbor set within communication radius.
3. Filter neighbors violating hard constraints:
   - residual energy < `E_min`,
   - trust < `T_min`,
   - unstable link < `L_min`.
4. Compute feature vectors for remaining neighbors.
5. Predict supervised utility and RL Q-value.
6. Apply security risk penalty and load-balancing factor.
7. Select best-scoring neighbor.
8. Forward packet and log outcome.
9. Update RL reward and trust metrics.
10. If no valid neighbor: initiate controlled route discovery fallback.

### Pseudocode
```text
Algorithm SecureMLRouteSelect(packet p, node i)
Input: neighbor table N_i, model_RF, Q-table, trust table
Output: next hop n*

1: C <- {}   // candidate set
2: for each neighbor j in N_i do
3:     if E_res_j >= E_min and Trust_j >= T_min and LinkStability_ij >= L_min then
4:         x_ij <- ExtractFeatures(i, j)
5:         U_ij <- RF_Predict(model_RF, x_ij)
6:         Q_ij <- QValue(i, j)
7:         Risk_ij <- SecurityRisk(j)
8:         LB_ij <- LoadBalanceFactor(j)
9:         Score_ij <- alpha*U_ij + (1-alpha)*Q_ij - beta*Risk_ij + gamma*LB_ij
10:         C <- C U {(j, Score_ij)}
11:    end if
12: end for
13: if C is empty then
14:    return FallbackRouteDiscovery(i, p)
15: end if
16: n* <- argmax_j Score_ij in C
17: Forward(p, n*)
18: r <- ObserveReward(p, n*)
19: UpdateQ(i, n*, r)
20: UpdateTrust(n*, forwarding_behavior)
21: return n*
```

### Energy efficiency guarantees
- Penalize high depletion-rate nodes.
- Reward balanced forwarding to avoid energy holes.
- Minimize retransmissions by considering reliability features.
- Reduce expensive global route discoveries through local predictive selection.

---

## 7) Simulation & Implementation

### Simulator
**NS-3** (recommended: v3.38+), with Energy Framework + FlowMonitor.

### Network setup (example research configuration)
- Area: `500m x 500m`.
- Nodes: 100, 200, 300 (scalability sweep).
- Radio model: IEEE 802.15.4 / LR-WPAN.
- Traffic: CBR + event bursts.
- Packet size: 64 bytes.
- Simulation time: 1000 s per run.
- Initial node energy: 2 Joules.
- Sink position: center and corner (two scenarios).
- Attack ratio: 0%, 10%, 20% malicious nodes.
- Repetitions: 20 seeds/scenario for statistical confidence.

### Integration of ML with NS-3
Two practical options:
1. **Offline loop**: NS-3 logs -> Python ML training -> export model -> NS-3 inference plugin.
2. **Online co-simulation**: NS-3 communicates with Python via sockets/IPC for runtime scoring.

Recommended FYP path:
- Implement inference in C++ (or lightweight ONNX runtime) for deterministic simulations.
- Keep training in Python (scikit-learn, PyTorch).

### Tools and libraries
- NS-3, FlowMonitor, NetAnim.
- Python: pandas, numpy, scikit-learn, matplotlib, seaborn.
- RL: stable-baselines3 or custom Q-learning.
- Security analytics: scikit-learn anomaly modules.
- Versioning/CI: Git + GitHub Actions.

---

## 8) Performance Metrics

1. **Packet Delivery Ratio (PDR)**
   \[
   PDR = \frac{Packets\ Received\ at\ Sink}{Packets\ Generated\ by\ Sources}
   \]

2. **End-to-End Delay**
   Average time from source transmission to sink reception.

3. **Energy Consumption**
   Total and per-delivered-packet energy usage.

4. **Network Lifetime**
   - FND: First Node Dies
   - HND: Half Nodes Die
   - LND: Last Node Dies

5. **Routing Overhead**
   Ratio or count of control packets per delivered data packet.

6. (Optional) **Security Metrics**
   Detection Rate, False Positive Rate, mitigation latency.

---

## 9) Comparison & Results

### Baselines
- **AODV**: reactive shortest-path style.
- **LEACH**: cluster-based energy-aware baseline.
- **PEGASIS**: chain-based energy-saving baseline.

### Example result table (illustrative but realistic)
Scenario: 200 nodes, 10% malicious, 1000 s simulation.

| Protocol            | PDR (%) | Avg Delay (ms) | Energy/Packet (mJ) | FND (s) | Routing Overhead |
|---------------------|--------:|---------------:|--------------------:|--------:|-----------------:|
| AODV                | 82.4    | 168            | 2.35                | 420     | 0.36             |
| LEACH               | 86.7    | 143            | 1.92                | 510     | 0.28             |
| PEGASIS             | 88.1    | 201            | 1.75                | 560     | 0.24             |
| Proposed Secure-ML  | 93.9    | 119            | 1.48                | 690     | 0.22             |

### Interpretation
- Higher PDR due to reliability + trust-aware forwarding.
- Lower delay because model avoids congested/unstable links.
- Better energy profile through load balancing and retransmission reduction.
- Longer lifetime from preventing relay hotspot exhaustion.
- Lower overhead than AODV due to reduced route rediscovery.

### Why ML-based routing is better
It optimizes a **multi-objective decision surface** (energy, reliability, delay, trust) that rule-based protocols cannot jointly tune in real time.

---

## 10) Advantages & Limitations

### Technical strengths
1. Joint optimization of energy and security.
2. Adaptive routing under dynamic topology and traffic.
3. Modular architecture (can replace ML model without redesigning protocol stack).
4. Strong publication potential due to comparative and attack-focused evaluation.

### Practical limitations
1. Training data quality strongly affects generalization.
2. On-node ML inference may increase CPU/memory consumption.
3. RL convergence can be slow in very large state spaces.
4. Security detectors may show false positives under harsh channel conditions.

### Deployment challenges
- Heterogeneous hardware capabilities.
- Model update synchronization.
- Explainability for safety-critical domains.
- Compliance with low-power communication standards.

---

## 11) Future Scope

1. **Federated Learning**
   - Train distributed models without centralizing raw node data.
   - Improves privacy and scales to multi-sink deployments.

2. **Edge Computing Integration**
   - Push heavy inference and retraining to gateway edge nodes.
   - Keep ultra-light scoring at constrained sensors.

3. **Blockchain-Assisted Security**
   - Immutable trust/alert ledger for collaborative intrusion evidence.
   - Use lightweight consortium chain for cluster heads only.

4. **Real-Time IoT Deployment**
   - Port to Contiki-NG/RIOT on hardware testbeds.
   - Validate under real RF noise, interference, and duty-cycling.

---

## 12) Viva Preparation

### 10 common viva questions with confident answers
1. **Why is machine learning needed in WSN routing?**
   - Because routing is a multi-objective, dynamic optimization problem where fixed heuristics fail under changing energy/link/security conditions.

2. **Why not only use LEACH or PEGASIS?**
   - They improve energy under specific assumptions but are weak against adversarial behavior and do not adapt policy based on continuous feedback.

3. **What is your novelty?**
   - A hybrid supervised + RL route scorer integrated with trust-aware attack detection and policy-level mitigation.

4. **How did you avoid overfitting in ML training?**
   - Scenario-wise train/validation/test splits, cross-seed evaluation, feature importance checks, and robust metrics across attack densities.

5. **How is reward designed in RL?**
   - Weighted positive reward for successful low-delay delivery; penalties for high energy cost, drops, and risky/malicious path selection.

6. **How do you detect sinkhole attacks specifically?**
   - By identifying route-attraction anomalies (excessive route replies with unrealistically good metrics) plus forwarding inconsistency over time.

7. **What if trusted nodes suddenly become malicious?**
   - Trust is time-decayed and behavior-driven; abrupt deviations trigger rapid trust reduction and quarantine.

8. **What is the computational overhead?**
   - Inference complexity is bounded by neighbor count; training is sink-side. Runtime overhead is lower than repeated route discoveries in reactive protocols.

9. **How do you prove statistical significance?**
   - Multi-seed simulations, confidence intervals, and paired tests for key metrics against each baseline protocol.

10. **Can this be deployed in real IoT systems?**
   - Yes, via edge-assisted inference and lightweight on-node features; future work includes hardware validation and federated updates.

---

## 13) Resume & Interview Description

### Three resume bullet points
- Designed and implemented a hybrid ML-driven secure routing protocol for WSNs in NS-3, combining Random Forest route scoring with online Q-learning adaptation.
- Built an attack-aware trust engine to detect and mitigate Sybil, sinkhole, and selective forwarding attacks, improving packet delivery and network lifetime under adversarial scenarios.
- Conducted large-scale comparative evaluation against AODV, LEACH, and PEGASIS across multi-seed experiments, demonstrating measurable gains in PDR, delay, energy efficiency, and routing overhead.

### Interview paragraph
“In my final-year project, I developed an industry-grade routing framework for wireless sensor networks that jointly optimizes energy efficiency and security using machine learning. I created a simulation-to-ML pipeline in NS-3, engineered routing and trust features, trained a supervised model for next-hop quality prediction, and added reinforcement learning for online adaptation under changing network conditions. I also implemented attack detection for Sybil, sinkhole, and selective forwarding behaviors with automated mitigation through trust-based route exclusion. In comparative experiments against AODV, LEACH, and PEGASIS, the framework achieved higher packet delivery, lower delay, lower energy per packet, and longer network lifetime, making it suitable for publication extension and real IoT deployment pathways.”

---

## Suggested Implementation Milestones (Optional but recommended)
1. Week 1–2: Literature review and problem formalization.
2. Week 3–4: NS-3 baseline protocol setup and metrics pipeline.
3. Week 5–6: Dataset generation + feature engineering.
4. Week 7–8: Supervised model training + inference integration.
5. Week 9–10: RL adaptation integration.
6. Week 11: Attack module and trust engine.
7. Week 12: Comparative evaluation + statistical analysis.
8. Week 13: Thesis writing, diagrams, viva prep deck.

This structure is complete enough for a major project, strong viva defense, and extension into a conference/journal paper.
