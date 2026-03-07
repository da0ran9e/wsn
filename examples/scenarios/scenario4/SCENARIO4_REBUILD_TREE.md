# Scenario4 - Proposed Folder Tree (Rebuild Preparation)

Mục tiêu: chuẩn bị cấu trúc mới cho `scenario4` theo hướng tách rõ tầng orchestration và tầng routing.

## 1) Cây thư mục đề xuất

```text
src/wsn/
├── examples/
│   ├── example4.cc
│   └── scenarios/
│       └── scenario4/
│           ├── README.md
│           ├── scenario4-params.h              [Scenario + Routing parameters centralized]
│           ├── scenario4-api.h
│           ├── scenario4-api.cc
│           ├── scenario4-config.h
│           ├── scenario4-config.cc
│           ├── scenario4-scheduler.h
│           ├── scenario4-scheduler.cc
│           ├── scenario4-metrics.h
│           ├── scenario4-metrics.cc
│           ├── scenario4-visualizer.h
│           └── scenario4-visualizer.cc
│
└── model/routing/scenario4/
    ├── fragment.h
    ├── fragment.cc
    ├── helper/
    │   ├── calc-utils.h                        [Common calculations: geometry, thresholds, scoring]
    │   └── calc-utils.cc
    ├── packet-header.h                         [All packet types: startup, fragment, cooperation, etc.]
    ├── packet-header.cc
    ├── node-routing.h
    ├── node-routing.cc
    ├── base-station-node/
    │   ├── base-station-node.h                 [BS node logic using callbacks/globals]
    │   ├── base-station-node.cc
    │   ├── network-setup.h                     [Network initialization]
    │   ├── network-setup.cc
    │   ├── region-selection.h                  [Suspicious region detection]
    │   ├── region-selection.cc
    │   ├── uav-control.h                       [UAV waypoint planning & control]
    │   ├── uav-control.cc
    │   ├── fragment-generator.h                [Fragment generation logic]
    │   └── fragment-generator.cc
    ├── ground-node-routing/
    │   ├── ground-node-routing.h
    │   ├── ground-node-routing.cc              [MOVED: Main ground routing logic]
    │   ├── startup-phase.h
    │   ├── startup-phase.cc
    │   ├── cell-cooperation.h                  [Cell cooperation protocol]
    │   └── cell-cooperation.cc                 [Fragment sharing logic]
    └── uav-node-routing/
        ├── uav-node-routing.h                  [Basic UAV routing]
        ├── uav-node-routing.cc
        ├── fragment-broadcast.h                [Fragment TX only]
        └── fragment-broadcast.cc
```

## 2) Tối giản `example4.cc`

`example4.cc` chỉ giữ:
- Parse CLI
- Enable log
- Gọi API cấp cao (`Build`, `Schedule`, `Run`)
- `Simulator::Destroy()`

## 3) Gợi ý API runner

```cpp
struct Scenario4RunConfig {
  // ground + uav + fragments + timing + BS location
};

class Scenario4Runner {
public:
  explicit Scenario4Runner(const Scenario4RunConfig& cfg);
  void Build();
  void Schedule();
  void Run();
};
```

## 4) Kiến trúc phân tầng mới

### A. Scenario Layer (orchestration only)
- `scenario4-params.h`: Tất cả parameters (scenario + routing) tập trung tại đây
- `scenario4-api.*`: Runner interface (Build, Schedule, Run)
- `scenario4-scheduler.*`: Event timeline management
- `scenario4-metrics.*`: Statistics collection
- `scenario4-visualizer.*`: Network visualization

### B. Routing Layer (BS + ground + UAV logic)

#### B.1. Base Station Node (`base-station-node/`)
- Là một node trong simulator (không phải orchestration layer)
- Giao tiếp qua **callbacks và global variables** thay vì network packets
- Trách nhiệm:
  - `network-setup.*`: Khởi tạo ground network topology
  - `region-selection.*`: Chọn suspicious region từ topology
  - `uav-control.*`: Tính waypoint, gửi command đến UAV qua callback
  - `fragment-generator.*`: Tạo file fragments với confidence phân bố

#### B.2. Ground Node Routing (`ground-node-routing/`)
- `ground-node-routing.cc`: Logic chính của ground node (RX/TX, fragment processing)
- `startup-phase.*`: Network discovery & topology building
- `cell-cooperation.*`: Protocol chia sẻ fragment trong cell

#### B.3. UAV Node Routing (`uav-node-routing/`)
- `uav-node-routing.cc`: Logic chính của UAV node
- `fragment-broadcast.*`: Periodic fragment transmission

