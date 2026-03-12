# P1D — UAV–Node Contact Time Modeling

**Pha**: P1 — Minimal Physical Realism  
**Ưu tiên**: ★★★☆☆  
**Effort**: ~0.5 ngày  
**Phụ thuộc**: P1A (cần `d3D`, elevation angle), P1B/P1C (cần R_eff = effective range)  
**Trạng thái**: Chưa triển khai

---

## 1. Vấn đề hiện tại

### 1.1. Không có khái niệm "thời gian tiếp xúc"

Trong scenario5, UAV broadcast được mô hình hóa theo rounds:

```cpp
static constexpr double kBroadcastInterval = 1.0;  // giây
static constexpr int    kMaxBroadcastRounds = 8;
```

UAV bay qua một waypoint segment, broadcast 8 rounds. Không có kiểm tra:
- Node này có trong vùng phủ sóng của UAV đủ lâu để nhận đủ fragments không?
- UAV đi qua nhanh quá → node chỉ nhận được 2–3 fragments thay vì 8

### 1.2. Hệ quả

- Tất cả nodes trong cell đều có cơ hội nhận đủ 8 fragments bất kể UAV bay nhanh hay chậm
- Không thể nghiên cứu "coverage vs speed tradeoff" của UAV
- Metric `lastUavContactTime` trong `GroundNetworkState` được set nhưng không ảnh hưởng routing

---

## 2. Công thức contact time

### 2.1. Mô hình geometric

Giả sử UAV bay thẳng với tốc độ $v$ (m/s), độ cao $h$ (m), node ở khoảng cách ngang $d_\perp$ (m) tính từ trajectory:

$$T_{\text{contact},i} = \frac{2\sqrt{R_{\text{eff}}^2 - d_{\perp,i}^2}}{v}$$

Trong đó $R_{\text{eff}}$ là effective radio range tính theo P1B:

$$R_{\text{eff}} = d_0 \cdot 10^{\frac{P_{tx} - P_{rx,\min} - PL_0}{10n}}$$

Với $P_{rx,\min} = -95$ dBm (sensitivity), $P_{tx} = 0$ dBm, $PL_0 = 40.05$ dB, $n = 2.0$ (LoS, h=100m):
$$R_{\text{eff}} \approx 10^{(0 + 95 - 40.05)/20} \approx 562 \text{ m}$$

Tuy nhiên, với shadowing sigma = 4 dB (LoS) và link margin = 10 dB, effective range thực tế cần giảm factor ~2 → $R_{\text{eff}} \approx 250$–$350$ m. Đây là param cần calibrate với P1B+P1C.

### 2.2. Max fragments nhận được per pass

$$N_{\text{max},i} = \left\lfloor \frac{T_{\text{contact},i}}{T_{\text{interval}}} \right\rfloor = \left\lfloor \frac{T_{\text{contact},i}}{1.0} \right\rfloor$$

Nếu $N_{\text{max},i} < N_{\text{fragments}}$ thì node bị underserved — đây là constraint UAV routing phải xử lý.

### 2.3. Perpendicular distance $d_\perp$

Với waypoint segment từ $W_A$ đến $W_B$, node tại $G$:

$$d_\perp = \frac{|(W_B - W_A) \times (W_A - G)|}{|W_B - W_A|}$$

(cross product 2D = absolute value of determinant)

```cpp
double PerpDistToSegment(double gx, double gy,
                          double ax, double ay,
                          double bx, double by) {
    double dx = bx - ax, dy = by - ay;
    double len = std::sqrt(dx*dx + dy*dy);
    if (len < 1e-6) return std::sqrt((gx-ax)*(gx-ax) + (gy-ay)*(gy-ay));
    return std::abs(dx*(ay-gy) - dy*(ax-gx)) / len;
}
```

---

## 3. Nơi tính contact time

### 3.1. Option A: Tính offline trong `network-setup.cc` (khuyến nghị)

Trong `SetupGroundNeighbors()` hoặc thêm function `ComputeContactTimes()`:
- Lặp qua waypoint segments của từng UAV
- Tính `T_contact` cho mỗi (node, segment) pair
- Lưu vào `GroundNetworkState.estimatedContactTimePerUav[]`

