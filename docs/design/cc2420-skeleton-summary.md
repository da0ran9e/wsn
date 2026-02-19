# CC2420 v1.0 Implementation - Skeleton Files Created

## Summary

Successfully created complete skeleton structure for CC2420 radio implementation in ns-3 WSN module.

**Structure Refined**: Original 7-component design optimized to 5 core components by merging StateMachine into PHY and ChannelAdapter into PHY (following SpectrumPhy pattern).

---

## File Organization

### 1. Core Components (src/wsn/model/radio/cc2420/)

#### **cc2420-header.h** / **cc2420-header.cc**
- **Purpose**: MAC frame header (FCF, DSN, PAN ID, addresses)
- **Inheritance**: `Header`
- **Key Methods**:
  - Frame type, security, ACK request accessors
  - PAN ID and address getters/setters
  - Serialize/Deserialize for packet processing
- **Simplified Format** (v1.0): 11 bytes (no full IEEE 802.15.4 beacon support)

#### **cc2420-trailer.h** / **cc2420-trailer.cc**
- **Purpose**: MAC frame trailer (FCS, LQI)
- **Inheritance**: `Trailer`
- **Key Methods**:
  - Frame Check Sequence (2 bytes)
  - Link Quality Indicator (1 byte, on reception)

#### **cc2420-phy.h** / **cc2420-phy.cc**
- **Purpose**: Physical layer with 6-state machine
- **Inheritance**: `SpectrumPhy` (for SpectrumChannel integration)
- **Key Components**:
  - **6 PHY States**: SLEEP (1.4mW) → IDLE (62mW) → RX (62mW) → TX (29-57mW) → CCA (62mW) → SWITCHING
  - **Signal Reception** (Castalia-style): `ReceivedSignal` list tracking
  - **SIMPLE_COLLISION_MODEL**: Binary collision (interference > sensitivity - 6dB = fatal)
  - **Callbacks**: `PdDataIndication`, `PdDataConfirm`, `PlmeCcaConfirm`, `StateChange`
- **Key Methods**:
  - `TransmitPacket()` - TX request
  - `SetState()` / `GetState()` - State machine
  - `PerformCCA()` - Clear Channel Assessment
  - `GetRSSI()` - Received Signal Strength Indicator

#### **cc2420-mac.h** / **cc2420-mac.cc**
- **Purpose**: MAC layer with unslotted CSMA-CA
- **Inheritance**: `Object`
- **Key Components**:
  - **MAC States**: MAC_IDLE, MAC_CSMA_BACKOFF, MAC_CCA, MAC_SENDING, MAC_ACK_PENDING, MAC_FRAME_RECEPTION
  - **Unslotted CSMA-CA** (v1.0): No beacon, no superframe
  - **Parameters**: macMinBE (3), macMaxBE (5), macMaxCSMABackoffs (4), macMaxFrameRetries (3)
  - **TX Queue**: std::queue for buffered packets
- **Key Methods**:
  - `McpsDataRequest()` - Data transmission request
  - `StartCSMACA()` / `BackoffExpired()` - Backoff algorithm
  - `DoCCA()` / `HandleCCAResult()` - Channel assessment
  - `AttemptTransmission()` - Send current packet

#### **cc2420-net-device.h** / **cc2420-net-device.cc**
- **Purpose**: Standard NetDevice wrapper
- **Inheritance**: `NetDevice`
- **Key Responsibilities**:
  - Component assembly (MAC + PHY)
  - Upper layer integration (ReceiveCallback)
  - Packet transmission entry point
- **Key Methods**:
  - `SetMac()` / `GetMac()`
  - `SetPhy()` / `GetPhy()`
  - `Send()` - NetDevice::Send() override
  - Address management

#### **cc2420-energy-model.h** / **cc2420-energy-model.cc**
- **Purpose**: State-based energy consumption tracking
- **Inheritance**: `DeviceEnergyModel`
- **Key Components**:
  - **PowerConfig**: Castalia CC2420 power values
    - Sleep: 1.4 mW
    - Idle/RX: 62 mW
    - TX: 8 levels (29.04 - 57.42 mW)
    - CCA: 62 mW
  - **State Transition Delays**: SLEEP→RX (0.05ms), RX↔TX (0.01ms), etc.
  - **Energy Tracking**: Accumulate per-state power consumption over time
- **Key Methods**:
  - `HandlePhyStateChange()` - Callback from PHY on state transition
  - `GetTotalEnergyConsumption()` - Return accumulated energy (J)
  - `UpdateEnergyConsumption()` - Calculate energy for elapsed state duration

---

### 2. Helper Class (src/wsn/helper/)

#### **cc2420-helper.h** / **cc2420-helper.cc**
- **Purpose**: Easy installation of CC2420 devices on nodes
- **Key Methods**:
  - `SetChannel()` - Attach to SpectrumChannel
  - `SetMacAttribute()` / `SetPhyAttribute()` / `SetEnergyAttribute()` - Configure components
  - `Install(NodeContainer)` - Batch installation
  - `Install(Ptr<Node>)` - Single node installation
- **Responsible for**:
  - Creating MAC + PHY + EnergyModel instances
  - Wiring callbacks and references
  - Attaching to nodes
  - Address allocation

---

### 3. Test Suite (src/wsn/test/)

#### **cc2420-phy-test.cc**
- Test stubs for:
  - State transitions
  - CCA operation
  - TX/RX operations
  - SIMPLE_COLLISION_MODEL

#### **cc2420-mac-test.cc**
- Test stubs for:
  - Frame transmission
  - Frame reception
  - CSMA-CA backoff
  - ACK handling
  - Retry logic

