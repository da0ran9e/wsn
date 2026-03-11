# GroundNetworkState - Thiết kế trạng thái Node

## Tổng quan

`GroundNetworkState` lưu trữ **tất cả trạng thái** của ground node trong suốt vòng đời mạng Scenario5, từ khởi tạo đến kết thúc simulation.

Struct được lưu trong global map:
```cpp
std::map<uint32_t, GroundNetworkState> g_groundNetworkPerNode;
```

## Vòng đời Node

```
Initialization → Startup Phase → Cell Formation → Fragment Collection → Cooperation → Topology Reporting
```

## Cấu trúc Chi tiết

### 1. Node Identity (Định danh)

| Trường | Kiểu | Mô tả |
|--------|------|-------|
| `nodeId` | `uint32_t` | ID duy nhất của node trong ns-3 |
| `cellId` | `int32_t` | Cell ID dựa trên vị trí địa lý (tính từ coordinates) |
| `isCellLeader` | `bool` | Node này có phải cell leader không (chưa implement) |

**Khởi tạo:** [InitializeGroundNodeRouting()](ground-node-routing.cc#L28)
- `nodeId` = từ NodeContainer
- `cellId` = tính từ vị trí sử dụng `helper::ComputeCellId()`
- `isCellLeader` = false (mặc định)

---

### 2. Neighbor Discovery (Khám phá láng giềng)

| Trường | Kiểu | Mô tả |
|--------|------|-------|
| `neighbors` | `std::set<uint32_t>` | Danh sách node IDs trong tầm phát hiện |
| `neighborRssi` | `std::map<uint32_t, double>` | RSSI (dBm) từ mỗi neighbor |
| `neighborDistance` | `std::map<uint32_t, double>` | Khoảng cách (m) đến mỗi neighbor |
| `startupComplete` | `bool` | Đã hoàn thành startup discovery chưa |

**Cập nhật trong:** [RunStartupPhase()](startup-phase.cc#L20)
- Tính khoảng cách giữa các nodes
- Build neighbor map cho nodes trong range
- Mô phỏng packet exchange và RSSI

---

### 3. Fragment Management (Quản lý mảnh dữ liệu)

| Trường | Kiểu | Mô tả |
|--------|------|-------|
| `fragments` | `FragmentCollection` | Collection các fragment node đang giữ |
| `confidence` | `double` | Tổng confidence = `fragments.totalConfidence` |
| `fragmentLastUpdateTime` | `std::map<uint32_t, double>` | Timestamp cập nhật mỗi fragment (fragmentId → time) |
| `fragmentsReceivedFromUav` | `uint32_t` | Số fragment nhận từ UAV broadcast |
| `fragmentsReceivedFromPeers` | `uint32_t` | Số fragment nhận từ cell peers (cooperation) |

**Khởi tạo:**
- Mỗi node generate random fragments: `GenerateFragments(numFragments)`
- Initial confidence tính từ fragment collection

**Cập nhật trong:** [OnGroundNodeReceivePacket()](ground-node-routing.cc#L91) với `PACKET_TYPE_FRAGMENT`
- So sánh confidence mới vs cũ
- Chỉ update nếu confidence cao hơn
- Track timestamp và source (UAV/peer)

---

### 4. Cell Cooperation (Hợp tác trong cell)

| Trường | Kiểu | Mô tả |
|--------|------|-------|
| `cellPeers` | `std::set<uint32_t>` | Danh sách node IDs cùng cell |
| `peerConfidence` | `std::map<uint32_t, double>` | Confidence của từng peer (thông tin chia sẻ) |
| `cooperationRequestsSent` | `uint32_t` | Số lần request sharing fragment |
| `cooperationRequestsReceived` | `uint32_t` | Số lần nhận request từ peer |
| `lastCooperationTime` | `double` | Timestamp lần cooperation gần nhất |
| `cooperationEnabled` | `bool` | Node có tham gia cooperation không |

**Sử dụng trong:** [RequestFragmentSharing()](cell-cooperation.cc#L34), [ShareFragments()](cell-cooperation.cc#L15)
- Check threshold trước khi cooperation
- Chia sẻ fragment với peers cùng cell
- Track cooperation activity

---

### 5. Communication Statistics (Thống kê truyền thông)

| Trường | Kiểu | Mô tả |
|--------|------|-------|
| `packetCount` | `uint32_t` | Tổng số packet nhận được (all types) |
| `startupPacketsReceived` | `uint32_t` | Packet nhận trong startup phase |
| `fragmentPacketsReceived` | `uint32_t` | Packet fragment từ UAV/peers |
| `cooperationPacketsReceived` | `uint32_t` | Packet cooperation từ peers |
| `packetsSent` | `uint32_t` | Tổng số packet đã gửi |
| `totalBytesReceived` | `double` | Tổng bytes nhận được |
| `totalBytesSent` | `double` | Tổng bytes đã gửi |

**Cập nhật trong:** [OnGroundNodeReceivePacket()](ground-node-routing.cc#L91)
- Mỗi lần nhận packet: increment counters tương ứng
- Accumulate bytes cho bandwidth tracking

---

### 6. UAV Interaction (Tương tác với UAV)

| Trường | Kiểu | Mô tả |
|--------|------|-------|
| `uavsInRange` | `std::set<uint32_t>` | Danh sách UAV node IDs đang trong tầm |
| `uavRssi` | `std::map<uint32_t, double>` | RSSI từ mỗi UAV |
| `lastUavContactTime` | `double` | Timestamp lần cuối liên lạc với UAV |
| `uavEncounters` | `uint32_t` | Số lần gặp UAV (encounter events) |

**Cập nhật trong:** [OnGroundNodeReceivePacket()](ground-node-routing.cc#L91) khi nhận fragment
- Track UAV presence
- Monitor signal strength
- Count UAV visits

---

### 7. Topology Reporting (Báo cáo topology)

| Trường | Kiểu | Mô tả |
|--------|------|-------|
| `lastTopologyReportTime` | `double` | Timestamp lần cuối báo cáo topology |
| `topologyReportCount` | `uint32_t` | Số lần đã gửi topology lên BS |

**Sử dụng trong:** [SendTopologyToBS()](ground-node-routing.cc#L165)
- Định kỳ build topology snapshot
- Cache cho BS pull-based access
- [BuildTopologySnapshot()](ground-node-routing.cc#L129) aggregate tất cả node states

---

### 8. Energy & Resource (Năng lượng và tài nguyên)

| Trường | Kiểu | Mô tả |
|--------|------|-------|
| `remainingEnergy` | `double` | Năng lượng còn lại (joules) |
| `energyConsumedTx` | `double` | Năng lượng tiêu thụ truyền |
| `energyConsumedRx` | `double` | Năng lượng tiêu thụ nhận |

**Model:**
- Initial energy: 1000 joules
- Rx cost: ~1mJ per KB (`0.001 * bytes / 1024`)
- Tx cost: sẽ implement khi có transmission tracking

**Cập nhật trong:** [OnGroundNodeReceivePacket()](ground-node-routing.cc#L91)

---

### 9. Timing (Thời gian)

| Trường | Kiểu | Mô tả |
|--------|------|-------|
| `initializationTime` | `double` | Thời điểm khởi tạo node |
| `lastActivityTime` | `double` | Thời điểm hoạt động gần nhất |

**Mục đích:** Tracking node lifetime và activity patterns

---

## Sử dụng

### 1. Khởi tạo
```cpp
InitializeGroundNodeRouting(groundNodes, numFragments);
// → Tạo GroundNetworkState cho mỗi node, populate vào g_groundNetworkPerNode
```

### 2. Truy cập state
```cpp
if (g_groundNetworkPerNode.find(nodeId) != g_groundNetworkPerNode.end()) {
    GroundNetworkState& state = g_groundNetworkPerNode[nodeId];
    // Đọc/ghi các trường
}
```

### 3. Update state khi nhận packet
```cpp
OnGroundNodeReceivePacket(nodeId, packet, rssiDbm);
// → Tự động update counters, fragments, energy, timing
```

### 4. Build topology snapshot cho BS
```cpp
GlobalTopology topo = BuildTopologySnapshot();
// → Aggregate tất cả states thành topology view
```

---

## Mở rộng trong tương lai

### Cell Leader Selection
- Sử dụng `isCellLeader` flag
- Algorithm: chọn node có confidence cao nhất trong cell
- Cell peers coordinate qua cooperation protocol

### Source Tracking
- Cần extend `PacketHeader` thêm `sourceNodeId` field
- Phân biệt chính xác fragment từ UAV vs peer
- Track communication patterns

### Energy Model
- Implement chi tiết hơn: idle/sleep/tx/rx power states
- Battery depletion simulation
- Node death khi energy = 0

### QoS Metrics
- Latency tracking (packet send → receive)
- Throughput calculation
- Packet loss rate

---

## Liên kết File

- Header: [ground-node-routing.h](ground-node-routing.h#L23)
- Implementation: [ground-node-routing.cc](ground-node-routing.cc#L28)
- Startup phase: [startup-phase.cc](startup-phase.cc#L20)
- Cooperation: [cell-cooperation.cc](cell-cooperation.cc#L15)
- Fragment struct: [../fragment.h](../fragment.h)
