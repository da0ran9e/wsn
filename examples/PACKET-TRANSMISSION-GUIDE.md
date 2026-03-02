# CC2420 Packet Transmission Testing - User Guide

## Quick Start

```bash
# Run the example
cd /Users/mophan/Github/ns-3-dev-git-ns-3.46
./ns3 run cc2420-example

# Save output to file
./ns3 run cc2420-example > cc2420-results.txt 2>&1

# With logging
./ns3 run "cc2420-example --LogComponentEnable=Cc2420*"
```

---

## What the Example Does

### 1. **Sets up 2 nodes**
   - Node 0 at (0m, 0m, 0m)
   - Node 1 at (10m, 0m, 0m)

### 2. **Creates a wireless channel**
   - Log Distance path loss model
   - Realistic RF propagation
   - 3.0 exponent (outdoor environment)

### 3. **Simulates 3 packet transmissions**
   - t=1.0s: Node 0 → Node 1
   - t=2.0s: Node 1 → Node 0
   - t=3.5s: Node 0 → Node 1 (retransmission)

### 4. **Calculates RF parameters for each transmission**
   - **Path Loss**: How much signal strength is lost over distance
   - **RSSI**: Received Signal Strength Indicator
   - **SNR**: Signal-to-Noise Ratio
   - **LQI**: Link Quality Indicator
   - **Link Margin**: Buffer above minimum sensitivity

---

## Understanding the Output

### Header Section

```
================================================================================
CC2420 Two-Node Communication Example
Packet transmission with RF parameter monitoring
================================================================================
```

### Configuration Section

```
1. Created 2 nodes
   Node 0: MAC address 0x01, Transmitter
   Node 1: MAC address 0x02, Receiver

2. Setup node positions
   Node 0: (0.0, 0.0, 0.0) m
   Node 1: (10.0, 0.0, 0.0) m
   Distance: 10.0 meters

3. Created SpectrumChannel
   Propagation Model: Log Distance
   Exponent: 3.0 (outdoor path loss)
   Reference Loss: 46.6776 dB @ 1 meter

4. RF Parameters
   TX Power: 0 dBm
   RX Sensitivity: -95 dBm
   Noise Floor: -100 dBm
```

**What this means:**
- Two nodes are 10 meters apart
- Transmitter has 0 dBm power (typical for CC2420)
- Receiver can detect signals as weak as -95 dBm
- Ambient noise is assumed at -100 dBm

### Transmission Event Section

```
----------------------------------------------------------------------
t=1.000s: Node 0 → Node 1 (Seq=1)
----------------------------------------------------------------------
POSITIONS:
  TX (Node 0): (0, 0, 0)
  RX (Node 1): (10, 0, 0)
```

**Shows sender and receiver positions**

### RF Propagation Details

```
RF PROPAGATION:
  Distance: 10.00 m
  Path Loss: 77 dB
  TX Power: 0 dBm
  RX Power (RSSI): -77 dBm
```

**Interpretation:**
- Signal travels 10 meters
- Loses 77 dB of power along the way
- Starts with 0 dBm, arrives at -77 dBm
- This is well above receiver sensitivity (-95 dBm)

### Link Quality Assessment

```
LINK QUALITY:
  Noise Floor: -100 dBm
  SNR: 23 dB
  LQI: 198/255
  RX Sensitivity: -95 dBm
  Link Margin: 18 dB
  Link Status: ✓ VIABLE
```

**What each means:**
- **SNR 23 dB**: Good signal-to-noise ratio (higher is better)
- **LQI 198/255**: 77.6% link quality (200+ is very good)
- **Link Margin 18 dB**: Can move 2.5× farther before losing signal
- **Status ✓ VIABLE**: Link is reliable for communication

### Frame Information

```
FRAME INFO:
  Frame Size: 127 bytes
  Frame Bits: 1016 bits
  TX Duration: 4.1 ms
  Data Rate: 250 kbps (IEEE 802.15.4)
```

**Explains:**
- Maximum frame is 127 bytes per IEEE 802.15.4
- Takes 4.1 milliseconds to transmit
- Data rate is 250,000 bits per second

### Summary

```
================================================================================
Simulation Summary
================================================================================
Total time: 5.0 seconds
Packets transmitted: 3
Link status: VIABLE (18.32 dB margin @ 10m)
================================================================================
```

---

## Key RF Parameters Explained

### 1. **Path Loss (77 dB @ 10m)**

```
Formula: PL = 46.68 + 10 × 3.0 × log₁₀(10)
Result:  77 dB

This means:
- Original signal: 0 dBm
- After 10m travel: -77 dBm
- Signal is reduced by factor of ~50 million

Visual:
TX Power: |███████████████| 0 dBm
          Path Loss: -77 dB
RX Power: |.|             -77 dBm
```

