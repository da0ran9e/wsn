# P4 — Urban Obstruction via ns-3 Buildings Module

**Pha**: P4 — Extended Physical Environment (Post-First-Paper)  
**Ưu tiên**: ★★☆☆☆  
**Effort**: ~3–5 ngày  
**Phụ thuộc**: P1A–P3 phải ổn định  
**Trạng thái**: Nghiên cứu / Defer

---

## 1. Vấn đề và động lực

### 1.1. Môi trường hiện tại: open field hoàn toàn

Scenario5 chạy trong môi trường không có vật cản. Tất cả nodes có LoS (Line of Sight) tới UAV. Điều này:
- Không phản ánh môi trường đô thị thực tế
- Không thể so sánh "UAV coverage in urban vs rural"
- Research claim về "air-to-ground communication" không có urban obstruction validation

### 1.2. Khi nào cần P4

P4 trở thành cần thiết khi paper hướng đến:
- Journal (không chỉ conference) yêu cầu environmental validation
- Reviewer hỏi "how does performance degrade in dense urban environment?"
- Comparison với ITU-R models cho non-terrestrial networks (NTN)

**Quyết định hiện tại: DEFER đến sau khi có first paper submission**. P1A–P3 đủ cho conference paper.

---

## 2. ns-3 Buildings Module — Capabilities

### 2.1. Module có sẵn

ns-3 có `buildings` module tại `src/buildings/`:

```
src/buildings/
  model/
    building.h/.cc                    — building geometry (box shape)
    building-list.h/.cc               — global building registry
    mobility-building-info.h/.cc      — per-node building attachment
    buildings-propagation-loss-model.h/.cc
    hybrid-buildings-propagation-loss-model.h/.cc
    itu-r-1411-los-propagation-loss-model.h/.cc
    itu-r-1411-nlos-orthogonal-loss-model.h/.cc
    oh-buildings-propagation-loss-model.h/.cc
  helper/
    buildings-helper.h/.cc            — attach MobilityBuildingInfo to nodes
```

### 2.2. Key components

**`Building`**: Hình hộp chữ nhật với tọa độ $(x_{min}, x_{max}, y_{min}, y_{max}, z_{min}, z_{max})$:
```cpp
Ptr<Building> b = CreateObject<Building>();
b->SetBoundaries(Box(10.0, 50.0, 20.0, 80.0, 0.0, 15.0));  // 15m high building
b->SetBuildingType(Building::Residential);
b->SetExtWallsType(Building::ConcreteWithWindows);
```

**`MobilityBuildingInfo`**: Aggregated onto each `MobilityModel` to track indoor/outdoor status:
```cpp
BuildingsHelper::Install(nodeContainer);  // attaches MobilityBuildingInfo to all nodes
BuildingsHelper::MakeMobilityModelConsistent();  // update indoor status
```

**`HybridBuildingsPropagationLossModel`**: Combines multiple models:
- Outdoor-outdoor: `OkumuraHata` or `ITU-R P.1411`
- Outdoor-indoor / indoor-indoor: includes wall attenuation
- Automatically switches based on node positions relative to buildings

---

## 3. Setup Cost — Tại sao P4 phức tạp

### 3.1. Node attachment overhead

Mọi node trong simulation phải có `MobilityBuildingInfo`:

```cpp
// Phải gọi cho TẤT CẢ nodes (UAV + ground + BS):
BuildingsHelper::Install(allNodes);
BuildingsHelper::MakeMobilityModelConsistent();
```

Nếu quên attach một node → crash hoặc silent wrong results.

### 3.2. Propagation model wiring

`HybridBuildingsPropagationLossModel` cần được wired vào channel. Scenario5 dùng direct dispatch (không qua channel) → không thể dùng trực tiếp. Cần:

**Option A**: Gọi `CalcRxPower()` trực tiếp (như P1B), nhưng cần cả `sender` và `receiver` `Ptr<MobilityModel>`:
```cpp
double rxPow = lossModel->CalcRxPower(txPow, uavMobility, groundMobility);
```
Điều này khả thi nếu scenario5 giữ `Ptr<MobilityModel>` cho từng node (hiện đã có qua `GetObject<MobilityModel>()`).

