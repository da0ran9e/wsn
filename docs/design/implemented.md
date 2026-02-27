# UAV Scenario Implementation Summary

## Implementation Status

### ✅ Phase 0: Cell Forming (IMPLEMENTED)

**Status**: Core implementation complete and tested
- Hex-grid cell coordinate mapping: ✅
- HELLO beacon protocol: ✅
- Fitness score calculation: ✅
- Cell Leader election: ✅
- Cell member feedback: ✅
- Intra-cell routing table: ✅

**Files Created**: 
- `src/wsn/model/uav/cell-forming-packet.h`: Packet structures (HELLO, CL_ANNOUNCEMENT, MEMBER_FEEDBACK)
- `src/wsn/model/uav/cell-forming.h`: Cell forming module interface
- `src/wsn/model/uav/cell-forming.cc`: Cell forming implementation (~600 lines)
- `src/wsn/examples/cell-forming-example.cc`: Standalone example

**Key Features**:
- Configurable `cellRadius`, `gridOffset`, timing parameters
- Automatic neighbor discovery via HELLO broadcasts
- Fitness-based CL election with tie-breaking
- State machine: DISCOVERING → ELECTED_CL/AWAITING_CL → CELL_FORMED → ROUTING_READY
- Callbacks for packet transmission and state changes

**Build Status**: ✅ Clean build (no errors/warnings)

**Testing**: ✅ Verified with 3x3 and 5x5 grids
- Nodes successfully form cells
- Cell Leaders elected per cell
- Neighbor discovery working
- HELLO broadcasts ongoing

---

### ✅ Phase 1: UAV Fragment Broadcasting (IMPLEMENTED)

#### 0.1 Cell coordinate mapping (hex grid)
Each node computes its cell using axial coordinates derived from position `(x,y)` and `cellRadius`:

- Fractional axial coords:
  - $q_f = (\sqrt{3}/3 \cdot x - 1/3 \cdot y) / cellRadius$
  - $r_f = (2/3 \cdot y) / cellRadius$
  - $s_f = -q_f - r_f$
- Round to nearest integer cube coords `(q,r,s)` with error correction:
  - If $|q-q_f|$ is largest: $q=-r-s$
  - Else if $|r-r_f|$ is largest: $r=-q-s$
  - Else: $s=-q-r$
- Cell ID: `cellId = q + r * grid_offset` (grid_offset = 100)
- Cell color (TDMA reuse): `color = ((q - r) % 3 + 3) % 3`

#### 0.2 Neighbor discovery (HELLO) 
- Nodes broadcast `HELLO` periodically with interval `helloInterval`.
- Each HELLO carries: `(nodeId, x, y, cellId, neighborList)`.
  - `neighborList` contains IDs and positions of all 1-hop neighbors (already discovered).
- Receiver inserts sender as neighbor only if distance ≤ `cellRadius`.
- Receiver also processes `neighborList` to build 2-hop neighbor information:
  - For each neighbor in sender's `neighborList`, receiver knows "sender's neighbor" (2-hop path).
  - 2-hop neighbors are stored separately but influence fitness comparison (see section 0.3).
- **Data structure per node**:
  - `neighbors`: direct 1-hop neighbors with `(nodeId, x, y, cellId, distance)`
  - `twoHopNeighbors`: indirect 2-hop neighbors with `(nodeId, viaNeighbor, cellId, distance)`

#### 0.3 Fitness score (per node, inside its cell)
- Compute cell center (based on `cellId`, using hex grid center formulas):
  - $center_x = cellRadius(\sqrt{3}q + \sqrt{3}/2 \cdot r)$
  - $center_y = cellRadius(3/2 \cdot r)$
- Node fitness: $fitness = 1/(1 + distance(node, center))$
- **Fitness comparison**: Determine if this node should become Cell Leader:
  - Collect all known nodes in the same `cellId` (from 1-hop and 2-hop neighbors).
  - For each candidate node:
    - Calculate that node's fitness (distance to shared cell center).
    - Use node's distance estimate (from HELLO exchange).
  - This node becomes CL candidate if its `fitness > max(neighbor.fitness, twoHopNeighbor.fitness)` in same cell.
  - Ties broken by `nodeId` (lower ID wins).
- **Election delay**: Higher fitness → shorter election delay (central nodes declare first).

#### 0.4 Cell Leader (CL) election timing
- If node’s fitness is better than best neighbor, it schedules a CL election:
  - `CL_ELECTION_TIMER`
- At timer: node becomes `CELL_LEADER`, sends `CL_ANNOUNCEMENT` to neighbors.

#### 0.5 CL announcement & confirmation
- CL sends `CL_ANNOUNCEMENT` to neighbors with:
  - `(x, y, cellId, fitnessScore)`
- Neighbors update their `clId` if announcement is better for their cell.

#### 0.6 Cell building (distributed feedback)
After CL announces itself (in 0.5), nodes in the cell provide feedback to help CL build a complete topology:

