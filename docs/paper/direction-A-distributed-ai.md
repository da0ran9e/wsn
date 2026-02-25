# Direction A: Distributed AI Across Edge-Cloud Continuum

**Conference Track**: Special Session on Distributed AI across Edge-Cloud Continuum  
**Alignment**: ⭐⭐⭐⭐ Perfect Match  
**Complexity**: Medium  
**Implementation Timeline**: 6-8 weeks

---

## 1. Core Concept

**Problem Statement**: Current idea assumes nodes make detection decisions independently, then cooperate. This direction reframes the entire system as a **3-tier distributed AI architecture** where inference decisions are coordinated across tiers.

**New Framing**:
```
Tier 1 (Local - Node Level):
├─ Edge inference: CNN on RPi 4 → local confidence $C_i(t)$
├─ Latency: ~50-100 ms per fragment
└─ Processing: Independent (unaware of other nodes)

Tier 2 (Cell Level - Cooperative):
├─ Cell Leader (CL) aggregates local confidences
├─ Fusion strategy: Bayesian, voting, or weighted ensemble
├─ Request routing: Which node requests which fragments?
├─ Latency: ~100-200 ms for intra-cell coordination
└─ Communication: Manifest exchange → resource allocation

Tier 3 (Cloud Level - Central Optimization):
├─ Base Station (BS) receives cell aggregates
├─ Global fusion: Combine signals from all cells
├─ UAV optimization: Adaptive path/schedule based on partial signals
├─ Latency: ~500-1000 ms (BS decision round-trip)
└─ Communication: Cell-level uplink (WiFi or LTE backhaul)
```

**Key Insight**: Information **flows upward** (node → cell → cloud) AND **decisions propagate downward** (cloud → cell → node updates)

---

## 2. How This Affects the Core Idea

### 2.1 Problem Formulation Changes

**Original (Current)**:
$$\min_{\mathcal{P}, \mathcal{S}} \mathbb{E}[T_{detect}]$$
- Single objective: minimize time to any successful detection
- Assumes each node acts independently after receiving fragments

**Distributed AI Version**:
$$\min_{\mathcal{P}, \mathcal{S}, \pi} \mathbb{E}[T_{detect}]$$
Subject to:
- $T_{edge}(C_i) \leq \tau_1$ - edge inference latency bound
- $T_{cell}(\text{fusion}) \leq \tau_2$ - cell coordination latency
- $T_{cloud}(\text{global}) \leq \tau_3$ - cloud decision latency
- $B_{cell \to cloud} \leq B_{backhaul}$ - bandwidth constraint (uplink aggregates)
- $\pi$ = cooperation policy (who aggregates at each tier)

**New Decision Variables**:
- $\pi^{(1)}$: Node-level inference policy (which model, when to process)
- $\pi^{(2)}$: Cell-level fusion policy (Bayesian vs voting vs weighted)
- $\pi^{(3)}$: Cloud-level adaptation (modify path based on partial signals)

### 2.2 Detection Model Evolution

**Original (Current)**:
- $C_i(t)$ = node $i$'s confidence from fragments received
- Threshold: $C_i > \theta_{high}$ → alert

**Distributed AI Version**:

```
Step 1 (Tier 1 - Local):
├─ Node i receives fragment f_j
├─ Run CNN inference: softmax output → p_ij = P(target | f_j, local_view_i)
├─ Bayesian update: C_i(t) ← combine p_ij with previous state
├─ Check threshold: if C_i > θ_local → broadcast "high confidence"

Step 2 (Tier 2 - Cell Level):
├─ CL receives broadcast from nodes with C_i > θ_local
├─ CL runs fusion algorithm:
│   └─ Option A (Voting): majority vote of high-confidence nodes
│   └─ Option B (Bayesian): global fusion C_cell = P(target | all local evidence)
│   └─ Option C (Weighted): weighted average by node reputation/SNR
├─ Cell confidence: C_cell(t)
├─ Check cell threshold: if C_cell > θ_cell → request uplink to BS

Step 3 (Tier 3 - Cloud Level):
├─ BS receives C_cell from multiple cells
├─ Global fusion: C_global(t) = aggregate(C_cell^(1), ..., C_cell^(m))
├─ Decision 1: Alert triggered? (if C_global > θ_global)
├─ Decision 2: Adapt UAV? (move to high-confidence cell next)
└─ Feedback: Send priority guidance back to CL (which nodes to focus on)
```

