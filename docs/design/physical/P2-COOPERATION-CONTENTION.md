# P2 — Cooperation Slot Budget and Channel Contention

**Pha**: P2 — Ground-to-Ground Channel Realism  
**Ưu tiên**: ★★★☆☆  
**Effort**: ~1 ngày  
**Phụ thuộc**: P1A–P1C hoàn thành (cần biết delivery rate baseline thực tế trước)  
**Trạng thái**: Chưa triển khai

---

## 1. Vấn đề hiện tại

### 1.1. Cooperation không có chi phí kênh

Trong `cell-cooperation.cc`:

```cpp
void CellCooperation::ShareFragments(uint32_t sourceNodeId, uint32_t targetNodeId, ...) {
    // Schedule fragment delivery instantly
    Simulator::Schedule(Seconds(0), &GroundNodeRouting::OnReceiveFromPeer, ...);
}

void CellCooperation::RequestFragmentSharing(uint32_t requesterId, ...) {
    // Broadcast request to all neighbors — no slot limit
    for (auto neighborId : cell.members) {
        if (neighborId == requesterId) continue;
        ShareFragments(neighborId, requesterId, ...);
    }
}
```

Không có giới hạn:
- Một node có thể request từ tất cả neighbors cùng lúc
- Tất cả neighbors có thể share cùng lúc
- Không có collision, không có backoff, không có budget

### 1.2. Hệ quả

- Cooperation hoàn hảo → fragment completion rate cao nhân tạo
- Không thể đo "overhead của cooperation" hay "contention cost"
- Cell với nhiều nodes và nhiều missing fragments không bị penalize bởi channel congestion

---

## 2. Model được chọn: Slot Budget per Time Window

Không cần implement full CSMA/CA (quá phức tạp, không cần thiết cho nghiên cứu này). Thay vào đó, dùng **soft slot budget**: mỗi cell có một "cooperation bandwidth" giới hạn, tính bằng số lượng peer-to-peer transmissions cho phép trong mỗi time window.

### 2.1. Rationale

IEEE 802.15.4 CSMA/CA ở tầng MAC sẽ tự nhiên limit throughput. Thay vì simulate tầng MAC đầy đủ (cần ns-3 LrWpan module wiring, phức tạp), ta model *effect* của contention bằng:

$$T_{\text{coop/window}} \leq S_{\text{budget}}$$

Trong đó $S_{\text{budget}}$ là số slots cooperation cho phép trong một window $\Delta t$ (ví dụ: 5 giây).

### 2.2. Giá trị budget hợp lý

IEEE 802.15.4 ở 2.4 GHz, DSSS:
- Data rate: 250 kbps
- Fragment size: 80 bytes = 640 bits → ~2.56 ms per transmission
- Duty cycle cooperation: giả sử 20% channel time cho cooperation
- Window 5 giây: 5000 ms × 20% / 2.56 ms ≈ **390 transmissions per window**

Nhưng với 10–20 nodes per cell, collision rate cao → effective throughput chỉ còn 50–60%. Budget thực tế:
- Tham số mặc định: `cooperationSlotBudget = 50` per cell per 5-second window
- Đây là tunable param — điều chỉnh theo số nodes per cell

---

## 3. Implementation

### 3.1. Cấu trúc state per cell

Trong `cell-cooperation.h` hoặc thêm struct:

```cpp
struct CellCooperationState {
    uint32_t slotsUsedThisWindow   = 0;
    double   windowStartTime       = 0.0;
    uint32_t windowDurationS       = 5;    // seconds
    uint32_t slotBudgetPerWindow   = 50;   // tunable
    uint32_t requestsRejected      = 0;    // stat
    uint32_t requestsGranted       = 0;    // stat
};
```

### 3.2. Thay đổi trong `RequestFragmentSharing()`

