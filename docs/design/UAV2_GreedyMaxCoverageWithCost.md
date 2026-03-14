# Thiết kế & Triển khai: UAV2 — Greedy Max‑Coverage with Cost

Mục tiêu
- Thay thuật toán Greedy Set Cover hiện tại cho UAV2 bằng một heuristic cân bằng giữa độ phủ (coverage) và chi phí di chuyển (travel cost).
- Thuật toán cố gắng chọn tập waypoints ít (hoặc chi phí thấp) để che phủ tất cả `suspicious nodes` trong bán kính phát `broadcastRadius`.

Tổng quan ý tưởng
- Tập candidate C gồm: (1) vị trí của từng suspicious node, và (2) một số điểm trung tâm (centroids) sinh bởi k‑means hoặc medoid trên các node — cho phép chọn điểm waypoint nằm giữa các node (không chỉ trên node). 
- Precompute CoverageSet(c) cho mỗi candidate c ∈ C: tập node có khoảng cách ≤ broadcastRadius.
- Lặp greedy: tại mỗi bước, từ vị trí hiện tại curPos, chọn candidate c* tối đa hóa:

  score(c) = gain(c) / (cost(curPos,c)^α + ε)

  với gain(c) = |CoverageSet(c) \ covered|, cost(...) = travel time (distance / uavSpeed) hoặc chỉ distance; α điều chỉnh trade‑off (α=1 cân bằng), ε nhỏ để tránh chia 0.

- Sau khi chọn c*, thêm waypoint, cập nhật covered, curPos ← c*, lặp đến khi covered = all nodes.
- (Tùy chọn) sau chọn tập waypoints, chạy 2‑opt/TSP heuristic để giảm tổng travel distance giữa các waypoints đã chọn.

Lý thuyết & nguồn tham khảo
- Greedy Set Cover (approximation): Chvátal, V. "A greedy heuristic for the set-covering problem." (1979) — cơ sở cho greedy coverage. (tham khảo thuật toán tập phủ).
- Coverage + travel trade‑off & trajectory planning: xem các bài trong `src/wsn/docs/paper/refs/related-works/UAV-Path-Planning/Conf-Abstracts.md`, gợi ý chính:
  - "Optimizing UAV-Based Data Collection in IoT Networks With Dynamic Service Time and Buffer-Aware Trajectory Planning" — dùng cost-aware waypoint selection (tham khảo cho cost term).
  - "Collaborative Data Acquisition for UAV-Aided IoT Based on Time-Balancing Scheduling" — cân bằng dịch vụ/độ trễ giữa các vùng.
  - "Holistic Path Planning for Multi-Drone Data Collection" — chiến lược tập trung cho multi‑UAV và TSP‑style ordering.

Chi tiết triển khai (bước‑bước)
1) Tham số cấu hình (thêm vào `params`):
   - `UAV2_USE_CENTROIDS` (bool, default=true)
   - `UAV2_NUM_CENTROIDS` (int, default = min(8, N/4))
   - `UAV2_ALPHA` (double, default = 1.0)
   - `UAV2_SCORE_EPS` (double, default = 1e-6)
   - `UAV2_USE_TRAVEL_TIME` (bool, default=true) — cost = distance/uavSpeed nếu true else distance.

2) Chuẩn bị candidate set C:
   - Nếu `UAV2_USE_CENTROIDS`:
     a) lấy tọa độ tất cả suspiciousNodePositions (x,y).
     b) chạy k‑means (k = `UAV2_NUM_CENTROIDS`) trên 2‑d points để tạo centroids.
     c) C = suspicious node positions ∪ centroids.
   - Ngược lại C = suspicious node positions.

3) Precompute CoverageSet(c) cho mỗi c ∈ C:
   - lưu vector<uint32_t> hoặc bitset đại diện các node trong bán kính `broadcastRadius`.
   - Tính gain nhanh bằng phép set difference với covered (có thể dùng vector<bool> indexed by nodeId→index).

4) Greedy selection loop:
   - covered = empty set; curPos = uav2StartPos; path2.waypoints.clear(); currentTime2 = 0.
   - while covered.size() < suspiciousNodes.size():
       for c in C:
         newGain = |CoverageSet(c) \ covered|
         travel = distance(curPos, c)
         cost = (UAV2_USE_TRAVEL_TIME ? travel / uav2Speed : travel)
         score = newGain / (pow(cost, UAV2_ALPHA) + UAV2_SCORE_EPS)
       choose c* = argmax score (tie-break: larger newGain, then smaller cost, deterministic by candidate index)
       if newGain(c*) == 0: break (cannot cover remaining nodes) — log warning
       append waypoint at c* (position.z = altitude), arrivalTime = currentTime2 + cost
       currentTime2 += cost + uav2HoverTime
       covered ∪= CoverageSet(c*); curPos = c*