**New Confidence Hierarchy**:
- $C_i(t)$: Local (node-level), range [0, 1]
- $C_{cell}(t)$: Regional (cell-level), aggregated from nodes
- $C_{global}(t)$: Global (cloud-level), aggregated from cells
- **Final Alert**: triggered at any tier (whichever reaches $\theta$ first)

### 2.3 Latency Decomposition

**Original**: Total time = UAV flight time + fragment transmission + local processing

**Distributed AI**: Explicit latency budget per tier
$$T_{detect} = T_{UAV flight} + T_{TX} + T_{edge} + T_{cell} + T_{cloud}$$

Where:
- $T_{edge}$ = CNN inference on RPi 4 (50-100 ms per node)
- $T_{cell}$ = CL fusion + manifest exchange (100-200 ms)
- $T_{cloud}$ = BS global fusion + decision + feedback (500-1000 ms)

**Tradeoff**: Skip cloud tier → faster alert (no adaptation), but less optimal UAV path next

### 2.4 Cooperation Strategy Evolution

**Original**: Intra-cell only, reactive (nodes request fragments when $C_i < \theta$)

**Distributed AI**: Multi-tier, proactive + reactive
- **Tier 1**: Nodes broadcast "I have confident match" (minimal data)
- **Tier 2**: CL proactively sends high-value fragments to low-confidence nodes (before they request)
- **Tier 3**: BS sends guidance "focus on fragments related to [specific features]"

Example:
```
Timeline in Distributed AI:
t=0 ms: UAV broadcasts fragment f_1 (contains face)
t=50 ms: Node A, B, C receive & process (CNN inference)
t=100 ms: A and B reach C_i > 0.7 → broadcast confidence beacon
t=120 ms: CL receives beacons from A, B → C_cell = 0.65 (below threshold)
t=130 ms: CL calculates missing fragments: f_2, f_3 (body/clothing)
t=140 ms: CL requests f_2, f_3 from Node D which has them → relay via 802.15.4
t=200 ms: C_cell updated with f_2, f_3 evidence → C_cell now 0.85
t=210 ms: C_cell > θ_cell → send uplink to BS
t=220 ms: BS global fusion complete → ALERT
(vs. original: would wait for complete manifest syncing, ~400-500 ms)
```

---

## 3. Implementation Approach

### 3.1 Phase 1: Simulation Framework Setup (Weeks 1-2)

**Goal**: Modify ns-3 to support multi-tier architecture

**Components to Add**:

```c++
// Tier 1: Node-level inference module
class EdgeInferenceModule {
  float infer(Fragment f, LocalVideo video);  // CNN on fragment
  void updateConfidence(float inference_result);
  void broadcastBeacon(float confidence_level);  // Only if C_i > local_threshold
};

// Tier 2: Cell Leader fusion module
class CellFusionModule {
  float fuseConfidences(vector<float> node_confidences);  // Bayesian/voting/weighted
  vector<Fragment> prioritizeRequests(set<Fragment> missing);
  void aggregateManifests(vector<Manifest> node_manifests);
  void sendUplinkToBS(float cell_confidence, Metadata context);
};

// Tier 3: Base Station optimization module
class CloudOptimizationModule {
  float globalFusion(vector<float> cell_confidences);
  bool shouldAlert(float global_confidence);
  void adaptUAVPath(vector<float> cell_confidences_by_location);  // Adjust Piov
  void sendGuidance(CellID priority_cell, vector<FeatureType> focus_features);
};
```

**Integration Points**:
- Node-level: Hook into existing Bayesian update logic
- Cell-level: Add CL as special node with fusion algorithm
- Cloud-level: Add external observer (BS) with global state
- Latency injection: Explicit delays $\tau_1, \tau_2, \tau_3$ for each tier

### 3.2 Phase 2: Fusion Algorithms (Weeks 2-3)

Implement 3 fusion strategies at Tier 2 & 3:

**Algorithm 1: Bayesian Fusion** (Information-theoretic optimal)
```python
def bayesian_fusion(node_confidences, prior=0.5):
    """
    C_cell = P(target | evidence_node_1, evidence_node_2, ...)
    Assume independence (strong assumption, may not hold)
    """
    likelihood = 1.0
    for c_i in node_confidences:
        # Assume C_i = P(evidence_i | target), convert to likelihood ratio
        likelihood_ratio = (c_i / (1 - c_i)) if c_i > 0 else 1.0
        likelihood *= likelihood_ratio
    
    posterior = (likelihood * prior) / (likelihood * prior + (1-prior))
    return posterior
```

