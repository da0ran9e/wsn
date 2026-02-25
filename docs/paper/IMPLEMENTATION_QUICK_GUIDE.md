# Implementation Documentation Guide

**Quick Navigation for coding work**

### For **Code Builders** (AI Agents)

## 🔑 Key Numbers & Parameters (Copy-Paste Ready)

### Spatial
```
Deploy Area:           200m × 200m
Node Positions:        (50,50), (150,50), (50,150), (150,150)
UAV Waypoints:         (100,50)→(150,100)→(100,150)→(50,100)
UAV Height:            10m (constant)
UAV Speed:             5 m/s
```

### Timing
```
Simulation Duration:   120 seconds
Broadcast Interval:    0.5 seconds
Fragments per Loop:    102 (per ~51s cycle)
Expected Alert Time:   30–60 seconds into simulation
```

### Radio/Channel
```
TX Power:              0 dBm (1 mW)
RX Sensitivity:        -95 dBm (CC2420 spec)
Frequency:             2.4 GHz (WiFi/ZigBee ISM)
Path Loss Model:       Friis free-space
Typical Range:         ~100m
```

### Detection Thresholds
```
Confidence Threshold:  0.75 (alert when ≥75%)
Min Fragments:         5 (credibility check)
Base Confidence/Frag:  +0.10
Signal Strength Bonus: +0.10 (if > -85 dBm)
Sensor Diversity Bonus: +0.05 (new type)
Max per-Fragment:      0.25 (cap)
```

### Data Structures
```
Fragment Size:         44 bytes
Packet Size:           72 bytes (with UDP header)
Fragment ID:           0 to 255+ (uint32_t)
Sensor Types:          0 (Thermal), 1 (Motion), 2 (Acoustic)
```
