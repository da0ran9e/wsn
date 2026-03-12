# P1B — Air-to-Ground Path Loss and Shadowing

**Pha**: P1 — Minimal Physical Realism  
**Ưu tiên**: ★★★★★  
**Effort**: ~1 ngày  
**Phụ thuộc**: P1A (cần `d3D` và `elevAngle`)  
**Trạng thái**: Chưa triển khai

---

## 1. Vấn đề hiện tại

### 1.1. Formula hiện tại — không có cơ sở vật lý

Trong `fragment-broadcast.cc`:

```cpp
double syntheticRssi = -55.0 - 0.12 * d;
```

- Hệ số `0.12` không có nguồn gốc từ bất kỳ mô hình vật lý nào.
- Với d = 0: RSSI = −55 dBm → quá tốt (free-space tại d₀=1m, f=2.4GHz thực ra ~−40 dBm)
- Với d = 333: RSSI = −95 dBm → tình cờ bằng RX sensitivity CC2420
- Shadowing: không có — mọi node cùng khoảng cách đều nhận RSSI giống hệt nhau

### 1.2. Hệ quả nghiên cứu

- Không có spatial variability → không thể đo tác động của shadowing lên delivery ratio
- Không phân biệt được LoS vs NLoS → kết quả giống nhau bất kể môi trường
- UAV altitude không ảnh hưởng → không thể so sánh altitude policies

---

## 2. Model được chọn

### 2.1. Log-distance path loss + log-normal shadowing

Đây là model phổ biến nhất cho IEEE 802.15.4 / air-to-ground link và đủ để defend ở conference paper:

$$P_{rx}(d) = P_{tx} - PL_0 - 10n\log_{10}\!\left(\frac{d_{3D}}{d_0}\right) - X_\sigma$$

Trong đó:
- $P_{tx}$ = 0 dBm (CC2420 max TX power, đã có trong `cc2420-phy.h`)
- $PL_0$ = free-space path loss tại $d_0$ = 1 m, $f$ = 2.4 GHz:

$$PL_0 = 20\log_{10}\!\left(\frac{4\pi d_0 f}{c}\right) = 40.05\text{ dB}$$

- $n$ = path loss exponent, phụ thuộc LoS/NLoS
- $X_\sigma \sim \mathcal{N}(0, \sigma^2)$ = log-normal shadowing (đơn vị dB)

### 2.2. Path loss exponent theo elevation angle

Dùng `elevAngle` từ P1A để phân biệt chế độ:

| Elevation angle θ | Chế độ | n | σ (dB) |
|---|---|---|---|
| θ ≥ 40° | LoS dominant | 2.0 | 4.0 |
| 20° ≤ θ < 40° | Mixed | 2.5 | 6.0 |
| θ < 20° | NLoS dominant | 3.0 | 8.0 |

Đây là approximation theo khuyến nghị trong [ITU-R P.1410] và các paper NTN (Non-Terrestrial Network).

### 2.3. RX sensitivity filter

Sau khi tính $P_{rx}$:

```
if P_rx < RxSensitivity (-95 dBm) → packet not decodable (input cho P1C)
```

---

## 3. Tận dụng ns-3 — phân tích thực tế

### 3.1. `LogDistancePropagationLossModel` — có trong ns-3, nhưng không dùng trực tiếp được

ns-3 có sẵn:
```cpp
Ptr<LogDistancePropagationLossModel> lossModel = CreateObject<LogDistancePropagationLossModel>();
lossModel->SetAttribute("Exponent", DoubleValue(2.0));
lossModel->SetAttribute("ReferenceDistance", DoubleValue(1.0));
lossModel->SetAttribute("ReferenceLoss", DoubleValue(40.05));
// Gọi: double rxPower = lossModel->CalcRxPower(txPower, senderMobility, receiverMobility);
```

**Vấn đề**: `CalcRxPower()` cần `Ptr<MobilityModel>` — tức là phải wired vào SpectrumChannel pipeline. Trong scenario5, `BroadcastOneRound()` là routing-layer logic, không đi qua channel. Không thể gọi `CalcRxPower()` theo cách ns-3 thiết kế mà không restructure toàn bộ broadcast path.

**Kết luận**: Không dùng `LogDistancePropagationLossModel` trực tiếp. Thay vào đó, **port công thức ra utility function** với interface phù hợp cho routing layer.

### 3.2. `NormalRandomVariable` — có trong ns-3, dùng được

```cpp
#include "ns3/random-variable-stream.h"

// Trong air-ground-channel.cc:
Ptr<NormalRandomVariable> m_shadowing = CreateObject<NormalRandomVariable>();
m_shadowing->SetAttribute("Mean", DoubleValue(0.0));
m_shadowing->SetAttribute("Variance", DoubleValue(sigma * sigma));
double shadowingDb = m_shadowing->GetValue();
```

`NormalRandomVariable` đã có trong ns-3 core. **Tận dụng hoàn toàn** — không cần implement từ đầu. Quan trọng: seed của nó sẽ đồng bộ với `--seed` và `--runId` của ns-3 → reproducibility giữ được.

---

## 4. Thiết kế file mới: `air-ground-channel.h/.cc`

**Vị trí**: `src/wsn/model/routing/scenario5/helper/air-ground-channel.h/.cc`

Đặt trong `helper/` của scenario5 vì:
- Logic này dùng cho routing layer của scenario5
- Khi CC2420 PHY được hoàn thiện sau này, có thể migrate sang `model/radio/cc2420/`
- Không cần thêm vào CMakeLists riêng, chỉ cần add source file vào danh sách scenario5

### 4.1. Header