**Algorithm 2: Majority Voting** (Robust, simple)
```python
def voting_fusion(node_confidences, threshold=0.6):
    """
    Count nodes with high confidence, fusion by majority
    """
    high_conf = sum(1 for c in node_confidences if c > threshold)
    return high_conf / len(node_confidences)  # percentage voting
```

**Algorithm 3: Weighted Ensemble** (Adaptive)
```python
def weighted_fusion(node_confidences, weights=None):
    """
    Nodes with better SNR (higher link budget) get more weight
    weights[i] = quality_of_link_i or reputation_i
    """
    if weights is None:
        weights = [1.0] * len(node_confidences)
    
    weighted_sum = sum(c * w for c, w in zip(node_confidences, weights))
    total_weight = sum(weights)
    return weighted_sum / total_weight
```

**Simulation Comparison**:
- Metric: $T_{detect}$ for each algorithm
- Hypothesis: Bayesian fastest (optimal), voting most robust (noise tolerance)
- Analysis: Does fusion quality offset uplink latency?

### 3.3 Phase 3: Latency Analysis (Weeks 3-4)

**Detailed Latency Budgeting**:

```
Tier 1 (Node): T_edge = T_CNN + T_update
├─ T_CNN: 50-100 ms (RPi 4 MobileNet inference, empirical)
└─ T_update: 10-20 ms (Bayesian state update)

Tier 2 (Cell): T_cell = T_manifest_exchange + T_fusion + T_relay
├─ T_manifest_exchange: 50-100 ms (broadcast all nodes' fragment lists)
├─ T_fusion: 10-20 ms (aggregation computation)
└─ T_relay: 30-200 ms (request/response for missing fragments)

Tier 3 (Cloud): T_cloud = T_uplink + T_global_fusion + T_feedback
├─ T_uplink: 200-500 ms (backhaul latency to BS)
├─ T_global_fusion: 20-50 ms (aggregation across cells)
└─ T_feedback: 200-500 ms (downlink guidance)
```

**Trade-offs to Analyze**:
1. **Skip Tier 2 coordination**: $T_{detect}$ faster, but less accurate (nodes act independently)
2. **Skip Tier 3 optimization**: $T_{detect}$ faster, but UAV doesn't adapt
3. **Early termination at Tier 1**: Alert immediately when any node reaches threshold (no fusion)

**Simulation Scenarios**:
- Scenario 1: All tiers active (full distributed AI)
- Scenario 2: Tier 2 + 3 disabled (baseline node-only)
- Scenario 3: Tier 3 disabled (no BS feedback, only cell-level)
- Scenario 4: Tier 2 + 3 with optimized latency (aggressive scheduling)

### 3.4 Phase 4: Adaptive Policy Learning (Weeks 5-6)

**Goal**: Learn optimal fusion strategy and tier-skipping policy

**Approach**: Multi-Armed Bandit (MAB) or Reinforcement Learning (RL)

```python
# Simplified: MAB to learn which fusion algorithm best
# Arms: (Bayesian, Voting, Weighted)
# Reward: 1 if detection correct & fast, 0 otherwise

class MABFusionSelector:
    def __init__(self):
        self.arms = ['bayesian', 'voting', 'weighted']
        self.rewards = [0, 0, 0]
        self.pulls = [0, 0, 0]
    
    def select_arm(self):
        # Thompson sampling: select arm with highest expected reward
        return argmax([r / (p + 1) for r, p in zip(self.rewards, self.pulls)])
    
    def update(self, arm, success):
        self.rewards[arm] += success
        self.pulls[arm] += 1
```

**Scenario**: Simulate 100 detection missions, learn which fusion works best for given terrain/SNR profile

### 3.5 Phase 5: Federated Learning Potential (Weeks 6-8)

**Stretch Goal**: Distributed model training

```python
# Each cell CL trains local model on confidences observed
# Periodically aggregate weights at BS (federated learning)

def federated_update(global_model, cell_models, aggregation='FedAvg'):
    """
    After each detection mission:
    1. Each cell CL trains on local node confidences (gradient descent)
    2. BS collects updated weights from all CLs
    3. Average weights globally
    4. Broadcast new weights back to CLs
    
    Benefits: Adapt to local terrain/camera conditions
    """
    if aggregation == 'FedAvg':
        avg_weights = mean(w for w in cell_models)
    return avg_weights
```

---

## 4. Detailed Analysis by Component

### 4.1 Tier 1 (Local Node Inference)

**Question**: Should all nodes run CNN, or only a subset?

