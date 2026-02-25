# Direction B: 6G Non-Terrestrial Network (NTN) Technologies

**Conference Track**: Special Session on 6G Non-Terrestrial Network Technologies Toward Global Coverage  
**Alignment**: ⭐⭐⭐⭐ Emerging/Hot Topic  
**Complexity**: Medium-High  
**Implementation Timeline**: 8-10 weeks

---

## 1. Core Concept

**Problem Statement**: Current idea focuses on UAV as delivery platform. This direction positions UAV as **6G air-RAN (Radio Access Network)** platform for gap-filling coverage in urban smart cities.

**6G Vision**: Next-generation wireless (6G) integrates multiple network layers:
- **Terrestrial**: Ground infrastructure (5G/6G base stations)
- **Non-Terrestrial (NTN)**: Air (UAVs, drones) + satellite + high-altitude platforms (HAP)
- **Integration**: Seamless handover, cooperative beamforming, network slicing

**New Framing**:
```
Traditional Smart City IoT:
├─ Fixed 5G base station (range ~500m, fixed location)
├─ Coverage gap in urban canyon/disaster areas
└─ Nodes fall back to WiFi mesh (limited bandwidth)

6G NTN Smart City:
├─ Macro layer: Fixed 5G/6G base stations (omnipresent)
├─ Air layer: UAV air-RAN (dynamic coverage)
├─ Satellite layer: LEO/GEO (emergency backup)
├─ Integration: UAV autonomously fills gaps
│   ├─ Recognizes coverage hole (via ground reports)
│   ├─ Deploys to area
│   ├─ Broadcasts person recognition data (primary application)
│   └─ Handover: Data transfer to ground as UAV leaves
└─ Benefit: Continuous coverage even during infrastructure failure
```

**Key Difference from Direction A**:
- A: Focuses on **inference architecture** (how to fuse decisions)
- B: Focuses on **network architecture** (how UAV integrates with ground infrastructure)

---

## 2. How This Affects the Core Idea

### 2.1 System-Level Integration

