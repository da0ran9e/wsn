# P1A — 3D Air-to-Ground Geometry

**Pha**: P1 — Minimal Physical Realism  
**Ưu tiên**: ★★★★★ (cần làm trước nhất)  
**Effort**: ~0.5 ngày  
**Trạng thái**: Chưa triển khai

---

## 1. Vấn đề hiện tại

### 1.1. Vị trí trong code

`src/wsn/model/routing/scenario5/uav-node-routing/fragment-broadcast.cc`, hàm `BroadcastOneRound()`:

```cpp
Vector up = uavMobility->GetPosition();   // up.z = altitude UAV
// ...
Vector gp = gm->GetPosition();            // gp.z = 0 (ground nodes)

// BUG: 2D only — bỏ qua hoàn toàn altitude
double d = helper::CalculateDistance(up.x, up.y, gp.x, gp.y);

// Hệ quả: d bị underestimate khi UAV bay cao
double syntheticRssi = -55.0 - 0.12 * d;
```

### 1.2. Tác động sai lệch

Với UAV bay ở h = 100 m trên một grid 200 m × 200 m:

| Khoảng cách 2D (d₂D) | d₃D thực | Sai lệch | Sai RSSI |
|---|---|---|---|
| 0 m (ngay dưới) | 100 m | −100 m | ~12 dBm |
| 50 m | 112 m | −62 m | ~7 dBm |
| 150 m | 180 m | −30 m | ~4 dBm |
| 300 m | 316 m | −16 m | ~2 dBm |

Với node ngay dưới UAV, RSSI hiện tính là −55 dBm (quá tốt), trong khi thực tế phải là ~−67 dBm. Đây là sai lệch **không nhỏ** khi nghiên cứu so sánh UAV2 policies ở các độ cao khác nhau.

---

## 2. Những gì cần thêm

### 2.1. Trong `calc-utils.h`

Thêm hai hàm mới — không phá vỡ gì hiện tại:

```cpp
/**
 * Calculate 3D Euclidean distance between UAV and ground node.
 */
inline double
CalculateDistance3D(double x1, double y1, double z1,
                    double x2, double y2, double z2)
{
    double dx = x2 - x1;
    double dy = y2 - y1;
    double dz = z2 - z1;
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

/**
 * Compute elevation angle from UAV to ground node (degrees).
 * d2D = horizontal distance, h = UAV altitude above ground.
 */
inline double
ComputeElevationAngleDeg(double d2D, double h)
{
    if (d2D < 0.001) return 90.0;  // directly below
    return std::atan2(h, d2D) * 180.0 / M_PI;
}
```

### 2.2. Trong `fragment-broadcast.cc`

Sửa `BroadcastOneRound()` — thay một dòng:

```cpp
// Trước:
double d = helper::CalculateDistance(up.x, up.y, gp.x, gp.y);

// Sau:
double d2D = helper::CalculateDistance(up.x, up.y, gp.x, gp.y);
double d3D = helper::CalculateDistance3D(up.x, up.y, up.z, gp.x, gp.y, gp.z);
double elevAngle = helper::ComputeElevationAngleDeg(d2D, up.z - gp.z);
```

`d3D` và `elevAngle` sau đó được truyền vào path loss function (P1B).

---

## 3. Những gì ns-3 đã có — tận dụng được

### 3.1. `MobilityModel::GetPosition()` → `Vector`

`Vector` trong ns-3 là struct 3D `{x, y, z}` — `up.z` đã có sẵn khi gọi `uavMobility->GetPosition()`. Code hiện tại đã có `Vector up = uavMobility->GetPosition()` nhưng **không dùng `up.z`**.

### 3.2. `WaypointMobilityModel`

UAV hiện dùng `WaypointMobilityModel` — waypoints đã có `z` component. Không cần thay đổi mobility setup.

---

## 4. Những gì cc2420-phy.h cần — không cần ở bước này

CC2420 PHY không liên quan đến P1A. 3D geometry là tính toán ở routing/broadcast layer trước khi quyết định có deliver hay không.

---

## 5. Elevation angle — tại sao quan trọng

Elevation angle `θ` là input cho:
- **P1B** (path loss): phân biệt LoS (θ cao) vs NLoS (θ thấp)
- **P1D** (contact time): góc quyết định thời gian node nằm trong coverage cone

Công thức LoS probability theo ITU-R (reference cho P1B/P4):

$$P_{LoS}(\theta) = \frac{1}{1 + a \cdot e^{-b(\theta - a)}}$$

Với urban: $a = 9.61$, $b = 0.16$. Ở bước P1A chỉ cần tính `θ`, chưa cần dùng công thức này.

---

## 6. Files cần sửa

| File | Thay đổi |
|---|---|
| `model/routing/scenario5/helper/calc-utils.h` | Thêm `CalculateDistance3D()` + `ComputeElevationAngleDeg()` |
| `model/routing/scenario5/uav-node-routing/fragment-broadcast.cc` | Sửa `BroadcastOneRound()` để dùng `d3D` và `elevAngle` |

Không cần sửa header của `fragment-broadcast.h` hay `StartFragmentBroadcast()`.

---

## 7. Không cần làm ở bước này

- Không cần thêm file mới
- Không cần thay đổi CMakeLists.txt
- Không cần đụng CC2420 PHY
- Không cần đụng orchestration layer (scenario5-scheduler, scenario5-api)
