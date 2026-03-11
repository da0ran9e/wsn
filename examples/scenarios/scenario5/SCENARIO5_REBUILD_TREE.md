# Scenario5 - Function Tree Analysis (Rebuild + Autorun Multi-Round Design)

Mục tiêu chính của tài liệu này là **phân tích cây function triển khai của `scenario5`**.

Trọng tâm:
- truy vết call flow thực tế từ `example5.cc`
- tách rõ function tree của orchestration layer và routing layer
- chỉ ra các điểm giao nhau giữa BS, ground nodes, UAV, cooperation
- thiết kế thêm function tree cho **autorun multi-round** dựa trên summary output hiện tại

Phần cây thư mục vẫn được giữ lại, nhưng chỉ đóng vai trò hỗ trợ cho phần phân tích function.

---

## 1) Cây thư mục đề xuất

```text
src/wsn/
├── examples/
│   ├── example5.cc
│   └── scenarios/
│       └── scenario5/
│           ├── README.md
│           ├── SCENARIO5_REBUILD_TREE.md
│           ├── scenario5-params.h              [Scenario + Routing parameters centralized]
│           ├── scenario5-api.h
│           ├── scenario5-api.cc
│           ├── scenario5-config.h
│           ├── scenario5-config.cc
│           ├── scenario5-scheduler.h
│           ├── scenario5-scheduler.cc
│           ├── scenario5-metrics.h
│           ├── scenario5-metrics.cc
│           ├── scenario5-visualizer.h
│           ├── scenario5-visualizer.cc
│           └── autorun/
│               ├── README.md
│               ├── scenario5-batch-config.yaml [Danh sách rounds / seeds / overrides]
│               ├── scenario5-batch-runner.py   [Điều phối nhiều lần chạy example5]
│               ├── scenario5-summary-parser.py [Parse file summary thành record]
│               ├── scenario5-aggregate.py      [Gom CSV / JSON / thống kê]
│               └── scenario5-report-template.md
│
└── model/routing/scenario5/
    ├── fragment.h
    ├── fragment.cc
    ├── scenario5-params.cc                     [Globals riêng của scenario5]
    ├── scenario5-routing-globals.cc            [Routing globals riêng của scenario5]
    ├── helper/
    │   ├── calc-utils.h                        [Common calculations: geometry, thresholds, scoring]
    │   └── calc-utils.cc
    ├── packet-header.h                         [All packet types: startup, fragment, cooperation, etc.]
    ├── packet-header.cc
    ├── node-routing.h                          [Mission state + shared routing API]
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
    │   ├── ground-node-routing.cc              [Main ground routing logic]
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

---

## 2) Function tree tổng quan của Scenario5

Đây là cây function quan trọng nhất của `scenario5` ở thời điểm hiện tại:

```text
example5.cc
└── main(argc, argv)
    ├── Parse CLI -> Scenario5RunConfig
    ├── Scenario5RunConfig::Validate(...)
    ├── Scenario5Runner runner(config)
    ├── runner.Build()
    │   ├── BuildGroundNetwork()
    │   ├── BuildBaseStation()
    │   ├── BuildUavNodes()
    │   ├── InstallProtocolStack()
    │   ├── routing::InitializeGroundNodeRouting(...)
    │   ├── routing::InitializeBaseStation(...)
    │   │   └── BaseStationNode::Initialize()
    │   │       ├── AssignCellIdAndColorForGroundNodes(...)
    │   │       ├── DiscoverNeighborsAndTwoHopsForGroundNodes(...)
    │   │       ├── SelectCellLeadersByNearestCellCenter(...)
    │   │       ├── SelectCrosscellGatewayPairs(...)
    │   │       ├── BuildIntraCellRoutingTrees()
    │   │       ├── EnhanceRoutingTreesForGatewayAccess()
    │   │       ├── ValidateIntraCellRoutingTrees()
    │   │       ├── FinalizeGroundNodeStateFields()
    │   │       ├── SelectSuspiciousRegionForBsInit()
    │   │       ├── GenerateFragmentsForBsInit()
    │   │       └── PlanUavFlightPathsForBsInit()
    │   └── routing::InitializeUavRouting(...)
    ├── runner.Schedule()
    │   └── ScheduleSingleScenario5Event(config)
    │       ├── Simulator::Schedule(..., routing::InitializeUavFlight)
    │       ├── Simulator::Schedule(..., routing::InitializeUavBroadcast)
    │       ├── Simulator::Schedule(..., routing::InitializeCellCooperationTimeout)
    │       └── Simulator::Stop(...)
    ├── runner.Run()
    │   ├── Simulator::Run()
    │   │   ├── routing::InitializeUavFlight()
    │   │   ├── routing::InitializeUavBroadcast()
    │   │   └── routing::InitializeCellCooperationTimeout()
    │   └── ReportScenario5Metrics()
    ├── WriteScenario5Summary(...)
    │   ├── routing::g_groundNetworkPerNode
    │   ├── routing::GetSuspiciousNodes()
    │   ├── routing::GetUavFlightPaths()
    │   ├── routing::GetBsGeneratedFragments()
    │   ├── routing::IsUav1MissionCompleted()
    │   ├── routing::GetUav1MissionCompletedTime()
    │   ├── routing::IsUav2MissionCompleted()
    │   └── routing::GetUav2MissionCompletedTime()
    └── Simulator::Destroy()
