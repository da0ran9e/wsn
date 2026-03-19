# System Model — Scenario 4

> Tài liệu này tổng hợp toàn bộ mô hình hệ thống đã cài đặt trong **scenario4**, phục vụ viết phần **System Model** cho bài báo.  
> File nguồn chính: `paper.tex` (Section II).

---

## 1. Tổng quan kiến trúc

Hệ thống nghiên cứu tập trung vào **một UAV thực hiện nhiệm vụ phát tán fragment và hỗ trợ phát hiện sớm**, hoạt động trên một mạng IoT đô thị dạng lưới. Kiến trúc gồm **ba thành phần chính**:

```
┌─────────────────────────────────────────────────────────────┐
│                      Smart City Area                        │
│                                                             │
│   [GN] [GN] [GN] ...  ←── N×N ground node grid            │
│   [GN] [GN] [GN] ...      spacing Δ metres                 │
│                                                             │
│         ↑ UAV (GMC coverage path)                           │
│                                                             │
│   [BS]  Base Station — planning & command                   │
└─────────────────────────────────────────────────────────────┘
```

| Thành phần | Số lượng | Vai trò |
|---|---|---|
| Ground Node (GN) | N × N | Thu nhận fragment, hợp tác intra-cell, phát cảnh báo |
| UAV | 1 | Bay phủ sóng theo GMC, broadcast fragment liên tục khi bay |
| Base Station (BS) | 1 | Thu topology, chọn vùng nghi vấn, lập kế hoạch đường bay, sinh fragment |

**Mặc định thực nghiệm:** `N = 20`, `Δ = 20 m`, 30% suspicious nodes, 10 fragments, tốc độ UAV = 20 m/s.

---

## 2. Mô hình mạng (Network Topology)

### 2.1 Lưới IoT

Các GN được bố trí trên lưới đều $N \times N$ với khoảng cách $\Delta$ m. Node ở hàng $r$, cột $c$ có tọa độ $(c \cdot \Delta,\; r \cdot \Delta,\; 0)$.

Tổng số node: $|\mathcal{N}| = N^2$.  
Node ID: $i = r \cdot N + c$ (row-major, 0-indexed).

### 2.2 Tổ chức Cell Lục giác

Các GN được nhóm thành **cell lục giác (hexagonal cell)** với bán kính $r_\text{cell} = 80$ m. Cell ID được tính theo hệ tọa độ trục (axial coordinates) của lưới lục giác:

$$\text{cellId} = q + r \cdot \text{gridOffset}$$

Trong mỗi cell:
- **Cell Leader (CL)** được bầu tự động dựa trên vị trí gần tâm cell nhất.
- **Cell Members** là các node còn lại trong cell.
- Mỗi cặp cell liền kề có **gateway pair** để định tuyến liên-cell.

### 2.3 Giai đoạn Startup Discovery

Trong $t_\text{startup} = 5$ s đầu tiên, mỗi GN:
1. Phát gói Beacon để khám phá neighbor.
2. Xây dựng bảng láng giềng (1-hop và 2-hop) + đo RSSI.
3. Hình thành Cell Forwarding Tree (CFT) nội-cell.
4. Đồng bộ thời gian (clock offset tracking).

Sau startup, BS nhận **topology snapshot** gồm vị trí, neighbor list, RSSI trung bình của toàn bộ mạng.

---

## 3. Mô hình Fragment và Confidence

### 3.1 Phân mảnh ảnh (Image Fragmentation)

Ảnh tham chiếu (recognition file) có kích thước $W \times H$ pixel (mặc định $416 \times 416$ px, RGB = 3 bytes/pixel) được phân thành $K$ fragment bằng **pixel-stride interleaving**:

$$\text{fragment}_{i} = \{ \text{pixel}_{j} \mid j \equiv i \pmod{K},\; j = 0,1,\ldots,W \cdot H - 1 \},\quad i = 0,\ldots,K-1$$

Mỗi fragment $i$ chứa $s_i = \lfloor W \cdot H / K \rfloor$ pixel (đồng đều). Kích thước byte: $b_i = s_i \times 3$.

