# Example4 Execution Flow (Scenario4)

Tài liệu này mô tả luồng thực thi `example4` theo mức hàm (function-level), kèm link bấm trực tiếp tới vị trí code.

## 1) Entry point

- [main()](../../example4.cc#L25)
  - Parse CLI vào `config`
  - Validate config: [Scenario4RunConfig::Validate()](../../scenarios/scenario4/scenario4-config.cc#L13)
  - Tạo runner: [Scenario4Runner::Scenario4Runner()](../../scenarios/scenario4/scenario4-api.cc#L32)
  - Gọi lần lượt:
    - [runner.Build()](../../example4.cc#L82) → [Scenario4Runner::Build()](../../scenarios/scenario4/scenario4-api.cc#L48)
    - [runner.Schedule()](../../example4.cc#L83) → [Scenario4Runner::Schedule()](../../scenarios/scenario4/scenario4-api.cc#L191)
    - [runner.Run()](../../example4.cc#L84) → [Scenario4Runner::Run()](../../scenarios/scenario4/scenario4-api.cc#L201)

---

## 2) Build phase (khởi tạo topology + routing)

- [Scenario4Runner::Build()](../../scenarios/scenario4/scenario4-api.cc#L48)
  - [BuildGroundNetwork()](../../scenarios/scenario4/scenario4-api.cc#L77)
    - Tạo `N x N` ground nodes, set vị trí grid
  - [BuildBaseStation()](../../scenarios/scenario4/scenario4-api.cc#L104)
    - Tạo BS node, set vị trí BS
  - [BuildUavNodes()](../../scenarios/scenario4/scenario4-api.cc#L127)
    - Tạo `numUavs`, spawn tại vị trí BS (cao độ `uavAltitude`)
  - [InstallProtocolStack()](../../scenarios/scenario4/scenario4-api.cc#L150)
    - WiFi + Internet stack cho ground/BS/UAV
  - Routing init:
    - [InitializeGroundNodeRouting()](../../../model/routing/scenario4/ground-node-routing/ground-node-routing.cc#L29)
    - [InitializeBaseStation()](../../../model/routing/scenario4/node-routing.cc#L23)
      - tạo singleton BS và gọi [BaseStationNode::Initialize()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L814)
      - pipeline BS init hiện tại:
        - [AssignCellIdAndColorForGroundNodes()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L35)
        - [DiscoverNeighborsAndTwoHopsForGroundNodes()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L75)
        - [SelectCellLeadersByNearestCellCenter()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L147)
        - [SelectCrosscellGatewayPairs()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L691)
        - [BuildIntraCellRoutingTrees()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L377)
        - [EnhanceRoutingTreesForGatewayAccess()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L528)
        - [ValidateIntraCellRoutingTrees()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L211)
        - [FinalizeGroundNodeStateFields()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L637)
    - loop UAV: [InitializeUavRouting()](../../../model/routing/scenario4/uav-node-routing/uav-node-routing.cc#L72)
      - đăng ký callback `g_bsUavCommandCallback = OnUavCommandReceived`
  - Visual dump node init:
    - [DumpScenario4InitialNodesToFile()](../../scenarios/scenario4/scenario4-visualizer.cc#L71)

---

## 3) Schedule phase (lập lịch event)

- [Scenario4Runner::Schedule()](../../scenarios/scenario4/scenario4-api.cc#L191)
  - [ScheduleScenario4Events()](../../scenarios/scenario4/scenario4-scheduler.cc#L36)
    - Tại `startupPhaseDuration`: [RunStartupPhase()](../../../model/routing/scenario4/ground-node-routing/startup-phase.cc#L20)
    - `+0.005s`: [DumpScenario4CellFormationSnapshot()](../../scenarios/scenario4/scenario4-visualizer.cc#L161)
    - `+0.01s`: [TickBaseStationControl()](../../../model/routing/scenario4/node-routing.cc#L31)
    - `+0.5s`: [SchedulePeriodicTopologyTick()](../../scenarios/scenario4/scenario4-scheduler.cc#L19)
      - mỗi vòng tick gọi:
        - [SendTopologyToBS()](../../../model/routing/scenario4/ground-node-routing/ground-node-routing.cc#L273)
        - [TickBaseStationControl()](../../../model/routing/scenario4/node-routing.cc#L31)

---

## 4) Runtime phase (khi Simulator chạy)

- [Scenario4Runner::Run()](../../scenarios/scenario4/scenario4-api.cc#L201)
  - [Simulator::Run()](../../scenarios/scenario4/scenario4-api.cc#L206)
  - Sau khi dừng sim: [ReportScenario4Metrics()](../../scenarios/scenario4/scenario4-metrics.cc#L16)

---

## 5) Luồng startup discovery (ground network)

- [RunStartupPhase()](../../../model/routing/scenario4/ground-node-routing/startup-phase.cc#L20)
  - Build neighbor map theo khoảng cách
  - Mô phỏng startup packet exchange:
    - gọi [OnGroundNodeReceivePacket()](../../../model/routing/scenario4/ground-node-routing/ground-node-routing.cc#L118) cho node đích
  - Cuối phase: [SendTopologyToBS()](../../../model/routing/scenario4/ground-node-routing/ground-node-routing.cc#L273)
    - bên trong gọi [BuildTopologySnapshot()](../../../model/routing/scenario4/ground-node-routing/ground-node-routing.cc#L237)

---

## 6) Luồng BS pull topology → chọn vùng nghi ngờ → ra lệnh UAV

- [TickBaseStationControl()](../../../model/routing/scenario4/node-routing.cc#L31)
  - đọc snapshot qua [GetLatestTopologySnapshotPtr()](../../../model/routing/scenario4/ground-node-routing/ground-node-routing.cc#L284)
  - nếu snapshot mới: gọi [BaseStationNode::ReceiveTopology()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L848)

- [BaseStationNode::ReceiveTopology()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L848)
  - [SelectSuspiciousRegion()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L873)
    - dùng [SelectSuspiciousRegionFromTopology()](../../../model/routing/scenario4/base-station-node/region-selection.cc#L11)
  - [CalculateFlightPath()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L893)
    - dùng [BuildGreedyFlightPath()](../../../model/routing/scenario4/base-station-node/uav-control.cc#L10)
  - [SendWaypointCommand()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L917)
    - đẩy qua callback `g_bsUavCommandCallback`

---

## 7) Luồng UAV nhận lệnh → bay waypoint → broadcast fragment

- Callback nhận lệnh UAV: [OnUavCommandReceived()](../../../model/routing/scenario4/uav-node-routing/uav-node-routing.cc#L41)
  - Schedule bay waypoint bằng [MoveSpecificUavToWaypoint()](../../../model/routing/scenario4/uav-node-routing/uav-node-routing.cc#L23)
  - Bắt đầu phát fragment: [StartFragmentBroadcast()](../../../model/routing/scenario4/uav-node-routing/fragment-broadcast.cc#L81)

- [StartFragmentBroadcast()](../../../model/routing/scenario4/uav-node-routing/fragment-broadcast.cc#L81)
  - schedule [BroadcastOneRound()](../../../model/routing/scenario4/uav-node-routing/fragment-broadcast.cc#L26)
  - mỗi round gửi fragment đến tất cả ground nodes qua:
    - [OnGroundNodeReceivePacket()](../../../model/routing/scenario4/ground-node-routing/ground-node-routing.cc#L118)

---

## 8) Luồng xử lý packet tại ground node + cell cooperation

- [OnGroundNodeReceivePacket()](../../../model/routing/scenario4/ground-node-routing/ground-node-routing.cc#L118)
  - Parse type `STARTUP` / `FRAGMENT` / `COOPERATION`
  - Với `FRAGMENT`:
    - update fragment/confidence local
    - trigger cell sharing: [RequestFragmentSharing()](../../../model/routing/scenario4/ground-node-routing/cell-cooperation.cc#L34)

- [RequestFragmentSharing()](../../../model/routing/scenario4/ground-node-routing/cell-cooperation.cc#L34)
  - check threshold
  - với peer cùng cell: [ShareFragments()](../../../model/routing/scenario4/ground-node-routing/cell-cooperation.cc#L15)

---

## 9) Visual logs được tạo ở đâu

- Node deployment log:
  - [DumpScenario4InitialNodesToFile()](../../scenarios/scenario4/scenario4-visualizer.cc#L71)
  - output: `../../visualize/results/scenario4_nodes_init.txt`

- Cell formation log:
  - [DumpScenario4CellFormationSnapshot()](../../scenarios/scenario4/scenario4-visualizer.cc#L161)
  - được schedule tại [ScheduleScenario4Events()](../../scenarios/scenario4/scenario4-scheduler.cc#L36)
  - output: `../../visualize/results/scenario4_cell_formation.txt`

---

## 10) Function-to-function call map (adjacency)

- [main()](../../example4.cc#L25)
  - [Scenario4RunConfig::Validate()](../../scenarios/scenario4/scenario4-config.cc#L13)
  - [Scenario4Runner::Scenario4Runner()](../../scenarios/scenario4/scenario4-api.cc#L32)
  - [Scenario4Runner::Build()](../../scenarios/scenario4/scenario4-api.cc#L48)
  - [Scenario4Runner::Schedule()](../../scenarios/scenario4/scenario4-api.cc#L191)
  - [Scenario4Runner::Run()](../../scenarios/scenario4/scenario4-api.cc#L201)

- [Scenario4Runner::Build()](../../scenarios/scenario4/scenario4-api.cc#L48)
  - [BuildGroundNetwork()](../../scenarios/scenario4/scenario4-api.cc#L77)
  - [BuildBaseStation()](../../scenarios/scenario4/scenario4-api.cc#L104)
  - [BuildUavNodes()](../../scenarios/scenario4/scenario4-api.cc#L127)
  - [InstallProtocolStack()](../../scenarios/scenario4/scenario4-api.cc#L150)
  - [InitializeGroundNodeRouting()](../../../model/routing/scenario4/ground-node-routing/ground-node-routing.cc#L29)
  - [InitializeBaseStation()](../../../model/routing/scenario4/node-routing.cc#L23)
  - [InitializeUavRouting()](../../../model/routing/scenario4/uav-node-routing/uav-node-routing.cc#L72)
  - [DumpScenario4InitialNodesToFile()](../../scenarios/scenario4/scenario4-visualizer.cc#L71)

- [InitializeBaseStation()](../../../model/routing/scenario4/node-routing.cc#L23)
  - [BaseStationNode::Initialize()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L814)
    - [AssignCellIdAndColorForGroundNodes()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L35)
    - [DiscoverNeighborsAndTwoHopsForGroundNodes()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L75)
    - [SelectCellLeadersByNearestCellCenter()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L147)
    - [SelectCrosscellGatewayPairs()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L691)
    - [BuildIntraCellRoutingTrees()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L377)
    - [EnhanceRoutingTreesForGatewayAccess()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L528)
    - [ValidateIntraCellRoutingTrees()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L211)
    - [FinalizeGroundNodeStateFields()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L637)

- [Scenario4Runner::Schedule()](../../scenarios/scenario4/scenario4-api.cc#L191)
  - [ScheduleScenario4Events()](../../scenarios/scenario4/scenario4-scheduler.cc#L36)

- [ScheduleScenario4Events()](../../scenarios/scenario4/scenario4-scheduler.cc#L36)
  - [RunStartupPhase()](../../../model/routing/scenario4/ground-node-routing/startup-phase.cc#L20)
  - [DumpScenario4CellFormationSnapshot()](../../scenarios/scenario4/scenario4-visualizer.cc#L161)
  - [TickBaseStationControl()](../../../model/routing/scenario4/node-routing.cc#L31)
  - [SchedulePeriodicTopologyTick()](../../scenarios/scenario4/scenario4-scheduler.cc#L19)

- [RunStartupPhase()](../../../model/routing/scenario4/ground-node-routing/startup-phase.cc#L20)
  - [OnGroundNodeReceivePacket()](../../../model/routing/scenario4/ground-node-routing/ground-node-routing.cc#L118)
  - [SendTopologyToBS()](../../../model/routing/scenario4/ground-node-routing/ground-node-routing.cc#L273)

- [SchedulePeriodicTopologyTick()](../../scenarios/scenario4/scenario4-scheduler.cc#L19)
  - [SendTopologyToBS()](../../../model/routing/scenario4/ground-node-routing/ground-node-routing.cc#L273)
  - [TickBaseStationControl()](../../../model/routing/scenario4/node-routing.cc#L31)
  - (self schedule) `SchedulePeriodicTopologyTick()`

- [TickBaseStationControl()](../../../model/routing/scenario4/node-routing.cc#L31)
  - [GetLatestTopologySnapshotPtr()](../../../model/routing/scenario4/ground-node-routing/ground-node-routing.cc#L284)
  - [BaseStationNode::ReceiveTopology()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L848)

- [BaseStationNode::ReceiveTopology()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L848)
  - [BaseStationNode::SelectSuspiciousRegion()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L873)
  - [BaseStationNode::CalculateFlightPath()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L893)
  - [BaseStationNode::SendWaypointCommand()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L917)

- [BaseStationNode::SelectSuspiciousRegion()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L873)
  - [SelectSuspiciousRegionFromTopology()](../../../model/routing/scenario4/base-station-node/region-selection.cc#L11)

- [BaseStationNode::CalculateFlightPath()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L893)
  - [BuildGreedyFlightPath()](../../../model/routing/scenario4/base-station-node/uav-control.cc#L10)

- [OnUavCommandReceived()](../../../model/routing/scenario4/uav-node-routing/uav-node-routing.cc#L41)
  - [MoveSpecificUavToWaypoint()](../../../model/routing/scenario4/uav-node-routing/uav-node-routing.cc#L23)
  - [StartFragmentBroadcast()](../../../model/routing/scenario4/uav-node-routing/fragment-broadcast.cc#L81)

- [BroadcastOneRound()](../../../model/routing/scenario4/uav-node-routing/fragment-broadcast.cc#L26)
  - [OnGroundNodeReceivePacket()](../../../model/routing/scenario4/ground-node-routing/ground-node-routing.cc#L118)
  - (self schedule) `BroadcastOneRound()`

- [OnGroundNodeReceivePacket()](../../../model/routing/scenario4/ground-node-routing/ground-node-routing.cc#L118)
  - [RequestFragmentSharing()](../../../model/routing/scenario4/ground-node-routing/cell-cooperation.cc#L34)

- [Scenario4Runner::Run()](../../scenarios/scenario4/scenario4-api.cc#L201)
  - [ReportScenario4Metrics()](../../scenarios/scenario4/scenario4-metrics.cc#L16)

---

## 11) Function-to-function call tree

- [main()](../../example4.cc#L25)
  - [Scenario4RunConfig::Validate()](../../scenarios/scenario4/scenario4-config.cc#L11)
  - [Scenario4Runner::Scenario4Runner()](../../scenarios/scenario4/scenario4-api.cc#L32)
  - [Scenario4Runner::Build()](../../scenarios/scenario4/scenario4-api.cc#L48)
    - [Scenario4Runner::BuildGroundNetwork()](../../scenarios/scenario4/scenario4-api.cc#L71)
    - [Scenario4Runner::BuildBaseStation()](../../scenarios/scenario4/scenario4-api.cc#L98)
    - [Scenario4Runner::BuildUavNodes()](../../scenarios/scenario4/scenario4-api.cc#L121)
    - [Scenario4Runner::InstallProtocolStack()](../../scenarios/scenario4/scenario4-api.cc#L144)
    - [InitializeGroundNodeRouting()](../../../model/routing/scenario4/ground-node-routing/ground-node-routing.cc#L32)
    - [InitializeBaseStation()](../../../model/routing/scenario4/node-routing.cc#L38)
      - [BaseStationNode::Initialize()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L1413)
        - [AssignCellIdAndColorForGroundNodes()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L43)
        - [DiscoverNeighborsAndTwoHopsForGroundNodes()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L105)
        - [SelectCellLeadersByNearestCellCenter()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L194)
        - [SelectCrosscellGatewayPairs()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L1290)
        - [BuildIntraCellRoutingTrees()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L438)
        - [EnhanceRoutingTreesForGatewayAccess()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L606)
        - [ValidateIntraCellRoutingTrees()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L272)
        - [FinalizeGroundNodeStateFields()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L715)
        - [SelectSuspiciousRegionForBsInit()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L769)
        - [GenerateFragmentsForBsInit()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L949)
          - [GenerateBsFragments()](../../../model/routing/scenario4/base-station-node/fragment-generator.cc#L11)
            - [GenerateFragments()](../../../model/routing/scenario4/fragment.cc#L62)
          - [SetBsGeneratedFragments()](../../../model/routing/scenario4/base-station-node/fragment-generator.cc#L23)
        - [PlanUavFlightPathsForBsInit()](../../../model/routing/scenario4/base-station-node/base-station-node.cc#L976)
          - [ClearUavFlightPaths()](../../../model/routing/scenario4/base-station-node/fragment-generator.cc#L44)
          - [SetUavFlightPath()](../../../model/routing/scenario4/base-station-node/fragment-generator.cc#L38)
    - [InitializeUavRouting()](../../../model/routing/scenario4/uav-node-routing/uav-node-routing.cc#L72)
  - [Scenario4Runner::Schedule()](../../scenarios/scenario4/scenario4-api.cc#L185)
    - [ScheduleSingleScenario4Event()](../../scenarios/scenario4/scenario4-scheduler.cc#L64)
      - [InitializeUavFlight()](../../../model/routing/scenario4/node-routing.cc#L103)
        - (for each waypoint) `Simulator::Schedule(...)` lambda
          - cập nhật vị trí UAV tại waypoint
          - ghi log `[EVENT] ... UAVWaypointArrival`
          - (UAV1) nếu vừa rời suspicious point thì ghi `[EVENT] ... UAV1MissionComplete`
      - [InitializeUavBroadcast()](../../../model/routing/scenario4/node-routing.cc#L232)
        - (for each cycle/fragment) `Simulator::Schedule(...)` lambda
          - [OnGroundNodeReceivePacket()](../../../model/routing/scenario4/ground-node-routing/ground-node-routing.cc#L123)
            - [RequestFragmentSharing()](../../../model/routing/scenario4/ground-node-routing/cell-cooperation.cc#L59)
              - [ShareFragments()](../../../model/routing/scenario4/ground-node-routing/cell-cooperation.cc#L21)
          - nếu node đạt ngưỡng cảnh báo, có thể trigger [MarkUav2MissionCompleted()](../../../model/routing/scenario4/node-routing.cc#L47)
      - [InitializeCellCooperationTimeout()](../../../model/routing/scenario4/node-routing.cc#L408)
        - `Simulator::Schedule(...)` timeout lambda
          - [RequestFragmentSharing()](../../../model/routing/scenario4/ground-node-routing/cell-cooperation.cc#L59)
            - [ShareFragments()](../../../model/routing/scenario4/ground-node-routing/cell-cooperation.cc#L21)
  - [Scenario4Runner::Run()](../../scenarios/scenario4/scenario4-api.cc#L195)
    - [Simulator::Run()](../../scenarios/scenario4/scenario4-api.cc#L200)
    - [ReportScenario4Metrics()](../../scenarios/scenario4/scenario4-metrics.cc#L16)
