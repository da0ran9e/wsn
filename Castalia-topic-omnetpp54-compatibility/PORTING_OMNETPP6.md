# Porting Castalia to OMNeT++ 6.x

Tài liệu này ghi lại các lỗi gặp phải khi build và chạy Castalia (phiên bản `omnetpp54-compatibility`) trên **OMNeT++ 6.3.0**, nguyên nhân, biện pháp sửa, và cách chạy simulation.

---

## Môi trường

- **OMNeT++**: 6.3.0 (build `251110-bb3d6a4731`)
- **Platform**: macOS arm64 (Apple Silicon), Clang
- **Simulation thử nghiệm**: `Simulations/CellularTest7/omnetpp.ini`

---

## Tổng hợp các sửa đổi

### 1. `--msg4` không còn được hỗ trợ

**File:** `src/makefrag`

**Error message:**
```
opp_msgtool: option is no longer supported: --msg4
make[1]: *** [Makefile:189: helpStructures/TimerServiceMessage_m.h] Error 1
```

**Nguyên nhân:**  
OMNeT++ 6.x loại bỏ hoàn toàn format msg4. `opp_msgtool` chỉ còn hỗ trợ msg6 (default). `makefrag` append `--msg4` vào lệnh `MSGC`.

**Biện pháp:**  
Comment out dòng `--msg4` trong `src/makefrag`:

```diff
-MSGC:=$(MSGC) --msg4
+# MSGC:=$(MSGC) --msg4
```

---

### 2. Msg files dùng syntax msg4 (cplusplus / class forward declaration)

**Files bị ảnh hưởng:**
- `src/node/application/valueReporting/ValueReportingPacket.msg`
- `src/node/communication/mac/tMac/TMacPacket.msg`
- `src/node/communication/mac/mac802154/Basic802154Packet.msg`
- `src/node/communication/mac/baselineBanMac/BaselineMacPacket.msg`
- `src/node/communication/mac/tunableMac/TunableMacPacket.msg`
- `src/node/communication/routing/multipathRingsRouting/MultipathRingsRoutingPacket.msg`
- `src/node/communication/routing/bypassRouting/BypassRoutingPacket.msg`
- `src/node/communication/routing/cellularRouting/CellularRoutingPacket.msg`
- `src/node/communication/routing/gstebRouting/GSTEBRouting.msg`
- `src/node/communication/routing/ssCellularRouting/SSCellularRouting.msg`

**Error message:**
```
node/application/valueReporting/ValueReportingPacket.msg:17: Error: Type declarations are not
needed with imports, try invoking the message compiler in legacy (4.x) mode using the --msg4 option
node/application/valueReporting/ValueReportingPacket.msg:25: Error: 'ValueReportingDataPacket':
unknown base class 'ApplicationPacket'
```

**Nguyên nhân:**  
Trong msg4, để kế thừa từ một packet ở file khác, cần dùng `cplusplus {{ #include ... }}` kết hợp với forward declaration `class X;`. Trong msg6, phải dùng `import`.

**Biện pháp:**

- **7 files chung** (cũng có trong omnetpp-6.3.0/samples/Castalia): copy trực tiếp bản đã được port từ omnetpp-6.3.0.

- **3 files đặc thù** của repo này: thay thủ công phần header:

```diff
-cplusplus {{
-#include "node/communication/routing/RoutingPacket_m.h"
-}}
-
-class RoutingPacket;
+import node.communication.routing.RoutingPacket;
```

---

### 3. Getter trả về `const` — không thể gán giá trị

**Files bị ảnh hưởng:**
- `src/node/application/VirtualApplication.cc` (line 196–198)
- `src/node/communication/mac/VirtualMac.cc` (line 194–198)
- `src/node/communication/radio/Radio.cc` (line 231–232)
- `src/node/communication/routing/VirtualRouting.cc` (line 55, 81–83)

**Error message:**
```
node/application/VirtualApplication.cc:198:44: error: no viable overloaded '='
    appPkt->getAppNetInfoExchange().timestamp = simTime();
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ^ ~~~~~~~~~
... 'this' argument has type 'const omnetpp::SimTime', but method is not marked const
```

```
node/communication/mac/VirtualMac.cc:194:39: error: cannot assign to return value because
function 'getNetMacInfoExchange' returns a const value
```

**Nguyên nhân:**  
Trong OMNeT++ 6.x, các getter cho struct fields trong msg-generated classes trả về `const` reference để đảm bảo immutability. Cần dùng getter riêng có hậu tố `ForUpdate` để lấy mutable reference.

**Biện pháp:**  
Thay tất cả các getter dạng `getXxx()` khi dùng để ghi bằng `getXxxForUpdate()`:

```diff
-appPkt->getAppNetInfoExchange().destination = string(dst);
-appPkt->getAppNetInfoExchange().source = selfAddress;
-appPkt->getAppNetInfoExchange().timestamp = simTime();
+appPkt->getAppNetInfoExchangeForUpdate().destination = string(dst);
+appPkt->getAppNetInfoExchangeForUpdate().source = selfAddress;
+appPkt->getAppNetInfoExchangeForUpdate().timestamp = simTime();
```

Áp dụng tương tự cho:
- `getNetMacInfoExchange()` → `getNetMacInfoExchangeForUpdate()`
- `getMacRadioInfoExchange()` → `getMacRadioInfoExchangeForUpdate()`
- `getAppNetInfoExchange()` → `getAppNetInfoExchangeForUpdate()`