```cpp
#pragma once
#include "ns3/random-variable-stream.h"
#include <cstdint>

namespace ns3 {
namespace wsn {
namespace scenario5 {
namespace helper {

/**
 * Configuration for air-to-ground channel model.
 * Set once per simulation run, read by all broadcast events.
 */
struct AirGroundChannelConfig
{
    double txPowerDbm     = 0.0;    // CC2420 TX power
    double noiseFloorDbm  = -100.0; // CC2420 noise floor
    double rxSensDbm      = -95.0;  // CC2420 RX sensitivity
    double refDistM       = 1.0;    // reference distance d0
    double refLossDb      = 40.05;  // free-space PL at d0, 2.4 GHz
    double shadowingSigmaLos  = 4.0;
    double shadowingSigmaMixed = 6.0;
    double shadowingSigmaNlos  = 8.0;
    double elevLosThreshDeg    = 40.0; // θ >= this → LoS
    double elevMixedThreshDeg  = 20.0; // θ >= this → Mixed
};

void InitAirGroundChannel(const AirGroundChannelConfig& config);

/**
 * Compute received power at a ground node from UAV broadcast.
 *
 * @param d3D    3D distance UAV–node (meters)
 * @param elevDeg elevation angle (degrees)
 * @return received power in dBm (includes shadowing)
 */
double ComputeRxPowerDbm(double d3D, double elevDeg);

/**
 * Check if received power is above sensitivity threshold.
 */
bool IsAboveSensitivity(double rxPowerDbm);

} // namespace helper
} // namespace scenario5
} // namespace wsn
} // namespace ns3
```

### 4.2. Implementation core

```cpp
namespace {
    AirGroundChannelConfig g_config;
    Ptr<NormalRandomVariable> g_rngLos;
    Ptr<NormalRandomVariable> g_rngMixed;
    Ptr<NormalRandomVariable> g_rngNlos;
}

void InitAirGroundChannel(const AirGroundChannelConfig& config) {
    g_config = config;
    g_rngLos   = CreateObject<NormalRandomVariable>();
    g_rngLos->SetAttribute("Mean", DoubleValue(0.0));
    g_rngLos->SetAttribute("Variance", DoubleValue(config.shadowingSigmaLos * config.shadowingSigmaLos));
    // similarly for Mixed and NLoS...
}

double ComputeRxPowerDbm(double d3D, double elevDeg) {
    double n, sigma;
    Ptr<NormalRandomVariable> rng;
    if (elevDeg >= g_config.elevLosThreshDeg) {
        n = 2.0; rng = g_rngLos;
    } else if (elevDeg >= g_config.elevMixedThreshDeg) {
        n = 2.5; rng = g_rngMixed;
    } else {
        n = 3.0; rng = g_rngNlos;
    }

    double d = std::max(d3D, g_config.refDistM);  // avoid log(0)
    double pathLoss = g_config.refLossDb
                    + 10.0 * n * std::log10(d / g_config.refDistM);
    double shadowing = rng->GetValue();  // N(0, σ²) in dB

    return g_config.txPowerDbm - pathLoss - shadowing;
}

bool IsAboveSensitivity(double rxPowerDbm) {
    return rxPowerDbm >= g_config.rxSensDbm;
}
```

---

## 5. Điểm khởi tạo trong orchestration layer

`InitAirGroundChannel()` cần được gọi một lần trong `scenario5-scheduler.cc` (hoặc `scenario5-api.cc`) trước khi simulation chạy, với config lấy từ `scenario5-params.h`:

```cpp
// Trong ScheduleSingleScenario5Event() hoặc Build():
AirGroundChannelConfig chanConfig;
chanConfig.txPowerDbm = params.uavTxPowerDbm;
// ... fill từ scenario5 params
helper::InitAirGroundChannel(chanConfig);
```

---

## 6. Parameters cần thêm vào `scenario5-params.h`

```cpp
double shadowingSigmaDb = 4.0;   // default LoS
double uavTxPowerDbm    = 0.0;   // CC2420 max
```

---

## 7. Quan hệ với CC2420 PHY

| Thành phần | Bây giờ | Sau khi CC2420 PHY hoàn thiện |
|---|---|---|
| `txPowerDbm` | lấy từ params | lấy từ `Cc2420Phy::GetTxPower()` |
| `rxSensDbm` | hardcode −95 dBm | lấy từ `Cc2420Phy::GetRxSensitivity()` |
| `noiseFloorDbm` | hardcode −100 dBm | lấy từ `Cc2420Phy::GetNoiseFloor()` |
| Path loss formula | trong `air-ground-channel.cc` | migrate vào `Cc2420Phy::StartRx()` |
| `NormalRandomVariable` | dùng ns-3 core | giữ nguyên |

CC2420 PHY đã khai báo `m_rxSensitivityDbm = -95.0` và `m_noiseFloorDbm = -100.0` — các hằng số này nhất quán với model đề xuất.

---

## 8. Files cần tạo / sửa

| File | Thay đổi |
|---|---|
| `helper/air-ground-channel.h` | **Tạo mới** — interface |
| `helper/air-ground-channel.cc` | **Tạo mới** — implementation |
| `uav-node-routing/fragment-broadcast.cc` | Sửa `BroadcastOneRound()` để gọi `ComputeRxPowerDbm()` |
| `examples/scenarios/scenario5/scenario5-params.h` | Thêm `shadowingSigmaDb`, `uavTxPowerDbm` |
| `examples/scenarios/scenario5/scenario5-scheduler.cc` | Gọi `InitAirGroundChannel()` |
| `src/wsn/CMakeLists.txt` | Thêm `air-ground-channel.cc` vào source list scenario5 |
