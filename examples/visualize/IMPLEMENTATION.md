# Scenario3 TXT Logger Implementation Summary

## What Was Added

### 1. **NetworkVisualLogger Class** (scenario3.cc, lines 31-115)
   - Manages file I/O for TXT log format
   - Methods:
     - `Open()`: Open log file
     - `Close()`: Close log file
     - `LogMeta(key, value)`: Write metadata
     - `LogNode(id, x, y, cell, leader, color)`: Write node info
     - `LogLink(a, b, kind)`: Write link info
     - `StartStep(t)`: Begin STEP entry
     - `AddSuspiciousCells(cells)`: Add suspicious cell IDs
     - `AddSuspiciousNodes(nodes)`: Add suspicious node IDs
     - `EndStep()`: Finalize STEP entry

### 2. **OnLogNetworkForVisualization Function** (scenario3.cc, lines 229-277)
   - Scheduled at t=completionTime+0.075s
   - Logs:
     - Metadata: scenario, gridSize, spacing, totalNodes, hexCellRadius
     - All nodes from g_globalNodeTopology with positions, cells, leaders, colors
     - All logical neighbor links
   - Called by ScheduleScenario3GlobalSetupPhaseCompletion

### 3. **Suspicious Region Logging** (scenario3.cc, lines 383-386)
   - Added to end of OnSelectSignalSourceLocation
   - Logs STEP with:
     - Timestamp (t)
     - All suspicious cell IDs
     - All suspicious node IDs
   - Creates timeline of region expansion

### 4. **Public API Functions** (scenario3.h + scenario3.cc)
   ```cpp
   void InitializeScenario3Visualizer(const std::string& filename = "network_log.txt");
   void CloseScenario3Visualizer();
   ```

### 5. **Example3 Integration** (example3.cc)
   - Line 77: `InitializeScenario3Visualizer("network_log.txt");`
   - Line 104: `CloseScenario3Visualizer();`
   - Ensures logger lifecycle matches simulation

## Output Files

All generated logs follow the TXT format:

```
META scenario=scenario3
META gridSize=5
META spacing=20
META totalNodes=25
META hexCellRadius=32
NODE id=0 x=0.0 y=0.0 cell=0 leader=0 color=0
NODE id=1 x=20.0 y=0.0 cell=0 leader=0 color=0
...
LINK a=0 b=1 kind=logical
LINK a=0 b=3 kind=logical
...
STEP t=6.10 suspiciousCells=1,49 suspiciousNodes=2,3,4,7,8,9
ENDSTEP
STEP t=6.11 suspiciousCells=1,49,95 suspiciousNodes=2,3,4,7,8,9,10,15
ENDSTEP
```

## File Locations

- **Logger class**: `/src/wsn/examples/scenarios/scenario3.cc` (lines 31-115)
- **Logging functions**: `/src/wsn/examples/scenarios/scenario3.cc` (lines 229-386)
- **Public API**: `/src/wsn/examples/scenarios/scenario3.h` (lines 240-249)
- **Integration**: `/src/wsn/examples/example3.cc` (lines 76-77, 104-105)
- **Visualizer**: `/src/wsn/examples/visualize/` (app.js, index.html, etc.)
- **Generated logs**: `/network_log.txt` (project root)

## Workflow

1. **Simulation runs**:
   ```bash
   ./ns3 run "example3 --gridSize=5 --simTime=7"
   ```

2. **Logger initializes** (example3.cc line 77):
   ```cpp
   InitializeScenario3Visualizer("network_log.txt");
   ```

3. **At t=6.075s**: OnLogNetworkForVisualization called
   - Outputs all nodes from g_globalNodeTopology
   - Outputs all logical links
   - Outputs metadata

4. **At t=6.1s**: OnSelectSignalSourceLocation called
   - Selects random signal source location
   - Expands suspicious region from selected cell
   - Logs STEP entries as region grows

5. **At simulation end**: CloseScenario3Visualizer called
   - File properly closed

6. **View in browser**:
   ```bash
   cp network_log.txt src/wsn/examples/visualize/
   cd src/wsn/examples/visualize
   python3 -m http.server 8080
   # open http://localhost:8080
   ```

## Testing

```bash
# Grid 3x3
./ns3 run "example3 --gridSize=3 --simTime=7"
# Output: 9 nodes, 3 cells, ~1KB log

# Grid 5x5
./ns3 run "example3 --gridSize=5 --simTime=7"
# Output: 25 nodes, 7 cells, ~3KB log

# Grid 7x7
./ns3 run "example3 --gridSize=7 --simTime=7"
# Output: 49 nodes, 12 cells, ~7KB log
```

## Integration Points

1. **g_globalNodeTopology**: Source of node data (global variable)
2. **GetNodesInCell()**: Helper to find nodes by cell
3. **GetNodeInfo()**: Helper to access node topology info
4. **suspicious region expansion**: Automatically logged per iteration
5. **Simulator timing**: All logs tied to simulation time

## Future Enhancements

- [ ] Real-time streaming via websocket
- [ ] Multi-step STEP entries (each expansion iteration)
- [ ] Intra-cell tree visualization
- [ ] Gateway information
- [ ] Node state (discovered, synchronized, etc.)
- [ ] Packet flow visualization
- [ ] Export to SVG/PNG
