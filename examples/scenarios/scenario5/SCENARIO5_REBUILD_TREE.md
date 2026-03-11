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

## 4) Kiến trúc tối giản (không lặp)

### 4.1. Vai trò các tầng

- `example5.cc`: single-run entrypoint (parse, validate, run, summary, destroy)
- `scenarios/scenario5/*`: orchestration (`config`, `api`, `scheduler`, `metrics`, `visualizer`)
- `model/routing/scenario5/*`: domain logic (BS, ground, UAV, cooperation, packet flow)

### 4.2. Runner API lõi

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

### 4.3. Pipeline single-run chuẩn

`Parse -> Validate -> Build -> Schedule -> Run -> Report -> Summary -> Destroy`

Lưu ý quan trọng: multi-round không nhúng vào C++ simulation path; mỗi round là một process riêng của `example5`.

---

## 5) Thiết kế autorun multi-round

### Nguyên tắc thiết kế

Autorun multi-round nên ở **ngoài** `example5.cc`, vì:
- mỗi run cần process state sạch
- `Simulator::Destroy()` nên kết thúc trọn vẹn mỗi round
- failure ở 1 round không làm hỏng toàn bộ process simulation nội bộ
- dễ mở rộng chạy nhiều seed / nhiều config / nhiều scenario

### 5.1. Cây module autorun đề xuất

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

### 5.2. Cây function end-to-end cho multi-round

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

## 6) Luồng dữ liệu cho autorun multi-round

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

## 7) Tách nhiệm vụ rõ giữa single-run và multi-round

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

## 8) Function tree triển khai nhanh cho autorun

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

## 9) File content highlights cho phase autorun

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

### 9.1. Mapping giữa single-run tree và multi-round tree

- `run_single_round(roundSpec)` gọi đúng một lần `example5`
- `example5` sinh đúng một file `scenario5_result_<seed>_<runId>.txt`
- `parse_summary_file(path)` chuyển output đó thành record machine-readable
- `aggregate_results(...)` làm việc trên records, không đọc trực tiếp stdout simulation

### 9.2. Vì sao cây multi-round phải nằm ngoài C++ simulation

- mỗi round cần state sạch của `Simulator`
- process isolation làm failure handling rõ ràng hơn
- parser/aggregator thay đổi mà không chạm vào logic routing
- sau này muốn parallel run chỉ cần mở rộng runner ngoài

## 10) Thứ tự ưu tiên khi triển khai autorun

1. Giữ `example5.cc` như single-run entrypoint
2. Chuẩn hóa format summary file nếu cần thêm field
3. Tạo `scenario5-summary-parser.py`
4. Tạo `scenario5-batch-runner.py`
5. Xuất `CSV` trước, `JSON` sau
6. Thêm `batch_report.md` tổng hợp thống kê
7. Cuối cùng mới nghĩ tới parallel run / resume / retry

---

## 11) Future Work / Phase-2 Improvements

### 11.1. Batch manifest chuẩn hóa
- thêm `experimentName`, `tag`, `notes`
- hỗ trợ matrix theo nhiều biến (`gridSize`, `numFragments`, `uavSpeed`)
- cho phép resume batch đang dở

### 11.2. Structured batch status
- `pending`, `running`, `ok`, `failed`, `missing-summary`, `parse-error`
- ghi trạng thái từng round vào `scenario5_rounds.json`

### 11.3. Reproducibility tốt hơn
- mỗi round lưu nguyên command đã chạy
- lưu snapshot config đã resolve sau merge override
- lưu timestamp bắt đầu / kết thúc

### 11.4. Report nâng cao
- thống kê:
  - tỉ lệ hoàn thành UAV1 / UAV2
  - median completion time
  - min / max / mean completion time
  - số round failed
- xuất markdown report để đọc nhanh

### 11.5. Parallel execution phase-2
- chạy nhiều process `./ns3 run` song song với concurrency giới hạn
- lock theo output path để tránh đè file
- chỉ làm sau khi single-thread batch ổn định

### 11.6. Test seam cho parser và aggregator
- unit test cho parser summary
- test round matrix builder
- test aggregate statistics với input giả lập

---

## 12) Kết luận thiết kế

`scenario5` hiện đã có nền tảng đúng để autorun multi-round:
- backend routing tách riêng khỏi `scenario4`
- runner `Build / Schedule / Run` độc lập
- config có `seed` và `runId`
- summary output ngắn, ổn định, dễ parse

Vì vậy hướng triển khai đúng là:
- giữ `example5` làm **single-run engine**
- thêm `autorun/` làm **multi-round control plane**
- dùng summary file làm **contract dữ liệu** giữa simulation và batch system