**Original Idea**: UAV is standalone broadcaster (doesn't interact with ground infrastructure beyond BS)

**6G NTN Version**: UAV is part of cellular network

```
Architectural Changes:
┌─────────────────────────────────────────┐
│ Network Slicing (6G capability)         │
├─────────────────────────────────────────┤
│ Slice 1: Person Recognition (Our app)   │
│ Slice 2: Regular IoT monitoring          │
│ Slice 3: Emergency services              │
└─────────────────────────────────────────┘
         ↓
┌─────────────────────────────────────────┐
│ RAN Coordination Layer (New)             │
├─────────────────────────────────────────┤
│ Ground RAN: 5G base stations (macro)     │
│ UAV RAN: Dynamic air coverage (micro)    │
│ Satellite RAN: Backup coverage (optional)│
└─────────────────────────────────────────┘
```

**Integration Point**: UAV broadcasts fragments on **cellular channel** (licensed or shared spectrum) instead of **proprietary 802.15.4**

### 2.2 Spectrum Choices

**Original**: 2.4 GHz ISM band (unlicensed, conflict with WiFi/Bluetooth)

**6G NTN Options**:

| Spectrum | Frequency | Bandwidth | Use Case | Pro | Con |
|----------|-----------|-----------|----------|-----|-----|
| **NR-U** | 5 GHz (unlicensed) | 20-160 MHz | LTE/5G unlicensed | Regulated, higher BW | Shared with WiFi |
| **mmWave** | 28/39/73 GHz | 200+ MHz | 5G/6G backhaul | Extreme BW | High path loss, LoS only |
| **Sub-6** | 2-6 GHz | 20-100 MHz | Cellular (5G bands) | Licensed, reliable | Requires license |
| **Terrestrial Broadcast** | 2-6 GHz | 5-20 MHz | DVB-T style | Simple, ubiquitous | Low BW |

**6G NTN Smart Choice**:
- **Uplink (Ground→UAV)**: Sub-6 GHz licensed band (coordinated with BS)
- **Downlink (UAV→Nodes)**: mmWave (28 GHz) for ultra-high bandwidth
- **Benefit**: 1-2 GHz throughput downlink → single fragment transmission in <1 ms

### 2.3 Standardization & Protocol

**Original**: Proprietary protocol (802.15.4 Layer 2 broadcast)

**6G NTN Version**: Align with 3GPP standards

```
3GPP NR-based UAV Communication:
├─ Uplink: UAV ↔ Ground BS (standard NR protocol)
├─ Downlink (UAV broadcast): Modified NR-D2D (device-to-device)
│   └─ Sidelink broadcast (PC5 interface)
│   └─ Nodes tune to sidelink channel
└─ Management: BS orchestrates UAV deployment, spectrum allocation
```

**Implication**: Nodes need **cellular modem** (not just 802.15.4), but benefits:
- Standardized handover (UAV → ground BS)
- Guaranteed QoS (network slicing)
- Easier deployment (vendor support)

### 2.4 Coverage & Deployment Analysis

**Question**: At what altitude does UAV provide maximum coverage?

**Propagation Model Changes**:

```
Ground-based BS coverage:
├─ Antenna height: 30-50 m
├─ Pattern: Omnidirectional horizontal
├─ Range: 500 m typical (urban)
├─ Limitation: Blocked by buildings

UAV air-RAN coverage:
├─ Antenna height: 50-500 m (UAV altitude)
├─ Pattern: Downward-looking (steerable)
├─ Range: 1-5 km typical (LoS better from air)
├─ Advantage: Above building blockage
```

**Mathematical Analysis**:

Free-space path loss:
$$PL(d, h) = 32.45 + 20\log_{10}(d) + 20\log_{10}(f)$$

Where:
- $d$ = horizontal distance (m)
- $f$ = frequency (MHz)
- $h$ = UAV altitude (m)

For practical urban scenario:
$$\text{Range}(h) \approx \sqrt{d^2 + h^2} \text{ (3D geometry)}$$

**Coverage Area as Function of Altitude**:

| UAV Altitude | Free-Space Range | Urban Range (with shadowing) | Coverage Area |
|--------------|------------------|------------------------------|---------------|
| **50 m** | 400-600 m | 150-250 m | 0.07 km² |
| **100 m** | 600-900 m | 250-400 m | 0.2 km² |
| **150 m** | 800-1200 m | 350-500 m | 0.4 km² |
| **250 m** | 1200-1800 m | 500-800 m | 1.0 km² |

**Deployment Optimization**:

Problem: Minimize number of UAVs to cover city
$$\min_{\{UAV_i\}} \left| \{UAV_i\} \right|$$
Subject to:
- Coverage: $\bigcup_i A_i(h_i) \supseteq \text{City}$ (union of coverage areas)
- Altitude: $h_i \leq h_{max}$ (regulatory limit)
- Overlap: $A_i \cap A_j$ allowed (cooperative beamforming)

**Solution**: Optimization via geometry (circle packing problem)

Example for 1 km² city:
- 50 m altitude: Need ~15 UAVs (0.07 km² each)
- 100 m altitude: Need ~5 UAVs (0.2 km² each)
- 200 m altitude: Need ~1 UAV (if within altitude limit)

**Implication**: Trade altitude vs number of UAVs

---

## 3. Implementation Approach

### 3.1 Phase 1: 6G Channel Modeling (Weeks 1-2)

**Goal**: Model UAV-to-ground propagation accurately (differs from fixed BS)

**Current Model** (in ns-3):
- Friis free-space formula (too idealistic)
- Urban fading (Nakagami-m model)
- Shadowing (log-normal model)

**6G NTN Enhancement**:

```python
# Air-to-ground channel model (ITU-R P.1410-series)

def air_to_ground_pathloss(distance_m, altitude_m, frequency_ghz, urban=True):
    """
    Improved model accounting for elevation angle & LoS probability
    """
    # 3D distance
    distance_3d = sqrt(distance_m**2 + altitude_m**2)
    
    # Elevation angle
    elevation_angle = arctan(altitude_m / distance_m)
    
    # LoS probability (increases with altitude)
    los_prob = 1.0 / (1.0 + 5.0 * exp(-0.7 * (elevation_angle - 5)))
    
    # Free-space (LoS)
    pl_los = 32.45 + 20*log10(distance_3d) + 20*log10(frequency_ghz)
    
    # Ground-bounce (NLoS)
    pl_nlos = pl_los + 30  # additional loss
    
    # Average path loss
    pl = los_prob * pl_los + (1 - los_prob) * pl_nlos
    
    if urban:
        pl += 8  # additional urban shadowing
    
    return pl
```

**Ns-3 Integration**:
```c++
// Add new propagation model
class AirToGroundPropagationModel : public PropagationModel {
  double GetLoss(Ptr<MobilityModel> a, Ptr<MobilityModel> b) override;
  // Check if 'a' is UAV (high altitude), 'b' is ground node
  // Apply air-to-ground model
};
```

**Validation**: Compare simulation results with measurements from literature (e.g., NASA drone experiments)

### 3.2 Phase 2: UAV Integration with Cellular Network (Weeks 2-3)

**Goal**: Simulate UAV as mini-base-station (air-RAN)

**Components**:

```c++
// UAV-RAN node (different from regular ground node)
class UAVCellularRAN : public NetDevice {
  // Cellular modem (transmit fragments as NR sidelink broadcast)
  void broadcastFragments(vector<Fragment> fragments) {
    // Use licensed spectrum (e.g., Band N41, 2.6 GHz)
    // Frame structure: slots, symbols (3GPP NR physical layer)
    transmitWithMCS(fragments, modulation_coding_scheme);
  }
  
  // Receive control signals from ground BS
  void receiveUplinkControl(ControlSignal cmd) {
    // BS tells UAV: new target, adjust power, frequency, etc.
  }
  
  // Measure link quality to nodes
  vector<float> estimateSNR(vector<NodeID> ground_nodes) {
    // Calculate SNR for each ground node (use air-to-ground model)
  }
};

// Ground BS (orchestrates UAV deployment)
class MobileNetworkControlNode : public NetDevice {
  void coordinateUAV(UAVCellularRAN* uav, Fragment* target_data) {
    // 1. Determine UAV deployment (altitude, location)
    // 2. Schedule fragment transmission
    // 3. Monitor coverage holes
    // 4. Request UAV relocation if needed
  }
  
  void handleCoverage Report(CoverageReport report) {
    // Nodes report signal strength, gaps
    // If gap found: instruct UAV to fill
  }
};
```

**Protocol Design**: UAV-BS coordination

```
Timeline:
t=0: BS detects coverage hole (via node reports)
t=10 ms: BS sends UAV placement command (altitude, waypoint)
t=50 ms: UAV confirms deployment (transmit person recognition data)
t=100-500 ms: UAV broadcasts fragments (persons in area)
t=600 ms: Nodes report detections back to BS
t=1000 ms: UAV relocates or returns (BS decision)
```

### 3.3 Phase 3: Spectrum Efficiency Analysis (Weeks 3-4)

**Goal**: Compare spectrum options for UAV broadcast

**Scenario Setup**:

```
Network:
├─ City area: 1 km²
├─ Ground infrastructure: 3 base stations (macro coverage)
├─ UAV: Single drone (variable altitude)
├─ Nodes: 50 static IoT devices (camera nodes)
├─ Target: Broadcast 20 fragments (total 10 MB)

Spectrum Options:
├─ Option 1: 2.4 GHz ISM (802.15.4, original idea)
│  └─ BW: 2 MHz, Data rate: 250 kbps
├─ Option 2: 5 GHz NR-U (unlicensed NR)
│  └─ BW: 80 MHz, Data rate: 100 Mbps
├─ Option 3: 28 GHz mmWave (5G/6G backhaul repurposed)
│  └─ BW: 200 MHz, Data rate: 1+ Gbps
└─ Option 4: Sub-6 GHz licensed (5G NR)
   └─ BW: 20 MHz, Data rate: 50 Mbps
```

**Comparison Metrics**:

| Metric | 802.15.4 ISM | NR-U 5GHz | mmWave 28GHz | NR Sub-6 |
|--------|-------------|----------|-------------|----------|
| **Data Rate** | 250 kbps | 100 Mbps | 1+ Gbps | 50 Mbps |
| **Fragment TX Time** (10 MB) | ~8 min | 0.8 s | 0.01 s | 1.6 s |
| **Propagation Range** | 150-250 m | 300-500 m | 50-100 m (LoS) | 400-800 m |
| **Regulation** | Unlicensed | Unlicensed | Licensed (backhaul) | Licensed |
| **Node Hardware** | 802.15.4 radio | Cellular modem | Specialized | Cellular modem |
| **Urban Blockage** | Moderate | Moderate | High | Low |
| **Coverage Reliability** | Medium | Medium-High | Low | High |

**Analysis**:

- **Time-to-detection**: mmWave fastest (0.01 s TX), but poorest coverage
- **Coverage probability**: Sub-6 best (reliability), but slower than NR-U
- **6G optimality**: NR-U 5GHz sweet spot (fast + reliable + unlicensed)

**Conclusion**: Switch from 802.15.4 ISM → **5 GHz NR-U** for 6G use case

### 3.4 Phase 4: 6G Network Slicing (Weeks 4-5)

**Goal**: Simulate multiple services sharing UAV capacity

**Concept**: 6G network slicing allows multiple logical networks on shared physical infrastructure

```
Slice 1: Person Recognition (URLLC - Ultra-Reliable Low-Latency)
├─ Latency requirement: <100 ms
├─ Reliability: >99.99%
└─ Broadcast fragments every 200 ms

Slice 2: Regular IoT Monitoring (eMBB - Enhanced Mobile Broadband)
├─ Latency requirement: <500 ms
├─ Reliability: >95%
└─ Data rate: 1 Mbps

Slice 3: Emergency Alert (URLLC)
├─ Latency requirement: <50 ms
├─ Priority: Override other slices if alert received
└─ Data rate: 10 kbps
```

**Implementation**: Resource allocation across slices

```python
class NetworkSliceManager:
  def allocate_uav_resources(self, slices):
    """
    Divide UAV transmission time among slices
    URLLC gets priority (interrupt eMBB if needed)
    """
    urllc_slices = [s for s in slices if s.reliability_target > 0.999]
    embb_slices = [s for s in slices if s.data_rate_requirement > 1000]
    
    # Allocate 70% to URLLC, 30% to eMBB (adaptive)
    for slice in urllc_slices:
      slice.allocated_time = 0.70 * total_time / len(urllc_slices)
    
    for slice in embb_slices:
      slice.allocated_time = 0.30 * total_time / len(embb_slices)
    
    return allocation
```

**Simulation Scenarios**:

- **Scenario 1**: Person recognition only (Slice 1)
  - Expected: Optimal performance (100% UAV resources)
- **Scenario 2**: Person recognition + IoT monitoring
  - Expected: Slight latency increase, resource sharing overhead ~5-10%
- **Scenario 3**: Add emergency alert (interrupt)
  - Expected: Alert gets priority, person recognition delays temporarily

### 3.5 Phase 5: Handover Between UAV & Ground BS (Weeks 5-6)

**Goal**: Seamless transition as UAV leaves coverage area

**Problem**: Fragments received from UAV, but UAV departs. Ground BS must take over fragment distribution.

```
Timeline (Extended Mission):
t=0: UAV enters cell, broadcasts fragments (alt 100m)
t=200 ms: UAV passes cell border
t=300 ms: UAV out of cell coverage (altitude too high)
t=400 ms: PROBLEM: Nodes didn't receive all fragments

Solution (Handover Protocol):
t=0-200 ms: UAV broadcasts fragments (Slice 1)
t=150 ms: UAV detects it's leaving → signals BS
t=200 ms: BS ground base station (macro coverage) starts filling gaps
t=200-400 ms: BS relays remaining fragments via ground link
t=400 ms: All fragments received, detection complete
```

**Handover Decision Logic**:

```python
def should_trigger_handover(uav_position, cell_coverage):
  """
  UAV checks: Can ground BS reach nodes?
  """
  uav_coverage = compute_uav_coverage(uav_position)
  bs_coverage = cell_coverage  # Ground base station
  
  if uav_coverage.shrinking() and bs_coverage.good():
    return True  # Handover to BS
  return False
```

**Handover Overhead**:
- Signaling: 10-20 ms
- Fragment retransmission delay: 50-100 ms
- **Total**: ~150 ms additional latency
- **Impact on $T_{detect}$**: +150 ms if handover needed

### 3.6 Phase 6: UAV Deployment Optimization (Weeks 6-8)

**Goal**: Determine optimal UAV fleet size, altitude, and deployment pattern

**Problem Formulation**:

$$\min_{N_{UAV}, h, \mathcal{L}} N_{UAV} \cdot \text{cost}(\text{UAV})$$

Subject to:
- Coverage: $\bigcup_i \text{CoverageArea}(h_i, \mathcal{L}_i) \supseteq \text{City}$
- Altitude: $h_i \leq h_{max}$ (regulatory)
- Detection time: $\mathbb{E}[T_{detect}] \leq T_{deadline}$
- Energy: Each UAV battery ≥ mission duration

**Optimization Approach**:

1. **Greedy Algorithm** (Fast, local optimum):
   - Start with 1 UAV at minimum altitude
   - If coverage < 100%: Add UAV at gap location
   - Repeat until full coverage

2. **Genetic Algorithm** (Better, slow):
   - Population: Deployment configurations
   - Fitness: Coverage + cost
   - Crossover/mutation to explore space

3. **Simulation-based Optimization** (ns-3):
   - For each deployment config, run detection scenario
   - Measure: $T_{detect}$, coverage, cost
   - Vary: Number of UAVs, altitude, locations

**Results Expected**:

```
City Coverage (1 km²):
├─ 1 UAV @200m altitude: 80% coverage, T_detect = 300 ms
├─ 2 UAVs @150m altitude: 95% coverage, T_detect = 280 ms
├─ 3 UAVs @100m altitude: 100% coverage, T_detect = 250 ms
└─ Optimization trade-off: More UAVs → better coverage + faster detection, but higher cost
```

---

## 4. Detailed Analysis by Component

### 4.1 Air-to-Ground Propagation Empirical Validation

**Data Sources**:
- NASA drone measurement campaigns (various frequencies)
- 3GPP NR aerial UE studies (simulation results)
- Commercial drone operators (5G mmWave tests)

**Key Findings to Incorporate**:
- Elevation angle effect (steep angles → better LoS)
- Frequency dependency (lower freq → better building penetration)
- Urban vs rural difference (canyon effect in cities)

**Simulation Validation**:
- Compare ns-3 model predictions vs. empirical data
- Adjust model parameters (shadowing margin, LoS probability)

### 4.2 Licensed vs Unlicensed Spectrum Trade-offs

**Licensed (Sub-6 GHz NR)**:
- Pros: Guaranteed interference-free, high reliability
- Cons: Requires spectrum license ($), coordination with operators
- 6G advantage: Likely deployed in disaster response scenarios (emergency spectrum access)

**Unlicensed (5 GHz NR-U)**:
- Pros: No license needed, available globally, high BW
- Cons: Interference with WiFi, requires Listen-Before-Talk protocol
- 6G advantage: Immediate deployment possible (no regulatory delay)

**Recommendation**: 
- **Simulation baseline**: NR-U (no licensing complexity)
- **Real deployment**: Negotiate licensed band with operator (e.g., spectrum sharing)

### 4.3 Scalability Analysis (Multiple UAVs)

**Challenges with Multiple UAVs**:

1. **Interference**: If UAVs broadcast on same channel → receiver confusion
   - Solution: Frequency division (each UAV on diff channel)
   - Trade-off: Fewer channels available per UAV

2. **Coordination**: Which UAV covers which area?
   - Solution: Centralized (BS decides) or distributed (UAVs negotiate)
   - Trade-off: Centralized simpler but single point of failure

3. **Handover Complexity**: Node switches from UAV1 → UAV2
   - Solution: Coordinated handover (both UAVs participate)
   - Trade-off: Additional protocol complexity

**Simulation Approach**:
- Baseline: 1 UAV (no interference)
- Scenario 1: 2 UAVs (same channel, measure interference)
- Scenario 2: 2 UAVs (different channels, measure gain)
- Scenario 3: 4 UAVs with centralized coordination

### 4.4 Regulatory & Legal Considerations

**Beyond Simulation, Important for Real Deployment**:

1. **Spectrum Licensing**:
   - Who grants license for UAV broadcast?
   - Can civilian drones use cellular bands?
   - 6G standards being developed by 3GPP (will clarify)

2. **Flight Regulations**:
   - Altitude limits vary by country (50-200m typical)
   - Urban airspace restrictions (safety zones, no-fly zones)
   - Insurance & liability

3. **Privacy Concerns**:
   - Person recognition triggers GDPR/local privacy laws
   - Should not broadcast to unrelated persons
   - Encryption, authorization needed

**6G NTN Perspective**:
- Standardization bodies (3GPP) working on drone communication rules
- Simulation should assume regulatory compliance (spectrum allocation, altitude limits)

---

## 5. Integration with Current Idea.md

**Modifications Needed**:

1. **Introduction/Motivation**:
   - Add "6G vision" context
   - Position person recognition as UAV-enabled application

2. **System Model**:
   - Expand network architecture diagram (add ground BS, multi-layer)
   - Spectrum choices section (replace 802.15.4 ISM with NR-U 5GHz discussion)

3. **Communications**:
   - Replace "Layer 2 802.15.4 broadcast" with "3GPP NR-D2D sidelink broadcast"
   - Add air-to-ground propagation model
   - Explain spectrum allocation

4. **Hardware**:
   - UAV: Add cellular modem (uplink to BS, control)
   - Nodes: Upgrade to cellular radio (instead of 802.15.4)
   - Base Station: Add RAN orchestration capability

5. **New Section**: "6G Network Integration"
   - Air-to-ground architecture
   - Handover protocol
   - Network slicing (person recognition slice + others)
   - Multi-UAV deployment optimization

---

## 6. Expected Contributions to Paper

### 6.1 Scientific Novelty
- First to formalize UAV-as-air-RAN for person recognition
- Air-to-ground propagation modeling for NTN
- Handover protocol for UAV-ground base station
- Network slicing application (sharing UAV resources)

### 6.2 Experimental Insights
- Altitude vs coverage trade-off quantified
- Spectrum efficiency comparison (ISM vs NR-U vs mmWave)
- Deployment optimization (number of UAVs needed)
- Handover latency impact on $T_{detect}$

### 6.3 Standardization Alignment
- Positions work in context of 3GPP NR standardization
- Addresses 6G NTN standardization gaps
- Practical guidance for 6G system design

---

## 7. Simulation Scenarios

### Scenario 1: Single UAV at Fixed Altitude
```
- UAV altitude: 100 m
- Spectrum: 5 GHz NR-U
- Nodes: 50 static cameras
- Expected: T_detect = 250-400 ms (compare to 802.15.4: 500-800 ms)
```

### Scenario 2: Altitude Sweep
```
- Vary UAV altitude: 50, 100, 150, 250 m
- Measure: Coverage area, T_detect, handover latency
- Expected: Tradeoff between coverage vs low-altitude better for targets
```

### Scenario 3: Multi-UAV Deployment
```
- Number of UAVs: 1, 2, 3, 4
- Spectrum: Divided among UAVs (avoid interference)
- Measure: Coverage, redundancy, cost
- Expected: Diminishing returns after 3 UAVs
```

### Scenario 4: Handover During Mission
```
- UAV starts mission, passes cell boundary
- Measure: Fragment reception rate during handover
- Expected: Small drop during 100-200 ms handover window
```

### Scenario 5: Network Slicing
```
- Slice 1 (person recognition): 70% resources
- Slice 2 (IoT monitoring): 30% resources
- Measure: Latency for each slice
- Expected: Slice 1 meets URLLC (<100 ms), Slice 2 eMBB (~500 ms)
```

---

## 8. Open Research Questions

1. **Optimal Altitude**: What altitude maximizes both coverage AND detection accuracy? (Higher altitude → broader coverage, but weaker signals)

2. **Spectrum Coexistence**: How to coordinate UAV spectrum with ground 5G without interference?

3. **Handover Timing**: When should UAV→ground handover occur? (Too early: gap, too late: interference)

4. **AI for Deployment**: Can ML predict optimal UAV placement given city topology?

5. **Reliability Guarantee**: How to ensure 99.99% reliability (6G requirement) with mobile UAVs?

---

## 9. Related Work Positioning

- **UAV Communications**: Build on [cite drone communication literature]
- **6G NTN**: Complement [cite 3GPP NTN studies]
- **Air-to-Ground Channels**: Leverage [cite propagation models]
- **Network Slicing**: Extend [cite 5G slicing work]

**Novelty**: First to apply 6G NTN slicing to time-critical person recognition

---

## 10. Timeline & Effort Estimate

| Phase | Duration | Effort | Deliverable |
|-------|----------|--------|-------------|
| Channel modeling | 2 weeks | High | Air-to-ground propagation model |
| Cellular network integration | 1 week | High | UAV-RAN + BS orchestration |
| Spectrum analysis | 1 week | Medium | Spectrum efficiency comparison |
| Network slicing | 1 week | Medium | Slice allocation + scenarios |
| Handover protocol | 1 week | Medium | Handover simulator |
| Deployment optimization | 2 weeks | High | Multi-UAV optimization |
| Paper writing | 2 weeks | High | 18-22 pages |

**Total**: 8-10 weeks

**Minimum for publication**: Phases 1-2 (3 weeks) = propagation model + spectrum efficiency analysis

---

## 11. Positioning vs Direction A

| Aspect | Direction A (Distributed AI) | Direction B (6G NTN) |
|--------|--------|---------|
| **Focus** | How to fuse decisions across tiers | How to integrate with cellular |
| **Problem** | Inference latency, cooperation | Coverage gaps, spectrum efficiency |
| **Novelty** | Multi-tier fusion algorithms | Air-ground architecture, NTN |
| **Complexity** | Medium (software-focused) | Medium-High (PHY layer) |
| **Conference Fit** | Special session (Distributed AI) | Special session (6G NTN) |
| **Implementation Effort** | 6-8 weeks | 8-10 weeks |
| **Can Combine?** | Yes, but 16-18 weeks | Both + shared simulation framework |

---
