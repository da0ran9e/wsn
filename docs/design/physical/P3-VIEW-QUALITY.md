# P3 — Sensing View Quality per Node

**Pha**: P3 — Sensing Geometry and Coverage  
**Ưu tiên**: ★★★☆☆  
**Effort**: ~0.5 ngày  
**Phụ thuộc**: P1A–P1C hoàn thành  
**Trạng thái**: Chưa triển khai

---

## 1. Vấn đề hiện tại

### 1.1. Tất cả nodes đều có sensing quality bằng nhau

Trong scenario5, fragments mang `confidence` value được gán bởi các nodes khi phát hiện event. Nhưng hiện tại không có model nào tính đến:

- Node ở gần suspicious region có góc nhìn tốt hơn node ở xa
- Node bị che khuất bởi địa hình có quality thấp hơn
- Confidence được truyền đi mà không bị scale theo geometry

Mọi fragment đều mang `confidence` đồng nhất bất kể vị trí node trong cell so với suspicious region.

### 1.2. Hệ quả cho nghiên cứu

- Không thể đo "spatial diversity gain" của cooperative sensing
- Không có basis để justify "select nodes with higher view quality for relay"
- Metric `viewQuality` nếu có sẽ tạo heterogeneity thực sự trong fragment utility

---

## 2. Định nghĩa View Quality

**`viewQuality` ∈ [0.3, 1.0]**: scalar đo mức độ "hữu ích" của observation từ một node.

### 2.1. Distance-based decay (đơn giản, defend được)

$$q_i = \max\!\left(q_{\min},\; 1.0 - \alpha \cdot \frac{d(G_i, R_c)}{R_{\text{cell}}}\right)$$

Trong đó:
- $G_i$ = vị trí ground node $i$
- $R_c$ = center của suspicious region (từ `SelectSuspiciousRegion()`)
- $R_{\text{cell}}$ = bán kính cell
- $\alpha$ = decay coefficient (ví dụ: 0.7 → node ở rìa cell có quality ≈ 0.3)
- $q_{\min}$ = 0.3 (ngưỡng tối thiểu, mọi node đều có giá trị nhất định)

### 2.2. Angle-based quality (nâng cao)

Nếu suspicious region có direction từ BS (ví dụ: phía đông bắc cell), nodes "nhìn thẳng vào" region có quality cao hơn nodes nhìn từ phía sau:

$$q_i = q_{\text{dist},i} \cdot \max(0.5,\; \cos\theta_i)$$

Trong đó $\theta_i$ là góc giữa hướng từ $G_i$ tới $R_c$ và hướng "optimal viewing direction".

**Bắt đầu với distance-based, thêm angle sau nếu cần**.

---

## 3. Integration vào fragment routing

### 3.1. Gán viewQuality — trong `network-setup.cc`

Sau khi `SelectSuspiciousRegion()` trả về center $R_c$:

```cpp
void NetworkSetup::AssignViewQuality(
        const Vector& regionCenter,
        double cellRadius,
        const std::map<uint32_t, Vector>& nodePositions,
        std::map<uint32_t, GroundNetworkState>& states)
{
    constexpr double alpha    = 0.7;
    constexpr double qMin     = 0.3;
    for (auto& [nodeId, pos] : nodePositions) {
        double d = helper::CalculateDistance(pos.x, pos.y, regionCenter.x, regionCenter.y);
        double q = std::max(qMin, 1.0 - alpha * (d / cellRadius));
        states[nodeId].viewQuality = q;
    }
}
```

### 3.2. Field mới trong `GroundNetworkState`

```cpp
// Trong ground-node-routing.h:
double viewQuality = 1.0;  // default = full quality until computed
```

### 3.3. Sử dụng trong fragment reception

Trong `OnGroundNodeReceivePacket()` (ground-node-routing.cc), khi xử lý `PACKET_TYPE_FRAGMENT`:

```cpp
// Scale effective confidence by view quality:
double effectiveConfidence = fragment.confidence * m_state.viewQuality;
fragment.effectiveConfidence = effectiveConfidence;
// Dùng effectiveConfidence trong fusion/aggregation thay vì raw confidence
```

### 3.4. Sử dụng trong cooperation selection

Trong `RequestFragmentSharing()` hoặc routing decision (cell-cooperation.cc):

```cpp
// Ưu tiên nhận fragments từ nodes có viewQuality cao hơn
std::sort(candidates.begin(), candidates.end(), [&](uint32_t a, uint32_t b) {
    return states[a].viewQuality > states[b].viewQuality;
});
// Chỉ request từ top-K candidates
```

Đây là "utility-aware relay selection" — research contribution của paper.

---

## 4. Quan hệ với suspicious region selection

`SelectSuspiciousRegion()` trong `region-selection.cc` đã chọn một region center trong cell. Sau khi chọn, `AssignViewQuality()` sẽ compute quality cho tất cả nodes dựa trên distance đến center đó.

**Lưu ý**: Mỗi simulation run (khác `runId`) sẽ có suspicious region khác nhau → view quality distribution khác nhau → delivery behavior khác nhau. Đây là nguồn gốc của variance trong batch results, cần document trong paper.

---

## 5. Tận dụng ns-3

### 5.1. Không có ns-3 module nào cho sensing quality

View quality là application-level metric, không có ns-3 module tương ứng. **Hoàn toàn implement từ đầu**, nhưng logic đơn giản (chỉ là distance formula).

### 5.2. `MobilityModel::GetPosition()` → `Vector`

Dùng để get node positions khi compute view quality:

```cpp
Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
Vector pos = mob->GetPosition();
```

---

## 6. View quality trong simulation output

Thêm vào per-node stats output:

```
Node 5: viewQuality=0.87, fragments_rx=7/8, avgRssi=-72.3 dBm
Node 12: viewQuality=0.31, fragments_rx=3/8, avgRssi=-88.1 dBm
```

Tương quan `viewQuality` ↔ `fragments_rx` sẽ là một data point thú vị trong paper.

---

## 7. Tham số mới trong `scenario5-params.h`

```cpp
double viewQualityDecayAlpha = 0.7;   // distance decay coefficient
double viewQualityMin        = 0.3;   // minimum quality floor
```

---

## 8. Files cần sửa

| File | Thay đổi |
|---|---|
| `ground-node-routing/ground-node-routing.h` | Thêm field `viewQuality` vào `GroundNetworkState` |
| `ground-node-routing/ground-node-routing.cc` | Dùng `viewQuality` trong `OnGroundNodeReceivePacket()` |
| `ground-node-routing/cell-cooperation.cc` | Dùng `viewQuality` trong candidate selection |
| `base-station-node/network-setup.cc` | Thêm `AssignViewQuality()` sau `SelectSuspiciousRegion()` |
| `examples/scenarios/scenario5/scenario5-params.h` | Thêm `viewQualityDecayAlpha`, `viewQualityMin` |

---

## 9. Không cần làm ở bước này

- Không cần model actual sensing physics (camera FoV, acoustic range, etc.)
- Không cần terrain/elevation model (P4 scope)
- Không cần dynamic view quality update khi node moves (nodes là ground-static)
- Angle-based quality (subsection 2.2) có thể để V2 nếu reviewer yêu cầu
