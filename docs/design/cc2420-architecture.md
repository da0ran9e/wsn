# Version 1.0: Basic PHY + MAC + NetDevice
**Folder Structure (Version 1.0)**
``` 
    src/wsn/
    ├── model/
    │    ├── radio/
    │    │     └── cc2420/
    │    │          ├── cc2420-net-device.h
    │    │          ├── cc2420-net-device.cc
    │    │          │
    │    │          ├── cc2420-mac.h
    │    │          ├── cc2420-mac.cc
    │    │          │
    │    │          ├── cc2420-phy.h
    │    │          ├── cc2420-phy.cc
    │    │          │
    │    │          ├── cc2420-state-machine.h
    │    │          ├── cc2420-state-machine.cc
    │    │          │
    │    │          ├── cc2420-energy-model.h
    │    │          ├── cc2420-energy-model.cc
    │    │          │
    │    │          ├── cc2420-channel-adapter.h
    │    │          ├── cc2420-channel-adapter.cc
    │    │          │
    │    │          ├── cc2420-header.h
    │    │          └── cc2420-header.cc
    │    │
    │    └── ...
    │
    ├── test/
    │    ├── cc2420-state-test.cc
    │    ├── cc2420-energy-test.cc
    │    ├── cc2420-phy-test.cc
    │    └── cc2420-test-suite.cc
    │
    └── helper/
        └── cc2420-helper.h
        └── cc2420-helper.cc
```

1. `Cc2420NetDevice`

Implements the ns-3 NetDevice interface:

- Packet transmission entry point
- Binding to Node
- Binding to Channel
- Forwarding to MAC layer
- Delivering received packets upward
- This is the integration layer between CC2420 and ns-3 core.

2. `Cc2420Mac`

Implements a simplified unslotted CSMA-CA mechanism:

- Basic backoff
- CCA request to PHY
- Frame enqueue/dequeue
- No beacon mode
- No GTS
- No superframe structure

3. `Cc2420Phy`

Implements packet-level physical layer behavior:

- Packet transmission duration modeling
- Reception handling
- Collision detection (SIMPLE model by default)
- Optional additive interference mode
- Interaction with SpectrumChannel
- No symbol-level simulation is included in v1.0.

4. `Cc2420StateMachine`

Maintains radio states:

RADIO_SLEEP

RADIO_IDLE

RADIO_RX

RADIO_TX

RADIO_CCA

RADIO_SWITCHING

5. `Cc2420EnergyModel`

Implements state-based energy consumption:

$$ Energy = Current(state) × Voltage × Duration $$

- TX current (per power level)
- RX current
- Idle current
- Sleep current
- Energy updates occur on every state transition.

6. `Cc2420ChannelAdapter`

Acts as the interface between Cc2420Phy and ns-3 SpectrumChannel:

- Forwarding transmission to channel
- Receiving spectrum events
- Handling interference model selection

7. `Cc2420Header`

Defines the MAC frame format used in v1.0:

- Frame control
- Sequence number
- Source address
- Destination address
- Payload
- No full IEEE 802.15.4 compliance in v1.0.

6. Testing Structure

Located in:`src/wsn/test/`