---

### 4. `cStringTokenizer::setDelimiter()` đổi tên

**File:** `src/wirelessChannel/defaultChannel/WirelessChannelTemporal.cc` (line 112)

**Error message:**
```
wirelessChannel/defaultChannel/WirelessChannelTemporal.cc:112:7: error: no member named
'setDelimiter' in 'omnetpp::cStringTokenizer'
```

**Nguyên nhân:**  
OMNeT++ 6.x đổi tên API: `setDelimiter()` → `setDelimiterChars()`.

**Biện pháp:**
```diff
-t.setDelimiter(",");
+t.setDelimiterChars(",");
```

---

### 5. NED: gate vector size cần prefix `parent.`

**Files bị ảnh hưởng:**
- `src/SensorNetwork.ned` (lines 37–38, 45–46, 51–52)
- `src/node/Node.ned` (lines 48–49)

**Error message:**
```
(wirelessChannel.defaultChannel.WirelessChannel)SN.wirelessChannel has no parameter named
'numNodes' (did you mean 'parent.numNodes'?), at SensorNetwork.ned:37 -- in module
(WirelessChannel) SN.wirelessChannel (id=2), during network setup
```

```
(node.Node)SN.node[0] has no parameter named 'numPhysicalProcesses'
(did you mean 'parent.numPhysicalProcesses'?), at SensorNetwork.ned:51
```

```
'sizeof(toPhysicalProcess)': Module SN.node[0].SensorManager has no gate vector or submodule
vector named 'toPhysicalProcess', at Node.ned:48
```

**Nguyên nhân:**  
Trong OMNeT++ 6.x, khi khai báo gate vector size trong phần `gates:` của một submodule, tên parameter hoặc gate được tìm kiếm trong scope của **submodule đó** (không phải compound module cha). Phải dùng `parent.` để tham chiếu đến parameter/gate của module cha.

**Biện pháp — SensorNetwork.ned:**
```diff
 wirelessChannel: <wirelessChannelName> like wirelessChannel.iWirelessChannel {
  gates:
-   toNode[numNodes];
-   fromNode[numNodes];
+   toNode[parent.numNodes];
+   fromNode[parent.numNodes];
 }

 physicalProcess[numPhysicalProcesses]: <physicalProcessName> like physicalProcess.iPhysicalProcess {
  gates:
-   toNode[numNodes];
-   fromNode[numNodes];
+   toNode[parent.numNodes];
+   fromNode[parent.numNodes];
 }

 node[numNodes]: node.Node {
  gates:
-   toPhysicalProcess[numPhysicalProcesses];
-   fromPhysicalProcess[numPhysicalProcesses];
+   toPhysicalProcess[parent.numPhysicalProcesses];
+   fromPhysicalProcess[parent.numPhysicalProcesses];
 }
```

**Biện pháp — Node.ned:**
```diff
 SensorManager: node.sensorManager.SensorManager {
  gates:
-   fromNodeContainerModule[sizeof(toPhysicalProcess)];
-   toNodeContainerModule[sizeof(toPhysicalProcess)];
+   fromNodeContainerModule[sizeof(parent.toPhysicalProcess)];
+   toNodeContainerModule[sizeof(parent.toPhysicalProcess)];
 }
```

---

## Cách build

```bash
# 1. Thiết lập môi trường OMNeT++ 6.x
source /path/to/omnetpp-6.3.0/setenv

# 2. Vào thư mục Castalia
cd ns-3-dev-git-ns-3.46/src/wsn/Castalia-topic-omnetpp54-compatibility

# 3. (Nếu chưa có src/Makefile) Tạo Makefile
make makefiles

# 4. Build
make
```

Binary output: `out/clang-release/src/CastaliaBin`

---

## Cách chạy CellularTest7

```bash
source /path/to/omnetpp-6.3.0/setenv

CASTALIA_BIN=/path/to/Castalia-topic-omnetpp54-compatibility/out/clang-release/src/CastaliaBin
cd Simulations/CellularTest7

# Chạy command-line (không cần GUI)
$CASTALIA_BIN -u Cmdenv --sim-time-limit=100s

# Chạy với giới hạn thời gian mặc định từ ini (5000000s — rất lâu)
$CASTALIA_BIN -u Cmdenv
```

**Kết quả chạy thử:**
```
Loading NED files from .../Castalia-topic-omnetpp54-compatibility/src:  35

Preparing for running configuration General, run #0...
Setting up network "SN"...
Initializing...

Running simulation...
** Event #0   t=0   Elapsed: 1.6e-05s (0m 00s)  0% completed
** Event #4964   t=100   Elapsed: 0.001251s (0m 00s)  100% completed

<!> Simulation time limit reached -- at t=100s, event #4964
```

---

## Tóm tắt nhanh

| # | Vấn đề | File | Biện pháp |
|---|--------|------|-----------|
| 1 | `--msg4` không hỗ trợ | `src/makefrag` | Comment out |
| 2 | Msg4 syntax trong `.msg` files | 10 files `.msg` | Thay `cplusplus/class` bằng `import` |
| 3 | Getter `const` không thể gán | 4 files `.cc` | Dùng `getXxxForUpdate()` |
| 4 | `setDelimiter()` đổi tên | `WirelessChannelTemporal.cc` | Đổi thành `setDelimiterChars()` |
| 5 | NED gate size cần `parent.` | `SensorNetwork.ned`, `Node.ned` | Thêm prefix `parent.` |
