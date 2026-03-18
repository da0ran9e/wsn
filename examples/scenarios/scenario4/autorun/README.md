# Scenario4 Autorun v1

Chạy tự động nhiều rounds cho `example4`, gom metric thời gian hoàn thành nhiệm vụ của 2 UAV, và chỉ giữ lại một file summary `.txt` cuối cùng.

## Mục tiêu

- Tự động chạy N rounds (mặc định `--rounds=100`)
- Parse block `=== SCENARIO4 SUMMARY ===` được append vào cuối mỗi file:
  - `scenario4_result_<seed>_<runId>.txt`
- Gom metric:
  - `uav1CompletedTime` — UAV1 rời suspicious point (sau khi scan xong)
  - `uav2CompletedTime` — UAV2 nhận tín hiệu sớm dừng từ ground node đạt `alertThreshold`
  - `suspiciousNodes` — số node khả nghi được xác định trong lưới
- Xuất kết quả duy nhất:
  - `scenario4_batch_summary.txt` — gồm `[CONFIGS]`, `[PARAMS]`, `[ROUNDS]`, `[CONCLUSION]`
- Sau khi parse xong mỗi round, xóa ngay:
  - `scenario4_result_<seed>_<runId>.txt`
  - `stdout/stderr` log của round đó

## File

- `scenario4-batch-runner.py` — script chính

## Cách chạy (từ root repo)

```bash
python3 src/wsn/examples/scenarios/scenario4/autorun/scenario4-batch-runner.py \
  --repo-root . \
  --rounds 100 \
  --start-seed 42 \
  --grid-size 10 \
  --sim-time 200
```

Build trước khi chạy (nếu có thay đổi code):

```bash
python3 src/wsn/examples/scenarios/scenario4/autorun/scenario4-batch-runner.py \
  --repo-root . \
  --build-first \
  --rounds 100
```

Truyền thêm tham số tùy chỉnh cho `example4`:

```bash
python3 src/wsn/examples/scenarios/scenario4/autorun/scenario4-batch-runner.py \
  --repo-root . \
  --rounds 50 \
  --extra-args "--alertThreshold=0.75 --cooperationThreshold=0.5"
```

## Tham số

| Tham số | Mặc định | Mô tả |
|---|---|---|
| `--repo-root` | `.` | Đường dẫn tới root repo ns-3 |
| `--rounds` | `100` | Số rounds chạy |
| `--start-seed` | `42` | Seed bắt đầu (mỗi round tăng 1) |
| `--start-run-id` | `1` | runId bắt đầu (mỗi round tăng 1) |
| `--sim-time` | `200.0` | simTime cho mỗi round (s). Early-stop tự động khi cả 2 UAV hoàn thành. |
| `--grid-size` | `10` | gridSize (N×N) |
| `--grid-spacing` | `20.0` | gridSpacing (m) |
| `--num-fragments` | `10` | numFragments |
| `--num-uavs` | `2` | numUavs |
| `--timeout-sec` | `300` | Wall-clock timeout mỗi round (s) |
| `--build-first` | off | Chạy `./ns3 build` trước batch |
| `--extra-args` | `""` | Tham số bổ sung truyền thẳng vào `example4` |

## Output

Sinh ra tại: `src/wsn/examples/visualize/results/batch/scenario4/`

```
batch/scenario4/
└── scenario4_batch_summary.txt   ← file duy nhất được giữ lại
```

### Cấu trúc `scenario4_batch_summary.txt`

```
SCENARIO4 AUTORUN REPORT

[CONFIGS]
rounds=100
startSeed=42
...

[PARAMS]
cellRadius=...          ← lấy từ summary của round đầu tiên thành công
neighborDiscoveryRadius=...
...

[ROUNDS]
round=1 seed=42 runId=1 status=ok
  durationSec=12.345
  suspiciousNodes=25
  uav1CompletedTime=33.344
  uav2CompletedTime=99.412

round=2 seed=43 runId=2 status=ok
  ...

[CONCLUSION]
totalRounds=100
okRounds=98
failedRounds=2
uav1CompletionCount=61
uav2CompletionCount=98
bothCompletionCount=61
uav1CompletionRate=0.622
uav2CompletionRate=1.000
bothCompletionRate=0.622
uav2EarlierCount=...
uav1EarlierCount=...
uav2EarlierRate=...
uav1MeanCompletionTime=...
uav2MeanCompletionTime=...
...
```

### Status của mỗi round

| Status | Ý nghĩa |
|---|---|
| `ok` | Chạy thành công, summary hợp lệ |
| `failed-return` | Process trả về exit code khác 0 |
| `timeout` | Vượt quá `--timeout-sec` |
| `missing-summary` | File result không tồn tại sau khi chạy |
| `invalid-summary` | File tồn tại nhưng `SCENARIO` field không phải `scenario4` |

## Lưu ý

- `simTime=200` thường đủ vì early-stop sẽ kết thúc simulation sớm khi cả 2 UAV hoàn thành.
- UAV1 chỉ hoàn thành khi nó thực sự đi qua suspicious point (phụ thuộc vào seed/topology).
- UAV2 hoàn thành khi bất kỳ ground node nào đạt `alertThreshold` (mặc định 0.8).