Stride interleaving đảm bảo mỗi fragment là **mẫu không gian đều** của toàn ảnh, tránh thiên lệch vùng ảnh.

### 3.2 Mô hình Confidence

Confidence của fragment $i$ dựa trên **xác suất nhận dạng từng phần** (union probability):

$$p_i = 1 - (1 - p_{\text{base}})^{s_i / (W \cdot H)}$$

với $p_{\text{base}} = C_{\text{master}} = 0.90$ là confidence của ảnh đầy đủ.

Confidence tích lũy tại node $n$ khi giữ tập fragment $\mathcal{F}_n$:

$$C_n = 1 - \prod_{i \in \mathcal{F}_n} (1 - p_i)$$

Tính chất quan trọng: khi node $n$ giữ **toàn bộ** $K$ fragment, $C_n = C_{\text{master}} = 0.90$.

### 3.3 Ngưỡng điều khiển

| Ngưỡng | Ký hiệu | Giá trị | Hành động |
|---|---|---|---|
| Cooperation threshold | $\tau_\text{coop}$ | 0.30 | Kích hoạt chia sẻ fragment intra-cell |
| Alert threshold | $\tau_\text{alert}$ | 0.75 | Phát cảnh báo đến BS + đánh dấu nhiệm vụ hoàn thành |

---

## 4. Lập kế hoạch đường bay UAV (Path Planning)

### 4.1 Chọn vùng nghi vấn (Suspicious Region Selection)

Từ topology snapshot, BS xác định tập **suspicious nodes** $\mathcal{P}$:
- Chọn top $\rho = 30\%$ node có mật độ kết nối (degree) cao nhất — đại diện cho các nút giao thông, hub đô thị.
- Đặt **seed node** $n^* = \arg\min_{n \in \mathcal{P}} \|pos(n) - \text{centroid}(\mathcal{P})\|$ — node gần tâm vùng nghi vấn nhất, dùng làm "mục tiêu quan sát chính".

### 4.2 UAV — Greedy Max-Coverage with Cost (GMC)

UAV áp dụng thuật toán **GMC** đề xuất:

**Bước 1 — Xây dựng tập ứng viên waypoint $\mathcal{C}$:**

$$\mathcal{C} = \mathcal{P} \;\cup\; \text{KMeans}(\mathcal{P},\; k),\quad k = \min(k_{\max},\; \lfloor |\mathcal{P}| / 4 \rfloor)$$

K-means với $k_{\max} = 8$, tối đa 20 iterations, tạo ra $k$ **hub centroid** có thể phủ nhiều suspicious node trong một lần ghé.

**Bước 2 — Precompute Coverage Set:**

$$\text{CS}(c) = \{ p \in \mathcal{P} \mid d(c, p) \le R_b \},\quad R_b = 50 \text{ m}$$

**Bước 3 — Greedy Selection:**

Tại mỗi vòng lặp, chọn waypoint $c^*$ tối đa hóa:

$$\text{score}(c) = \frac{|\text{CS}(c) \setminus \text{Covered}|}{\left(d(x_t, c) / v_\text{UAV}\right)^\alpha + \varepsilon}$$

với $\alpha = 1.0$ (cân bằng gain/cost), $\varepsilon = 10^{-6}$, $v_\text{UAV} = 20$ m/s.

Cập nhật: $\text{Covered} \leftarrow \text{Covered} \cup \text{CS}(c^*)$, tiếp tục đến khi $\text{Covered} = \mathcal{P}$.

UAV **không hover** — broadcast fragment liên tục khi đang bay, lặp lại toàn bộ quỹ đạo tuần hoàn cho đến khi kết thúc mô phỏng hoặc đến khi nhiệm vụ hoàn thành.

---

## 5. Giao thức vật lý (Physical Layer)

### 5.1 Radio Model — CC2420 (IEEE 802.15.4)