- **Node feedback (CL_MEMBER_ANNOUNCE)**:
  - Each non-CL node in the cell sends a packet to CL with:
    - `(nodeId, x, y, cellId, neighborList, twoHopNeighborList)`
    - Includes all neighbors discovered via HELLO (1-hop and 2-hop)
  - This gives CL full visibility of cell members and edges to neighboring cells.
  
- **CL topology integration**:
  - CL collects feedback from all cell members.
  - Merges all neighbor tables to get complete cell topology.
  - Identifies which members connect to which neighboring cells (via their neighbors).
  - Prepares for routing table computation (section 0.7).

- **Each node stores (local view)**:
  - `myCell`: own `cellId`
  - `myColor`: TDMA color (derived from q, r)
  - `clId`: Cell Leader ID for my cell (from CL_ANNOUNCEMENT)
  - `neighbors`: direct 1-hop neighbors with positions
  - `twoHopNeighbors`: indirect 2-hop neighbor info
  
- **Each CL stores (in addition)**:
  - `cellMembers`: all node IDs in its cell (collected from feedback)
  - `memberLocations`: positions of all members (for routing decisions)
  - `memberNeighbors`: merged neighbor table from all cell members
  - `neighboringCells`: list of adjacent `cellId` values (derived from merged neighbors)
  - `intraCell_RoutingTable`: table of next-hops per member (computed in section 0.7)

#### 0.7 Intra-cell routing table (CGW & member paths)
After collecting topology feedback, CL computes routing paths within the cell:

- **CGW selection per neighbor cell**:
  - For each neighboring `cellId`, CL finds members that have neighbors in that cell.
  - Among candidates, prefer node **closest to cell border** (minimize distance to border line).
  - Fallback: if multiple candidates, choose by lowest `nodeId`.
  - **Note**: Multiple neighboring cells may share the same CGW node (if beneficial). One node can gateway to multiple cells.

- **Intra-cell routing table**:
  - CL computes: for each cell member, what is the next-hop toward each neighboring cell.
  - Table format: `member_id → (neighbor_cell_id → next_hop_neighbor)`
  - If member is a CGW, next-hop leads to the inter-cell link.
  - If member is not a CGW, next-hop is a neighbor inside the cell (eventually reaching a CGW).
  - Includes both 1-hop and multi-hop paths computed via cell-local graph.

- **Distribution (optional)**:
  - CL can optionally broadcast this routing table to cell members (for efficiency).
  - Or: each node queries CL dynamically when needed (higher latency, lower overhead).
  - For now, assume **CL maintains table locally** for answering route queries.

#### 0.8 Next-hop routing (within cell and inter-cell)
Routing decisions combine cell-local and inter-cell forwarding:

**Within-cell routing**:
- Source in cell A sends to dest in cell A → forward directly (1-hop or multi-hop within A).
- Use cell neighbors as next-hops (HELLO-based neighbor list).

**Inter-cell routing**:
- Source in cell A, dest in cell B (different cells):
  1. Source determines path: A → B (may go through intermediate cells).
  2. Source sends packet to its cell's **CGW facing B** (from CGW_TABLE).
  3. CGW forwards to next cell's CGW (via inter-cell link).
  4. Repeat until reaching dest cell.
  5. Final CGW delivers to dest (intra-cell routing).
- **CGW selection at each hop**: Use CL's CGW_TABLE (who is the gateway to the next-cell in path).

**Path computation** (optional, can be pre-computed):
- Dijkstra on cell graph: nodes = cells, edges = neighboring cells.
- Distance = 1 per edge (unit distance) or based on cell load/quality.
- Result: sequence of `cellId` values from source cell to dest cell.

#### Phase 0 parameters (from Castalia logic)
- `cellRadius`: Distance within which nodes discover each other and form a cell
- `gridOffset`: Scaling factor in `cellId = q + r * gridOffset` (e.g., 100)
- `helloInterval`: Interval between HELLO beacon broadcasts (e.g., 1-2 seconds)
- `numberHelloIntervals`: How many HELLO intervals before full 1-hop and 2-hop neighbor discovery (e.g., 2-5)
- `clElectionDelayInterval`: Base delay for CL election (modified by fitness score)
  - Actual delay = `clElectionDelayInterval * (1 - fitness)` (lower fitness = longer delay)
- `clCalculationTime`: Delay after CL election before computing routing table (allow feedback collection)
- `clConfirmationTime`: Optional delay before CL distribution of intra-cell routing table

#### Phase 0 outcome
- **Each node has**: `cellId`, `color`, `clId`, `neighbors`, `twoHopNeighbors`.
- **Each cell (via CL) has**: 
  - `cellMembers` (all node IDs)
  - `memberLocations` (positions for routing decisions)
  - `intraCell_RoutingTable` (next-hop per member toward each neighbor cell)
  - `neighboringCells` (list of adjacent cell IDs)
- **Routing ready**: 
  - Intra-cell paths established (next-hop computed for each member)
  - CGWs identified (gateways per neighboring cell)
  - Ready for inter-cell relaying in Phase 1
- **Metrics**: Cell formation time, CL distribution, % nodes in valid cells, routing table completeness


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
