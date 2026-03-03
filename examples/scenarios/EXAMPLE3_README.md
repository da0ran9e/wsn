# Example3: Multi-UAV Grid Scenario with Fragment-Based Confidence Tracking

## Overview

Example3 extends the Scenario1 (single-UAV) framework with support for **multiple UAVs** operating simultaneously over a ground sensor network grid. Each UAV follows an independent trajectory and broadcasts packets with unique fragment sets to ground nodes.

## Key Features

### Multi-UAV Support
- **Configurable number of UAVs**: Set `--numUavs=N` to deploy N UAVs (default: 2)
- **Independent trajectories**: Each UAV follows a different flight path:
  - **UAV 0**: Horizontal west-to-east (along grid center Y axis)
  - **UAV 1**: Vertical north-to-south (along grid center X axis)  
  - **UAV 2+**: Diagonal patterns with 45-degree angle spacing
- **Unique fragment sets**: Each UAV generates its own fragment set with unique IDs (based on UAV index)

### Fragment-Based Confidence Tracking
- **Per-UAV fragments**: Fragments are identified by `uavIndex * 1000 + fragmentId`
- **Deduplication**: Ground nodes track received fragment IDs and skip duplicates
- **Confidence accumulation**: Each unique fragment contributes its base confidence
- **Alert mechanism**: Nodes trigger alerts when confidence ≥ 0.75 threshold

### Network Architecture
- **CC2420 802.15.4 Radio Stack**: Full PHY/MAC layer implementation
- **Path loss propagation**: Log-distance model with configurable parameters
- **Fragmented packet headers**: Each packet carries sender identity, sensor type, and confidence

## File Structure

### Main Example
- **`example3.cc`**: Driver program for multi-UAV simulation
  - Parses command-line arguments
  - Sets up ground network
  - Creates multiple UAVs with fragments
  - Prints simulation summary and per-node statistics

### Scenario Setup
- **`scenarios/scenario3.h`**: Configuration structures and function declarations
- **`scenarios/scenario3.cc`**: Network initialization and UAV scheduling
  - `SetupScenario3Network()`: Creates grid nodes with CC2420 devices
  - `ScheduleScenario3UavFragmentTraffic()`: Creates UAV with trajectory + fragments
  - Multi-UAV trajectory calculation using waypoints

### Routing Layer  
Located in `model/routing/scenario3/`:

- **`ground-node-routing.h/cc`**: Ground node packet reception and statistics
  - Fragment deduplication and confidence accumulation
  - Alert triggering logic
  - Per-node statistics tracking
  
- **`uav-node-routing.h/cc`**: UAV broadcast scheduling and fragment generation
  - Per-UAV fragment management using maps indexed by UAV index
  - Periodic broadcast scheduling with recursive rescheduling
  - Fragment rotation across multiple UAVs
  
- **`fragment.h/cc`**: Network packet structure for fragments
  - Includes UAV source ID to track which UAV sent each fragment
  - Serializes/deserializes 32-byte network packets
  - Supports double-precision confidence values
  
- **`parameters.h`**: Configuration constants
  - Grid parameters (size, spacing, TX power, RX sensitivity)
  - UAV parameters (altitude, speed, broadcast interval)
  - Channel/propagation parameters (path loss, reference loss)
  - Timing parameters (simulation time, broadcast offsets)
  
- **`node-routing.h/cc`**: Compatibility wrapper delegating to ground/UAV routing

## Command-Line Parameters

```bash
./ns3 run "example3 [options]"
```

### Ground Network Parameters
- `--gridSize=N`: Grid size (N×N nodes, default: 10)
- `--spacing=METERS`: Distance between nodes in meters (default: 20)
- `--txPower=DBM`: TX power in dBm (default: 0)
- `--rxSensitivity=DBM`: RX sensitivity threshold in dBm (default: -95)

### Simulation Parameters
- `--simTime=SECONDS`: Simulation duration (default: 60)

### UAV Parameters  
- `--numUavs=N`: Number of UAVs to deploy (default: 2)
- `--uavAltitude=METERS`: UAV altitude above ground (default: 20)
- `--uavBroadcastInterval=SECONDS`: Time between broadcasts (default: 1)

### Fragment Parameters
- `--numFragments=N`: Fragments per UAV (default: 10)
- `--totalConfidence=VALUE`: Sum of all fragment confidences (default: 1.0)

## Example Usage

### Basic Multi-UAV Scenario (2 UAVs)
```bash
./ns3 run "example3 --simTime=30 --gridSize=10 --spacing=20"
```

### High-Density Coverage (3 UAVs, tight grid)
```bash
./ns3 run "example3 --gridSize=8 --spacing=10 --numUavs=3 --simTime=60"
```

### Parameter Tuning for Coverage
```bash
./ns3 run "example3 --gridSize=10 --spacing=20 --uavAltitude=20 --txPower=5 --numUavs=2"
```

### Extended Mission (longer flight, more broadcasts)
```bash
./ns3 run "example3 --simTime=90 --gridSize=15 --numUavs=3 --uavBroadcastInterval=0.5"
```

## Output Interpretation

### Initialization Output
```
UAV 0 network fragment 0: type=1 conf=0.0561
...
Generated 10 network fragments for UAV 0 with total confidence 1
UAV #1 periodic broadcasts scheduled: 37 broadcasts from t=1s to t=37s
```
Shows fragment set generation for each UAV with type distribution and confidence levels.

