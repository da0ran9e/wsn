# UAV-to-Ground Communication Range Functions

## Overview

Đã triển khai các hàm tính toán phạm vi giao tiếp hiệu quả giữa UAV và các ground nodes dựa trên:
- **Khoảng cách 3D Euclidean** giữa UAV và ground node
- **Free Space Path Loss Model** cho tần số 2.4 GHz (CC2420 radio)
- **Công suất phát và độ nhạy thu** để xác định khả năng giao tiếp

## Implementation Files

### 1. [scenario3.h](scenario3.h)
Định nghĩa struct và function declarations:

```cpp
struct UavGroundCommRange
{
    double distance3D;           // Khoảng cách 3D (meters)
    double pathLossDb;          // Path loss (dB)
    double receivedPowerDbm;    // Công suất thu (dBm)
    bool isInRange;             // Có thể giao tiếp?
    double linkMarginDb;        // Link margin (rxPower - rxSensitivity)
};
```

### 2. [scenario3.cc](scenario3.cc)
Triển khai 3 hàm chính:

#### Function 1: `CalculateUavGroundCommRange()`
```cpp
UavGroundCommRange CalculateUavGroundCommRange(
    double uavX, double uavY, double uavZ,
    double groundX, double groundY,
    double txPowerDbm, double rxSensitivityDbm);
```

**Mô tả**: Tính toán chi tiết phạm vi giao tiếp giữa một vị trí UAV và một ground node

**Thuật toán**:
1. Tính khoảng cách 3D: `distance = sqrt(dx² + dy² + dz²)`
2. Tính path loss: `FSPL(dB) = 40.2 + 20*log₁₀(distance)`
3. Tính công suất thu: `RxPower = TxPower - PathLoss`
4. Kiểm tra link: `isInRange = (RxPower >= RxSensitivity)`
5. Tính link margin: `margin = RxPower - RxSensitivity`

**Path Loss Model**: Free Space Path Loss cho 2.4 GHz
- FSPL(dB) = 20log₁₀(d) + 20log₁₀(f) + 20log₁₀(4π/c)
- Với f = 2.4 GHz: FSPL(dB) ≈ 40.2 + 20log₁₀(d_meters)

---

#### Function 2: `GetGroundNodesInUavRange()`
```cpp
std::vector<uint32_t> GetGroundNodesInUavRange(
    double uavX, double uavY, double uavZ,
    double txPowerDbm, double rxSensitivityDbm);
```

**Mô tả**: Tìm tất cả ground nodes trong phạm vi giao tiếp của UAV

**Thuật toán**:
1. Lặp qua toàn bộ `g_globalNodeTopology`
2. Với mỗi node, gọi `CalculateUavGroundCommRange()`
3. Thêm node vào danh sách nếu `isInRange == true`
4. Trả về vector các node IDs

**Use Case**: Xác định coverage area của UAV tại một vị trí cụ thể

---

#### Function 3: `CalculateMaxUavCommRange()`
```cpp
double CalculateMaxUavCommRange(
    double altitude,
    double txPowerDbm,
    double rxSensitivityDbm);
```

**Mô tả**: Tính phạm vi giao tiếp ngang tối đa của UAV tại độ cao cho trước

**Thuật toán**:
1. Tính path loss budget: `maxPathLoss = TxPower - RxSensitivity`
2. Từ FSPL, tính max 3D distance: `d_max = 10^((maxPathLoss - 40.2) / 20)`
3. Tính horizontal range: `r_max = sqrt(d_max² - altitude²)`

**Công thức hình học**:
```
         UAV (altitude h)
           /|
        d / | h
         /  |
        /   |
    ----r----
    Ground
```
- d = max 3D distance
- h = altitude
- r = max horizontal range = sqrt(d² - h²)

---

## Test Results

### Test 1: Communication Range at Different Positions

| UAV Position | Ground Pos | Distance(m) | PathLoss(dB) | RxPower(dBm) | InRange? | Margin(dB) |
|-------------|-----------|------------|-------------|-------------|----------|-----------|
| (50,50,50) | (50,50,0) | 50.0 | 74.2 | -74.2 | ✅ YES | 20.8 |
| (50,50,50) | (100,50,0) | 70.7 | 77.2 | -77.2 | ✅ YES | 17.8 |
| (50,50,100) | (50,50,0) | 100.0 | 80.2 | -80.2 | ✅ YES | 14.8 |
| (50,50,200) | (50,50,0) | 200.0 | 86.2 | -86.2 | ✅ YES | 8.8 |
| (50,50,50) | (500,50,0) | 452.8 | 93.3 | -93.3 | ✅ YES | 1.7 |

**Parameters**: TxPower = 0 dBm, RxSensitivity = -95 dBm

---

### Test 2: Maximum Horizontal Range vs Altitude

| Altitude (m) | Max Horizontal Range (m) | Coverage Area (km²) |
|-------------|------------------------|-------------------|
| 10 | 549.4 | 0.948 |
| 50 | 547.3 | 0.941 |
| 100 | 540.4 | 0.917 |
| 150 | 528.7 | 0.878 |
| 200 | 511.9 | 0.823 |
| 250 | 489.4 | 0.752 |
| 300 | 460.4 | 0.666 |

**Observations**:
- ⬆️ Altitude tăng → ⬇️ Horizontal range giảm (vì 3D distance constraint)
- Coverage area = π × r²
- At 100m altitude: ~540m horizontal range, ~0.92 km² coverage

---

### Test 4: Path Loss Model Validation

| Distance (m) | Path Loss (dB) | Signal @ 0dBm |
|-------------|---------------|--------------|
| 1 | 40.2 | -40.2 dBm |
| 10 | 60.2 | -60.2 dBm |
| 50 | 74.2 | -74.2 dBm |
| 100 | 80.2 | -80.2 dBm |
| 500 | 94.2 | -94.2 dBm |
| 1000 | 100.2 | -100.2 dBm |