#### B.4. Packet Headers (`packet-header.*`)
- Định nghĩa tất cả packet types:
  - `StartupPhasePacket`: Discovery messages
  - `FragmentPacket`: Fragment data
  - `CooperationPacket`: Cell cooperation requests
  - `UavCommandPacket`: BS → UAV commands (via callback, not network)

#### B.5. Helper (`helper/`)
- Chứa các hàm tính toán dùng chung để tránh lặp logic giữa BS/ground/UAV
- Ví dụ:
    - Tính khoảng cách, cell index, vùng lân cận
    - Tính confidence score, suspicious score
    - Utility chuẩn hóa dữ liệu đầu vào cho region-selection và UAV planning
- Quy tắc: chỉ chứa pure utility (không giữ state mô phỏng, không schedule event)

### C. Base Station Communication Model
- **BS → Ground nodes**: Callbacks/globals (không qua network)
- **BS → UAV**: Callbacks để gửi waypoint commands
- **Ground/UAV → BS**: Callbacks để báo cáo metrics/topology
- **Lợi ích**: 
  - Không overhead network packets
  - BS có thể đặt xa mạng mà vẫn điều khiển được
  - Tách biệt control plane (BS) và data plane (ground/UAV)

## 5) Pipeline event chuẩn

1. Build network (ground nodes + BS node far from grid)
2. Startup phase (topology discovery)
3. BS receives topology snapshot
4. BS selects suspicious region
5. BS calculates UAV flight path (waypoints)
6. UAV node creation at starting position
7. BS sends waypoint commands to UAV
8. UAV follows waypoints + fragment broadcast
9. Ground nodes: cell cooperation when threshold reached
10. Final metrics report from BS

## 6) File content highlights

### `scenario4-params.h` (centralized parameters)
```cpp
namespace scenario4 {
namespace params {
    // ===== SCENARIO PARAMETERS =====
    // Grid parameters
    const uint32_t DEFAULT_GRID_SIZE = 10;
    const double DEFAULT_SPACING = 40.0;
    
    // BS location (far from network)
    const double BS_POSITION_X = -1000.0;
    const double BS_POSITION_Y = -1000.0;
    
    // UAV parameters
    const double DEFAULT_UAV_ALTITUDE = 50.0;
    const double DEFAULT_UAV_SPEED = 10.0;
    
    // Timing constants
    const double STARTUP_PHASE_DURATION = 5.0;
    const double UAV_PLANNING_DELAY = 0.2;
    const double FRAGMENT_BROADCAST_INTERVAL = 1.0;
    
    // ===== ROUTING PARAMETERS =====
    // Thresholds
    const double COOPERATION_THRESHOLD = 0.35;
    const double ALERT_THRESHOLD = 0.75;
    const double SUSPICIOUS_COVERAGE_PERCENT = 0.30;
    
    // Network parameters
    const double TX_POWER_DBM = 0.0;
    const double RX_SENSITIVITY_DBM = -95.0;
    const uint32_t PACKET_SIZE = 100;
    const double HEX_CELL_RADIUS = 80.0;
}
}
```

### `base-station-node/base-station-node.h`
```cpp
namespace scenario4 {
namespace routing {

// Global callbacks for BS communication
extern std::function<void(const GlobalTopology&)> g_bsTopologyCallback;
extern std::function<void(uint32_t, const UavFlightPath&)> g_bsUavCommandCallback;

class BaseStationNode {
public:
    BaseStationNode(uint32_t nodeId);
    
    // Called by ground nodes via callback (not network)
    void ReceiveTopology(const GlobalTopology& topology);
    
    // Region selection & UAV planning
    std::set<uint32_t> SelectSuspiciousRegion();
    UavFlightPath CalculateFlightPath(const std::set<uint32_t>& targets);
    
    // Send command to UAV via callback
    void SendWaypointCommand(uint32_t uavNodeId, const UavFlightPath& path);
    
    // Collect metrics from ground nodes
    void CollectMetrics();
    
private:
    uint32_t m_nodeId;
    GlobalTopology m_topology;
    std::set<uint32_t> m_suspiciousNodes;
};

} // namespace routing
} // namespace scenario4
```

### `ground-node-routing/ground-node-routing.cc`
```cpp
namespace scenario4 {
namespace routing {

// Main ground node RX/TX logic
void OnGroundNodeReceivePacket(uint32_t nodeId, Ptr<Packet> packet, double rssiDbm) {
    // Try extract packet header
    Ptr<Packet> copy = packet->Copy();
    
    // Check packet type from unified packet-header
    PacketHeader header;
    copy->RemoveHeader(header);
    
    switch (header.GetType()) {
        case PACKET_TYPE_STARTUP:
            HandleStartupPhase(nodeId, packet, rssiDbm);
            break;
        case PACKET_TYPE_FRAGMENT:
            HandleFragment(nodeId, packet, rssiDbm);
            break;
        case PACKET_TYPE_COOPERATION:
            HandleCooperation(nodeId, packet, rssiDbm);
            break;
    }
}

} // namespace routing
} // namespace scenario4
```

