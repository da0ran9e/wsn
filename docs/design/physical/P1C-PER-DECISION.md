# P1C — PER-Based Reception Decision Filter

**Pha**: P1 — Minimal Physical Realism  
**Ưu tiên**: ★★★★★  
**Effort**: ~0.5 ngày  
**Phụ thuộc**: P1A (cần `d3D`), P1B (cần `rxPowerDbm`)  
**Trạng thái**: Chưa triển khai

---

## 1. Vấn đề hiện tại — đây là vấn đề cốt lõi nhất

### 1.1. All-receive logic trong `fragment-broadcast.cc`

```cpp
// Toàn bộ vòng lặp hiện tại (simplification):
for (auto& [nodeId, gp] : groundNodePositions) {
    double d    = helper::CalculateDistance(up.x, up.y, gp.x, gp.y);  // 2D
    double rssi = -55.0 - 0.12 * d;  // synthetic
    OnGroundNodeReceivePacket(nodeId, p, rssi);  // LUÔN GỌI — không có filter
}
```

Mọi ground node **luôn nhận được packet** bất kể khoảng cách, RSSI, hay môi trường.

### 1.2. Hệ quả

- Delivery ratio luôn = 100% về mặt physical layer → không có gì để tối ưu
- Mọi thử nghiệm routing đều chạy trong môi trường lý tưởng
- Không có basis để so sánh "UAV bay thấp vs bay cao" về delivery

---

## 2. Model PER cho CC2420 (O-QPSK / IEEE 802.15.4)

### 2.1. BER formula cho O-QPSK

CC2420 dùng O-QPSK với chip spreading. BER theo SNR:

$$\text{BER}(\text{SNR}) \approx \frac{1}{2}\text{erfc}\!\left(\sqrt{0.8 \cdot 10^{\text{SNR}/10}}\right)$$

Trong đó $\text{SNR} = P_{rx}[\text{dBm}] - P_{noise}[\text{dBm}]$ (cả hai trong dBm, SNR tính bằng linear không phải dB khi đưa vào erfc).

**Lưu ý**: `std::erfc()` đã có trong `<cmath>` của C++11 — không cần implement.

### 2.2. PER formula

Với fragment size $s_f$ (bytes), giả sử independent bit errors:

$$\text{PER}(s_f, \text{SNR}) = 1 - (1 - \text{BER}(\text{SNR}))^{8 \cdot s_f}$$

Fragment size điển hình của scenario5 = 60–120 bytes (cần check constant trong `fragment-broadcast.h`).

### 2.3. PER theo ngưỡng thực tế

| SNR (dB) | BER | PER (sf=100 bytes) |
|---|---|---|
| 20 | ~0 | ~0% |
| 10 | ~10⁻⁴ | ~7.7% |
| 6 | ~10⁻³ | ~55% |
| 3 | ~10⁻² | ~99.9% |
| 0 | ~0.07 | ~100% |

CC2420 sensitivity tại PER=1% ≈ SNR 5 dB, tức P_rx ≈ −95 dBm khi noise floor = −100 dBm. Nhất quán với `m_rxSensitivityDbm = -95.0` trong `cc2420-phy.h`.

---

## 3. Thay đổi cốt lõi trong `fragment-broadcast.cc`

Đây là thay đổi chính — toàn bộ logic reception trở thành probabilistic:

```cpp
// Cần include thêm:
#include "air-ground-channel.h"
#include "calc-utils.h"
#include "ns3/random-variable-stream.h"
#include <cmath>

// Một lần trong constructor hoặc Init():
Ptr<UniformRandomVariable> m_perRng = CreateObject<UniformRandomVariable>();

// Trong BroadcastOneRound():
for (auto& [nodeId, gp] : groundNodePositions) {
    Vector up = uavMobility->GetPosition();
    Vector gvec = Vector(gp.x, gp.y, 0.0);

    double d3D   = helper::CalculateDistance3D(up, gvec);          // P1A
    double elev  = helper::ComputeElevationAngleDeg(up, gvec);     // P1A
    double rxPow = helper::ComputeRxPowerDbm(d3D, elev);           // P1B

    if (!helper::IsAboveSensitivity(rxPow)) {
        // Node out of range — log if needed
        state.fragmentsLostToRange++;
        continue;
    }

    double snrLinear = std::pow(10.0, (rxPow - kNoiseFloorDbm) / 10.0);
    double ber = 0.5 * std::erfc(std::sqrt(0.8 * snrLinear));
    double per = 1.0 - std::pow(1.0 - ber, 8.0 * kFragmentSizeBytes);

    if (m_perRng->GetValue() < per) {
        // Packet lost to channel error
        state.fragmentsLostToPer++;
        continue;
    }

    OnGroundNodeReceivePacket(nodeId, p, rxPow);  // chỉ gọi khi packet survive
}
```