```

### Nhận xét chính

- `example5.cc` chỉ là **entrypoint của một round**.
- `Scenario5Runner` là orchestration root cho toàn bộ single-run flow.
- `BaseStationNode::Initialize()` đang là nhánh function sâu nhất và quan trọng nhất ở phase chuẩn bị dữ liệu.
- Phần runtime thật sự bắt đầu sau `ScheduleSingleScenario5Event(config)` khi `Simulator::Run()` kích hoạt các callback đã được schedule.
- Summary file là điểm kết thúc của cây function single-run và cũng là contract đầu vào cho batch runner multi-round.

## 3) Phân tích theo từng nhánh function

### 3.1. Entry branch: `main()` -> `Scenario5Runner`

Nhánh này chịu trách nhiệm setup toàn cục cho một run:

```text
main()
├── parse config
├── validate config
├── create runner
├── Build()
├── Schedule()
├── Run()
├── WriteScenario5Summary()
└── Destroy()
```

Ý nghĩa:
- đây là nhánh ổn định nhất để giữ cho `example5` mỏng
- nếu cần autorun nhiều round, chỉ lặp ở ngoài executable chứ không thay đổi nhánh này
- `seed` và `runId` đi từ đây xuống toàn bộ simulation

### 3.2. Build branch: `Build()`

`Build()` hiện là phase dựng trạng thái ban đầu trước khi chạy event loop:

```text
Scenario5Runner::Build()
├── BuildGroundNetwork()
├── BuildBaseStation()
├── BuildUavNodes()
├── InstallProtocolStack()
├── routing::InitializeGroundNodeRouting(...)
├── routing::InitializeBaseStation(...)
└── routing::InitializeUavRouting(...)
```

Điểm quan trọng:
- `BuildGroundNetwork()` tạo node graph vật lý
- `InitializeGroundNodeRouting(...)` tạo state map `g_groundNetworkPerNode`
- `InitializeBaseStation(...)` gọi vào `BaseStationNode::Initialize()`, đây là nơi cây function domain thực sự mở rộng mạnh nhất

### 3.3. Base station initialization branch

Đây là nhánh function cần được coi là **trung tâm của scenario5**:

```text
routing::InitializeBaseStation(nodeId)
└── BaseStationNode::Initialize()
    ├── AssignCellIdAndColorForGroundNodes(cellRadius)
    ├── DiscoverNeighborsAndTwoHopsForGroundNodes(neighborRadius)
    ├── SelectCellLeadersByNearestCellCenter(cellRadius)
    ├── SelectCrosscellGatewayPairs(neighborRadius)
    ├── BuildIntraCellRoutingTrees()
    ├── EnhanceRoutingTreesForGatewayAccess()
    ├── ValidateIntraCellRoutingTrees()
    ├── FinalizeGroundNodeStateFields()
    ├── SelectSuspiciousRegionForBsInit()
    ├── GenerateFragmentsForBsInit()
    └── PlanUavFlightPathsForBsInit()
```

Phân nhóm trách nhiệm:
- **Topology shaping**:
  - `AssignCellIdAndColorForGroundNodes(...)`
  - `DiscoverNeighborsAndTwoHopsForGroundNodes(...)`
  - `SelectCellLeadersByNearestCellCenter(...)`
- **Routing preparation**:
  - `SelectCrosscellGatewayPairs(...)`
  - `BuildIntraCellRoutingTrees()`
  - `EnhanceRoutingTreesForGatewayAccess()`
  - `ValidateIntraCellRoutingTrees()`
- **Mission preparation**:
  - `SelectSuspiciousRegionForBsInit()`
  - `GenerateFragmentsForBsInit()`
  - `PlanUavFlightPathsForBsInit()`

Kết luận quan trọng:
- `scenario5` không chờ startup phase để mới có suspicious region hay fragment pool
- ngay trong BS initialization đã chuẩn bị sẵn:
  - suspicious nodes
  - fragment pool cho UAV2 broadcast
  - flight paths cho UAV

### 3.4. Scheduler branch

`Schedule()` hiện dùng nhánh single-event đơn giản:

```text
Scenario5Runner::Schedule()
└── ScheduleSingleScenario5Event(config)
    ├── schedule routing::InitializeUavFlight()
    ├── schedule routing::InitializeUavBroadcast()
    ├── schedule routing::InitializeCellCooperationTimeout()
    └── schedule Simulator::Stop(config.simTime)
```

Ý nghĩa:
- scheduler hiện rất mỏng
- startup discovery branch hiện không được dùng trong single-run path mặc định
- đây là điểm tốt cho autorun vì mỗi round có timeline dễ dự đoán

### 3.5. Runtime branch: UAV flight + UAV broadcast + cooperation timeout

Sau `Simulator::Run()`, cây function thực tế tách ra ba nhánh lớn:

```text
Simulator::Run()
├── routing::InitializeUavFlight()
├── routing::InitializeUavBroadcast()
└── routing::InitializeCellCooperationTimeout()
```

Ba nhánh này cùng đọc state đã được BS chuẩn bị từ trước.

### 3.6. UAV flight branch

```text
routing::InitializeUavFlight()
├── GetUavFlightPaths()
├── for each UAV path
│   └── Simulator::Schedule(arrivalTime, lambda waypoint arrival)
│       ├── NodeList::GetNode(uavNodeId)
│       ├── MobilityModel::SetPosition(...)
│       └── maybe mark UAV1 mission complete
└── update mission timestamps
```

Ý nghĩa domain:
- UAV1 completion không dựa vào fragment reception
- UAV1 completion được đánh dấu khi UAV1 rời suspicious point trong chuỗi waypoint

### 3.7. UAV broadcast branch

```text
routing::InitializeUavBroadcast()
├── GetUavFlightPaths()
├── select UAV2 path
├── GetBsGeneratedFragments()
├── compute broadcastStartTime / broadcastEndTime
├── for each cycle
│   └── Simulator::Schedule(currentTime, lambda broadcast fragment)
│       ├── NodeList::GetNode(uav2NodeId)
│       ├── find ground nodes in radius
│       ├── build FragmentPacket + PacketHeader
│       └── OnGroundNodeReceivePacket(groundNodeId, pkt, rssi)
└── stop at last UAV2 waypoint
```

Điểm quan trọng:
- UAV2 là nguồn fragment chính trong runtime path hiện tại
- nhánh này nối trực tiếp sang ground-node receive branch
- completion của UAV2 được quyết định gián tiếp khi suspicious seed node đạt `ALERT_THRESHOLD`

### 3.8. Ground receive branch

```text
OnGroundNodeReceivePacket(nodeId, packet, rssi)
├── update node statistics
├── remove PacketHeader
└── switch(packetType)
    ├── PACKET_TYPE_STARTUP
    │   └── update neighbor discovery state
    └── PACKET_TYPE_FRAGMENT
        ├── remove FragmentPacket
        ├── update fragment pool if newer/better
        ├── recompute confidence
        ├── update UAV / peer counters
        └── maybe MarkUav2MissionCompleted(nodeId, confidence)