**Ưu điểm**: Tính một lần, không overhead trong broadcast loop.  
**Nhược điểm**: UAV trajectory phải được plan trước khi simulation chạy (đã đúng trong scenario5).

### 3.2. Option B: Tính online trong `BroadcastOneRound()`

Mỗi broadcast round, check xem còn trong contact window không.

**Nhược điểm**: Phải lưu state giữa các rounds, phức tạp hơn.

**Chọn Option A**.

---

## 4. Dùng contact time trong routing

### 4.1. Field mới trong `GroundNetworkState`

```cpp
// Trong ground-node-routing.h:
double estimatedContactTimeS = 0.0;   // seconds — per UAV pass
uint32_t maxFragsDeliverablePerPass = 0;
```

### 4.2. Sử dụng trong routing decision

Trong cooperative routing (`cell-cooperation.cc`):

```cpp
// Node có ít thời gian contact → ưu tiên nhận fragments từ peers thay vì trực tiếp từ UAV
bool NeedsPeerRelay(const GroundNetworkState& state, uint32_t numFragments) {
    return state.maxFragsDeliverablePerPass < numFragments;
}
```

Đây là research hook: nodes bị "shadowed" bởi trajectory (d_⊥ lớn) cần cooperative relay nhiều hơn → driver cho P2/P3.

---

## 5. Tận dụng ns-3

### 5.1. `WaypointMobilityModel`

ns-3 có `WaypointMobilityModel` để get danh sách waypoints:

```cpp
Ptr<WaypointMobilityModel> wpm = uavNode->GetObject<WaypointMobilityModel>();
// Tuy nhiên: WaypointMobilityModel không expose waypoint list after construction
```

**Vấn đề**: Sau khi waypoints được set, `GetObject<WaypointMobilityModel>()` không có API để lấy lại danh sách waypoints. Waypoints trong scenario5 được set trong `scenario5-scheduler.cc` và không lưu trữ ngoài `WaypointMobilityModel`.

**Giải pháp**: Lưu waypoint list vào `scenario5-params.h` hoặc một struct riêng khi plan UAV path. Khi gọi `wpm->AddWaypoint()`, đồng thời push vào `vector<Vector> uavWaypoints`.

### 5.2. `MobilityModel::GetPosition()` → `Vector`

Có thể dùng trong broadcast loop để get current UAV position: đã dùng trong code hiện tại.

---

## 6. Contact time vs broadcast schedule

Scenario5 hiện dùng:
```cpp
Simulator::Schedule(Seconds(t), &UavNodeRouting::BroadcastOneRound, this, uavId);
```

Khoảng cách giữa broadcasts = 1 giây. Với UAV speed điển hình (10–20 m/s), trong 8 rounds UAV di chuyển 80–160 m. Node nằm ở rìa của trajectory (d_⊥ gần R_eff) sẽ bị phủ sóng trong ít hơn 8 rounds.

**Số rounds thực tế nhận được**:
```
N_actual = min(N_max_i, kMaxBroadcastRounds)
```

Khi N_actual < kMaxBroadcastRounds → node bị underserved → fragment completion rate thấp → cần cooperation (P2) hoặc UAV path optimization.

---

## 7. Files cần sửa

| File | Thay đổi |
|---|---|
| `ground-node-routing/ground-node-routing.h` | Thêm `estimatedContactTimeS`, `maxFragsDeliverablePerPass` |
| `helper/calc-utils.h` | Thêm `PerpDistToSegment()`, `ComputeContactTime()` |
| `base-station-node/network-setup.cc` | Thêm `ComputeContactTimes()` sau UAV path planning |
| `examples/scenarios/scenario5/scenario5-scheduler.cc` | Lưu waypoint list khi plan UAV path |

---

## 8. Không cần làm ở bước này

- Không cần simulate actual UAV mobility update (đã có `WaypointMobilityModel`)
- Không cần model Doppler effect (tốc độ UAV quá thấp để ảnh hưởng 2.4 GHz)
- Không cần xét multi-UAV interference — scenario5 hai UAV broadcast đến hai cell khác nhau
- Contact time chỉ cần tính once per simulation setup, không cần realtime update