#### **cc2420-energy-test.cc**
- Test stubs for:
  - Power consumption per state
  - State transition energy cost
  - Energy depletion detection
  - TX power level impact

---

## Key Design Decisions

### 1. **5 Core Components**
| Component | Inheritance | Role |
|-----------|-------------|------|
| Cc2420Phy | SpectrumPhy | PHY layer + state machine + signal reception |
| Cc2420Mac | Object | MAC layer + unslotted CSMA-CA |
| Cc2420NetDevice | NetDevice | Device wrapper + upper layer interface |
| Cc2420Header | Header | MAC frame format |
| Cc2420Trailer | Trailer | CRC + LQI |
| Cc2420EnergyModel | DeviceEnergyModel | Energy consumption tracking |

### 2. **Castalia Integration**
- **6 PHY States** (SLEEP/IDLE/RX/TX/CCA/SWITCHING) from Castalia radio.h
- **CC2420 Power Values** (Castalia specs) hardcoded in PowerConfig
- **SIMPLE_COLLISION_MODEL** logic: interference > (sensitivity - 6dB) → packet destroyed
- **ReceivedSignal tracking** for interference calculation
- **Unslotted CSMA-CA** parameters from Castalia Basic802154.h

### 3. **LR-WPAN Pattern**
- SpectrumPhy integration (like LrWpanPhy)
- NetDevice composition pattern
- Callback-based inter-layer communication (no OMNeT++ messages)
- TypeId attribute system for configuration

### 4. **Energy Model**
- State-based (6 states) tracking per Castalia
- Tracks energy consumption on each state transition
- Reports to ns-3 EnergySource for depletion detection
- No separate transitions modeled (merged into states for v1.0)

---

## Implementation Roadmap

### Phase 1: Frame Format ✅
- [ ] cc2420-header.cc Serialize/Deserialize
- [ ] cc2420-trailer.cc Serialize/Deserialize

### Phase 2: PHY Layer
- [ ] State machine implementation (transitions, delays)
- [ ] SIMPLE_COLLISION_MODEL logic
- [ ] Signal reception processing (ReceivedSignal list)
- [ ] CCA implementation
- [ ] SpectrumPhy integration

### Phase 3: Energy Model
- [ ] State entry/exit tracking
- [ ] Power consumption calculation
- [ ] EnergySource integration
- [ ] Energy depletion handling

### Phase 4: MAC Layer
- [ ] CSMA-CA backoff algorithm
- [ ] TX/RX queue management
- [ ] ACK handling (simplified)
- [ ] Frame formatting (header + payload + trailer)
- [ ] Retry logic

### Phase 5: NetDevice Integration
- [ ] Callback wiring (PHY↔MAC↔NetDevice)
- [ ] Packet reception upward
- [ ] Address resolution
- [ ] Link status

### Phase 6: Testing
- [ ] Unit tests for each component
- [ ] Integration tests
- [ ] Example scenarios

---

## File Statistics

| Category | Count | Files |
|----------|-------|-------|
| Headers | 6 | cc2420-{header,trailer,phy,mac,net-device,energy-model}.h |
| Implementations | 6 | cc2420-{header,trailer,phy,mac,net-device,energy-model}.cc |
| Helper | 2 | cc2420-helper.{h,cc} |
| Tests | 3 | cc2420-{phy,mac,energy}-test.cc |
| **Total** | **17** | |

---

## Next Steps

1. **Update CMakeLists.txt** in src/wsn/ to include new CC2420 source files
2. **Implement Phase 1** (Frame format Serialize/Deserialize)
3. **Build and verify** compilation
4. **Implement phases** 2-5 sequentially
5. **Create example scenario** (e.g., cc2420-example.cc) for demo

---

## Architecture Visualization

```
┌─────────────────────────────────────────────────────────┐
│                     Upper Layers                         │
│              (Routing, Applications)                     │
└────────────────────┬────────────────────────────────────┘
                     │ NetDevice API
        ┌────────────┴─────────────┐
        │   Cc2420NetDevice        │
        │   (wrapper layer)        │
        └────────────┬─────────────┘
          ┌──────────┴──────────┐
          │                     │
    ┌─────▼──────────┐    ┌────▼──────────────┐
    │  Cc2420Mac     │    │   Cc2420Phy       │
    │  (MAC layer)   │    │   (PHY layer)     │
    │ Unslotted      │    │  6-state machine  │
    │ CSMA-CA        │    │  SIMPLE_COLLISION │
    └─────┬──────────┘    │  SpectrumPhy      │
          │               └────┬──────────────┘
          │                    │
          ├────────────────────┤
          │                    │
   ┌──────▼──────────┐   ┌─────▼─────────────┐
   │ Cc2420Header    │   │ Cc2420Trailer     │
   │ (MAC frame)     │   │ (CRC/LQI)         │
   └─────────────────┘   └───────────────────┘
          │
          │
    ┌─────▼────────────────────┐
    │ Cc2420EnergyModel         │
    │ (energy tracking)         │
    │ 6-state power model       │
    └──────────────────────────┘
          │
          ▼
    EnergySource
```

---

## Compilation Notes

All files follow ns-3 conventions:
- GPL-2.0-only SPDX header
- Namespace: `ns3::cc2420`
- NS_LOG_COMPONENT_DEFINE for debugging
- NS_OBJECT_ENSURE_REGISTERED for TypeId
- Callback pattern for inter-layer communication
- Doxygen comments for public API

Ready for implementation!
