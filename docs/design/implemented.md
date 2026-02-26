# UAV Scenario Implementation Summary

## Version 0:

### Core Features
- **UAV flight path**: Single-pass waypoint path (North → East → South → West), no looping.
- **Broadcast model**: Periodic broadcast with configurable interval.
- **Fragmented file model**:
  - A file is partitioned into `n` fragments (`--numFragments`).
  - Fragment confidences are randomized but normalized to sum to **1.0**.
  - UAV broadcasts **one fragment per broadcast**, sequentially, then repeats from fragment 0.
- **Deduplication**: Ground nodes ignore already received fragment IDs.
- **Confidence accumulation**: Each received fragment contributes its `baseConfidence` directly; confidence saturates at 1.0.
- **Alert logic**: Alerts trigger once confidence reaches the threshold (default 0.75).

### Parameters
- `--gridSize`: N×N ground node grid size
- `--gridSpacing`: Distance between ground nodes (m)
- `--uavAltitude`: UAV flight altitude (m)
- `--uavSpeed`: UAV speed (m/s)
- `--interval`: Broadcast interval (s)
- `--duration`: Simulation duration (s)
- `--numFragments`: Number of fragments in the file

### Implementation Files
- **Fragment data**: src/wsn/model/uav/fragment.h
- **UAV MAC** (broadcast + fragment sequencing): src/wsn/model/uav/uav-mac.h, src/wsn/model/uav/uav-mac.cc
- **Ground node MAC** (deduplication + confidence): src/wsn/model/uav/ground-node-mac.h, src/wsn/model/uav/ground-node-mac.cc
- **Scenario example**: src/wsn/examples/uav-example.cc
- **Build integration**: src/wsn/CMakeLists.txt

### Runtime Behavior
- UAV broadcasts one fragment per interval.
- Ground nodes receive fragments if RSSI ≥ sensitivity and fragment ID is new.
- Confidence increases by fragment confidence contributions until reaching 1.0.
- Per-node stats include packets, fragment count, confidence, and alert state.

### Current Validation (Representative)
- Broadcast sequencing verified to be **one fragment per broadcast**.
- Fragment confidences normalized to **sum = 1.0**.
- Deduplication verified: repeated fragment IDs are ignored at nodes.
- Confidence varies by coverage (nodes closer to the UAV trajectory reach 1.0 sooner).
