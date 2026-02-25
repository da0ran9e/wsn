# Implementation & Simulation Guide: UAV-IoT Fragment Detection

**For AI Coding Agents**: This document provides precise, actionable specifications for implementing a UAV-assisted person detection system in ns-3. It includes exact file paths, class signatures, data structures, algorithms, and testing procedures.

---

## 1. PROJECT STRUCTURE & BUILD SETUP

### 1.1 Directory Organization

```
wsn/
├── model/uav/                            
│   ├── fragment.h                    # Fragment data structure
│   ├── ground-node-mac.h             # Camera sensor node base class
│   ├── ground-node-mac.cc
│   ├── uav-mac.h                     # UAV mobility & broadcast
│   ├── uav-mac.cc
│   ├── uav-wireless-channel.h            # Channel model (Friis path loss)
│   └── uav-wireless-channel.cc
│
└── examples/                         
    └── uav-example.csv                     
```

### 1.2 Build Instructions

```bash
# From ns-3 root
cd /Users/mophan/Github/ns-3-dev-git-ns-3.46

# Configure (first time)
./ns3 configure --enable-examples --enable-modules=wsn

# Build
./ns3 build

# Run simulation
./ns3 run uav-example

# Clean rebuild if needed
./ns3 clean
./ns3 build
```
## 3. COMMUNICATION MODEL & CHANNEL

### 3.1 Packet Format (UDP-based)

```
┌──────────────┬─────────────────────────────────┐
│   UDP Header │   Fragment Payload (44 bytes)   │
│   (28 bytes) │                                 │
├──────────────┼─────────────────────────────────┤
│ Src/Dst Port │ fragmentId (4B)                 │
│ Length       │ sensorType (4B)                 │
│ Checksum     │ baseConfidence (8B)             │
│              │ broadcastPosition X/Y/Z (12B)  │
│              │ timestamp (8B)                  │
│              │ txPowerDbm (4B)                 │
└──────────────┴─────────────────────────────────┘
Total: 72 bytes per packet
```

### 3.2 Reception Model (Friis Free-Space)

```cpp
// Path loss formula (Friis)
double pathLoss_dB = 20 * log10(4 * π * distance / wavelength);
double rxPower_dBm = txPower_dBm - pathLoss_dB;

// Parameters
- TX Power: 0 dBm (1 mW)
- Frequency: 2.4 GHz (WiFi/ZigBee ISM band)
- Wavelength: ~0.125 m
- Typical Range: ~100m (free space)
- RX Sensitivity: -95 dBm (CC2420 standard)

// Reception threshold
if (rxPower_dBm >= -95.0) → Successful RX
else → Packet lost
```

### 3.3 Broadcast Mechanism

```cpp
// Pseudocode: UAV broadcast loop
void UAV::Update(double deltaTime) {
    // 1. Move along waypoints
    UpdatePosition(deltaTime);
    
    // 2. Periodic broadcast
    if (timeSinceLastBroadcast >= broadcastInterval) {
        Fragment frag;
        frag.fragmentId = m_fragmentCounter++;
        frag.sensorType = rand() % 3;  // Random type
        frag.baseConfidence = 0.5 + random(-0.2, +0.2);
        frag.broadcastPosition = GetPosition();
        frag.timestamp = Simulator::Now().GetNanoSeconds();
        frag.txPowerDbm = 0;
        
        // Broadcast to all ground nodes
        for (GroundNode* node : m_groundNodes) {
            if (node->CanReceive(GetPosition(), frag.txPowerDbm)) {
                node->ReceiveFragment(frag);
            }
        }
        
        timeSinceLastBroadcast = 0.0;
    }
}
```

---

## 4. DETECTION & RECOGNITION LOGIC

### 4.1 Confidence Accumulation Algorithm

**Location**: `model/recognition-node.cc`

```cpp
void RecognitionNode::ProcessFragment(const Fragment& frag) {
    // 1. Receive and store
    ReceiveFragment(frag);
    m_fragmentsProcessed++;
    
    // 2. Evaluate confidence contribution
    double delta = EvaluateConfidenceFromFragment(frag);
    
    // 3. Accumulate
    m_confidence = std::min(m_confidence + delta, 1.0);
    
    // 4. Log
    NS_LOG_INFO("Node " << m_nodeId << " processed frag " 
        << frag.fragmentId << " | Confidence: " << m_confidence);
    
    // 5. Check alert
    if (m_confidence >= m_confidenceThreshold && !m_alerted) {
        m_alerted = true;
        NS_LOG_WARN("ALERT at t=" << Simulator::Now().GetSeconds() 
            << "s, node " << m_nodeId);
    }
}

double RecognitionNode::EvaluateConfidenceFromFragment(
    const Fragment& frag) {
    
    // Base: +0.10 per fragment (simple linear)
    double base = 0.10;
    
    // Bonus: +0.10 if signal is strong
    double signal = CalcSignalStrength(frag.broadcastPosition, 
                                       frag.txPowerDbm);
    double bonus_strength = (signal > -85.0) ? 0.10 : 0.00;
    
    // Bonus: +0.05 if sensor type is new
    std::set<uint32_t> types_seen;
    for (const auto& prev : GetReceivedFragments()) {
        types_seen.insert(prev.sensorType);
    }
    double bonus_diversity = (types_seen.count(frag.sensorType) == 0) 
                              ? 0.05 : 0.00;
    
    double total = base + bonus_strength + bonus_diversity;
    return std::min(total, 0.25);  // Cap per-fragment
}
```

### 4.2 Alert Threshold Logic

```cpp
bool RecognitionNode::CheckAlertCondition() {
    return (m_confidence >= m_confidenceThreshold) 
        && (m_fragmentsProcessed >= 5);  // Min fragments
}
```

---

## 5. SCENARIO SPECIFICATION

### 5.1 Network Topology

```
Deploy Area: 200m × 200m

Ground Nodes (4):
  N0 (50, 50)   — Sensor node
  N1 (150, 50)  — Recognizer (ACTIVE detector)
  N2 (50, 150)  — Sensor node
  N3 (150, 150) — Sensor node

Base Station:
  BS (100, 100) — Logging/metrics collection (passive)

UAV:
  Start: (100, 50, 10m height)
  Speed: 5 m/s
  Broadcast Interval: 0.5s
```

### 5.2 UAV Flight Path (Circular)

```cpp
std::vector<Vector3D> waypoints = {
    Vector3D(100, 50, 10),      // North
    Vector3D(150, 100, 10),     // East
    Vector3D(100, 150, 10),     // South
    Vector3D(50, 100, 10),      // West
};

// Total perimeter: ~256m at 5 m/s = ~51s per loop
// Broadcast every 0.5s ≈ 102 fragments per loop
```

### 5.3 Simulation Timeline

| Phase | Duration | Events |
|-------|----------|--------|
| **Init** | 0.0–1.0s | Nodes boot, UAV at start position |
| **Detection** | 1.0–120.0s | UAV circulates, broadcasts fragments |
| **Alert** | ~30–60s | RecognitionNode confidence ≥ 0.75 |
| **Cooldown** | 60.1–120.0s | Continue measurement |
| **End** | 120.0s | Simulation stops |

### 5.4 Simulation Parameters

```cpp
simulationDuration = 120.0;  // seconds
uavBroadcastInterval = 0.5;  // seconds
txPowerDbm = 0;              // dBm
rxSensitivity = -95.0;       // dBm
confidenceThreshold = 0.75;  // Alert threshold
minFragmentsForAlert = 5;    // Credibility
uavSpeed = 5.0;              // m/s
```