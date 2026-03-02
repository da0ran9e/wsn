# WSN Radio MAC Integration Guide

**Status**: Framework Complete - Ready for LR-WPAN Integration  
**Date**: 28 February 2026

---

## 1. Overview

The WSN Radio MAC layer provides a unified interface for both Phase 0 (Cell Forming) and Phase 1 (UAV Broadcasting) to transmit packets over the CC2420/LR-WPAN (IEEE 802.15.4) radio.

### Architecture

```
┌─────────────────────────────────────────────────┐
│  Phase 0 (CellForming)  │  Phase 1 (UavMac)    │
│  - HELLO               │  - Fragments         │
│  - CL Announcement     │                       │
│  - Member Feedback     │                       │
└────────────┬──────────────────┬────────────────┘
             │                  │
             └──────────┬───────┘
                        │
┌───────────────────────▼────────────────────────┐
│     WsnRadioMac (MAC Frame Layer)              │
│  - Packet serialization                        │
│  - Header construction                         │
│  - Transmission scheduling                     │
│  - Reception callbacks                         │
└────────────┬──────────────────────────────────┘
             │
┌────────────▼──────────────────────────────────┐
│     LR-WPAN Radio (CC2420)                    │
│  - PHY layer (868/915 MHz)                    │
│  - Channel access (CSMA-CA)                   │
│  - Path loss simulation                       │
│  - Reception sensitivity                      │
└────────────┬──────────────────────────────────┘
             │
┌────────────▼──────────────────────────────────┐
│     Wireless Channel                          │
│  - Signal propagation                         │
│  - Interference modeling                      │
└───────────────────────────────────────────────┘
```

---

## 2. Components

### 2.1 MAC Frame (mac-frame.h/cc)

**WsnMacHeader**: MAC frame header with 7-byte overhead
```
| Type (1B) | SeqNum (2B) | SrcId (2B) | DstId (2B) |
```

**Packet Types**:
- `HELLO_PACKET` (1): Phase 0 neighbor discovery
- `CL_ANNOUNCEMENT` (2): Phase 0 leader announcement
- `MEMBER_FEEDBACK` (3): Phase 0 member feedback
- `UAV_FRAGMENT` (4): Phase 1 fragment broadcast

**Serialization Functions**:
- `SerializeHelloPacket()` / `DeserializeHelloPacket()`
- `SerializeCLAnnouncement()` / `DeserializeCLAnnouncement()`
- `SerializeMemberFeedback()` / `DeserializeMemberFeedback()`
- `SerializeFragment()` / `DeserializeFragment()`

### 2.2 WSN Radio MAC (wsn-radio-mac.h/cc)

**Key Methods**:

```cpp
// Attach to node and radio device
void AttachToNode(Ptr<Node> node, Ptr<NetDevice> radioDevice);

// Phase 0 transmission
bool SendHelloPacket(const HelloPacket& hello);
bool SendCLAnnouncement(const CLAnnouncementPacket& ann);
bool SendMemberFeedback(const CLMemberFeedbackPacket& feedback);

// Phase 1 transmission
bool SendFragment(const Fragment& frag);

// Reception callbacks
void SetHelloReceivedCallback(HelloReceivedCallback cb);
void SetCLAnnouncementReceivedCallback(CLAnnouncementReceivedCallback cb);
void SetMemberFeedbackReceivedCallback(MemberFeedbackReceivedCallback cb);
void SetFragmentReceivedCallback(FragmentReceivedCallback cb);

// Statistics
uint32_t GetPacketsSent() const;
uint32_t GetPacketsReceived() const;
uint32_t GetTransmissionFailures() const;
```

---

## 3. Integration with CellForming

### 3.1 Current Architecture (Callback-based)

CellForming module uses callbacks to deliver packets:

```cpp
// Phase 0 module's callbacks
cellForming->SetHelloCallback([](const HelloPacket& hello) {
    // Current: delivers to all nodes directly via callback
});

cellForming->SetCLAnnouncementCallback([](const CLAnnouncementPacket& ann) {
    // Current: delivers to cell members directly via callback
});
```

### 3.2 Proposed Radio Integration

Replace callbacks with radio transmission:

```cpp
// Phase 0 module with radio
Ptr<WsnRadioMac> radioMac = ...;

cellForming->SetHelloCallback([radioMac](const HelloPacket& hello) {
    radioMac->SendHelloPacket(hello);  // Transmit via radio
});

cellForming->SetCLAnnouncementCallback([radioMac](const CLAnnouncementPacket& ann) {
    radioMac->SendCLAnnouncement(ann);  // Transmit via radio
});

// Receive callbacks
radioMac->SetHelloReceivedCallback(
    MakeCallback(&CellForming::HandleHelloPacket, cellForming)
);
```

