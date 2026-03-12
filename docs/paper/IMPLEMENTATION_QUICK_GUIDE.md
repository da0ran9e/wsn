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