```

Đây là nhánh hấp thụ dữ liệu chính của ground nodes.

Quan hệ phụ thuộc:
- nhận fragment từ UAV broadcast
- cập nhật `g_groundNetworkPerNode`
- là nguồn dữ liệu cho summary cuối run
- có thể kích hoạt `MarkUav2MissionCompleted(...)`

### 3.9. Cooperation timeout branch

```text
routing::InitializeCellCooperationTimeout()
├── GetUavFlightPaths()
├── read UAV2 last waypoint time
└── Simulator::Schedule(timeout, lambda)
    ├── for each ground node
    │   └── RequestFragmentSharing(nodeId, cellId)
    │       ├── inspect peers in same cell
    │       └── ShareFragments(peerId, nodeId)
    └── write timeout event
```

Vai trò:
- đây là nhánh fallback để các node chia sẻ fragment sau khi UAV2 kết thúc broadcast phase
- cooperation không phải nguồn fragment đầu tiên, mà là nguồn hợp nhất sau broadcast

### 3.10. Startup discovery branch (hiện có nhưng không nằm trên default single-run path)

Code vẫn có sẵn một nhánh startup riêng:

```text
RunStartupPhase()
├── build neighbor map by distance
├── exchange startup packets
│   └── OnGroundNodeReceivePacket(..., PACKET_TYPE_STARTUP, ...)
├── compute 2-hop neighbors
├── derive cell peers
├── select deterministic cell leaders
└── SendTopologyToBS()
```

Nhận định:
- đây là nhánh phù hợp nếu sau này muốn quay lại flow “discover topology -> gửi BS -> BS tick control”
- nhưng ở flow mặc định hiện tại, trọng tâm thực tế vẫn là nhánh BS initialization ở `Build()`

### 3.11. Function-to-function tree (11 nhánh chính) cho Scenario5

Phần này copy lại tinh thần cây function-to-function từ `scenario4`, nhưng đổi tên hàm và flow theo `scenario5` hiện tại.

```text
(1) example5.cc
└── main(argc, argv)
  ├── Parse CLI -> Scenario5RunConfig
  ├── Scenario5RunConfig::Validate(...)
  ├── Scenario5Runner runner(config)
  ├── runner.Build()
  ├── runner.Schedule()
  ├── runner.Run()
  ├── WriteScenario5Summary(...)
  └── Simulator::Destroy()

(2) scenario5-api.cc
└── Scenario5Runner::Build()
  ├── BuildGroundNetwork()
  ├── BuildBaseStation()
  ├── BuildUavNodes()
  ├── InstallProtocolStack()
  ├── routing::InitializeGroundNodeRouting(...)
  ├── routing::InitializeBaseStation(...)
  └── routing::InitializeUavRouting(...)

(3) node-routing.cc
└── InitializeBaseStation(nodeId)
  └── BaseStationNode::Initialize()
    ├── AssignCellIdAndColorForGroundNodes(...)
    ├── DiscoverNeighborsAndTwoHopsForGroundNodes(...)
    ├── SelectCellLeadersByNearestCellCenter(...)
    ├── SelectCrosscellGatewayPairs(...)
    ├── BuildIntraCellRoutingTrees()
    ├── EnhanceRoutingTreesForGatewayAccess()
    ├── ValidateIntraCellRoutingTrees()
    ├── FinalizeGroundNodeStateFields()
    ├── SelectSuspiciousRegionForBsInit()
    ├── GenerateFragmentsForBsInit()
    └── PlanUavFlightPathsForBsInit()

(4) scenario5-scheduler.cc
└── ScheduleSingleScenario5Event(config)
  ├── Simulator::Schedule(..., routing::InitializeUavFlight)
  ├── Simulator::Schedule(..., routing::InitializeUavBroadcast)
  ├── Simulator::Schedule(..., routing::InitializeCellCooperationTimeout)
  └── Simulator::Stop(...)

(5) node-routing.cc
└── InitializeUavFlight()
  ├── GetUavFlightPaths()
  ├── for each UAV path
  │   └── Simulator::Schedule(arrivalTime, waypoint lambda)
  │       ├── NodeList::GetNode(uavNodeId)
  │       ├── MobilityModel::SetPosition(...)
  │       └── maybe mark UAV1 mission complete
  └── complete UAV movement scheduling

(6) base-station-node/base-station-node.cc
└── PlanUavFlightPathsForBsInit()
  ├── prepare UAV1 path for suspicious-point mission
  ├── prepare UAV2 path for coverage/broadcast mission
  ├── SetUavFlightPath(uav1NodeId, path1)
  └── SetUavFlightPath(uav2NodeId, path2)

(7) node-routing.cc
└── InitializeUavBroadcast()
  ├── GetUavFlightPaths()
  ├── select UAV2 path
  ├── GetBsGeneratedFragments()
  ├── compute broadcastStartTime / broadcastEndTime
  └── Simulator::Schedule(..., broadcast lambda)
    ├── create FragmentPacket + PacketHeader
    ├── scan ground nodes in broadcast radius
    └── OnGroundNodeReceivePacket(groundNodeId, pkt, rssi)

(8) ground-node-routing/ground-node-routing.cc
└── OnGroundNodeReceivePacket(nodeId, packet, rssi)
  ├── update statistics
  ├── remove PacketHeader
  └── switch(packetType)
    ├── PACKET_TYPE_STARTUP
    └── PACKET_TYPE_FRAGMENT
      ├── remove FragmentPacket
      ├── update fragment pool / confidence
      └── maybe MarkUav2MissionCompleted(nodeId, confidence)