**Option A: All nodes**
- Pro: Maximum redundancy, robust to node failure
- Con: High compute cost, all RPi 4s running inference simultaneously → thermal throttle
- Latency: 50-100 ms per node (parallel, but cascade if one slow)

**Option B: Selected nodes** (based on camera coverage)
- Pro: Lower compute cost, faster (fewer nodes to wait for)
- Con: Miss detections in uncovered areas
- Latency: 30-50 ms (fewer nodes)

**Option C: Adaptive** (cluster nodes by coverage area, select one per cluster)
- Pro: Balanced cost/coverage
- Con: Need clustering algorithm pre-deployment
- Latency: 40-70 ms (cluster representative nodes)

**Recommendation for Simulation**:
- Baseline: All nodes (Option A) - simplest
- Sensitivity analysis: Vary number of active nodes → impact on $T_{detect}$ vs compute cost
- Plot: $T_{detect}$ vs node count (should be sublinear benefit after ~5 nodes)

### 4.2 Tier 2 (Cell-Level Fusion)

**Critical Decision**: Fusion algorithm choice significantly impacts accuracy AND latency

**Comparison Matrix**:

| Algorithm | Accuracy | Latency | Robustness | Computation |
|-----------|----------|---------|-----------|-------------|
| **Bayesian** | Highest (if model correct) | 20 ms | Medium (fails if correlated) | Low |
| **Voting** | Medium | 15 ms | High (majority rule) | Very Low |
| **Weighted** | Medium-High | 25 ms | High (can ignore outliers) | Low |

**Correlation Issue**: Fragments are likely **correlated** (same target → same appearance). Bayesian assumes independence → may overestimate confidence.

**Fix**: Use Chow-Liu tree or copula to model fragment correlation
- Benefit: More accurate fusion
- Cost: +50 ms computation (too expensive for real-time Tier 2)

**Recommendation**:
- **Tier 2**: Use Voting or Weighted (fast, robust)
- **Tier 3** (Cloud): Use Bayesian with correlation model (can afford compute)

### 4.3 Tier 3 (Cloud-Level Optimization & Feedback)

**New Capability**: BS can **adapt UAV path dynamically** based on partial signals

**Example Scenario**:
```
Initial plan: UAV visits cells [A, B, C, D] in order
Mid-mission (after cell A visited):
- C_cell^A = 0.3 (low confidence, target not in A)
- BS decision: Increase dwell time in cells B, C (high pedestrian density)
- New plan: Visit [A, B, B', C, D] with dwell_B doubled
```

**Implementation**: Modify Path Planning Problem 1
$$\mathcal{P}^* = \arg\min_{\mathcal{P}} \mathbb{E}[T_{detect}(\mathcal{P}, \text{feedback})]$$

Where feedback modifies:
- Cell visit order (prize-collecting TSP with dynamic prizes)
- Dwell time per cell (adaptive based on $C_{cell}$ history)
- Fragment scheduling order (prioritize high-value fragments for cells with low confidence)

**Feedback Loop Complexity**:
- Uplink latency: 200-500 ms (BS gets signal)
- Computation: 50-100 ms (BS decision)
- Downlink latency: 200-500 ms (UAV receives new plan)
- **Total**: ~500-1000 ms feedback loop
- **Implication**: Only viable to adapt every 2-3 fragments (otherwise outdated)

**Simulation Analysis**:
- Metric: benefit of dynamic adaptation vs uplink latency cost
- Hypothesis: Adaptation helps if target location uncertain; hurts if known
- Scenario: prior $P(\text{target in cell})$ varies → measure breakeven point

### 4.4 Communication Protocol Changes

**New Uplink Traffic**:
- Old: Nodes only request fragments (downlink-heavy)
- New: Nodes broadcast confidence beacons, CL aggregates & sends uplink

**Uplink Design**:
```
Packet format:
[Cell_ID: 2B][Confidence: 1B][Feature_Mask: 2B][Quality_Metrics: 3B]
= 8 bytes per update

Frequency: Every 200 ms (if confidence updated)
Per cell: ~5-10 nodes active
Bandwidth: ~10 nodes × 8 bytes / 200 ms = 0.4 kbps per cell
= negligible (well below WiFi/LTE)
```

**Backhaul Assumption**: LTE or WiFi connection from BS to CL (not bottleneck)

---

## 5. Integration with Current Idea.md

**Modifications Needed**:

1. **Problem Formulation Section**:
   - Add explicit latency variables $\tau_1, \tau_2, \tau_3$
   - Rewrite objective to include tier latencies
   - Define cooperation policy $\pi$ as decision variable