### `packet-header.h` (unified packet definitions)
```cpp
namespace scenario4 {
namespace routing {

enum PacketType {
    PACKET_TYPE_STARTUP = 1,
    PACKET_TYPE_FRAGMENT = 2,
    PACKET_TYPE_COOPERATION = 3,
    PACKET_TYPE_UAV_COMMAND = 4  // Used in callback, not actual network packet
};

class PacketHeader : public ns3::Header {
public:
    void SetType(PacketType type);
    PacketType GetType() const;
    // ... Serialize/Deserialize
};

class StartupPhasePacket : public ns3::Header {
    // Discovery message fields
};

class FragmentPacket : public ns3::Header {
    // Fragment data fields
};

class CooperationPacket : public ns3::Header {
    // Cooperation request fields
};

} // namespace routing
} // namespace scenario4
```

### `helper/calc-utils.h` (common computation utilities)
```cpp
namespace scenario4 {
namespace routing {
namespace helper {

double CalculateDistance(double x1, double y1, double x2, double y2);
int32_t ComputeCellId(double x, double y, double cellRadius);
double ComputeSuspiciousScore(double confidence, uint32_t packetCount, double rssiDbm);
bool IsCooperationTriggered(double confidence, double threshold);

} // namespace helper
} // namespace routing
} // namespace scenario4
```

### `ground-node-routing/cell-cooperation.cc`
```cpp
namespace scenario4 {
namespace routing {

void InitializeCellCooperation() {
    // Setup cooperation state for all nodes
}

void RequestFragmentSharing(uint32_t nodeId, int32_t cellId) {
    // Node requests fragments from cell peers
    GroundNetworkState& state = g_groundNetworkPerNode[nodeId];
    
    if (state.confidence >= params::COOPERATION_THRESHOLD) {
        // Trigger cooperation via direct function call (no network packet)
        for (uint32_t peerId : GetNodesInCell(cellId)) {
            if (peerId != nodeId) {
                ShareFragments(peerId, nodeId);
            }
        }
    }
}

void ShareFragments(uint32_t fromNode, uint32_t toNode) {
    // Direct data sharing without network transmission
    // (simulates instant in-cell communication)
}

} // namespace routing
} // namespace scenario4
```

## 7) File này dùng cho bước "chuẩn bị xây lại"

Khi bắt đầu triển khai, ưu tiên tạo trước:
1. `scenario4-params.h` (constants & shared variables)
2. `helper/calc-utils.*` (common computation utilities)
3. `scenario4-api.*` (runner interface)
4. `scenario4-config.*` (config structures)
5. `scenario4-base-station.*` (BS node + UAV control)
6. `ground-node-routing/cell-cooperation.*` (cooperation protocol)
7. `scenario4-network-setup.*` (ground + BS setup)
8. `scenario4-scheduler.*` (event timeline)

## 8) Future Work / Phase-2 Improvements

Các cải tiến kiến trúc sau khi hoàn thành Phase-1 (ít rủi ro, lợi ích cao):

### 8.1. Chuẩn hóa interface giữa tầng
- **Mục tiêu**: Định nghĩa contract rõ ràng cho mọi component chính
- **Thực hiện**:
  - Tạo interface abstract cho `Scenario4Runner`, `BaseStationNode`, `GroundNodeRouting`, `UavNodeRouting`
  - Mọi giao tiếp qua callback dùng struct input/output cố định (tránh truyền tham số rời rạc)
  - Example: `struct TopologyReport { std::map<uint32_t, NodeInfo> nodes; double timestamp; }`
- **Lợi ích**: Dễ mock/test, tách biệt dependencies, tài liệu tự mô tả

### 8.2. Control Plane Callback Bus
- **Mục tiêu**: Tập trung quản lý callbacks thay vì global rải rác
- **Thực hiện**:
  - Tạo class `ControlBus` với các method: `RegisterTopologyCallback()`, `RegisterUavCommandCallback()`, `NotifyTopology()`, `SendUavCommand()`
  - Singleton hoặc dependency injection vào các component
  - Example:
    ```cpp
    class ControlBus {
    public:
        void RegisterTopologyObserver(std::function<void(const TopologySnapshot&)> cb);
        void NotifyTopology(const TopologySnapshot& snapshot);
    };
    ```
- **Lợi ích**: Dễ trace communication flow, tránh global pollution, testable