---

## 4. Constants cần định nghĩa

Trong `fragment-broadcast.cc` hoặc đưa vào `scenario5-params.h`:

```cpp
static constexpr double kNoiseFloorDbm    = -100.0;  // CC2420, matches cc2420-phy.h
static constexpr double kFragmentSizeBytes = 80.0;   // kiểm tra lại với code thực
```

`kNoiseFloorDbm` phải nhất quán với `Cc2420Phy::m_noiseFloorDbm`. Khi CC2420 PHY được wired sau này, thay bằng `m_phy->GetNoiseFloor()`.

---

## 5. Fields mới trong `GroundNetworkState`

Thêm vào `ground-node-routing.h` trong struct `GroundNetworkState`:

```cpp
uint32_t fragmentsLostToRange = 0;  // below sensitivity threshold
uint32_t fragmentsLostToPer   = 0;  // above sensitivity but failed PER check
```

Và cập nhật output/logging trong `ground-node-routing.cc` để include các fields này trong stats.

---

## 6. Calibration sau khi thêm PER filter

**Cảnh báo quan trọng**: Sau khi thêm P1A + P1B + P1C, delivery rate sẽ giảm đáng kể so với baseline. Cần recalibrate các params:

| Param | Trước | Sau (ước tính cần tuning) |
|---|---|---|
| UAV altitude | 50m | 100–150m (tăng để tăng LoS probability) |
| UAV tx power | 0 dBm | thử 3–5 dBm (hoặc giữ 0 dBm và tuning altitude) |
| Shadow sigma | N/A | bắt đầu = 0 (tắt shadowing), sau tăng dần 4→8 dB |
| Num broadcast rounds | 8 | có thể cần tăng nếu PER cao |

**Kế hoạch tuning**:
1. Bật P1A + P1B với sigma = 0 → xem baseline delivery rate thay đổi thế nào
2. Bật P1C → xem PER-induced loss vs range-induced loss
3. Tăng sigma từng bước và xem impact
4. Document operating point cho paper

---

## 7. Quan hệ với CC2420 PHY

PER calculation này **hiện tại** được đặt trong routing layer (`fragment-broadcast.cc`) vì scenario5 bypass CC2420 PHY. Trong tương lai:

- `Cc2420Phy::StartRx()` cần implement BER/PER và return `RxOutcome` enum
- Routing layer sẽ gọi `m_phy->TryReceive(rxPower) → bool`
- Logic PER trong `fragment-broadcast.cc` sẽ được remove và delegate vào PHY

Migration path này clear — không cần design lại hoàn toàn khi nâng cấp PHY.

---

## 8. Tận dụng ns-3

| ns-3 component | Dùng ở đâu | Status |
|---|---|---|
| `UniformRandomVariable` | PER sampling (dice roll) | ✅ Dùng trực tiếp |
| `std::erfc()` từ `<cmath>` | BER formula | ✅ C++ standard, không cần ns-3 |
| `Cc2420Phy` constants | noise floor, sensitivity | Bây giờ hardcode, sau migrate |

---

## 9. Files cần sửa

| File | Thay đổi |
|---|---|
| `uav-node-routing/fragment-broadcast.cc` | **Thay đổi chính** — add PER filter loop |
| `uav-node-routing/fragment-broadcast.h` | Thêm `Ptr<UniformRandomVariable> m_perRng` |
| `ground-node-routing/ground-node-routing.h` | Thêm `fragmentsLostToRange`, `fragmentsLostToPer` |
| `ground-node-routing/ground-node-routing.cc` | Update stats output |
| `helper/air-ground-channel.h/.cc` | Depends on P1B — phải có trước |

---

## 10. Không cần làm ở bước này

- Không cần implement CSMA/CA collision trong broadcast round này
- Không cần model interference giữa các UAV (scenario5 có 2 UAV — chúng broadcast tuần tự)
- Không cần thay đổi CC2420 PHY `StartRx()` — vẫn là stub, không trên critical path
- Không cần multi-path fading (Rayleigh/Rician) — log-normal shadowing đã đủ cho paper đầu tiên