5) Postprocessing (tùy chọn):
   - nếu path2.waypoints.size() > 1 và `UAV2_OPTIMIZE_ROUTE` true: apply 2‑opt on waypoint order to reduce travel; recompute arrival times.

6) Logging & determinism:
   - Seed PRNG deterministically (params seed) trước k‑means and tie-breaks.
   - Log chosen candidates, gain, cost, score cho mỗi vòng vào `g_resultFileStream`.

Dữ liệu & cấu trúc trong code (chỉ dẫn file)
- Thay thế block UAV2 trong `PlanUavFlightPathsForBsInit()`:
  - Đưa precompute + greedy selection vào hàm mới `BuildUav2CoveragePath(const vector<pair<uint32_t,Vector>>& suspiciousNodePositions, uint32_t uav2NodeId, UavFlightPath &path2)`.
  - Gọi `SetUavFlightPath(uav2NodeId, path2)` sau khi tính.
- Hàm phụ đề xuất:
  - `ComputeCentroids(const vector<Vector>& points, int k) -> vector<Vector>` (simple k‑means with max iter 20)
  - `ComputeCoverageSets(const vector<Vector>& candidates, const vector<pair<uint32_t,Vector>>& nodes, double radius) -> vector<vector<uint32_t>>`
  - `SelectBestCandidate(const vector<vector<uint32_t>>& coverageSets, const vector<bool>& covered, const Vector& curPos, double speed, double alpha, double eps) -> int index`

Độ phức tạp & tối ưu
- Precompute: O(|C|·N) distance checks.
- Greedy loop: mỗi vòng O(|C|) để tính score (với gain cập nhật nhanh nếu coverageSets precomputed), tổng O(k·|C|).
- Kích thước |C| thường ≤ N + k_centroids; chọn k_centroids nhỏ (≤ N/4) để giữ hiệu năng.

Kiểm thử & kiểm chứng
- Test đơn vị: small scenarios (N=1..20) với tọa độ cố định — kiểm tra coverage hoàn tất, số waypoints ≤ N, và reproduction (dùng seed).
- So sánh với hiện tại: đánh giá metric coverage, total travel distance, totalTime, waypoints count; chạy vài random seeds và report aggregate.

Ví dụ tham số khởi tạo (mặc định thử nghiệm)
- `UAV2_USE_CENTROIDS = true`
- `UAV2_NUM_CENTROIDS = max(1, min(8, N/4))`
- `UAV2_ALPHA = 1.0`
- `UAV2_SCORE_EPS = 1e-6`

Nguồn / đọc thêm (từ repository)
- Xem tập tài liệu path planning: [src/wsn/docs/paper/refs/related-works/UAV-Path-Planning/Conf-Abstracts.md](src/wsn/docs/paper/refs/related-works/UAV-Path-Planning/Conf-Abstracts.md#priority-ranking-for-writing)
- Bài tham khảo chính để dẫn (ví dụ thuật toán cost-aware):
  - "Optimizing UAV‑Based Data Collection in IoT Networks With Dynamic Service Time and Buffer‑Aware Trajectory Planning" — (Conf‑Abstracts Tier 1)
  - "Collaborative Data Acquisition for UAV‑Aided IoT Based on Time‑Balancing Scheduling" — (Tier 2)
  - Chvátal V. (1979) "A greedy heuristic for the set-covering problem" — lý thuyết greedy set cover approximation.

Ghi chú triển khai nhanh
- Bắt đầu bằng việc cài đặt các hàm phụ trong cùng namespace (routing:: helper functions). Giữ API nhỏ, test composition riêng rẽ.
- Không xoá hoàn toàn thuật toán cũ ngay — giữ như fallback khi `UAV2_USE_NEW_ALGO=false`.

---

Nếu bạn đồng ý, tôi sẽ: (A) implement file‑level patch thay block UAV2 trong `PlanUavFlightPathsForBsInit` bằng lời gọi `BuildUav2CoveragePath(...)`, (B) thêm các hàm phụ (centroid, coverage precompute, selector), và (C) thêm cấu hình param mặc định vào `params`.