| Thông số | Giá trị |
|---|---|
| Tần số | 2.4 GHz |
| Tốc độ dữ liệu | 250 kbps (O-QPSK) |
| Công suất phát | 0 dBm |
| Ngưỡng thu | −95 dBm |
| DSSS Processing Gain | 9.03 dB |
| Modulation | O-QPSK |

### 5.2 Mô hình suy hao đường truyền (Path Loss)

$$PL(d) = PL_0 + 10n\log_{10}\!\left(\frac{d}{d_0}\right) + X_\sigma + X_f + L_h$$

với $PL_0 = 40.05$ dB tại $d_0 = 1$ m.

Profile truyền sóng theo góc ngẩng $\theta$ (UAV–GN elevation angle):

| Profile | Điều kiện | $n$ | $\sigma$ (dB) | K (Rician) |
|---|---|---|---|---|
| LoS | $\theta \ge 40°$ | 2.0 | 4 | 15 |
| Mixed | $20° \le \theta < 40°$ | 2.5 | 6 | 6 |
| NLoS | $\theta < 20°$ | 3.0 | 8 | 0 (Rayleigh) |
| Ground | air-to-ground, low alt | 3.2 | 7 | 0 |

Fast fading: $\sigma_f = 5.57 / \sqrt{1 + K}$ (dB).

### 5.3 Contact-Window Validation

Mỗi gói tin được chấp nhận **chỉ khi** công suất thu vượt ngưỡng trong **toàn bộ thời gian truyền**:

$$P_{rx}(t) \ge P_\text{sens} \quad \forall t \in [0,\; T_\text{air} + T_g]$$

với $T_\text{air} = 8L / R$ (airtime của gói $L$ bytes), $T_g = 2$ ms (guard interval).  
Vị trí UAV và GN được nội suy tuyến tính theo bước $T_\text{step} = 1$ ms.

### 5.4 BER và Packet Error Rate

$$\text{BER} = \frac{1}{2} \operatorname{erfc}\!\left(\sqrt{\text{SNR}_\text{lin} \cdot G_{p,\text{lin}}}\right)$$

$$\text{PER} = 1 - (1 - \text{BER})^{8L}$$

Quyết định drop packet: Bernoulli trial độc lập với xác suất $\text{PER}$.

---

## 6. Giao thức hợp tác intra-cell (Cell Cooperation Protocol)

### 6.1 Kích hoạt hợp tác

Khi $C_n \ge \tau_\text{coop} = 0.30$, node $n$ phát **manifest packet** liệt kê các fragment ID đang giữ đến toàn bộ cell peers.

### 6.2 Fragment Sharing

Mỗi cell peer nhận manifest sẽ đáp lại bằng cách gửi fragment mà $n$ còn thiếu — **ShareFragments(fromNode, toNode)**. Confidence của `toNode` được cập nhật ngay sau khi nhận fragment mới.

**Staggered scheduling**: Để tránh collision storm trong cùng cell, delay của mỗi request được tính:

$$t_\text{delay} = l \cdot \delta_\text{level} + \text{Uniform}(0, J_\text{max})$$

với $l$ = độ sâu trong CFT, $\delta_\text{level} = 20$ ms, $J_\text{max} = 15$ ms.

### 6.3 Global Cooperation Timeout

Sau một khoảng thời gian cố định kể từ startup, BS kích hoạt **global cooperation** buộc toàn bộ mạng chia sẻ fragment, đảm bảo không có node nào bị bỏ lỡ do kết nối kém với UAV.

### 6.4 Hoàn thành nhiệm vụ UAV

Nhiệm vụ của UAV được đánh dấu hoàn thành khi confidence của **seed node** $n^*$ đạt $\tau_\text{alert}$:

$$C_{n^*}(t^*) \ge \tau_\text{alert} \;\Rightarrow\; \text{MISSION\_COMPLETE at } t^*$$

Cơ chế phát hiện được kích hoạt theo **ba đường**:
1. **Direct reception**: Khi UAV broadcast fragment trực tiếp đến $n^*$.
2. **ShareFragments path**: Sau khi `ShareFragments(peer, n*)` cập nhật confidence của $n^*$.
3. **Periodic fallback check**: Mỗi 0.5 s, kiểm tra $C_{n^*}$ để bắt các trường hợp bị bỏ lỡ.