### 8.3. State Machine cho vòng đời simulation
- **Mục tiêu**: Đảm bảo các phase được thực hiện đúng thứ tự
- **Thực hiện**:
  - Thêm enum `SimulationState { INIT, STARTUP, TOPOLOGY_READY, PLANNING, UAV_ACTIVE, COOPERATION, FINALIZED }`
  - Validate state transition: `INIT -> STARTUP -> TOPOLOGY_READY -> ...`
  - Chặn gọi sai thứ tự (ví dụ: chưa có topology mà đã gọi `SelectSuspiciousRegion()`)
  - Log rõ mỗi lần chuyển state
- **Lợi ích**: Debug dễ hơn, fail-fast khi sai logic, reproducible behavior

### 8.4. DTO/Model dùng chung
- **Mục tiêu**: Giảm coupling giữa các module
- **Thực hiện**:
  - Tạo các struct/class dữ liệu chung:
    - `TopologySnapshot`: thông tin tất cả nodes, neighbors, cells
    - `SuspiciousRegion`: tập node IDs + metadata (score, coverage)
    - `UavFlightPath`: danh sách waypoints + timing
    - `MetricsSnapshot`: confidence, packet counts, cooperation events
  - Đặt trong namespace riêng: `scenario4::model::`
- **Lợi ích**: Single source of truth, dễ serialize/log, giảm phụ thuộc chéo

### 8.5. Early validation cho config
- **Mục tiêu**: Fail-fast ngay khi có config sai
- **Thực hiện**:
  - `scenario4-config.cc` implement method `bool Validate(std::string& errorMsg)`
  - Kiểm tra:
    - Thresholds hợp lệ (0 < cooperation < alert < 1)
    - Grid size > 0, spacing > 0
    - BS position không trùng với ground nodes
    - Timing hợp lý (startup < total sim time)
  - Gọi `Validate()` trong `Scenario4Runner::Build()`
- **Lợi ích**: Tránh mất thời gian chạy simulation sai config, clear error messages

### 8.6. Deterministic & Reproducible
- **Mục tiêu**: Kết quả giống nhau với cùng parameters
- **Thực hiện**:
  - Đưa `seed` và `run` ID vào `Scenario4RunConfig`
  - Log toàn bộ config parameters khi bắt đầu simulation
  - Sử dụng RNG với seed cố định cho random placement/timing
  - Output format: `scenario4_grid10_seed42_run1.log`
- **Lợi ích**: So sánh kết quả giữa các lần chạy, debug dễ hơn, scientific reproducibility

### 8.7. Nâng cấp helper theo domain
- **Mục tiêu**: Tổ chức tốt hơn khi số lượng utility tăng
- **Thực hiện**:
  - Nếu `calc-utils.*` phình to, tách thành:
    - `geometry-utils.*`: distance, cell ID, grid position
    - `score-utils.*`: suspicious score, confidence aggregation
    - `threshold-utils.*`: cooperation trigger, alert detection
  - Giữ nguyên quy tắc: pure functions, không state, không schedule
- **Lợi ích**: Dễ tìm hàm cần dùng, test riêng từng domain

### 8.8. Test seam ngay từ đầu
- **Mục tiêu**: Viết unit test song song với implementation
- **Thực hiện**:
  - Thiết kế mock points cho:
    - `RegionSelection`: mock topology input → verify suspicious nodes output
    - `UavPlanning`: mock suspicious region → verify waypoint sequence
    - `CooperationTrigger`: mock confidence values → verify cooperation calls
  - Tạo test suite trong `src/wsn/test/scenario4/`
  - Example:
    ```cpp
    TEST(RegionSelection, SelectsTop30PercentNodes) {
        TopologySnapshot mockTopo = CreateMockTopology();
        auto suspiciousNodes = SelectSuspiciousRegion(mockTopo);
        EXPECT_EQ(suspiciousNodes.size(), mockTopo.totalNodes * 0.3);
    }
    ```
- **Lợi ích**: Catch bugs sớm, regression testing, confidence khi refactor

### 8.9. Logging & Observability tốt hơn
- **Mục tiêu**: Debug và analyze kết quả dễ dàng hơn
- **Thực hiện**:
  - Structured logging với levels: `TRACE`, `DEBUG`, `INFO`, `WARN`, `ERROR`
  - Log key events:
    - Topology snapshot received (số nodes, cells)
    - Suspicious region selected (node IDs, scores)
    - UAV waypoints calculated (path, estimated time)
    - Cooperation triggered (from/to nodes, fragments shared)
  - Export metrics theo timeline: `metrics_timeline.csv`
- **Lợi ích**: Visualize simulation progress, compare scenarios, paper figures
