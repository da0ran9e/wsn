# Scenario 4 - Clean Architecture Implementation

## Overview

Scenario 4 implements a clean, layered architecture for WSN simulation with:

- **Separated orchestration and routing layers**
- **Base station node with callback communication**
- **Ground node cooperation protocol**
- **UAV waypoint planning and fragment broadcast**

## Architecture

### Scenario Layer (Orchestration)
Located in `src/wsn/examples/scenarios/scenario4/`

- **scenario4-params.h**: Centralized parameters
- **scenario4-config.h/cc**: Configuration structures with validation
- **scenario4-api.h/cc**: Runner interface (Build, Schedule, Run)

### Routing Layer
Located in `src/wsn/model/routing/scenario4/`

- **helper/calc-utils**: Common computation utilities
- **packet-header**: Unified packet definitions
- **base-station-node/**: BS logic with callback communication
- **ground-node-routing/**: Ground node routing and cooperation
- **uav-node-routing/**: UAV routing and broadcast

## Usage

### Build

```bash
./ns3 configure --enable-examples
./ns3 build
```

### Run

```bash
# Default configuration
./ns3 run example4

# Custom parameters
./ns3 run "example4 --gridSize=15 --gridSpacing=50 --simTime=120 --numFragments=20"

# Multiple runs with different seeds
./ns3 run "example4 --seed=1 --runId=1"
./ns3 run "example4 --seed=2 --runId=2"
```

### Parameters

- `--gridSize`: Size of ground node grid (N x N) [default: 10]
- `--gridSpacing`: Spacing between nodes in meters [default: 40.0]
- `--simTime`: Total simulation time in seconds [default: 60.0]
- `--numFragments`: Number of file fragments [default: 10]
- `--seed`: Random seed for reproducibility [default: 42]
- `--runId`: Run ID for multiple executions [default: 1]

## Implementation Status

### Phase 1 (Current)
- ✅ Example4.cc entry point
- ✅ Centralized parameters
- ✅ Configuration with validation
- ✅ Helper utilities
- ✅ API runner skeleton
- ✅ Unified packet headers

### Phase 2 (TODO)
- ⏳ Base station node implementation
- ⏳ Ground node routing with cooperation
- ⏳ UAV node routing with broadcast
- ⏳ Event scheduler
- ⏳ Metrics collection
- ⏳ Visualization

### Phase 3 (Future)
- Control callback bus
- State machine
- DTO/Model layer
- Unit tests
- Advanced logging

## Design Principles

1. **Layer Separation**: Clear boundary between orchestration and routing
2. **Callback Communication**: BS uses callbacks instead of network packets for control plane
3. **Centralized Configuration**: All parameters in one place
4. **Validation First**: Fail-fast on invalid configuration
5. **Reproducibility**: Seed-based deterministic execution

## References

See [SCENARIO4_REBUILD_TREE.md](SCENARIO4_REBUILD_TREE.md) for detailed architecture design.