**Note**: At 500m, signal is -94.2 dBm (barely above -95 dBm sensitivity)

---

## Usage Examples

### Example 1: Check if UAV can communicate with specific node
```cpp
#include "scenarios/scenario3.h"

// UAV at (100, 100, 50), ground node at (120, 80, 0)
// Radio: 0 dBm TX, -95 dBm sensitivity
UavGroundCommRange range = CalculateUavGroundCommRange(
    100, 100, 50,  // UAV position
    120, 80, 0,    // Ground node position (z=0)
    0.0,           // txPowerDbm
    -95.0          // rxSensitivityDbm
);

if (range.isInRange) {
    NS_LOG_INFO("Communication possible! Distance: " << range.distance3D 
                << "m, Link margin: " << range.linkMarginDb << " dB");
} else {
    NS_LOG_WARN("Out of range. Need " << -range.linkMarginDb 
                << " dB more power.");
}
```

### Example 2: Find all nodes in UAV coverage
```cpp
// After global topology is built (after global setup phase)
double uavX = 100.0, uavY = 100.0, uavZ = 50.0;

std::vector<uint32_t> nodesInRange = GetGroundNodesInUavRange(
    uavX, uavY, uavZ, 
    0.0,    // txPowerDbm
    -95.0   // rxSensitivityDbm
);

NS_LOG_INFO("UAV at (" << uavX << "," << uavY << "," << uavZ << "): " 
            << nodesInRange.size() << " nodes in coverage");

for (uint32_t nodeId : nodesInRange) {
    const scenario3::GlobalNodeInfo* info = scenario3::GetNodeInfo(nodeId);
    if (info) {
        NS_LOG_INFO("  - Node " << nodeId << " at cell " << info->cellId);
    }
}
```

### Example 3: Plan UAV altitude for desired coverage
```cpp
// Want to cover all nodes within 300m horizontal radius
double desiredRadius = 300.0; // meters
double txPower = 0.0;         // dBm
double rxSensitivity = -95.0; // dBm

// Try different altitudes
for (double altitude = 10; altitude <= 200; altitude += 10) {
    double maxRange = CalculateMaxUavCommRange(altitude, txPower, rxSensitivity);
    
    if (maxRange >= desiredRadius) {
        NS_LOG_INFO("Altitude " << altitude << "m can cover " 
                    << desiredRadius << "m radius");
        NS_LOG_INFO("  Max range: " << maxRange << "m");
        break;
    }
}
```

### Example 4: Real-time UAV coverage monitoring
```cpp
// In UAV mobility update callback
void OnUavPositionUpdate(Ptr<Node> uavNode) {
    Ptr<MobilityModel> mobility = uavNode->GetObject<MobilityModel>();
    Vector pos = mobility->GetPosition();
    
    std::vector<uint32_t> coverage = GetGroundNodesInUavRange(
        pos.x, pos.y, pos.z,
        0.0, -95.0
    );
    
    NS_LOG_INFO("t=" << Simulator::Now().GetSeconds() 
                << "s UAV at (" << pos.x << "," << pos.y << "," << pos.z << "): "
                << coverage.size() << " nodes in range");
}
```

---

## Technical Details

### Radio Parameters (CC2420)
- **Frequency**: 2.4 GHz (IEEE 802.15.4)
- **Default TX Power**: 0 dBm
- **RX Sensitivity**: -95 dBm
- **Max Path Loss Budget**: 95 dB

### Path Loss Model
**Free Space Path Loss (FSPL)**:
```
FSPL(dB) = 20 log₁₀(d) + 20 log₁₀(f) + 20 log₁₀(4π/c)

Where:
- d = distance (meters)
- f = frequency (Hz)
- c = speed of light (3×10⁸ m/s)

For f = 2.4 GHz:
FSPL(dB) = 40.2 + 20 log₁₀(d)
```

**Assumptions**:
- Line-of-sight propagation (valid for UAV-to-ground)
- Free space (no obstacles, reflections)
- Isotropic antennas (0 dBi gain)

**Limitations**:
- Does not model multipath fading
- Ignores atmospheric attenuation
- Assumes perfect antenna alignment

### Distance Calculations

**3D Euclidean Distance**:
```
d = √[(x₂ - x₁)² + (y₂ - y₁)² + (z₂ - z₁)²]
```

**Horizontal Distance** (for max range calculation):
```
Given max 3D distance d_max and altitude h:
r_horizontal = √(d_max² - h²)
```

---

## Integration with Scenario3

Các hàm này tích hợp với:

1. **Global Topology** (`g_globalNodeTopology`):
   - Truy cập vị trí tất cả ground nodes
   - Lặp qua nodes để tìm coverage

2. **UAV Mobility**:
   - Tính coverage tại mỗi waypoint
   - Monitor real-time communication range

3. **Fragment Broadcasting**:
   - Xác định nodes nào nhận được UAV broadcast
   - Optimize UAV trajectory cho maximum coverage

4. **Network Visualization**:
   - Hiển thị UAV coverage area
   - Highlight nodes in/out of range

---

## Build & Test

```bash
# Build project
./ns3 build

# Run test program
./ns3 run test_uav_range

# Run with scenario3 simulation
./ns3 run "example3 --gridSize=5 --simTime=10"
```

---

## Future Enhancements

- [ ] Add multipath fading model (Rayleigh/Rician)
- [ ] Include antenna gain parameters
- [ ] Model building/terrain obstruction
- [ ] Add Doppler shift for moving UAV
- [ ] Implement adaptive power control
- [ ] Log coverage statistics to TXT visualizer
- [ ] Create heatmap of signal strength