2. **Detection Model Section**:
   - Add Tier 2 fusion algorithms (voting, Bayesian, weighted)
   - Explain confidence hierarchy ($C_i$, $C_{cell}$, $C_{global}$)
   - Define $\theta_{local}$, $\theta_{cell}$, $\theta_{global}$ thresholds

3. **Phase Descriptions**:
   - Phase 0: Add CL initialization, cell structure formation
   - Phase 1: Include adaptive path policy as optimization output
   - Phase 2: Explicit UAV→node broadcast (no change needed)
   - Phase 3: Add Tier 2 aggregation timeline
   - Phase 4: Add Tier 3 uplink alert to BS

4. **New Section**: "3-Tier Distributed AI Architecture"
   - Diagram showing tier interactions
   - Latency budget table
   - Fusion algorithm comparison
   - Feedback loop description

---

## 6. Expected Contributions to Paper

### 6.1 Scientific Novelty
- **First to formalize** distributed AI architecture for UAV-IoT person recognition
- Multi-tier fusion with explicit latency decomposition
- Adaptive path optimization based on partial signals (cloud feedback)

### 6.2 Experimental Insights
- Fusion algorithm performance comparison (Bayesian vs voting)
- Latency-accuracy tradeoff quantified ($T_{detect}$ vs $P_D$)
- Scalability analysis (vs number of nodes/cells)
- Benefit of cloud adaptation (adaptive path vs fixed path)

### 6.3 Implementation Details
- ns-3 multi-tier architecture design
- Protocol specification (beacon format, uplink design)
- Adaptive policy learning (MAB/RL)

---

## 7. Simulation Scenarios

### Scenario 1: Baseline (No Distributed AI)
```
- All nodes act independently
- No cell-level fusion
- No cloud feedback
- Expected: T_detect ~500-800 ms
```

### Scenario 2: Full Distributed AI (Voting Fusion)
```
- Tier 1: All nodes run CNN
- Tier 2: CL votes across node confidences
- Tier 3: BS global fusion + adaptive path
- Expected: T_detect ~300-500 ms (significant improvement)
```

### Scenario 3: Bayesian Fusion (Optimal Theory)
```
- Same as Scenario 2, but Bayesian instead of voting
- Expected: T_detect ~250-450 ms (faster, but assumes independence)
```

### Scenario 4: Adaptive Path
```
- Compare Scenario 2 (fixed path pre-planned)
- vs adaptive path (BS modifies mid-mission)
- Expected benefit: ~50-100 ms savings if target location uncertain
```

### Scenario 5: Latency Sensitivity
```
- Vary τ_1, τ_2, τ_3 independently
- Measure T_detect for each
- Find: which tier latency is bottleneck?
```

---

## 8. Open Research Questions

1. **Fusion Under Correlation**: How to model fragment correlation in fusion? (Current work assumes independence)
2. **Tier Skipping Policy**: When is it worth skipping Tier 2 or 3? (Optimization problem)
3. **Optimal Latency Budget**: How to allocate latency budget across tiers? (Given constraint $\tau_1 + \tau_2 + \tau_3 \leq T_{max}$)
4. **Federated Learning Convergence**: Can cell models converge while target location changes? (Non-stationary environment)
5. **Scalability**: How does performance scale with 100+ cells? (Uplink bandwidth, cloud compute)

---

## 9. Related Work Positioning

- **Distributed Inference**: Extend [cite work on federated learning]
- **Multi-tier IoT**: Compare with [cite hierarchical IoT architecture papers]
- **Adaptive UAV**: Build on [cite UAV path adaptation literature]
- **Cooperative Detection**: Leverage [cite cooperative sensing theory]

**Novelty**: First to combine all 4 aspects for person recognition use case

---

## 10. Timeline & Effort Estimate

| Phase | Duration | Effort | Deliverable |
|-------|----------|--------|-------------|
| Setup ns-3 framework | 2 weeks | High | Multi-tier modules |
| Fusion algorithms | 1 week | Medium | 3 fusion implementations |
| Latency analysis | 1 week | High | Latency decomposition + scenarios |
| Adaptive policy | 2 weeks | High | MAB selector + RL (optional) |
| Federated learning | 2 weeks | Medium | Distributed training (optional) |
| Paper writing | 2 weeks | High | 15-20 pages |

**Total**: 6-8 weeks (excluding optional federated learning)

**Minimum for publication**: Phases 1-3 (4 weeks) = latency analysis + fusion comparison + simulation

---