(9) node-routing.cc
└── MarkUav2MissionCompleted(triggerNodeId, triggerConfidence)
  ├── set g_uav2MissionCompleted = true
  ├── set g_uav2MissionCompletedTime = Simulator::Now()
  └── stop future broadcast work implicitly via g_uav2MissionCompleted checks

(10) node-routing.cc
└── InitializeCellCooperationTimeout()
  ├── GetUavFlightPaths()
  ├── read UAV2 last waypoint time
  └── Simulator::Schedule(timeout, lambda)
    ├── for each node needing help
    └── RequestFragmentSharing(nodeId, cellId)
      └── ShareFragments(peerId, nodeId)

(11) example5.cc
└── WriteScenario5Summary(...)
  ├── read g_groundNetworkPerNode
  ├── read GetSuspiciousNodes()
  ├── read GetUavFlightPaths()
  ├── read GetBsGeneratedFragments()
  ├── read GetUav1MissionCompletedTime()
  └── read GetUav2MissionCompletedTime()
```

Nhận xét ngắn cho cây (11):
- Nhánh `(3)` là phase chuẩn bị lớn nhất.
- Nhánh `(6) + (7) + (8) + (9)` là chuỗi function cốt lõi quyết định hiệu quả của `UAV2`.
- Nhánh `(10)` là fallback logic cho trường hợp `UAV2` không phủ đủ fragment để ground network tự hoàn tất.
- Nhánh `(11)` là điểm tổng hợp dữ liệu, rất quan trọng khi so sánh hiệu quả `UAV2` với `UAV1` qua nhiều round.

## 4) Các function/chức năng có thể nâng cấp cho UAV2

Phần này tập trung riêng vào `UAV2`, vì trong `scenario5` hiện tại `UAV2` là tác nhân broadcast/coverage chính.

### 4.1. `PlanUavFlightPathsForBsInit()`

Đây là nơi đáng nâng cấp nhất cho `UAV2`.

Hiện tại:
- `UAV2` được plan theo coverage-based path
- path vẫn chủ yếu tối ưu hình học và coverage tức thời

Có thể nâng cấp:
- ưu tiên waypoint theo mật độ `suspiciousNodes` thay vì chỉ theo khoảng cách
- thêm trọng số cho node/cell chưa đủ confidence
- tách chiến lược `UAV1` và `UAV2` thành 2 planner riêng thay vì chung một hàm lớn
- thêm cơ chế re-plan nếu `UAV2` hoàn thành sớm hoặc broadcast kém hiệu quả

Ý nghĩa:
- tăng xác suất fragment của `UAV2` đến đúng cell quan trọng
- giảm thời gian để suspicious seed node đạt `ALERT_THRESHOLD`

### 4.2. `InitializeUavBroadcast()`

Đây là function điều khiển năng lực thực chiến của `UAV2`.

Hiện tại:
- broadcast fragment theo chu kỳ cố định
- lặp qua toàn bộ fragment pool tuần tự
- dùng `FRAGMENT_BROADCAST_INTERVAL` cố định

Có thể nâng cấp:
- adaptive interval: tăng tốc broadcast ở vùng suspicious core, giảm ở vùng đã phủ đủ
- priority fragment ordering: phát trước fragment có confidence cao hoặc fragment còn thiếu ở seed cell
- broadcast theo cell state: nếu một cell đã đủ confidence thì bỏ qua hoặc giảm ưu tiên
- thêm stop condition thông minh hơn ngoài `g_uav2MissionCompleted`

Ý nghĩa:
- `UAV2` sẽ chuyển từ broadcast tuần tự sang broadcast có chiến lược
- giảm broadcast lãng phí, tăng completion rate

### 4.3. `GenerateFragmentsForBsInit()`

Function này chưa phải hàm UAV trực tiếp, nhưng ảnh hưởng mạnh tới hiệu quả `UAV2`.

Có thể nâng cấp:
- sinh fragment pool theo priority classes cho `UAV2`
- gắn metadata để `UAV2` biết fragment nào nên phát trước
- tạo fragment subsets riêng cho từng vùng hoặc từng phase bay

Ý nghĩa:
- nâng cấp `UAV2` không chỉ ở chuyển động mà còn ở nội dung broadcast

### 4.4. `OnGroundNodeReceivePacket()`

Đây là điểm tiếp nhận hiệu quả broadcast của `UAV2`.

Có thể nâng cấp:
- ghi thêm metric phân biệt fragment nhận từ `UAV2` theo cell / theo thời gian
- theo dõi fragment nào của `UAV2` tạo bước nhảy confidence lớn nhất
- thêm lightweight feedback state để BS/UAV2 biết vùng nào broadcast đang hiệu quả hoặc kém hiệu quả

Ý nghĩa:
- tạo nền cho closed-loop control của `UAV2`
- không chỉ biết `UAV2` broadcast, mà còn biết broadcast nào thực sự có giá trị

### 4.5. `MarkUav2MissionCompleted()`

Hiện tại completion của `UAV2` phụ thuộc vào việc suspicious seed node đạt `ALERT_THRESHOLD`.

Có thể nâng cấp:
- đổi từ single-node trigger sang multi-condition trigger
  - seed node đạt threshold
  - hoặc vùng suspicious đạt tỷ lệ coverage nhất định
  - hoặc số cell critical đạt confidence mục tiêu
- phân biệt `broadcast-complete` và `mission-complete`
- lưu completion reason để batch analysis tốt hơn

Ý nghĩa:
- metric hiệu quả của `UAV2` sẽ phản ánh nhiệm vụ thực tế tốt hơn
- tránh đánh giá `UAV2` chỉ bằng một trigger quá hẹp

### 4.6. `InitializeCellCooperationTimeout()`

Đây là fallback phase ngay sau khi `UAV2` kết thúc đường bay.

Có thể nâng cấp:
- kích hoạt cooperation theo từng vùng thay vì chờ đến waypoint cuối cùng của `UAV2`
- cho phép `UAV2` dùng progress của cooperation để quyết định có cần broadcast thêm hay không
- biến timeout này thành checkpoint mềm thay vì một hard deadline duy nhất

Ý nghĩa:
- `UAV2` và cooperation sẽ phối hợp chặt hơn
- giảm tình trạng `UAV2` broadcast xong rồi mới biết cell nào còn thiếu fragment

### 4.7. `BuildGreedyFlightPath()` / `uav-control.cc`

Đây là utility path planner nền cho UAV.

Có thể nâng cấp riêng cho `UAV2`:
- thêm planner mới kiểu coverage-aware thay vì greedy thuần khoảng cách
- tính cost function theo:
  - khoảng cách
  - số node trong bán kính broadcast
  - mức thiếu fragment của cell
  - expected confidence gain
- hỗ trợ waypoint clustering để `UAV2` đứng ở vị trí phủ nhiều node cùng lúc

Ý nghĩa:
- đây là nâng cấp có đòn bẩy lớn nhất nếu mục tiêu là làm `UAV2` hiệu quả hơn rõ rệt

### 4.8. `fragment-broadcast.cc::StartFragmentBroadcast()`

Hiện codebase đã có `StartFragmentBroadcast(uavNodeId)` trong `uav-node-routing/fragment-broadcast.cc`, nhưng default single-run flow đang dùng `node-routing.cc::InitializeUavBroadcast()` là chính.

Có thể nâng cấp:
- hợp nhất 2 đường broadcast để tránh logic phân tán
- nếu giữ riêng, thì biến `fragment-broadcast.cc` thành engine broadcast reusable cho `UAV2`
- đưa policy broadcast sang một module độc lập để dễ thay đổi chiến lược `UAV2`

Ý nghĩa:
- dễ thử nhiều chiến lược broadcast cho `UAV2` mà không chạm quá nhiều vào orchestration code

### 4.9. Hướng nâng cấp nên ưu tiên trước

Nếu chỉ chọn 3 điểm đầu để nâng cấp `UAV2`, nên ưu tiên:

1. `PlanUavFlightPathsForBsInit()`
   - để `UAV2` đi đúng nơi có giá trị hơn

2. `InitializeUavBroadcast()`
   - để `UAV2` phát đúng fragment, đúng thời điểm hơn

3. `MarkUav2MissionCompleted()`
   - để metric đánh giá `UAV2` phản ánh đúng hiệu quả thực tế

Lý do:
- một hàm quyết định **đi đâu**
- một hàm quyết định **phát gì / phát thế nào**
- một hàm quyết định **đánh giá hoàn thành ra sao**

### 4.10. Các hướng nghiên cứu học thuật nên mở rộng cho UAV2

Phần này không chỉ trả lời câu hỏi “nâng cấp code ở đâu”, mà còn trả lời câu hỏi “đề tài nghiên cứu có thể đóng góp gì về mặt học thuật”.

#### A. Bài toán nghiên cứu tổng quát

Có thể phát biểu `UAV2` trong `scenario5` như một bài toán nghiên cứu:

- `UAV2` là một **mobile broadcast agent** có tài nguyên thời gian hữu hạn.
- Mục tiêu của `UAV2` không chỉ là bay qua các waypoint, mà là tối đa hóa **information delivery effectiveness** vào vùng suspicious.
- Hiệu quả cần được đo đồng thời theo:
  - tốc độ hoàn thành mission
  - mức tăng confidence ở suspicious region
  - độ phủ fragment
  - mức giảm phụ thuộc vào fallback cooperation

Từ đó, `scenario5` có thể được trình bày như một bài toán tối ưu nhiều mục tiêu:

$$
\max \; f(UAV2) = \alpha \cdot \text{coverage gain}
\; + \; \beta \cdot \text{confidence gain}
\; - \; \gamma \cdot \text{completion time}
\; - \; \delta \cdot \text{redundant broadcasts}
$$

Trong đó các trọng số $\alpha, \beta, \gamma, \delta$ được chọn theo mục tiêu bài toán.

#### B. Các câu hỏi nghiên cứu (Research Questions)

##### RQ1. Path planning nào làm `UAV2` hiệu quả hơn?

So sánh:
- greedy path hiện tại
- coverage-aware path
- confidence-aware path
- hybrid path (coverage + urgency + distance)

Câu hỏi học thuật:
- planner nào giúp giảm `uav2CompletedTime` nhiều nhất?
- planner nào tăng completion rate tốt nhất khi số node hoặc số fragment tăng?

##### RQ2. Broadcast policy nào tối ưu hơn cho `UAV2`?

So sánh:
- tuần tự cố định như hiện tại
- priority-first broadcast
- adaptive interval broadcast
- feedback-driven broadcast

Câu hỏi học thuật:
- phát fragment theo priority có làm suspicious seed node đạt threshold sớm hơn không?
- adaptive broadcast có giảm số lần phát dư thừa không?

##### RQ3. Điều kiện mission completion nào phản ánh đúng hiệu quả `UAV2` hơn?

So sánh:
- single-node threshold hiện tại
- region-wide threshold
- multi-cell threshold
- weighted mission score

Câu hỏi học thuật:
- metric completion hiện tại có quá hẹp hay không?
- một mission metric đa điều kiện có tương quan tốt hơn với chất lượng phân phối fragment không?

##### RQ4. Cooperation và `UAV2` nên phối hợp theo cách nào?

So sánh:
- cooperation chỉ bật khi `UAV2` kết thúc
- cooperation kích hoạt theo checkpoint
- cooperation feedback vào planner/broadcast policy

Câu hỏi học thuật:
- cho cooperation chạy sớm hơn có giảm mission time tổng thể không?
- `UAV2` có nên điều chỉnh hành vi dựa trên trạng thái cooperation của ground network không?

#### C. Các giả thuyết nghiên cứu (Research Hypotheses)

Có thể viết rõ các giả thuyết để dùng trong báo cáo tiến độ hoặc chương phương pháp:

- **H1**: Coverage-aware planning cho `UAV2` làm tăng completion rate so với greedy planning thuần khoảng cách.
- **H2**: Priority-based fragment broadcast làm giảm thời gian đạt `ALERT_THRESHOLD` tại suspicious seed node.
- **H3**: Adaptive broadcast interval làm giảm số broadcast dư thừa mà không làm giảm mission success rate.
- **H4**: Mission completion đa điều kiện phản ánh hiệu quả `UAV2` tốt hơn single-threshold trigger.
- **H5**: Kết hợp `UAV2` planning với cooperation-aware scheduling làm giảm tổng mission completion time của hệ thống.

#### D. Các hướng phương pháp học thuật có thể triển khai

##### D.1. Heuristic optimization

Áp vào:
- `PlanUavFlightPathsForBsInit()`
- `BuildGreedyFlightPath()`

Các hướng:
- nearest gain-first heuristic
- weighted coverage heuristic
- cell-priority heuristic
- marginal confidence gain heuristic

Ưu điểm:
- dễ triển khai trong ns-3
- dễ làm ablation study
- phù hợp giai đoạn đầu của đề tài

##### D.2. Multi-objective optimization

Áp vào:
- route selection
- broadcast ordering
- stop condition

Các hướng:
- Pareto trade-off giữa `completion time` và `coverage gain`
- route scoring theo nhiều tiêu chí đồng thời
- so sánh các tập trọng số khác nhau

Ý nghĩa học thuật:
- giúp bài báo không chỉ nói “nhanh hơn”, mà còn chỉ ra trade-off có cấu trúc

##### D.3. Feedback-driven control

Áp vào:
- `InitializeUavBroadcast()`
- `OnGroundNodeReceivePacket()`
- `InitializeCellCooperationTimeout()`

Ý tưởng:
- ground nodes tạo feedback gọn về fragment deficit hoặc confidence deficit
- `UAV2` điều chỉnh fragment order, interval hoặc waypoint kế tiếp

Đây là hướng rất mạnh về học thuật vì nó biến hệ thống từ:
- open-loop broadcast

thành:
- closed-loop adaptive dissemination

##### D.4. Learning-based direction (nếu muốn đi xa hơn)

Áp vào:
- planner chọn waypoint kế tiếp
- scheduler chọn fragment kế tiếp

Các hướng:
- contextual bandit cho fragment selection
- reinforcement learning cho waypoint selection
- imitation learning từ heuristic planner tốt nhất

Lưu ý:
- nên xem đây là phase nghiên cứu sau, không phải bước đầu tiên
- cần baseline heuristic mạnh trước khi đưa mô hình học vào

#### E. Các metric học thuật nên bổ sung ngoài metric hiện tại

Ngoài `uav1CompletedTime` và `uav2CompletedTime`, nên thêm:

- `confidenceGainPerSecond`
- `coverageRatioAtCompletion`
- `redundantBroadcastCount`
- `fragmentNoveltyRate`
- `criticalCellSatisfactionRate`
- `timeToSeedThreshold`
- `timeToRegionThreshold`
- `cooperationDependencyRate`
- `broadcastEfficiency = useful_deliveries / total_broadcasts`

Đây là nhóm metric rất quan trọng để báo cáo vì:
- completion time đơn lẻ chưa mô tả hết hiệu quả `UAV2`
- cần thêm metric chất lượng phân phối và metric tài nguyên

#### F. Thiết kế thực nghiệm nên có trong báo cáo

##### F.1. Baseline groups

Nên có ít nhất 4 baseline:

1. `UAV2-greedy + sequential broadcast` (baseline hiện tại)
2. `UAV2-improved path + sequential broadcast`
3. `UAV2-greedy + priority broadcast`
4. `UAV2-improved path + priority/adaptive broadcast`

##### F.2. Independent variables

Nên thay đổi theo từng nhóm:

- `gridSize`
- `numFragments`
- `simTime`
- `UAV2_SPEED`
- `FRAGMENT_BROADCAST_INTERVAL`
- `UAV_BROADCAST_RADIUS`
- `ALERT_THRESHOLD`
- tỷ lệ suspicious coverage

##### F.3. Dependent variables

- `uav2CompletedTime`
- `uav2CompletionRate`
- `uav2EarlierCount` so với `UAV1`
- `averageEarlierTime`
- coverage metrics
- confidence metrics

##### F.4. Ablation study

Nên có ablation riêng cho `UAV2`:

- bỏ adaptive interval
- bỏ priority fragment
- bỏ replanning
- bỏ cooperation feedback

Mục tiêu:
- xác định thành phần nào của `UAV2` mang lại cải thiện lớn nhất

#### G. Các hướng đóng góp học thuật có thể viết vào báo cáo

Có thể mô tả đóng góp theo 3 tầng:

##### G.1. Đóng góp mô hình
- đề xuất mô hình `UAV2` như mobile fragment dissemination agent trong suspicious-region WSN

##### G.2. Đóng góp thuật toán
- đề xuất planner/broadcast policy mới cho `UAV2`
- hoặc đề xuất mission completion criterion mới phản ánh tốt hơn hiệu quả mạng

##### G.3. Đóng góp thực nghiệm
- chứng minh bằng multi-round ns-3 simulation rằng nâng cấp `UAV2` cải thiện:
  - completion rate
  - mission time
  - confidence gain
  - giảm phụ thuộc cooperation fallback

#### H. Hướng viết báo cáo tiến độ nghiên cứu

Một cấu trúc ngắn gọn, có tính học thuật, có thể là:

1. **Problem Statement**
  - `UAV2` hiện broadcast theo policy tĩnh, chưa tối ưu theo trạng thái mạng.

2. **Research Gap**
  - hệ hiện tại chưa có adaptive planning, chưa có feedback-driven broadcast, và chưa có metric completion đủ mạnh.

3. **Proposed Directions**
  - cải tiến path planning
  - cải tiến fragment scheduling
  - cải tiến mission completion criterion

4. **Experimental Plan**
  - dùng autorun multi-round để chạy baseline vs improved versions

5. **Expected Outcome**
  - `UAV2` hoàn thành sớm hơn, ổn định hơn, và phân phối fragment hiệu quả hơn.

#### I. Thứ tự nên triển khai để vừa có kết quả kỹ thuật, vừa có giá trị học thuật

Nên đi theo thứ tự:

1. thêm metric và logging học thuật cho `UAV2`
2. cải tiến `PlanUavFlightPathsForBsInit()`
3. cải tiến `InitializeUavBroadcast()`
4. mở rộng `MarkUav2MissionCompleted()` thành mission metric tốt hơn
5. chạy multi-round autorun để lấy dữ liệu baseline vs improved
6. cuối cùng mới thử feedback-driven hoặc learning-based UAV2

Lý do:
- có baseline rõ ràng trước
- có thể ra báo cáo sớm
- giảm rủi ro nghiên cứu đi quá xa khi chưa có metric nền

## 5) Kiến trúc tối giản (không lặp)

### 5.1. Vai trò các tầng

- `example5.cc`: single-run entrypoint (parse, validate, run, summary, destroy)
- `scenarios/scenario5/*`: orchestration (`config`, `api`, `scheduler`, `metrics`, `visualizer`)
- `model/routing/scenario5/*`: domain logic (BS, ground, UAV, cooperation, packet flow)

### 5.2. Runner API lõi

```cpp
struct Scenario5RunConfig {
  bool Validate(std::string& errorMsg) const;
};

class Scenario5Runner {
public:
  explicit Scenario5Runner(const Scenario5RunConfig& config);
  void Build();
  void Schedule();
  void Run();
};
```

### 5.3. Pipeline single-run chuẩn

`Parse -> Validate -> Build -> Schedule -> Run -> Report -> Summary -> Destroy`

Lưu ý quan trọng: multi-round không nhúng vào C++ simulation path; mỗi round là một process riêng của `example5`.

---

## 6) Thiết kế autorun multi-round

### Nguyên tắc thiết kế

Autorun multi-round nên ở **ngoài** `example5.cc`, vì:
- mỗi run cần process state sạch
- `Simulator::Destroy()` nên kết thúc trọn vẹn mỗi round
- failure ở 1 round không làm hỏng toàn bộ process simulation nội bộ
- dễ mở rộng chạy nhiều seed / nhiều config / nhiều scenario

### 6.1. Cây module autorun đề xuất

```text
scenario5/autorun/
└── scenario5-batch-runner.py
    ├── load_batch_config()
    ├── build_round_matrix()
    ├── run_single_round(roundSpec)
    │   ├── build_ns3_command(roundSpec)
    │   ├── execute_process(command)
    │   ├── resolve_summary_path(seed, runId)
    │   ├── parse_summary_file(path)
    │   └── build_round_result(...)
    ├── append_result_csv(record)
    ├── append_result_json(record)
    ├── aggregate_results(records)
    ├── write_batch_report(summaryStats)
    └── main()
```

### 6.2. Cây function end-to-end cho multi-round

```text
scenario5-batch-runner.py
└── main()
    ├── load_batch_config()
    ├── build_round_matrix()
    ├── for each roundSpec
    │   ├── run_single_round(roundSpec)
    │   │   ├── build_ns3_command(roundSpec)
    │   │   ├── subprocess.run("./ns3 run ... example5 ...")
    │   │   ├── resolve_summary_path(seed, runId)
    │   │   ├── parse_summary_file(summaryPath)
    │   │   └── return RoundResult
    │   ├── append_result_csv(...)
    │   └── append_result_json(...)
    ├── aggregate_results(allResults)
    └── write_batch_report(...)
```

### 5.3. Dữ liệu input cho autorun

`scenario5-batch-config.yaml` nên mô tả:
- số rounds
- danh sách seeds hoặc seed range
- base config cho tất cả rounds
- override per round nếu cần
- output directory
- có/không build trước khi chạy

Ví dụ:

```yaml
scenario: example5
buildFirst: true
outputDir: src/wsn/examples/visualize/results/batch/scenario5
baseConfig:
  gridSize: 20
  gridSpacing: 20
  simTime: 160
  numFragments: 10
  numUavs: 2
rounds:
  - seed: 222
    runId: 1
  - seed: 222
    runId: 2
  - seed: 333
    runId: 1
```

### 5.4. Record output chuẩn cho mỗi round

Mỗi round sau khi parse summary nên được normalize thành 1 record như sau:

```text
scenario
seed
runId
gridSize
gridSpacing
simTime
numFragments
numUavs
groundNodes
suspiciousNodes
uavPaths
generatedFragments
uav1CompletedTime
uav2CompletedTime
status
summaryFile
stdoutFile
stderrFile
```

### 5.5. File output batch nên có

```text
results/batch/scenario5/
├── scenario5_rounds.csv
├── scenario5_rounds.json
├── scenario5_batch_report.md
├── logs/
│   ├── round_seed222_run1.stdout.log
│   ├── round_seed222_run1.stderr.log
│   └── ...
└── raw-summary/
    ├── scenario5_result_222_1.txt
    ├── scenario5_result_222_2.txt
    └── ...
```

---

## 7) Luồng dữ liệu cho autorun multi-round

```text
Batch config (YAML/JSON)
    -> Round matrix builder
        -> ./ns3 run "example5 --seed=... --runId=..."
            -> example5 single-run simulation
                -> scenario5 summary file
                    -> summary parser
                        -> normalized round record
                            -> CSV / JSON aggregate
                                -> batch report
```

Điểm quan trọng:
- stdout chỉ để debug
- source of truth cho batch parser là **summary file**
- summary file hiện tại đã đủ ngắn và ổn định để parse line-by-line

---

## 8) Tách nhiệm vụ rõ giữa single-run và multi-round

### Single-run (`example5`)
- chịu trách nhiệm chạy 1 simulation
- validate config
- tạo output summary cuối run
- không biết gì về loop nhiều rounds

### Multi-round (`scenario5-batch-runner.py`)
- chịu trách nhiệm lặp nhiều rounds
- quyết định seed / runId / override
- gọi executable `example5`
- parse summary file
- gom kết quả và tạo báo cáo

Cách tách này giúp:
- `example5` đơn giản, dễ debug
- batch runner dễ thay đổi mà không ảnh hưởng logic simulation
- restart clean process cho từng round
- thuận lợi cho chạy dài hạn hoặc parallel hóa sau này

---

## 9) Function tree triển khai nhanh cho autorun

```text
scenario5-batch-runner.py
└── main()
  ├── load_batch_config()
  ├── build_round_matrix()
  ├── maybe_build_project_once()
  ├── for each roundSpec
  │   ├── run_single_round(roundSpec)
  │   │   ├── build_ns3_command(roundSpec)
  │   │   ├── subprocess.run("./ns3 run \"example5 ...\"")
  │   │   ├── resolve_summary_path(seed, runId)
  │   │   ├── parse_summary_file(path)
  │   │   └── return RoundResult
  │   ├── append_result_csv(roundResult)
  │   └── append_result_json(roundResult)
  ├── aggregate_results(allRoundResults)
  └── write_batch_report(aggregateStats)
```

## 10) File content highlights cho phase autorun

### `scenario5-batch-runner.py`
```python
class RoundSpec:
    scenario: str
    seed: int
    run_id: int
    cli_overrides: dict

class RoundResult:
    scenario: str
    seed: int
    run_id: int
    status: str
    summary_path: str
    values: dict


def run_single_round(round_spec: RoundSpec) -> RoundResult:
    command = build_ns3_command(round_spec)
    completed = subprocess.run(command, shell=True, text=True, capture_output=True)
    summary_path = resolve_summary_path(round_spec.seed, round_spec.run_id)
    values = parse_summary_file(summary_path) if os.path.exists(summary_path) else {}
    status = "ok" if completed.returncode == 0 and values else "failed"
    return RoundResult(...)
```

### `scenario5-summary-parser.py`
```python
def parse_summary_file(path: str) -> dict:
    result = {
        "scenario": None,
        "seed": None,
        "runId": None,
        "config": {},
        "params": {},
        "network": {},
        "mission": {},
    }

    current_section = None
    for raw_line in open(path, "r", encoding="utf-8"):
        line = raw_line.strip()
        if not line:
            continue
        if line.startswith("SCENARIO "):
            result["scenario"] = line.split()[1]
        elif line.startswith("RUN "):
            # parse seed + runId
            pass
        elif line.startswith("[") and line.endswith("]"):
            current_section = line[1:-1].lower()
        elif "=" in line and current_section:
            key, value = line.split("=", 1)
            result[current_section][key] = value

    return result
```

---

### 10.1. Mapping giữa single-run tree và multi-round tree

- `run_single_round(roundSpec)` gọi đúng một lần `example5`
- `example5` sinh đúng một file `scenario5_result_<seed>_<runId>.txt`
- `parse_summary_file(path)` chuyển output đó thành record machine-readable
- `aggregate_results(...)` làm việc trên records, không đọc trực tiếp stdout simulation

### 10.2. Vì sao cây multi-round phải nằm ngoài C++ simulation

- mỗi round cần state sạch của `Simulator`
- process isolation làm failure handling rõ ràng hơn
- parser/aggregator thay đổi mà không chạm vào logic routing
- sau này muốn parallel run chỉ cần mở rộng runner ngoài

## 11) Thứ tự ưu tiên khi triển khai autorun

1. Giữ `example5.cc` như single-run entrypoint
2. Chuẩn hóa format summary file nếu cần thêm field
3. Tạo `scenario5-summary-parser.py`
4. Tạo `scenario5-batch-runner.py`
5. Xuất `CSV` trước, `JSON` sau
6. Thêm `batch_report.md` tổng hợp thống kê
7. Cuối cùng mới nghĩ tới parallel run / resume / retry

---

## 12) Future Work / Phase-2 Improvements

### 12.1. Batch manifest chuẩn hóa
- thêm `experimentName`, `tag`, `notes`
- hỗ trợ matrix theo nhiều biến (`gridSize`, `numFragments`, `uavSpeed`)
- cho phép resume batch đang dở

### 12.2. Structured batch status
- `pending`, `running`, `ok`, `failed`, `missing-summary`, `parse-error`
- ghi trạng thái từng round vào `scenario5_rounds.json`

### 12.3. Reproducibility tốt hơn
- mỗi round lưu nguyên command đã chạy
- lưu snapshot config đã resolve sau merge override
- lưu timestamp bắt đầu / kết thúc

### 12.4. Report nâng cao
- thống kê:
  - tỉ lệ hoàn thành UAV1 / UAV2
  - median completion time
  - min / max / mean completion time
  - số round failed
- xuất markdown report để đọc nhanh

### 12.5. Parallel execution phase-2
- chạy nhiều process `./ns3 run` song song với concurrency giới hạn
- lock theo output path để tránh đè file
- chỉ làm sau khi single-thread batch ổn định

### 12.6. Test seam cho parser và aggregator
- unit test cho parser summary
- test round matrix builder
- test aggregate statistics với input giả lập

---

## 13) Kết luận thiết kế

`scenario5` hiện đã có nền tảng đúng để autorun multi-round:
- backend routing tách riêng khỏi `scenario4`
- runner `Build / Schedule / Run` độc lập
- config có `seed` và `runId`
- summary output ngắn, ổn định, dễ parse

Vì vậy hướng triển khai đúng là:
- giữ `example5` làm **single-run engine**
- thêm `autorun/` làm **multi-round control plane**
- dùng summary file làm **contract dữ liệu** giữa simulation và batch system