### 3.3 Implementation Steps

1. **Create radio device** (LR-WPAN)
   ```cpp
   Ptr<NetDevice> radioDevice = lrWpanHelper.Install(node);
   ```

2. **Attach WsnRadioMac**
   ```cpp
   Ptr<WsnRadioMac> radioMac = CreateObject<WsnRadioMac>();
   radioMac->AttachToNode(node, radioDevice);
   node->AggregateObject(radioMac);
   ```

3. **Wire callbacks**
   ```cpp
   cellForming->SetHelloCallback(
       MakeCallback(&WsnRadioMac::SendHelloPacket, radioMac)
   );
   radioMac->SetHelloReceivedCallback(
       MakeCallback(&CellForming::HandleHelloPacket, cellForming)
   );
   ```

---

## 4. Integration with UAV Phase 1

### 4.1 Current Architecture

UavMac sends fragments directly to ground nodes:

```cpp
void UavMac::DoBroadcast() {
    for (uint32_t i = 0; i < m_groundNodes.GetN(); i++) {
        Ptr<GroundNodeMac> groundMac = groundNode->GetObject<GroundNodeMac>();
        groundMac->ReceiveFragment(frag, rxPowerDbm);  // Direct call
    }
}
```

### 4.2 Proposed Radio Integration

Use radio for broadcast:

```cpp
void UavMac::DoBroadcast() {
    Ptr<WsnRadioMac> radioMac = m_uavNode->GetObject<WsnRadioMac>();
    if (radioMac) {
        radioMac->SendFragment(fragment);  // Transmit via radio
    }
}
```

### 4.3 Ground Node Reception

Ground nodes receive via radio:

```cpp
Ptr<WsnRadioMac> groundRadioMac = groundNode->GetObject<WsnRadioMac>();
groundRadioMac->SetFragmentReceivedCallback(
    MakeCallback(&GroundNodeMac::ReceiveFragment, groundMac)
);
```

---

## 5. Example: Full Integration in uav-example.cc

### 5.1 Setup Phase

```cpp
#include "ns3/lr-wpan-module.h"
#include "wsn-radio-mac.h"

// Create and install LR-WPAN radio on all nodes
ns3::lr_wpan::LrWpanHelper lrWpanHelper;
for (uint32_t i = 0; i < groundNodes.GetN(); i++) {
    Ptr<Node> node = groundNodes.Get(i);
    
    // Install radio device
    NetDeviceContainer devices = lrWpanHelper.Install(node);
    Ptr<NetDevice> radioDevice = devices.Get(0);
    
    // Attach WsnRadioMac
    Ptr<WsnRadioMac> radioMac = CreateObject<WsnRadioMac>();
    radioMac->AttachToNode(node, radioDevice);
    node->AggregateObject(radioMac);
    
    // Retrieve CellForming module
    Ptr<CellForming> cellForming = node->GetObject<CellForming>();
    
    // Wire Phase 0 callbacks to use radio
    cellForming->SetHelloCallback(
        MakeCallback(&WsnRadioMac::SendHelloPacket, radioMac)
    );
    radioMac->SetHelloReceivedCallback(
        MakeCallback(&CellForming::HandleHelloPacket, cellForming)
    );
    
    // Similar for CL announcement and member feedback...
}

// UAV radio setup
Ptr<Node> uavNode = uavNode.Get(0);
NetDeviceContainer uavDevices = lrWpanHelper.Install(uavNode);
Ptr<NetDevice> uavRadio = uavDevices.Get(0);
Ptr<WsnRadioMac> uavRadioMac = CreateObject<WsnRadioMac>();
uavRadioMac->AttachToNode(uavNode, uavRadio);
uavNode->AggregateObject(uavRadioMac);

// Wire Phase 1 callbacks
Ptr<UavMac> uavMac = uavNode->GetObject<UavMac>();
uavMac->SetUavRadioMac(uavRadioMac);  // Pass reference
```

### 5.2 Statistics Output

```
========================================
        WSN Radio MAC Statistics
========================================

  Phase 0 Packets:
    HELLO packets sent:        450
    CL announcements sent:      27
    Member feedback sent:       81

  Phase 1 Packets:
    Fragments sent:             92
    Total packets transmitted: 650

  Reception:
    Total packets received:    1240
    CL announcement RX:         27
    Member feedback RX:        162
    Fragment RX:              1051

  Reliability:
    Transmission failures:       12
    Packet loss rate:          1.8%
    Average RSSI:            -88.3 dBm

========================================
```