```cpp
void CellCooperation::RequestFragmentSharing(uint32_t requesterId, ...) {
    // Reset window if needed
    double now = Simulator::Now().GetSeconds();
    if (now - m_state.windowStartTime >= m_state.windowDurationS) {
        m_state.slotsUsedThisWindow = 0;
        m_state.windowStartTime = now;
    }

    uint32_t slotsNeeded = neighbors.size();  // one slot per neighbor
    if (m_state.slotsUsedThisWindow + slotsNeeded > m_state.slotBudgetPerWindow) {
        // Over budget — reject or partial serve
        m_state.requestsRejected++;
        // Log warning
        return;  // or: serve subset of neighbors
    }

    m_state.slotsUsedThisWindow += slotsNeeded;
    m_state.requestsGranted++;

    for (auto neighborId : cell.members) {
        if (neighborId == requesterId) continue;
        ShareFragments(neighborId, requesterId, ...);
    }
}
```

### 3.3. Delay model cho peer transmission

Thêm propagation delay vào `ShareFragments()` — hiện tại dùng `Seconds(0)`:

```cpp
// Ground node distance (peer-to-peer, không qua UAV)
double d2D = helper::CalculateDistance(src.x, src.y, dst.x, dst.y);
double propDelayS = d2D / 3e8;  // speed of light
double txDelayS   = (kFragmentSizeBytes * 8.0) / 250000.0;  // 250 kbps
Simulator::Schedule(Seconds(propDelayS + txDelayS), &GroundNodeRouting::OnReceiveFromPeer, ...);
```

Delay thực tế nhỏ (dưới 1 ms) nhưng non-zero → cần thiết cho correctness khi analyze timing.

---

## 4. Tận dụng ns-3

### 4.1. `Simulator::Now()`

Đã có sẵn, dùng để check window boundary:

```cpp
double now = Simulator::Now().GetSeconds();
```

### 4.2. `ns3::LrWpan`

ns-3 có `lr-wpan` module với CSMA/CA đầy đủ. Tuy nhiên:
- Scenario5 ground network không dùng `LrWpan` — dùng direct dispatch
- Wiring vào `LrWpan` MAC sẽ require restructure hoàn toàn cooperation layer
- **Không dùng** `LrWpan` cho P2 — slot budget model đủ để capture effect

### 4.3. `UniformRandomVariable` cho random backoff

Nếu muốn thêm randomness vào slot assignment (ví dụ: randomly defer request khi budget thấp):

```cpp
Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
double backoffS = rng->GetValue(0.0, 1.0);  // random backoff 0–1 giây
Simulator::Schedule(Seconds(backoffS), &CellCooperation::RetryRequest, ...);
```

---

## 5. Params mới trong `scenario5-params.h`

```cpp
uint32_t cooperationSlotBudget    = 50;   // per cell per window
double   cooperationWindowSeconds = 5.0;
```

---

## 6. Stats output

Thêm vào aggregated output của simulation:

```
Cell X cooperation:
  Requests granted: N
  Requests rejected: M (budget exceeded)
  Utilization: N/(N+M) * 100%
```

Đây là metric để chứng minh "cooperation overhead" trong paper.

---

## 7. Quan hệ với P1D (Contact Time)

Nodes có contact time thấp (d_⊥ lớn) sẽ:
1. Nhận ít fragments hơn từ UAV trực tiếp (P1D)
2. Gửi nhiều cooperation requests hơn (P2)
3. Cạnh tranh budget → một số requests bị từ chối

Đây là **cascading effect** quan trọng cho research narrative: UAV trajectory design ảnh hưởng đến load trên ground network cooperation.

---

## 8. Files cần sửa

| File | Thay đổi |
|---|---|
| `ground-node-routing/cell-cooperation.h` | Thêm `CellCooperationState` struct, slot budget fields |
| `ground-node-routing/cell-cooperation.cc` | Thêm budget check trong `RequestFragmentSharing()`, delay trong `ShareFragments()` |
| `examples/scenarios/scenario5/scenario5-params.h` | Thêm `cooperationSlotBudget`, `cooperationWindowSeconds` |
| `examples/scenarios/scenario5/scenario5-output.cc` | Log cooperation utilization stats |

---

## 9. Không cần làm ở bước này

- Không cần implement IEEE 802.15.4 CSMA/CA đầy đủ
- Không cần model hidden terminal problem
- Không cần model ACK/NACK cho peer transmissions
- Không cần multi-hop routing trong ground network (chỉ single-hop peer sharing trong cell)
- Cooperation giữa cells (inter-cell) vẫn ngoài scope
