# Scenario5 Autorun v1

Chạy tự động nhiều rounds cho `example5`, gom metric thời gian hoàn thành nhiệm vụ của 2 UAV, và chỉ giữ lại một file summary `.txt` cuối cùng.

## Mục tiêu

- Tự động chạy ~100 rounds (mặc định `--rounds=100`)
- Parse file summary của từng round:
  - `scenario5_result_<seed>_<runId>.txt`
- Gom metric:
  - `uav1CompletedTime`
  - `uav2CompletedTime`
- Xuất kết quả:
  - TXT report tổng hợp (`params`, `configs`, `rounds`, `conclusion`)
- Sau khi parse xong mỗi round, xóa:
  - `scenario5_result_<seed>_<runId>.txt`
  - `stdout/stderr` log của round đó
  - các output phụ như CSV/JSON/Markdown report

## File

- `scenario5-batch-runner.py`

## Cách chạy (từ root repo)

```bash
python3 src/wsn/examples/scenarios/scenario5/autorun/scenario5-batch-runner.py \
  --repo-root . \
  --rounds 100 \
  --start-seed 222 \
  --start-run-id 1 \
  --sim-time 10
```

Có thể thêm build trước khi chạy:

```bash
python3 src/wsn/examples/scenarios/scenario5/autorun/scenario5-batch-runner.py \
  --repo-root . \
  --build-first
```

## Output

Sinh ra dưới:

`src/wsn/examples/visualize/results/batch/scenario5/`

- `scenario5_rounds.csv`
- `scenario5_rounds.json`
- `scenario5_batch_summary.txt`
- `scenario5_batch_report.md`
- `logs/*.stdout.log`, `logs/*.stderr.log`
 
Sau khi batch hoàn tất, file cần giữ lại là:

- `scenario5_batch_summary.txt`

`scenario5_batch_summary.txt` gồm:
- `[CONFIGS]`: thông tin cấu hình batch run
- `[PARAMS]`: params lấy từ summary round đầu tiên parse thành công
- `[ROUNDS]`: tóm tắt từng round theo format tối giản
- `[CONCLUSION]`: metric kết luận theo format cố định

## Ghi chú

- Nếu round không có summary file hoặc command fail, thời gian hoàn thành UAV sẽ được ghi là `not-completed` trong file summary.
- File tổng hợp `.txt` là đầu ra văn bản duy nhất để lưu lâu dài.