---

## 6. Performance Impact

### 6.1 Additional Overhead

**Per Packet**:
- MAC header: 7 bytes (Type + SeqNum + SrcId + DstId)
- LR-WPAN PHY header: 6 bytes (SFD + Length + CRC)
- Total overhead: ~13 bytes per packet

**Example Payload Sizes**:
- HELLO packet: ~50-100 bytes → ~63-113 bytes with MAC
- CL Announcement: ~30 bytes → ~37 bytes with MAC
- Member Feedback: ~100-200 bytes → ~107-207 bytes with MAC
- Fragment: ~50 bytes → ~57 bytes with MAC

### 6.2 Transmission Delay

LR-WPAN transmission time for 64-byte packet:
- At 250 kbps: ~2 ms
- Including CSMA-CA contention: 2-10 ms average
- Phase 0: Not time-critical (1s HELLO interval)
- Phase 1: ~250 ms fragment interval >> 10 ms transmission

**Conclusion**: Minimal impact on simulation time and performance.

---

## 7. Implementation Checklist

### Phase 1: Framework (Complete)
- ✅ `mac-frame.h/cc`: MAC header and serialization
- ✅ `wsn-radio-mac.h/cc`: Radio MAC layer
- ✅ CMakeLists.txt: Build integration

### Phase 2: Integration (Next)
- ⬜ Modify `cell-forming.cc` to use `WsnRadioMac`
- ⬜ Modify `uav-mac.cc` to use `WsnRadioMac`
- ⬜ Modify `uav-example.cc` to set up radio devices
- ⬜ Add LR-WPAN helper instantiation
- ⬜ Update statistics output

### Phase 3: Testing
- ⬜ Verify HELLO delivery via radio
- ⬜ Verify CL election with radio delays
- ⬜ Verify fragment reception via radio
- ⬜ Compare performance: direct vs radio
- ⬜ Verify packet loss handling

### Phase 4: Optimization
- ⬜ Add packet fragmentation for large payloads
- ⬜ Implement retransmission strategy
- ⬜ Add collision avoidance improvements
- ⬜ Support multi-channel operation
- ⬜ Energy consumption modeling

---

## 8. File Summary

| File | Purpose | Lines |
|------|---------|-------|
| mac-frame.h | MAC header + serialization interface | 110 |
| mac-frame.cc | MAC header + serialization implementation | 190 |
| wsn-radio-mac.h | Radio MAC layer interface | 150 |
| wsn-radio-mac.cc | Radio MAC layer implementation | 220 |

**Total Addition**: ~670 lines of code

---

## 9. Backwards Compatibility

The new radio layer is **completely optional**:

1. **Existing code continues to work** with callback-based delivery
2. **Can be enabled** by attaching `WsnRadioMac` to nodes
3. **Can be disabled** by using null radio device
4. **Default behavior** remains callback-based for compatibility

### Migration Path

```cpp
// Option 1: Continue with callbacks (no change)
cellForming->SetHelloCallback([](const HelloPacket& hello) {
    // Direct delivery
});

// Option 2: Use radio layer (new)
Ptr<WsnRadioMac> radioMac = ...;
cellForming->SetHelloCallback(
    MakeCallback(&WsnRadioMac::SendHelloPacket, radioMac)
);
```

---

## 10. Future Enhancements

1. **Cross-Layer Optimization**
   - Share channel state information between PHY and MAC
   - Adaptive transmission power based on link quality
   - Dynamic fragment size based on channel conditions

2. **Advanced MAC Features**
   - Time-slotted operation (TDMA) for Phase 0 HELLO
   - Priority queuing for urgent packets
   - Acknowledgment/ARQ for critical Phase 0 messages

3. **Multi-Hop Support**
   - Routing through cell topology
   - Relay selection for weak links
   - Cooperative reception for improved coverage

4. **Energy Modeling**
   - Track transmission energy per packet
   - Sleep scheduling for Phase 0
   - Energy-aware routing decisions

---

## 11. References

- **IEEE 802.15.4**: LR-WPAN standard
- **NS-3 LR-WPAN Module**: /src/lr-wpan/
- **CC2420**: Texas Instruments 802.15.4 transceiver
- **WSN Phase 0**: Cell formation protocol
- **WSN Phase 1**: UAV broadcast protocol

---

**Status**: Ready for Phase 2 Integration  
**Next Step**: Integrate with actual CellForming and UavMac modules