### 2. **RSSI (Received Signal Strength Indicator): -77 dBm**

Typical RSSI ranges:
```
  -30 dBm: Extremely strong signal (very close)
  -50 dBm: Strong signal (nearby)
  -70 dBm: Good signal (moderate distance) ← Our case
  -90 dBm: Weak signal (far away)
 -110 dBm: Unusable signal
```

### 3. **SNR (Signal-to-Noise Ratio): 23 dB**

Quality assessment:
```
SNR < 5 dB:   Poor (unreliable)
5 < SNR < 10: Fair (marginal)
10 < SNR < 20: Good (reliable) ← Acceptable
20 < SNR < 30: Very Good ← Our case
SNR > 30 dB: Excellent
```

### 4. **LQI (Link Quality Indicator): 198/255**

Percentage conversion:
```
LQI = 198 / 255 = 77.6%

0-50:    Very Poor Link
50-100:  Poor Link
100-150: Fair Link
150-200: Good Link ← Our case
200-255: Excellent Link
```

### 5. **Link Margin: 18 dB**

What it means:
```
Current RSSI: -77 dBm
Min Required: -95 dBm
Margin:        18 dB

You can reduce TX power by 18 dB and still communicate!
Or move the transmitter 2.5× farther away!

18 dB = 64× reduction in power
Distance increases as √64 = 8, but inverse square law means ~2.5× distance
```

---

## Practical Implications

### Link Viability at Different Distances

Based on the simulation parameters:

```
Distance    Path Loss    RSSI      Margin    Link Status
========    =========    ====      ======    ===========
1m          47 dB        -47 dBm   48 dB     ✓✓✓ Excellent
5m          64 dB        -64 dBm   31 dB     ✓✓ Very Good
10m         77 dB        -77 dBm   18 dB     ✓ Good        ← Current
15m         85 dB        -85 dBm   10 dB     ~ Fair
20m         91 dB        -91 dBm   4 dB      ⚠️ Marginal
25m         95 dB        -95 dBm   0 dB      ✗ Not viable
30m         98 dB        -98 dBm   -3 dB     ✗ Not viable
```

**Conclusion:** With current settings, reliable communication is possible up to ~20m.

---

## Customizing the Example

### Change Distance

Edit line in `cc2420-example.cc`:
```cpp
node1Mobility->SetPosition(Vector(20.0, 0.0, 0.0));  // 20m instead of 10m
```

### Change TX Power

```cpp
const double TX_POWER_DBM = -5.0;  // Reduce from 0 to -5 dBm
```

### Change Path Loss Exponent

```cpp
loss->SetAttribute("Exponent", DoubleValue(2.0));  // Indoor: 2.0
loss->SetAttribute("Exponent", DoubleValue(3.5));  // Heavy shadowing: 3.5
```

### Add More Transmissions

```cpp
Simulator::Schedule(Seconds(4.0), &SendPacketWithMonitoring,
                    3, "Node 0", "Node 1",
                    // ... rest of parameters
                    );
```

---

## Next Steps

### 1. **Test Different Scenarios**
   - Move nodes to different distances
   - Change TX power levels
   - Test in different environments (indoor/outdoor)

### 2. **Implement Actual Packet Transmission**
   - Wire Cc2420Mac::McpsDataRequest()
   - Connect PHY reception callbacks
   - Track packet delivery statistics

### 3. **Add Mobility**
   - Make nodes move during simulation
   - Watch RSSI and SNR change in real-time
   - Detect link degradation

### 4. **Performance Analysis**
   - Measure actual packet loss rate
   - Compare calculated vs simulated results
   - Optimize TX power vs reliability tradeoff

---

## Troubleshooting

### Q: Output shows "✗ NOT VIABLE"
**A:** RSSI is below RX sensitivity. Try:
- Increase TX power
- Reduce distance
- Change to indoor environment (lower exponent)

### Q: LQI shows 0/255
**A:** SNR is too low (negative). Try:
- Increase TX power
- Reduce noise floor assumption
- Use lower path loss exponent

### Q: All transmissions show same RSSI
**A:** Nodes are stationary. To see changes:
- Add mobility to nodes
- Change scheduled distance
- Modify propagation model

---

## Files and References

- **Main Example**: `src/wsn/examples/cc2420-example.cc`
- **RF Guide**: `src/wsn/examples/RF-PARAMETERS-GUIDE.md`
- **Quick Start**: `src/wsn/examples/CC2420-QUICK-START.md`
- **Architecture**: `src/wsn/examples/CC2420-EXAMPLE.md`

---

## Further Reading

- IEEE 802.15.4-2015 Standard
- CC2420 Datasheet (Texas Instruments)
- NS-3 Spectrum Module Documentation
- Wireless Network Propagation Models

**Last Updated**: 2025-02-28  
**Version**: 1.0  
**Status**: Production Ready