**Option B**: Tạo wrapper trong `air-ground-channel.cc` gọi buildings model.

### 3.3. Building placement

Phải có building geometry phù hợp với simulation area (1km × 1km). Cần:
- Input file hoặc procedural generation
- Random building layout với seed (reproducibility)
- Ít nhất 20–50 buildings cho "urban" environment

### 3.4. LoS probability model

Thay vì đặt buildings manually, có thể dùng **ITU-R P.1410** LoS probability formula cho air-to-ground:

$$P_{\text{LoS}}(\theta) = \frac{1}{1 + C \cdot e^{-B(\theta - C)}}$$

Với $\theta$ = elevation angle, $B$ và $C$ là params phụ thuộc môi trường (suburban, urban, dense urban, highrise).

Sau đó:
$$PL = P_{\text{LoS}} \cdot PL_{\text{LoS}} + (1 - P_{\text{LoS}}) \cdot PL_{\text{NLoS}}$$

Đây là **stochastic approach** (không đặt buildings cụ thể) — đơn giản hơn nhưng less detailed. Phù hợp cho analytical comparison.

---

## 4. Hướng triển khai đề xuất cho P4

### Phase 4a (simpler): ITU-R LoS probability stochastic model

Thêm vào `air-ground-channel.cc`:

```cpp
double ComputeLosProbability(double elevDeg, Environment env) {
    // ITU-R P.1410, Table for urban environment:
    double B = 0.0954, C = 11.95;  // urban
    return 1.0 / (1.0 + C * std::exp(-B * (elevDeg - C)));
}

double ComputeRxPowerDbmUrban(double d3D, double elevDeg) {
    double pLos  = ComputeLosProbability(elevDeg, Environment::Urban);
    double pLoss = g_config.refLossDb + 10.0 * 2.0 * std::log10(d3D);  // LoS
    double nLoss = g_config.refLossDb + 10.0 * 3.5 * std::log10(d3D) + 20.0;  // NLoS penalty
    double avgLoss = pLos * pLoss + (1 - pLos) * nLoss;
    // Add shadowing...
    return g_config.txPowerDbm - avgLoss;
}
```

Không cần `Building` objects. Đủ để add "urban scenario" tab vào paper results.

### Phase 4b (full): ns-3 buildings + `HybridBuildingsPropagationLossModel`

Tốn nhiều effort hơn nhưng cho detailed spatial results (coverage map per building layout).

---

## 5. Dependency trên ns-3 module

`buildings` module đã có trong ns-3 3.46. Kiểm tra CMakeLists.txt của wsn module:

```cmake
# Trong src/wsn/CMakeLists.txt, cần thêm:
set(wsn_LIBRARIES_TO_LINK buildings)
```

Hiện tại wsn module có thể không link với `buildings` — cần kiểm tra trước.

---

## 6. Files sẽ cần sửa (khi triển khai)

| File | Thay đổi |
|---|---|
| `src/wsn/CMakeLists.txt` | Thêm `buildings` vào dependencies |
| `helper/air-ground-channel.h/.cc` | Thêm `ComputeLosProbability()`, urban model |
| `base-station-node/network-setup.cc` | Thêm building placement (nếu full P4b) |
| `examples/scenarios/scenario5/scenario5-params.h` | Thêm `environmentType` enum |

---

## 7. Không làm P4 cho paper đầu tiên

**Lý do**:
1. P1A + P1B + P1C đã đủ để claim "realistic air-to-ground channel"
2. P4 tăng setup complexity đáng kể → risk delay paper
3. "Future work" section có thể nói: "extension to urban obstruction using ns-3 buildings module"
4. Phase 4a (LoS probability stochastic) có thể thêm vào nếu có thời gian và reviewer yêu cầu

**Điều kiện để bắt đầu P4**:
- ✅ P1A–P3 đã stable và tested
- ✅ Baseline results published / submitted
- ✅ Reviewer hoặc advisor yêu cầu urban validation
