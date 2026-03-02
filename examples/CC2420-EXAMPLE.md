# CC2420 Two-Node Communication Example

## Overview

This example demonstrates a simple 2-node wireless sensor network using the CC2420 IEEE 802.15.4 MAC layer. The nodes are positioned 10 meters apart and the simulation is set up to test packet transmission and reception through the CC2420 MAC protocol stack.

## Architecture

```
cc2420-example.cc
├── Node 0 (Position: 0, 0, 0)
│   └── MobilityModel: ConstantPositionMobilityModel
├── Node 1 (Position: 10, 0, 0)
│   └── MobilityModel: ConstantPositionMobilityModel
└── SpectrumChannel
    ├── Propagation Loss Model: LogDistancePropagationLossModel
    │   ├── Exponent: 3.0
    │   ├── Reference Distance: 1.0 m
    │   └── Reference Loss: 46.6776 dBm
    └── Propagation Delay Model: ConstantSpeedPropagationDelayModel
```

## Key Components

### 1. Node Setup
- **Node Count**: 2 nodes
- **Mobility Model**: ConstantPositionMobilityModel (static nodes)
- **Node 0 Position**: (0, 0, 0)
- **Node 1 Position**: (10, 0, 0)
- **Distance**: 10 meters

### 2. Spectrum Channel Configuration
The nodes communicate over a SpectrumChannel with:
- **Path Loss Model**: Log Distance with 3.0 exponent
- **Reference Loss**: 46.6776 dBm @ 1 meter
- **Propagation Delay**: Constant speed propagation delay

### 3. CC2420 MAC Layer (Placeholder)
Currently, the example sets up the infrastructure for CC2420 MAC layer communication. The actual packet transmission/reception through the MAC layer will be implemented when the CC2420 MAC class is fully completed.

## MAC Configuration

Each node will be configured with:
```cpp
MacConfig {
    panId = 0x1234,              // PAN Identifier
    shortAddress = 0x01/0x02,    // 16-bit short address
    macMinBE = 3,                // Min Backoff Exponent
    macMaxBE = 5,                // Max Backoff Exponent
    macMaxCSMABackoffs = 4,      // Max CSMA-CA backoffs
    macMaxFrameRetries = 3,      // Max retransmissions
    txAckRequest = false,        // Request ACK on TX
    rxOnWhenIdle = true          // Keep RX on during idle
};
```

## Simulation Parameters

| Parameter | Value | Unit |
|-----------|-------|------|
| Simulation Duration | 6.0 | seconds |
| Node Separation | 10 | meters |
| TX Power | 0 | dBm |
| RX Sensitivity | -95 | dBm |
| Channel Frequency | 2400 | MHz |
| Data Rate | 250 | kbps |

## Future Enhancements

To complete the example for full packet transmission/reception testing:

1. **Implement CC2420Mac class fully**
   - McpsDataRequest() - MAC service primitive for sending packets
   - FrameReceptionCallback() - Handler for received packets
   - CSMA-CA state machine execution

2. **Implement CC2420Phy class**
   - SpectrumPhy interface integration
   - Signal reception and bit error calculation
   - SNR to BER conversion for PSK modulation

3. **Add Packet Transmission Scheduling**
   - Schedule data packets from Node 0 to Node 1
   - Schedule acknowledgments and retransmissions
   - Monitor reception statistics

4. **Packet Format Support**
   - CC2420Header with frame control, sequence, addresses
   - CC2420Trailer with FCS and LQI
   - Variable-length payload support (max 127 bytes)

5. **MAC State Machine**
   - Idle → CSMA-CA → CCA → TX → Idle
   - Idle → RX → Frame Reception → Indication
   - ACK handling and timeout management

## How to Run

```bash
# Build the WSN module
./ns3 build

# Run the example
./ns3 run cc2420-example

# Run with logging enabled
./ns3 run "cc2420-example --LogComponentEnable=Cc2420*"
```

## Expected Output

The simulation will:
1. Create 2 nodes with fixed positions 10m apart
2. Set up spectrum channel with path loss and propagation delay
3. Configure MAC layer parameters for both nodes
4. Run for 6 seconds
5. Log all events to console

## Files Modified

- `src/wsn/examples/cc2420-example.cc` - Main example file
- `src/wsn/CMakeLists.txt` - Build configuration (includes cc2420-mac.cc)
- `src/wsn/examples/CMakeLists.txt` - Example build configuration

## Related Components

- `src/wsn/model/radio/cc2420/cc2420-mac.h/cc` - MAC layer implementation
- `src/wsn/model/radio/cc2420/cc2420-phy.h/cc` - PHY layer (SpectrumPhy)
- `src/wsn/model/radio/cc2420/cc2420-header.h/cc` - MAC frame header
- `src/wsn/model/radio/cc2420/cc2420-trailer.h/cc` - MAC frame trailer (FCS)

## Notes

- This example focuses on **MAC layer packet transmission only**
- UAV scenarios are handled in separate examples (uav-example.cc)
- The CC2420 radio implementation follows IEEE 802.15.4 standard
- All packets route through the `McpsDataRequest()` interface in Cc2420Mac