### Broadcast Events
```
t=5s Ground Node 10 RX from 00:1a size=64 RSSI=-85.8 dBm
```
Records each packet reception with timestamp, receiving node, source UAV, and RSSI.

### Alert Events
```
*** ALERT *** Ground Node 2 at t=16s | Confidence: 0.866 | Fragments: 7
```
Printed when a node reaches the 0.75 confidence threshold.

### Summary Statistics
```
=== Network Simulation Summary ===
Nodes: 25 ground + 2 UAVs = 27 total
Grid dimensions: 80m x 80m
Ground node RX: 200
UAV TX: 38
Total nodes alerted: 12 / 25
```

### Per-Node Confidence
```
Node 2: confidence=0.896801 fragments=8 [ALERTED]
Node 6: confidence=1 fragments=12 [ALERTED]
Node 10: confidence=0.840871 fragments=8 [ALERTED]
```
Shows final confidence, fragment count, and alert status for each node.

## Implementation Details

### Multi-UAV Fragment Management
Fragments are stored per-UAV using `std::map<uint32_t, std::vector<UavNetworkFragment>>`:
- Key: UAV index (0, 1, 2, ...)
- Value: Vector of fragment structures

Fragment IDs are globally unique by multiplying UAV index: `fragmentId = uavIndex * 1000 + local_id`

This ensures no collision between fragments from different UAVs.

### Trajectory Calculation
Each UAV calculates its flight path based on:
- Grid dimensions: `(gridSize - 1) × spacing`
- Path length: Distance from start to end waypoint
- Flight time: `pathLength / speed`
- Total time: `firstTxTime + flightTime + return_flight_time`

For non-horizontal/vertical paths, angles are spaced 45 degrees apart.

### Fragment Distribution
Fragments are randomly distributed across sensor types (0-3):
- Type 0: Thermal sensor
- Type 1: Visual sensor
- Type 2: Acoustic sensor
- Type 3: Motion sensor

Confidence values are normalized so each fragment's confidence = (random_value / sum) × total_confidence

### Confidence Accumulation
Ground nodes accumulate confidence with deduplication:
```cpp
if (new_fragment_id not in received_set) {
    received_set.insert(fragment_id)
    confidence = min(1.0, confidence + fragment_confidence)
    if (confidence >= 0.75 && !already_alerted) {
        trigger_alert()
    }
}
```

## Comparison: Example3 vs. Example2

| Feature | Example2 (Scenario1) | Example3 (Scenario3) |
|---------|---------------------|---------------------|
| UAVs | 1 | Configurable (default 2) |
| Trajectory | Horizontal only | Horizontal/Vertical/Diagonal |
| Fragment ID scheme | Sequential (0-9) | Per-UAV (0-999, 1000-1999, ...) |
| Simulation time | 8s default | 60s default |
| Fragment generation | Once at start | Per-UAV during setup |
| Routing layer | scenario1/ | scenario3/ |

## Performance Characteristics

### Compilation
- Build time: ~5-10 seconds (incremental)
- Library size: ~5MB (wsn shared library)
- Executable size: ~2MB (example3)

### Runtime
- 10×10 grid, 2 UAVs, 30s simulation: ~2-5 seconds
- 15×15 grid, 3 UAVs, 60s simulation: ~10-20 seconds
- Scales approximately O(n × m × t) where n=ground nodes, m=UAVs, t=time

### Network Load
- Typical scenario: 300-600 packets in 30 seconds
- Fragment coverage: 80-100% of nodes for optimized parameters
- Alert rate: 40-100% depending on altitude and TX power

## Troubleshooting

### Low Reception Rates
**Symptom**: Few nodes receiving packets, mostly RSSI > -95 dBm
**Solution**: 
- Increase `--txPower` to 5-10 dBm
- Increase `--uavAltitude` to 25-30 meters
- Reduce `--spacing` for denser grid

### No Alerts Triggered
**Symptom**: Nodes receive packets but no alerts
**Solution**:
- Increase `--numFragments` to 15-20
- Increase `--uavBroadcastInterval` precision or reduce spacing
- Check RSSI values in logs to ensure packets are being received

### UAVs Spawning at Same Location
**Symptom**: Multiple UAVs at same position early in simulation
**Expected behavior**: Each UAV starts at different edge of grid based on trajectory direction

### Unexpected Fragment IDs in Output
**Debug info**: Fragment IDs should be unique per UAV:
- UAV 0: 0-999
- UAV 1: 1000-1999
- UAV 2: 2000-2999

If you see unexpected collision, check the fragment generation seed in `uav-node-routing.cc`.

## Extensions and Future Work

Potential enhancements:
1. **Cooperative UAVs**: Have UAVs communicate their positions for coordinated coverage
2. **Dynamic routing**: Ground nodes forward fragments to neighbors
3. **Energy modeling**: Track battery consumption per node
4. **Interference modeling**: Simulate packet collisions on shared channel
5. **Persistence**: Save results to file for offline analysis
6. **Visualization**: Generate mobility trace files for NetAnim

## References

- **NS-3 Spectrum Module**: CC2420 device implementation
- **WaypointMobilityModel**: UAV trajectory simulation
- **Scenario1 Documentation**: Single-UAV baseline
- **CC2420 Datasheet**: IEEE 802.15.4 radio specifications