---

## 7. Tiêu chí hoàn thành nhiệm vụ và Early-Stop

| Sự kiện | Điều kiện | Thời điểm ghi nhận |
|---|---|---|
| `UAVMissionComplete` | $C_{n^*} \ge \tau_\text{alert} = 0.75$ | Thời điểm confidence vượt ngưỡng |
| `EarlyStop` | Nhiệm vụ UAV đã hoàn thành | `Simulator::Stop(+1.0 s)` |

Metric chính: **mission completion time** $T_\text{complete}$, là thời điểm seed node đạt ngưỡng cảnh báo.  
Các chỉ số phụ gồm: số fragment được thu trực tiếp từ UAV, số fragment được bù qua hợp tác intra-cell, và quãng trễ từ lúc bắt đầu broadcast đến khi hoàn thành nhiệm vụ.

---

## 8. Luồng thực thi tổng thể

```
t = 0.0s   → Network initialized (N×N GNs + 1 UAV + BS)
t = 0..5s  → Startup phase: neighbor discovery, cell formation, topology build
t = 5.01s  → BS post-init: fragment generation + UAV path planning (GMC)
t = 5.02s  → BS control tick: receive topology snapshot
t = 5.10s  → InitializeUavFlight: schedule GMC waypoints for UAV
t = 5.20s  → InitializeUavBroadcast: UAV starts fragment broadcast loop
t = 5.30s  → InitializeCellCooperationTimeout: schedule global cooperation
t = 5.50s  → Periodic topology updates (every 1s)
             Periodic confidence snapshots (every 10s)
             Periodic mission-completion check (every 0.5s)
...
t = T_complete → UAVMissionComplete (seed node confidence ≥ 0.75)
t = T_complete + 1.0s → EarlyStop
```

---

## 9. Tham số mô phỏng tóm tắt

| Tham số | Ký hiệu | Giá trị mặc định |
|---|---|---|
| Grid size | $N$ | 20 |
| Grid spacing | $\Delta$ | 20 m |
| Number of fragments | $K$ | 10 |
| Master file confidence | $C_\text{master}$ | 0.90 |
| Cooperation threshold | $\tau_\text{coop}$ | 0.30 |
| Alert threshold | $\tau_\text{alert}$ | 0.75 |
| Suspicious fraction | $\rho$ | 30% |
| UAV speed | $v$ | 20 m/s |
| UAV altitude | $h$ | 20 m |
| UAV broadcast radius | $R_b$ | 50 m |
| Cell radius | $r_\text{cell}$ | 80 m |
| TX power | $P_\text{tx}$ | 0 dBm |
| RX sensitivity | $P_\text{sens}$ | −95 dBm |
| Radio bitrate | $R$ | 250 kbps |
| Image size | $W \times H$ | 416 × 416 px |
| Startup duration | $t_\text{startup}$ | 5 s |
| GMC cost exponent | $\alpha$ | 1.0 |
| GMC max centroids | $k_\text{max}$ | 8 |
| UAV hover time | — | 0 s (broadcast-while-flying) |

---

## 10. Mapping sang paper.tex

| Subsection trong paper | Nội dung tương ứng tại đây |
|---|---|
| II-A Network Topology | Mục 2 (lưới, cell, startup) |
| II-B Fragment Confidence Model | Mục 3 (stride fragment, union prob, ngưỡng) |
| II-C Intra-Cell Cooperative Protocol | Mục 6 (ShareFragments, stagger, timeout) |
| II-D Mission Completion Criterion | Mục 7 (completion, metrics, early-stop) |
| III Problem Formulation | Mục 4.2 (CS, coverage constraint, objective) |
| IV GMC Algorithm | Mục 4.2 (candidate set, score function, pseudocode) |
| V Physical Layer | Mục 5 (path loss, fading, contact-window, BER/PER) |
