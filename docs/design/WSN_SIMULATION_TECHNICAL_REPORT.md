# Technical Report: UAV-WSN Simulation — Full Implementation Record

**Project**: `src/wsn` — UAV-assisted Wireless Sensor Network simulation on ns-3 3.46  
**Report date**: 2026-03-13  
**Scope**: Toàn bộ công việc triển khai từ đầu đến nay — từ CC2420 radio (mà ns-3 không có sẵn), qua WSN object model, cell forming, routing abstractions, đến chuỗi kịch bản thử nghiệm Scenario1 → Scenario5 và hạ tầng autorun batch experiments.

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Project Context](#2-project-context)
3. [CC2420 Radio Implementation](#3-cc2420-radio-implementation)
4. [WSN Object Model](#4-wsn-object-model)
5. [Cell Forming MAC Layer (Phase 0)](#5-cell-forming-mac-layer-phase-0)
6. [WSN Routing Abstractions](#6-wsn-routing-abstractions)
7. [Scenario1 — Basic CC2420 Grid](#7-scenario1--basic-cc2420-grid)
8. [uav-example — Phase 0+1 Integration](#8-uav-example--phase-01-integration)
9. [Scenario3 — Multi-UAV With Startup Phase](#9-scenario3--multi-uav-with-startup-phase)
10. [Scenario4 — Full BS-Coordinated UAV Mission](#10-scenario4--full-bs-coordinated-uav-mission)
11. [Scenario5 — Independent Experimental Branch](#11-scenario5--independent-experimental-branch)
12. [Autorun Batch Runner](#12-autorun-batch-runner)
13. [Build-System Integration](#13-build-system-integration)
14. [Validation and Current Results](#14-validation-and-current-results)
15. [Current Limitations](#15-current-limitations)
16. [Research Directions for UAV2](#16-research-directions-for-uav2)
17. [Overall Assessment](#17-overall-assessment)

---

## 1. Executive Summary

Toàn bộ module `src/wsn` được xây dựng từ đầu trên nền ns-3 3.46. ns-3 không có sẵn bất kỳ thành phần nào phù hợp cho bài toán UAV-WSN theo cách này — mọi thứ từ radio hardware đến routing logic đến kịch bản thử nghiệm đều phải tự triển khai.

**Tóm tắt những gì đã được xây dựng**:

| Thành phần | Phạm vi | Trạng thái |
|---|---|---|
| CC2420 radio (PHY/MAC/NetDevice/Energy) | Mới hoàn toàn | ~50% functional |
| WSN object model (Node, App, MAC, Radio...) | Mới hoàn toàn | Skeleton + partial |
| Cell forming MAC (Phase 0) | Mới hoàn toàn | Functional |
| WSN routing abstractions | Mới hoàn toàn | Functional |
| Scenario1 — basic CC2420 grid | Kịch bản thứ nhất | Stable |
| uav-example — Phase 0+1 integration | Integration demo | Stable |
| Scenario3 — multi-UAV + startup phase | Kịch bản thứ ba | Stable |
| Scenario4 — full BS-coordinated UAV mission | Kịch bản thứ tư | Stable + bugfixed |
| Scenario5 — independent experimental branch | Kịch bản thứ năm | Stable |
| Autorun batch runner | Experimentation tooling | Operational |

---

## Project Phases

Để rõ ràng hoá lộ trình phát triển và ưu tiên công việc, báo cáo này chia công việc thành ba giai đoạn chính:

- **Giai đoạn 1 — MVP cho CC2420 & cơ chế truyền khi di chuyển**:
     - Xây dựng phiên bản MVP của `Cc2420` (PHY/MAC/NetDevice/Energy) đủ để các node truyền và nhận khi đang di chuyển.
     - Mục tiêu: đảm bảo node có thể gửi/nhận broadcast trong môi trường động (UAV và ground nodes di chuyển) để kiểm tra tính năng mạng cơ bản và logic routing trước khi đầu tư vào fidelity vật lý.

- **Giai đoạn 2 — Khung kịch bản cho bài toán chính**:
     - Xây dựng và chuẩn hoá các kịch bản (Scenario1..Scenario5), helper, và tooling (autorun, param sweep) phục vụ thí nghiệm chính (fragment broadcast, BS orchestration, cell formation).
     - Mục tiêu: reproducible experiments, scripting để chạy batch experiments, và baseline metrics cho routing/mission-level evaluation.

- **Giai đoạn 3 — Mô phỏng tính chất vật lý thực tế**:
     - Nâng cấp PHY lên spectrum-aware (SpectrumPhy integration), hoàn thiện interference accumulation, SINR→BER/PER error models, fading/Doppler nếu cần, và propagation model theo elevation (air‑ground) với shadowing.
     - Mục tiêu: đạt fidelity vật lý đủ để nghiên cứu tác động của interference, altitude policies, và chính xác hoá PER/LQI cho các kết luận nghiên cứu.

Giai đoạn phát triển nên thực hiện theo thứ tự trên: hoàn thành giai đoạn 1 để có nền tảng thử nghiệm nhanh, rồi mở rộng kịch bản và tooling (giai đoạn 2), sau đó đầu tư vào fidelity vật lý (giai đoạn 3) khi các kịch bản và metric đã ổn định.


## 2. Project Context

### 2.1. Bài toán nghiên cứu

Mô phỏng một hệ thống WSN hỗ trợ UAV theo kịch bản:
- Mạng lưới cảm biến ground nodes bố trí theo lưới.
- Base Station (BS) điều phối chọn vùng đáng ngờ (suspicious region).
- Hai UAV đảm nhận nhiệm vụ: UAV1 bay đến vùng nghi vấn, UAV2 phát broadcast fragments xuống ground nodes.
- Fragment-based confidence tracking: BS sinh fragments từ thông tin vùng nghi vấn → UAV2 broadcast → ground nodes tích lũy confidence → alert khi đủ ngưỡng.

### 2.2. Những gì ns-3 không có sẵn

ns-3 3.46 cung cấp:
- Scheduler, Event Loop, Channel, Mobility, Spectrum — dùng được.
- LR-WPAN module (802.15.4 generics) — dùng được một phần.

ns-3 **không có**:
- CC2420-specific PHY/MAC với state machine và energy model riêng.
- WSN application object model phù hợp.
- Cell-forming / cluster formation protocol.
- UAV broadcast với fragment-based confidence tracking.
- BS-coordinated mission planning với suspicious region detection.
- Multi-round batch experimentation tooling.

### 2.3. Tổ chức module

```
src/wsn/
├── model/
│   ├── radio/cc2420/          ← CC2420 radio hardware (hoàn toàn mới)
│   ├── objects/               ← WSN object model (hoàn toàn mới)
│   ├── mobility/              ← WSN mobility model
│   ├── uav/                   ← Cell forming + UAV MAC (hoàn toàn mới)
│   └── routing/
│       ├── bypass-routing/    ← Debug routing
│       ├── pecee-routing/     ← PECEE-compatible routing
│       ├── wsn-routing-*      ← Routing abstractions
│       ├── wsn-forwarder.*    ← Packet forwarding
│       ├── scenario1/         ← Scenario1 routing backend
│       ├── scenario3/         ← Scenario3 routing backend
│       ├── scenario4/         ← Scenario4 routing backend
│       └── scenario5/         ← Scenario5 routing backend
├── examples/
│   ├── example1.cc            ← CC2420 grid
│   ├── cc2420-example.cc      ← CC2420 verification
│   ├── uav-example.cc         ← Phase 0+1 integration
│   ├── cell-forming-example.cc
│   ├── example3.cc            ← Scenario3
│   ├── example4.cc            ← Scenario4
│   ├── example5.cc            ← Scenario5
│   └── scenarios/
│       ├── scenario3.h/.cc
│       ├── scenario4/
│       └── scenario5/
├── helper/                    ← CC2420Helper
└── docs/
```

---

## 3. CC2420 Radio Implementation

Phần này được chia làm hai phần nhỏ để tách rõ các kết quả đã có trong giai đoạn đầu (MVP) và các thành phần cần bổ sung khi nâng fidelity vật lý.

### 3.A — Phase 1: MVP CC2420 (early-stage implementations)

#### 3.A.1. Bối cảnh

CC2420 là chip radio IEEE 802.15.4 2.4 GHz (TelosB, MicaZ). Việc xây module CC2420 riêng nhằm đáp ứng các yêu cầu hardware‑specific: energy model, trạng thái phần cứng, và RF parameters theo datasheet.

#### 3.A.2. Các file đã triển khai (MVP)

`src/wsn/model/radio/cc2420/`:

| File | Nội dung |
|---|---|
| `cc2420-phy.h/.cc` | PHY layer skeleton, state machine, SpectrumPhy interface hooks |
| `cc2420-mac.h/.cc` | MAC layer cơ bản, CSMA-CA parameters, frame RX/TX handlers |
| `cc2420-net-device.h/.cc` | ns-3 NetDevice interface binding Node/Channel/MAC |
| `cc2420-energy-model.h/.cc` | Energy consumption per state (datasheet values) |
| `cc2420-header.h/.cc` | Frame header serialization |
| `cc2420-trailer.h/.cc` | Frame trailer (FCS) |

#### 3.A.3. PHY (MVP features)

Đã có trong MVP:
- State machine cơ bản: `SLEEP`, `IDLE`, `RX`, `TX`, `CCA`, `SWITCHING`.
- TypeId attributes: `TxPower`, `RxSensitivity`, `NoiseFloor`, `CCAThreshold`, và các tham số path-loss tham khảo.
- SpectrumPhy interface được khai báo (`SetChannel()`, `SetDevice()`, `SetMobility()`, `SetAntenna()`, `StartRx()`), nhưng xử lý tín hiệu còn là stub ở nhiều chỗ.
- Debug trace hooks và mapping energy/state đã có.

Những hạng mục MVP còn là TODO nhưng không ngăn các thử nghiệm kịch bản cơ bản:
- `StartRx()` detailed signal processing (đang là stub).
- `SetState()` validation matrix và một số logic state transition.

#### 3.A.4. MAC và helper

MAC MVP cung cấp:
- Basic CSMA state machine, TX queue, callbacks, và thống kê.
- Một số chi tiết (full backoff timing, ACK handling) còn thiếu nhưng không chặn kịch bản nghiên cứu hiện tại.

CC2420 helper (`src/wsn/helper/cc2420-helper.h/.cc`) hỗ trợ tạo NetDevice cho node container.

#### 3.A.5. Energy & verification

Energy theo datasheet (tóm tắt): Sleep 1.4 mW, Idle/RX/CCA 62 mW, TX(0 dBm) ≈ 52.2 mW.

Verification: `src/wsn/examples/cc2420-example.cc` chạy ~23 test; MVP hiện pass ~19/23 (các test liên quan RX processing chưa hoàn thiện).

#### 3.A.6. Trạng thái tổng thể (sau Phase 1)

MVP status: CC2420 ~50% functional — đủ cho packet-level scenarios, mission-level metrics, và framework validation.

### 3.B — Phase 3: Physical-fidelity components (spectrum & BER)

Phần này liệt kê các thành phần cần thiết khi nâng fidelity PHY để mô phỏng interference, BER và các hiệu ứng tần-số/thời-gian.

#### 3.B.1. Mục tiêu

- Spectrum-aware PHY: tích hợp đầy đủ với `SpectrumChannel` để nhận `SpectrumSignalParameters` (PSD), quản lý interference PSD theo bin, và tính SINR chính xác theo băng tần.
- Error model: chuyển SINR → BER → PER theo modulation/coding của IEEE 802.15.4 (O‑QPSK/DSSS) hoặc bảng tra cứu (Castalia-like).
- Propagation model spectrum-aware: trả `SpectrumValue` (PSD) cho mỗi receiver, bao gồm log‑distance + elevation-dependent exponent + log-normal shadowing.
- Optional: small‑scale fading (Rician/Nakagami) và Doppler nếu UAV tốc độ khiến hiệu ứng này đáng kể.

#### 3.B.2. Các hạng mục cần triển khai

- Hoàn thiện `Cc2420Phy::StartRx()`, `ProcessSignalStart()`, `ProcessSignalEnd()`, `UpdateInterference()` để quản lý received-signal list, cộng/loại PSD interference, và quyết định reception success tại EndRx.
- Viết `Cc2420ErrorModel` (SINR→BER/PER) hoặc tích hợp một error-model hiện có với tham số phù hợp cho 802.15.4.
- Cài đặt `SpectrumPropagationLossModel` chuyên dụng (vd. `Cc2420SpectrumPropagationLossModel`) để chuyển từ tx PSD → rx PSD theo distance/elevation/shadowing. (Một phiên bản đầu đã được thêm vào `src/wsn/model/propagation/`.)
- Thiết kế test/unit cho validation: PSD→dBm mapping, SINR→PER curves, và reproducibility kiểm tra seed/runId.

#### 3.B.3. Tại sao tách Phase 3 riêng

Việc thực hiện Phase 3 yêu cầu mức công việc và kiểm chứng lớn hơn (bin-wise PSD math, timing overlap handling, validation BER curves). Để không chậm tiến độ nghiên cứu, phương án hợp lý là:

1. Hoàn tất Phase 1 để có nền tảng running experiments nhanh.
2. Sau khi kịch bản và metrics ổn định (Phase 2), chuyển đầu tư vào Phase 3 để cải thiện fidelity vật lý và phân tích radio‑level.

---

## 4. WSN Object Model

### 4.1. Bối cảnh

Trước khi xây scenario, cần object model cho các entities trong mạng WSN. ns-3 chỉ có generic `Node`; không có `WsnNode`, `WsnApp`, `WsnMac`, v.v.

### 4.2. Các class đã triển khai

`src/wsn/model/objects/`:

| Class | Vai trò |
|---|---|
| `WsnObject` | Base class cho tất cả WSN objects |
| `WsnNode` | WSN node wrapper |
| `WsnApp` | Application layer (non-IP) |
| `WsnMac` | MAC layer abstraction |
| `WsnRadio` | Radio abstraction |
| `WsnRouting` | Routing abstraction |
| `WsnMobility` | Mobility abstraction |
| `WirelessChannel` | Channel abstraction |
| `ResourceManager` | Energy/memory resource management |
| `SensorNetwork` | Top-level network container |
| `WsnObjectFactory` | Factory pattern |
| `WsnObjectRegistry` | Lookup objects by ID/path |

### 4.3. WSN Scenario và INI Parser

`src/wsn/model/wsn-scenario.h/.cc`:
- Parse `.ini` file để configure topology và objects
- Dùng `WsnObjectRegistry` để build network từ config
- Tích hợp với `WsnTrace` để ghi trace file

`src/wsn/model/ini-parser.h/.cc`:
- INI file parser với section, key/value, comment

`src/wsn/model/wsn-trace.h/.cc`:
- Trace writer cho simulation events

### 4.4. Mobility Extension

`src/wsn/model/mobility/wsn-mobility-model.h/.cc`:
- Extension của ns-3 MobilityModel cho WSN-specific movement patterns

---

## 5. Cell Forming MAC Layer (Phase 0)

### 5.1. Bối cảnh

Cell forming là tiền đề cho toàn bộ kiến trúc UAV-WSN:
- Chia ground nodes thành cells theo lưới hex.
- Mỗi cell bầu Cell Leader (CL) theo fitness score.
- CL xây dựng intra-cell routing table.
- Output được dùng bởi BS để plan routes và chọn suspicious region.

Không có implementation nào phù hợp trong ns-3.

### 5.2. CellForming class

`src/wsn/model/uav/cell-forming.h/.cc`:

| Phase | Nội dung |
|---|---|
| 0.1 | Hex-grid cell coordinate calculation |
| 0.2 | HELLO beacon broadcasting, neighbor discovery |
| 0.3 | Fitness score calculation (degree, energy, stability) |
| 0.4–0.5 | Cell Leader election |
| 0.6 | Cell building via member feedback |
| 0.7 | Intra-cell routing table computation |

### 5.3. Cell Forming Packet types

`src/wsn/model/uav/cell-forming-packet.h`:
- `HelloPacket`: neighbor discovery beacon
- `CLAnnouncementPacket`: CL election broadcast
- `CLMemberFeedbackPacket`: member reporting to CL

### 5.4. CellFormingMac

`src/wsn/model/uav/cell-forming-mac.h/.cc`:
- MAC layer cho Phase 0 packet exchange
- Callbacks: `SetHelloCallback`, `SetCLAnnouncementCallback`, `SetMemberFeedbackCallback`
- Simulation model: deliver với propagation delay 10 ms

### 5.5. GroundNodeMac

`src/wsn/model/uav/ground-node-mac.h/.cc`:
- Ground node receive-side MAC
- RSSI tracking
- Fragment-based confidence accumulation
- Alert generation khi confidence vượt threshold

### 5.6. UavMac

`src/wsn/model/uav/uav-mac.h/.cc`:
- UAV broadcast controller
- Periodic broadcast scheduling
- Fragment generation và distribution
- TX power control
- Statistics

### 5.7. UavMacCc2420

`src/wsn/model/uav/uav-mac-cc2420.h/.cc`:
- Version của UavMac tích hợp với CC2420 NetDevice thật

### 5.8. WsnRadioMac + MacFrame

`src/wsn/model/uav/wsn-radio-mac.h/.cc`:
- Bridge layer giữa Phase 0/1 packet types và CC2420/LR-WPAN radio
- Serialize/deserialize `HelloPacket`, `CLAnnouncementPacket`, `CLMemberFeedbackPacket`, `Fragment`

`src/wsn/model/uav/mac-frame.h/.cc`:
- `WsnMacHeader` (7 bytes): `Type(1B) | SeqNum(2B) | SrcId(2B) | DstId(2B)`
- Types: `HELLO_PACKET`, `CL_ANNOUNCEMENT`, `MEMBER_FEEDBACK`, `UAV_FRAGMENT`

### 5.9. Fragment header

`src/wsn/model/uav/fragment.h`:
- Fragment struct: ID, confidence value, source info
- Dùng cho UAV broadcast payload

---

## 6. WSN Routing Abstractions

### 6.1. Base classes

`src/wsn/model/routing/`:
- `WsnRoutingProtocol`: base class, listen/send qua `WsnForwarder`
- `WsnForwarder`: packet forwarding middleware
- `WsnRoutingHeader`: generic header (source, destination, hop count, type)

### 6.2. Bypass Routing

`src/wsn/model/routing/bypass-routing/`:
- `BypassRoutingProtocol`: không có logic routing, chỉ forward thẳng
- Mục đích: debug và baseline comparison

### 6.3. PECEE Routing

`src/wsn/model/routing/pecee-routing/`:
- `PeceeRoutingProtocol`: tương thích với PECEE protocol
- `PeceeHeader`: PECEE-specific packet header
- Dùng cho thí nghiệm so sánh với PECEE baseline

---

## 7. Scenario1 — Basic CC2420 Grid

### 7.1. Mô tả

Kịch bản đầu tiên dùng CC2420 NetDevice thật:
- NxN grid của ground nodes
- CC2420 PHY/MAC/NetDevice với SpectrumChannel
- Traffic pattern đơn giản: center node và 4 góc gửi broadcasts
- Thống kê TX/RX per node

### 7.2. Files

- `src/wsn/examples/example1.cc`: entrypoint
- `src/wsn/model/routing/scenario1/node-routing.h/.cc`: `InitializeNodeRouting()`, `ScheduleTransmission()`, `ScheduleScenario1TrafficPattern()`
- `src/wsn/model/routing/scenario1/ground-node-routing.h/.cc`: receive callbacks
- `src/wsn/model/routing/scenario1/uav-node-routing.h/.cc`: UAV-side routing stub
- `src/wsn/model/routing/scenario1/fragment.h/.cc`: basic fragment header

### 7.3. Ý nghĩa

Scenario1 là lần đầu tiên xác nhận CC2420 NetDevice có thể gửi/nhận packet qua SpectrumChannel trong module tự build. Đây là milestone xác nhận base platform hoạt động.

---

## 8. uav-example — Phase 0+1 Integration

### 8.1. Mô tả

`src/wsn/examples/uav-example.cc` tích hợp Phase 0 (cell forming) với Phase 1 (UAV broadcast):
- Ground nodes tự hình thành cell topology với CL election
- UAV bắt đầu broadcast fragments sau khi cell formation hoàn tất

### 8.2. Simulation timeline

```
t=0s    : Ground nodes khởi tạo, Phase 0 bắt đầu
t=0–6s  : Cell formation (HELLO → CL election → routing tables)
t≈1.5s  : CL election hoàn tất (fitness-based)
t≈3s    : Routing tables sẵn sàng, tất cả CL ở ROUTING_READY
t=5s    : Status check cell formation
t=6s    : UAV bắt đầu broadcast fragments
t=6–T   : UAV broadcast trong khi cells operational
```

### 8.3. Kỹ thuật delivery

Phase 0 dùng callback-based delivery với delay 10 ms thay vì radio thật. Giữ Phase 0 độc lập với radio layer để tích hợp sạch.

### 8.4. cell-forming-example

`src/wsn/examples/cell-forming-example.cc`:
- Demo Phase 0 standalone
- In thống kê: số CL, số member, routing table status per cell

---

## 9. Scenario3 — Multi-UAV With Startup Phase

### 9.1. Mô tả

Bước nhảy lớn so với Scenario1:
- CC2420 NetDevice thật cho ground nodes
- Nhiều UAV hoạt động đồng thời
- Ground nodes chạy startup phase (neighbor discovery, routing setup)
- Fragment-based confidence tracking đầy đủ
- **Suspicious region detection** tự động
- Greedy flight path cho UAV

### 9.2. Files

**Orchestration layer**:
- `src/wsn/examples/example3.cc`
- `src/wsn/examples/scenarios/scenario3.h/.cc`: `SetupScenario3Network()`, suspicious region, flight path, scheduling

**Routing backend** (`src/wsn/model/routing/scenario3/`):

| File | Vai trò |
|---|---|
| `node-routing.h/.cc` | Compatibility wrapper |
| `parameters.h` | Constants |
| `fragment.h/.cc` | Fragment với confidence |
| `packet-header.h/.cc` | Packet headers |
| `ground-node-routing.h/.cc` | Ground node receive + cooperation |
| `uav-node-routing.h/.cc` | UAV periodic broadcast |
| `ground-node-routing/global-startup-phase.h/.cc` | Startup phase handlers |
| `ground-node-routing/global-startup-phase-packet.h/.cc` | Startup packet types |
| `uav-node-routing/` | (placeholder, trống) |

### 9.3. Tính năng mới trong Scenario3

**Startup phase**:
- Ground nodes trao đổi HELLO qua CC2420
- Build neighbor table và two-hop neighbor table
- Leader election và cell topology

**Suspicious region selection**:
- BS chọn vùng từ grid
- Xác định bằng tập node IDs trong radius xác định

**Greedy flight path**:
- Tính waypoints greedy để cover suspicious region
- Đảm bảo coverage tối đa trong sim time

**Fragment confidence**:
- BS sinh N fragments, mỗi mang confidence value
- UAV broadcast tuần tự
- Ground nodes tích lũy, dedup, tổng hợp
- Alert khi tổng confidence ≥ threshold

### 9.4. Tài liệu kèm

- `src/wsn/model/routing/scenario3/ARCHITECTURE_ANALYSIS.md`: phân tích kiến trúc và recommendations
- `src/wsn/model/routing/scenario3/ground-node-routing/GROUND_NODE_STATE.md`: state machine diagram

---

## 10. Scenario4 — Full BS-Coordinated UAV Mission

### 10.1. Mô tả

Kịch bản đầy đủ nhất:
- BS điều phối toàn bộ: network setup, cell assignment, routing trees, suspicious region, fragment pool, UAV flight plans
- Hai UAV với vai trò tách biệt (UAV1 — suspicious point recon, UAV2 — broadcast coverage)
- Cooperation fallback sau khi UAV2 hoàn thành path
- Mission completion tracking với timestamps

### 10.2. Files

**Orchestration layer** (`src/wsn/examples/scenarios/scenario4/`):
- `scenario4-api.h/.cc`, `scenario4-config.h/.cc`, `scenario4-scheduler.h/.cc`
- `scenario4-metrics.h/.cc`, `scenario4-visualizer.h/.cc`, `scenario4-params.h`

**Routing backend** (`src/wsn/model/routing/scenario4/`):

| Sub-layer | Files | Vai trò |
|---|---|---|
| Top-level | `node-routing.h/.cc` | Dispatcher, mission state, getters |
| Fragment | `fragment.h/.cc` | Fragment với confidence |
| Packet header | `packet-header.h/.cc` | Header format |
| Base station | `base-station-node/base-station-node.h/.cc` | BS orchestration root |
| BS network setup | `base-station-node/network-setup.h/.cc` | Grid init, cell assignment |
| BS region selection | `base-station-node/region-selection.h/.cc` | Suspicious region |
| BS fragment gen | `base-station-node/fragment-generator.h/.cc` | Fragment pool |
| BS UAV control | `base-station-node/uav-control.h/.cc` | UAV flight plan |
| Ground routing | `ground-node-routing/ground-node-routing.h/.cc` | RX + cooperation |
| Ground startup | `ground-node-routing/startup-phase.h/.cc` | Phase 0 tại routing layer |
| Cell cooperation | `ground-node-routing/cell-cooperation.h/.cc` | Post-UAV cooperation |
| UAV routing | `uav-node-routing/uav-node-routing.h/.cc` | UAV routing dispatcher |
| Fragment broadcast | `uav-node-routing/fragment-broadcast.h/.cc` | Periodic broadcast |
| Helper | `helper/` | Utility functions |
| Globals | `scenario4-routing-globals.cc` | Global state |
| Params | `scenario4-params.cc` | Default parameters |

### 10.3. BS initialization flow

```
BaseStationNode::Initialize()
├── SetupGridNetwork()            → positions, cell IDs, colors
├── DiscoverNeighbors()           → one-hop + two-hop neighbor tables
├── ElectCellLeaders()            → fitness-based CL election
├── BuildGatewayPairs()           → inter-cell gateway nodes
├── BuildRoutingTrees()           → intra/inter-cell routing trees
├── SelectSuspiciousRegion()      → candidate + suspicious + seed nodes
├── GenerateFragmentsForBsInit()  → fragment pool từ suspicious info
└── PlanUavFlightPathsForBsInit() → waypoints cho UAV1 + UAV2
```

### 10.4. Mission completion mechanism

Public getters trong `node-routing.cc`:
- `IsUav1MissionCompleted()` / `GetUav1MissionCompletedTime()`
- `IsUav2MissionCompleted()` / `GetUav2MissionCompletedTime()`

`MarkUav2MissionCompleted()` được trigger từ ground node khi seed node đạt `ALERT_THRESHOLD`.

### 10.5. Cooperation fallback

`InitializeCellCooperationTimeout()` kích hoạt sau khi UAV2 đi hết path:
- Ground nodes còn thiếu fragment chia sẻ trong cell
- Bù cho trường hợp broadcast UAV2 chưa đủ để complete mission

### 10.6. Bugfixes đã xử lý

#### 10.6.1. Confidence aggregation bug
- Lỗi: aggregation dùng average thay vì accumulate từ fragment pool.
- Tác động: confidence và alert trigger có thể sai.
- Fix: chỉnh logic trong `fragment.cc`.

#### 10.6.2. Random seed bug
- Lỗi: dùng `rand()` C-style, không đồng bộ với ns-3 RNG.
- Tác động: reproducibility kém giữa các runs.
- Fix: dùng ns-3 `UniformRandomVariable` trong `base-station-node.cc`.

#### 10.6.3. Visualizer post-mission bug
- Lỗi: visualizer render live UAV events sau khi mission hoàn tất.
- Fix: sửa frontend logic trong `app.js`.

### 10.7. Output redesign cho batch

Trong `example4.cc`:
- `g_resultFileStream = nullptr` → suppress detailed event logs
- `WriteScenario4Summary(seed, runId, ...)` → ghi summary cuối run

Format `scenario4_result_<seed>_<runId>.txt`:
```
SCENARIO=4
RUN=...
[CONFIG]
gridSize=... gridSpacing=... simTime=...
[PARAMS]
uav1Speed=... uav2Speed=... broadcastRadius=... numFragments=...
[NETWORK]
groundNodes=... suspiciousNodes=... generatedFragments=...
[MISSION]
uav1Completed=... uav1CompletedTime=...
uav2Completed=... uav2CompletedTime=...
```

### 10.8. Tài liệu kèm theo

- `src/wsn/examples/scenarios/scenario4/EXAMPLE4_EXECUTION_FLOW.md`: execution flow chi tiết
- `src/wsn/examples/scenarios/scenario4/SCENARIO4_REBUILD_TREE.md`: function tree + function-to-function 11 nhánh

---

## 11. Scenario5 — Independent Experimental Branch

### 11.1. Mục đích

Tách Scenario5 hoàn toàn khỏi Scenario4 để có nhánh thử nghiệm song song mà không làm ảnh hưởng baseline.

### 11.2. Quá trình tách

**Bước 1 — Tạo example5.cc**:
- Copy và chỉnh từ `example4.cc`
- Thêm target `example5` vào `examples/CMakeLists.txt`

**Bước 2 — Tách orchestration layer**:
- Tạo `src/wsn/examples/scenarios/scenario5/` với đầy đủ files:
  - `scenario5-api.h/.cc`, `scenario5-config.h/.cc`, `scenario5-scheduler.h/.cc`
  - `scenario5-metrics.h/.cc`, `scenario5-visualizer.h/.cc`, `scenario5-params.h`

**Bước 3 — Tách routing backend**:
- Copy `model/routing/scenario4/` → `model/routing/scenario5/`
- Đổi namespace và naming
- Đổi globals file names

**Vấn đề phát sinh**:

1. **Namespace alias collision**: files khai báo lại alias trong phạm vi đã có cùng tên → fix bằng cách bỏ alias thừa.

2. **Duplicate log component registration**: `NS_LOG_COMPONENT_DEFINE` vẫn giữ tên cũ sau khi copy → abort runtime do trùng tên.
   - Fix: đổi tên log components:
     - `BaseStationNode` → `Scenario5BaseStationNode`
     - `GroundNodeRouting` → `Scenario5GroundNodeRouting`
     - `UavNodeRouting` → `Scenario5UavNodeRouting`
     - `FragmentBroadcast` → `Scenario5FragmentBroadcast`

**Bước 4 — Build integration**:
- Thêm toàn bộ source/header scenario5 vào `src/wsn/CMakeLists.txt`

### 11.3. Kết quả

Scenario5 độc lập hoàn toàn:
- Entrypoint riêng (`example5.cc`)
- Orchestration layer riêng
- Routing backend riêng
- Output naming riêng (`scenario5_result_*`)

### 11.4. Tài liệu kèm theo

`src/wsn/examples/scenarios/scenario5/SCENARIO5_REBUILD_TREE.md` bao gồm:
- Function tree tổng quan
- Function-to-function tree 11 nhánh chính
- Phân tích các function có thể nâng cấp cho UAV2
- Research questions, hypotheses, methodology, experiment design

---

## 12. Autorun Batch Runner

### 12.1. Mục đích

Chạy nhiều rounds với seeds khác nhau để lấy dữ liệu thống kê, với:
- Seed và runId thay đổi mỗi round
- Parse output sau mỗi run
- Tổng hợp kết quả tự động
- Dọn dẹp intermediate files để tiết kiệm dung lượng

### 12.2. Files

- `src/wsn/examples/scenarios/scenario5/autorun/scenario5-batch-runner.py`
- `src/wsn/examples/scenarios/scenario5/autorun/README.md`

### 12.3. Cách dùng

```bash
python3 src/wsn/examples/scenarios/scenario5/autorun/scenario5-batch-runner.py \
  --repo-root . \
  --rounds 100 \
  --start-seed 1 \
  --start-run-id 1 \
  --sim-time 200
```

### 12.4. Luồng xử lý

```
for round in 1..N:
    seed = start_seed + round - 1
    ./ns3 run "example5 --seed=... --runId=... --simTime=..."
    parse scenario5_result_<seed>_<runId>.txt
    append round summary to batch TXT
    delete per-round result file + stdout/stderr logs

append [CONCLUSION] block to batch TXT
```

### 12.5. Output — chỉ một file cuối

`src/wsn/examples/visualize/results/batch/scenario5/scenario5_batch_summary.txt`

```
[CONFIGS]
rounds=... simTime=... gridSize=... gridSpacing=...

[PARAMS]
uav1Speed=... uav2Speed=... numFragments=...

[ROUNDS]
round=1 seed=1 runId=1
durationSec=...
suspiciousNodes=...
uav1CompletedTime=...
uav2CompletedTime=...
...

[CONCLUSION]
totalRounds=...
okRounds=...
failedRounds=...
uav1CompletionCount=...
uav2CompletionCount=...
uav2EarlierCount=...
uav1EarlierCount=...
earlierRate=...
uav1MeanCompletionTime=...
uav2MeanCompletionTime=...
averageEarlierTime=...
```

---

## 13. Build-System Integration

### 13.1. `src/wsn/CMakeLists.txt`

Library `libwsn` bao gồm:
- CC2420 radio sources
- WSN object model sources
- UAV MAC / cell forming sources
- Routing abstractions (bypass, pecee, wsn-routing)
- Scenario1, Scenario3, Scenario4, Scenario5 routing backends

### 13.2. `src/wsn/examples/CMakeLists.txt`

| Target | Sources chính |
|---|---|
| `example1` | scenario1 routing |
| `cc2420-example` | CC2420 module |
| `uav-example` | UAV MAC + cell forming |
| `cell-forming-example` | cell forming |
| `example3` | scenario3 orchestration |
| `example4` | scenario4 orchestration |
| `example5` | scenario5 orchestration |

---

## 14. Validation and Current Results

### 14.1. Build validation

Đã confirm clean build và run:
- `./ns3 run "example4 --simTime=10"` — ✅ OK
- `./ns3 run "example5 --simTime=10"` — ✅ OK
- `./ns3 run "example3 --gridSize=5 --simTime=7"` — ✅ OK
- `./ns3 run "uav-example --gridSize=10 --gridSpacing=50 --duration=80 --numFragments=8"` — ✅ OK

### 14.2. Suspicious region validation

```bash
for grid in 3 5 7 10 15 20; do
    ./ns3 run "example3 --gridSize=$grid --simTime=7" 2>&1 | grep "SUSPICIOUS REGION FINAL"
done
```
→ Xác nhận suspicious region selection hoạt động đúng trên nhiều grid sizes.

### 14.3. Batch run snapshot — 50 rounds

```bash
python3 .../scenario5-batch-runner.py --rounds 50 --start-seed 1 --sim-time 200
```

| Metric | Giá trị |
|---|---|
| `totalRounds` | 50 |
| `okRounds` | 50 |
| `failedRounds` | 0 |
| `uav1CompletionCount` | 31 |
| `uav2CompletionCount` | 28 |
| `uav2EarlierCount` | 18 |
| `uav1EarlierCount` | 1 |
| `earlierRate` | 0.947 |
| `uav1MeanCompletionTime` | 97.85 s |
| `uav2MeanCompletionTime` | 17.49 s |
| `averageEarlierTime` | 75.39 s |

### 14.4. Diễn giải kết quả batch

- **0 failed rounds**: simulation ổn định hoàn toàn trên 50 rounds.
- **UAV2 hoàn thành sớm hơn ~80 s** khi cả hai cùng có kết quả: đây là tín hiệu mạnh về speed-to-impact của UAV2.
- **UAV2 completion rate thấp hơn một chút** (28/50 vs 31/50): cho thấy có room để cải thiện robustness.
- Đây là baseline dữ liệu quan trọng cho mọi so sánh với phiên bản nâng cấp UAV2 sau này.

---

## 15. Current Limitations

### 15.1. CC2420 PHY chưa hoàn thiện

- Signal processing còn stub.
- Không ảnh hưởng mission completion time metrics.
- Sẽ cần nếu nghiên cứu radio performance (SNR, BER, collision).

### 15.2. Startup phase chưa phải default path

Code có `RunStartupPhase()` nhưng default flow dùng precomputed setup trong `Build()`. Dynamic topology feedback chưa hoàn toàn.

### 15.3. UAV2 planning còn heuristic đơn giản

UAV2 dùng:
- Pre-planned greedy path
- Static broadcast interval
- Không có feedback loop từ ground nodes

Phù hợp làm baseline nhưng chưa đủ cho nghiên cứu tối ưu.

### 15.4. Mission completion metric còn cơ bản

Chưa capture:
- Confidence gain per second
- Broadcast efficiency (useful vs redundant deliveries)
- Cooperation dependency rate
- Region coverage quality score

### 15.5. Documentation drift risk

Code phát triển nhanh, các docs execution flow dễ lệch khỏi implementation nếu không cập nhật thường xuyên.

---

## 16. Research Directions for UAV2

### 16.1. Tại sao UAV2 là target chính

UAV2 nằm tại giao điểm của:
- Mobility planning
- Fragment dissemination efficiency
- Mission-time optimization
- Cell cooperation interaction
- Measurable batch outcomes

### 16.2. Ba điểm đòn bẩy quan trọng nhất

1. **`PlanUavFlightPathsForBsInit()`** — tối ưu "UAV2 đi đâu": coverage-aware vs greedy path
2. **`InitializeUavBroadcast()`** — tối ưu "phát gì, lúc nào, thứ tự nào": priority fragment ordering
3. **`MarkUav2MissionCompleted()`** — tối ưu "đo hoàn thành thế nào": richer mission score

### 16.3. Research questions

- **RQ1**: Giải pháp path planning nào cho UAV2 tối thiểu hóa thời gian hoàn thành mission?
- **RQ2**: Thứ tự broadcast fragment nào tối ưu hóa tốc độ confidence accumulation?
- **RQ3**: Feedback từ ground nodes có cải thiện broadcast policy không?
- **RQ4**: Trade-off nào tồn tại giữa coverage completeness và mission time?

### 16.4. Hypotheses

- **H1**: Coverage-aware path planning giảm mission time so với greedy.
- **H2**: Priority fragment broadcast (high-confidence-gain first) cải thiện convergence.
- **H3**: Feedback-driven adaptive interval giảm redundant broadcasts.

### 16.5. Tài liệu nghiên cứu hiện có

`SCENARIO5_REBUILD_TREE.md` đã chứa đầy đủ:
- Function-to-function tree 11 nhánh
- Phân tích từng function có thể nâng cấp cho UAV2
- RQ, hypotheses, methodology directions, experiment design, contribution framing

---

## 17. Overall Assessment

### 17.1. Engineering Readiness

| Thành phần | Sẵn sàng |
|---|---|
| CC2420 radio | ✅ Đủ cho packet-level simulation |
| WSN object model | ✅ Đủ cho current scenarios |
| Cell forming | ✅ Stable |
| Routing abstractions | ✅ Stable |
| Scenario1 | ✅ Stable |
| Scenario3 | ✅ Stable |
| Scenario4 | ✅ Stable + bugfixed |
| Scenario5 | ✅ Stable, independent |
| Autorun | ✅ Operational |

### 17.2. Research Readiness

- **Baseline experiments**: có thể chạy ngay.
- **Ablation studies**: framework đã tách đủ để thêm/bỏ từng thành phần.
- **Publication-level contribution**: cần thêm UAV2 optimization và richer metrics.

### 17.3. Hành trình triển khai

```
Bắt đầu:     ns-3 core + lr-wpan only (không có gì cho WSN)
     ↓
Phase 0A:    CC2420 radio (PHY/MAC/NetDevice/Energy) — mới hoàn toàn
Phase 0B:    WSN object model — mới hoàn toàn
Phase 0C:    Cell forming MAC layer (Phase 0 protocol) — mới hoàn toàn
Phase 0D:    WSN routing abstractions (bypass, pecee) — mới hoàn toàn
     ↓
Scenario1:   First CC2420 packet exchange — baseline xác nhận
     ↓
uav-example: Phase 0 + Phase 1 integration — demo tích hợp
     ↓
Scenario3:   Multi-UAV + startup phase + suspicious region
     ↓
Scenario4:   Full BS-coordinated mission + bugfixes + output pipeline
     ↓
Scenario5:   Independent branch + autorun + 50-round batch results
     ↓
Hiện tại:    Stable baseline + research framework ready
             → Next: UAV2 optimization + ablation + publication
```

Hệ thống đã vượt qua giai đoạn "làm cho chạy được" và đang ở điểm chuyển sang giai đoạn **tối ưu hóa có phương pháp và tạo đóng góp nghiên cứu** cho IEEE ICCE 2026.
