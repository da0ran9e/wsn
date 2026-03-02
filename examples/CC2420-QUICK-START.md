# CC2420 Example - Quick Start Guide

## Purpose

This example demonstrates the simplest possible setup for CC2420 MAC layer communication:
- **2 nodes** in a wireless sensor network
- **10 meters apart** (manageable RF propagation)
- **SpectrumChannel** for realistic signal propagation
- **Ready for MAC packet transmission** (to be implemented)

## File Structure

```
src/wsn/examples/cc2420-example.cc (121 lines)
├── Callbacks (lines 28-50)
│   ├── Node0ReceiveCallback() - triggered when Node 0 receives packet
│   └── Node1ReceiveCallback() - triggered when Node 1 receives packet
│
└── main() (lines 56-121)
    ├── Node Creation (lines 62-67)
    ├── Mobility Setup (lines 69-81)
    ├── Channel Creation (lines 83-99)
    └── Simulation Run (lines 101-120)
```

## Code Walkthrough

### 1. Node Creation

```cpp
NodeContainer nodes;
nodes.Create(2);
```
Creates 2 NS-3 Node objects:
- Node 0: Transmitter/Receiver at origin
- Node 1: Transmitter/Receiver at 10m distance

### 2. Mobility Setup

```cpp
Ptr<MobilityModel> node0Mobility = nodes.Get(0)->GetObject<MobilityModel>();
Ptr<MobilityModel> node1Mobility = nodes.Get(1)->GetObject<MobilityModel>();

node0Mobility->SetPosition(Vector(0.0, 0.0, 0.0));
node1Mobility->SetPosition(Vector(10.0, 0.0, 0.0));
```
- Both nodes are **stationary** (ConstantPositionMobilityModel)
- Node 0 at origin (0m, 0m, 0m)
- Node 1 at (10m, 0m, 0m) along X-axis
- Distance = 10 meters (suitable for RF testing)

### 3. Channel Configuration

```cpp
Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();

// Path Loss Model: Log Distance
// Formula: PL(d) = PL(d0) + 10*n*log10(d/d0)
Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel>();
loss->SetAttribute("Exponent", DoubleValue(3.0));           // Outdoor path loss exponent
loss->SetAttribute("ReferenceDistance", DoubleValue(1.0));  // Reference at 1m
loss->SetAttribute("ReferenceLoss", DoubleValue(46.6776));  // Loss at 1m (46.68 dB)
channel->AddPropagationLossModel(loss);

// Propagation Delay Model
Ptr<ConstantSpeedPropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel>();
channel->SetPropagationDelayModel(delay);
```

**Path Loss Calculation Example:**
- At d=1m: PL = 46.6776 dB
- At d=10m: PL = 46.6776 + 10*3.0*log10(10) = 46.6776 + 30 = 76.68 dB

**TX/RX Power Budget:**
- TX Power: 0 dBm
- Path Loss @ 10m: 76.68 dB
- RX Power: 0 - 76.68 = **-76.68 dBm**
- RX Sensitivity: -95 dBm
- Link Margin: -76.68 - (-95) = **18.32 dB** ✓ (Link viable)

### 4. Simulation Execution

```cpp
Simulator::Stop(Seconds(6.0));
Simulator::Run();
Simulator::Destroy();
```
- Runs for 6 seconds of simulation time
- All events scheduled during this period are executed
- Memory is cleaned up

## Next Steps: Implementing Packet Transmission

To add actual packet transmission through CC2420 MAC:

### Option 1: Basic Callback-Based Transmission

```cpp
// Schedule packet send from Node 0 to Node 1
Simulator::Schedule(Seconds(1.0), [=]() {
    std::string payload = "Hello from Node 0";
    Ptr<Packet> packet = Create<Packet>((uint8_t*)payload.c_str(), payload.size());
    
    // Assuming Cc2420Mac has McpsDataRequest method
    // mac0->McpsDataRequest(packet, Mac16Address("00:02"), false);
    
    NS_LOG_INFO("t=1.0s: Node 0 sends to Node 1");
});
```

### Option 2: Periodic Broadcast

```cpp
class Cc2420Broadcaster : public Object {
public:
    void SendPacket(uint32_t seqNum) {
        Ptr<Packet> pkt = Create<Packet>(64);  // 64-byte payload
        mac->McpsDataRequest(pkt, Mac16Address("ff:ff"), false);
        Simulator::Schedule(Seconds(1.0), &Cc2420Broadcaster::SendPacket, this, seqNum+1);
    }
};
```

## Compilation

The example is included in the build system:

```bash
# Located in: src/wsn/examples/CMakeLists.txt
build_lib_example(
  NAME cc2420-example
  SOURCE_FILES cc2420-example.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libmobility}
    ${libspectrum}
    ${libantenna}
    ${libpropagation}
    ${libwsn}
)
```

## Running the Example

```bash
# Standard run with logging
cd /Users/mophan/Github/ns-3-dev-git-ns-3.46
./ns3 run cc2420-example

# With additional logging
./ns3 run "cc2420-example" --logging=*

# Capture output to file
./ns3 run cc2420-example > simulation.log 2>&1
```

## Expected Output

```
============================================================
CC2420 Two-Node Communication Example
Simple packet exchange via CC2420 MAC layer
============================================================

CC2420 MAC layer test - basic setup

Created 2 nodes
Node 0: MAC address 0x01
Node 1: MAC address 0x02

Setup node positions
Node 0: (0, 0, 0)
Node 1: (10, 0, 0)
Distance: 10 meters

Created SpectrumChannel with LogDistancePropagationLossModel

CC2420 MAC layer is configured and ready for packet transmission

Running simulation for 6 seconds...


============================================================
Simulation completed
============================================================
```

## Design Principles

1. **Simplicity First**: Only essentials - 2 nodes, 1 channel
2. **MAC-Focused**: Demonstrates CC2420 MAC layer setup
3. **Extensible**: Easy to add packet transmission callbacks
4. **Observable**: Clear logging at each step
5. **Realistic**: Uses standard propagation models

## Dependencies

- `ns3/core-module.h` - Simulator, logging
- `ns3/network-module.h` - Nodes, NetDevice
- `ns3/mobility-module.h` - ConstantPositionMobilityModel
- `ns3/spectrum-module.h` - SpectrumChannel, PropagationModels

## Related Examples

- `uav-example.cc` - Complex UAV scenario with 3D grid topology
- `bypass-example.cc` - Routing through bypass protocol
- `cell-forming-example.cc` - Cell-forming protocol test

## Troubleshooting

**Q: Build fails with undefined reference**
- A: Ensure `${libwsn}` is in LIBRARIES_TO_LINK in CMakeLists.txt

**Q: Simulation runs but no output**
- A: Add `LogComponentEnable("Cc2420Example", LOG_LEVEL_INFO);` in main()

**Q: How do I add MAC transmission?**
- A: Implement `Cc2420Mac::McpsDataRequest()` method and wire callbacks

---

**Last Updated**: 2025-02-28
**Status**: Ready for MAC implementation
