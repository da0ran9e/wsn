# SONIC — Social-Origin Network with Incremental Credibility
## Ý tưởng nghiên cứu: Trust-Aware Self-Organizing IoT Networks

> **Trạng thái:** Ý tưởng đang phát triển — Phase 1 (Initialization) đã được phác thảo  
> **Hướng target:** IEEE WCNC / ICC  
> **Nền tảng:** Mở rộng từ PECEE (HUST) sang bài toán trust

---

## 1. Mô tả Pitch

### Vấn đề

Các hệ thống mạng cảm biến IoT hiện tại được triển khai với một giả định ngầm nguy hiểm: **các node tin tưởng nhau mặc định từ khi bật nguồn**. Không có cơ chế nào để các node "làm quen" và đánh giá lẫn nhau trước khi bắt đầu chia sẻ thông tin quan trọng.

Trong thực tế — đặc biệt trong môi trường công nghiệp quan trọng như nhà máy lọc dầu, khu công nghiệp hóa chất — điều này tạo ra một cửa sổ tấn công ngay từ thời điểm khởi động: kẻ tấn công đã cài sẵn trong mạng có thể hành động ngay lập tức mà không bị nhận diện.

### Câu hỏi nghiên cứu trung tâm

> *Nếu các node IoT hoàn toàn không biết gì về nhau khi vừa bật nguồn — như những người mù trong một căn phòng lạ — chúng có thể tự tổ chức thành các nhóm tin cậy, phát hiện kẻ giả mạo, và xây dựng nền móng trust trước khi bắt đầu vận hành thực sự không?*

### Insight cốt lõi

Thay vì coi trust là điều kiện để vào nhóm, SONIC đề xuất đảo ngược:

```
Truyền thống:   TIN TƯỞNG → nhóm lại → chia sẻ thông tin
SONIC:          Nhóm sơ bộ → quan sát → XÂY DỰNG TRUST → nhóm thực sự
```

Nhóm sơ bộ dựa trên **tín hiệu vật lý** (RSSI, MAC pattern) — thứ khó giả mạo trong thời gian ngắn. Trust thực sự được xây dựng dần qua tương tác trong nhóm đó.

---

## 2. Bối cảnh và Ứng dụng

### Bài toán hẹp: Nhà máy lọc dầu / Khu công nghiệp hóa chất

Sau khi triển khai, hàng trăm node cảm biến IoT được bật đồng thời trong môi trường:
- **Không có GPS** (tín hiệu GPS bị chặn trong nhà máy)
- **Không có server cấu hình** (infrastructure có thể bị gián đoạn)
- **Có kẻ tấn công tiềm ẩn** đã được cài sẵn (supply chain attack, insider threat)
- **Yêu cầu self-organization hoàn toàn** trong vài giây đến vài phút

Mỗi node sau khi hình thành nhóm và xây dựng trust cơ bản sẽ sẵn sàng cho **Phase 2**: nhận fragment từ UAV trinh sát và ra quyết định phân loại mức nguy hiểm phi tập trung.

### Liên hệ với PECEE

PECEE (HUST, 2025) đã xây dựng framework phân vùng mạng WSN dựa trên hexagon cell với GPS. SONIC là phần tiếp theo giải quyết câu hỏi PECEE bỏ ngỏ: **"Các node trong cùng cell có thực sự tin nhau không, và làm thế nào để xây dựng trust đó?"** — nhưng không sử dụng lại cấu trúc hexagon hay GPS.

---

## 3. Ý tưởng Chi tiết: Phase 1 — Initialization

### Phép ẩn dụ: Những người mù trong phòng lạ

Mỗi node giống như một người mù vừa bước vào một căn phòng có nhiều người lạ. Họ:
- **Không thấy** ai (không có GPS, không có topology map)
- **Có thể nghe** giọng và ước lượng khoảng cách (RSSI)
- **Nhận ra giọng quen** qua đặc điểm riêng (MAC address, transmission pattern)
- **Thận trọng** với giọng lạ hoặc cố tình bắt chước người khác

### Hai kịch bản song song

Phase 1 không giả định môi trường an toàn hay nguy hiểm — nó **tự đánh giá** và điều chỉnh hành vi:

| | Kịch bản AN TOÀN | Kịch bản CẢNH GIÁC |
|---|---|---|
| **Dấu hiệu nhận biết** | Mật độ node bình thường, RSSI nhất quán, không có pattern bất thường | RSSI bất thường, có node phát quá sớm, có ID trùng lặp |
| **Hành vi tự giới thiệu** | Chia sẻ đầy đủ metadata | Chỉ chia sẻ ID + cross-check ngay |
| **Tốc độ nhóm hóa** | Nhanh hơn | Thận trọng hơn, yêu cầu xác nhận nhiều hơn |

---

### Bước 1 — Passive Listening (Lắng nghe thụ động)

**Thời gian:** $T_{listen}$ (vài giây)

Mỗi node **im lặng hoàn toàn** và thu thập:

```
Với mỗi tín hiệu nhận được:
  - Node_ID (từ MAC layer)
  - RSSI (cường độ tín hiệu)
  - Transmission timestamp pattern
  - Tần suất phát
```

**Tín hiệu cảnh báo phát hiện trong bước này:**
- Node phát tín hiệu khi tất cả đang im lặng → đáng ngờ (kẻ tấn công không biết protocol)
- Hai MAC khác nhau có pattern phát giống hệt nhau → clone attack
- RSSI quá mạnh so với khoảng cách ước lượng → giả mạo vị trí

**Kết quả:** Danh sách hàng xóm thô $\mathcal{N}_{raw}(n)$ với RSSI và anomaly flag.

---

### Bước 2 — Conditional Self-Introduction (Tự giới thiệu có điều kiện)

**Thời gian:** $T_{intro}$ (vài giây)

Node đánh giá môi trường từ Bước 1 và quyết định mức độ chia sẻ:

**Trường hợp AN TOÀN** — không có anomaly flag:
```
Beacon = [Node_ID | Sensor_type | Approx_location | Timestamp]
```

**Trường hợp CẢNH GIÁC** — có anomaly flag:
```
Beacon = [Node_ID | Timestamp]  ← tối giản
+ Cross-check: "Tao nghe thấy những ID này: {list}. Mày có nghe thấy ai trùng không?"
```

**Cross-check ID** là cơ chế đặc biệt trong chế độ cảnh giác:
- Node A và B trao đổi danh sách ID mình đã nghe
- Nếu node X được A nghe với RSSI = -60 dBm, nhưng B (ở gần A) nghe X với RSSI = -90 dBm → X có thể đang giả mạo vị trí
- Collective detection: không node nào đủ thông tin một mình, nhưng tập thể phát hiện được inconsistency

---

### Bước 3 — Emergent Star-Topology Grouping (Nhóm hóa hình sao nổi lên)

**Mục tiêu:** Tạo các nhóm nhỏ có topology hình sao, **đều nhau về mặt không gian** mà **không cần GPS**.

#### 3a. Chọn node trung tâm bằng Randomized MIS

Mỗi node tính một **priority score**:

$$r_n = \text{Hash}(\text{MAC}_n \;\|\; \text{RSSI\_signature}_n \;\|\; \text{timestamp})$$

Trong đó `RSSI_signature` là vector RSSI quan sát được từ hàng xóm — khó giả mạo vì phụ thuộc vị trí vật lý thực.

**Thuật toán chọn trung tâm (Randomized MIS):**

```
Round 0:  Mỗi node phát r_n
Round 1:  So sánh r_n với tất cả hàng xóm
          IF r_n > r_m ∀m ∈ N(n):
              → Tuyên bố là CENTER
              → Phát: "Tôi là trung tâm"
          ELSE:
              → Chờ
Round 2:  Node chưa có nhóm nghe thông báo CENTER
          → Chọn CENTER có RSSI mạnh nhất (gần nhất)
          → Gửi pairing request
          → Phát: "Tôi đã có nhóm"
Round 3:  CENTER xác nhận danh sách thành viên
          → Nhóm sao hình thành
```

**Tại sao tạo ra phân bố đều:** Randomized MIS đảm bảo không có hai CENTER nào là hàng xóm trực tiếp của nhau. Kết quả là các nhóm sao không chồng lấp và tự nhiên phủ đều không gian — tương tự hexagon cell nhưng không cần GPS.

```
Mật độ cao:              Mật độ thấp:

  L   L   L              L         L
   \  |  /                \       /
    C   C                  C
   /  |  \                 |
  L   L   L                L

Nhiều nhóm nhỏ,         Ít nhóm hơn,
đều nhau                tự điều chỉnh
```

#### 3b. Mutual pairing và conflict resolution

Node chọn `best neighbor` dựa trên score sơ bộ:

$$\text{Score}(m) = w_1 \cdot \text{RSSI\_consistency}(m) + w_2 \cdot \text{ID\_verified}(m) + w_3 \cdot \text{timing\_regularity}(m)$$

Nếu hai node (A, B) đều muốn ghép với cùng một CENTER C → cả ba hình thành **tam giác** (transitive trust tự nhiên nhất có thể có ở giai đoạn này).

#### 3c. Điều kiện thành viên nhóm

Node muốn vào nhóm phải thỏa mãn:
1. Được ít nhất **một** node trong nhóm xác nhận (nghe thấy rõ)
2. Không bị **bất kỳ** node nào trong nhóm flag là đáng ngờ
3. Tự mình nghe thấy được ít nhất một node trong nhóm

→ Đảm bảo liên kết vật lý thực, không có thành viên "ẩn"

#### 3d. Giới hạn kích thước

Nhóm dừng mở rộng khi:
- Không còn node thỏa điều kiện 3c, **hoặc**
- Đạt kích thước tối đa $G_{max}$ (tham số thiết kế)

---

### Bước 4 — Selective Filtering (Chỉ khi cảnh giác)

Sau khi nhóm hình thành, thành viên **so sánh ghi nhận lẫn nhau** về node đáng ngờ:

**Ví dụ collective anomaly detection:**
```
Node A: "Tôi nghe X với RSSI = -60 dBm"
Node B: "Tôi nghe X với RSSI = -90 dBm"
A và B cách nhau 2m, X tự nhận ở giữa
→ Chênh lệch 30 dBm là bất thường về mặt vật lý
→ X bị flag bởi collective vote
```

**Cơ chế xử lý:**
- Node bị flag bởi đa số → **không blacklist vĩnh viễn**
- Bị đẩy ra nhóm hiện tại → phải **xin vào nhóm khác**
- Nhóm mới nhận node bị đẩy ra **biết lịch sử đó** → theo dõi chặt hơn

→ Tạo ra **trust reputation lan truyền tự nhiên** mà không cần infrastructure tập trung

---

### Kết quả sau Phase 1

```
ĐẦU VÀO:  N node hoàn toàn xa lạ, không có GPS, không có config
                        ↓
                    PHASE 1
                        ↓
ĐẦU RA:   Các nhóm sao đều nhau về không gian
           + Mỗi thành viên có Physical Trust với nhau
           + Danh sách node đáng ngờ đã được identify
           + Nền móng để xây dựng Behavioral Trust (Phase 2)
```

**Physical Trust** (được xây dựng trong Phase 1):
- Xác nhận sự tồn tại và vị trí tương đối
- Xác nhận ID không bị giả mạo
- Xác nhận hành vi phát tín hiệu bình thường

**Behavioral Trust** (sẽ được xây dựng trong Phase 2):
- Trust để nhận thông tin (Receive Trust — RT)
- Trust để chuyển tiếp thông tin (Send Trust — ST)

---

## 4. Dual Trust Model (Sơ lược — Phase 2+)

Một đóng góp conceptual quan trọng: **phân tách trust thành hai chiều độc lập**

```
RECEIVE TRUST (RT):  "Tôi có chấp nhận thông tin từ node này không?"
                     → Bảo vệ khỏi data poisoning
                     → Câu hỏi: nguồn này có đáng tin không?

SEND TRUST (ST):     "Tôi có chuyển tiếp thông tin qua node này không?"
                     → Bảo vệ khỏi selective forwarding / relay sabotage
                     → Câu hỏi: node này có relay trung thực không?
```

Hai loại này **không đối xứng**:
- Node A có thể có RT cao với B (tin nhận từ B) nhưng ST thấp (B hay drop packet)
- Node C có thể có ST cao với D (D relay tốt) nhưng RT thấp (D gửi dữ liệu lạ)

---

## 5. Positioning: Khoảng trống trong Literature

| | Literature hiện tại | SONIC |
|---|---|---|
| **Khởi tạo trust** | Giả định sẵn (PKI, pre-shared key) hoặc bỏ qua | Xây dựng từ đầu qua physical observation |
| **Phân vùng mạng** | Cần GPS (PECEE, LEACH-C) hoặc centralized | GPS-free, pure self-organization |
| **Threat model** | Static (tỷ lệ Byzantine cố định) | Dynamic từ thời điểm bật nguồn |
| **Trust loại** | Một chiều (reputation score) | Hai chiều (RT + ST) |
| **Cold start** | Blind (prior = 0.5) | Informed (physical trust làm prior) |
| **Phát hiện clone** | Cần infrastructure | Collective RSSI cross-check |

---

## 6. Các Bài Nghiên cứu Liên quan

### 6.1 Phân vùng mạng và Self-Organization

**[1] Arapoglu & Dagdeviren (2020)** — *"Distributed Self-Stabilizing Capacitated Maximal Independent Set Construction in Wireless Sensor Networks"* — Springer Wireless Personal Communications  
→ Thuật toán MIS phân tán tự ổn định cho WSN, không cần thông tin toàn cục. **Liên quan trực tiếp đến Sub-bước 3a.**

**[2] Dagdeviren (2018)** — *"An energy-efficient, self-stabilizing and distributed algorithm for maximal independent set construction in wireless sensor networks"* — ScienceDirect Computer Networks  
→ Phân tích lý thuyết MIS với ba trạng thái (IN/OUT/WAIT), phù hợp implement trên IoT resource-constrained.

**[3] Zhu et al. (2010)** — *"Constructing weakly connected dominating set for secure clustering in distributed sensor network"* — Journal of Combinatorial Optimization  
→ Kết nối giữa MIS và secure clustering — chứng minh MIS là nền tảng tốt cho phân vùng an toàn.

**[4] Mir & Meziane (2024)** — *"Novel adaptive DCOPA using dynamic weighting for vector of performances indicators optimization of IoT networks"* — Expert Systems with Applications  
→ Clustering phân tán thích nghi cho IoT, không cần GPS, điều chỉnh theo mật độ.

### 6.2 Neighbor Discovery

**[5] Hybrid Approach to Neighbour Discovery (HAND) (2022)** — IASC  
→ Kết hợp nhiều phương pháp ND để tối ưu energy và latency. RSSI-based group formation là nền tảng. **Liên quan đến Bước 1-2.**

**[6] NIST SP 1800-36 (2025)** — *"Trusted IoT Device Network-Layer Onboarding and Lifecycle Management"*  
→ Framework NIST cho onboarding an toàn, nhấn mạnh zero-touch và per-device credentials. Là baseline so sánh cho SONIC.

### 6.3 Trust Management trong IoT/WSN

**[7] IoT Trust Model Survey (Springer, 2023)** — *"A survey on IoT trust model frameworks"*  
→ Survey toàn diện về trust frameworks trong IoT. Mendoza et al. [67 trong survey] đề xuất framework gần nhất với SONIC: neighbor discovery + trust table exchange. SONIC khác ở chỗ không cần server.

**[8] Game Theory for Decentralized IoT Trust (ScienceDirect, 2020)** — *"Robust Decentralised Trust Management for the Internet of Things by Using Game Theory"*  
→ Bayesian Signaling Game + Dempster-Shafer cho trust IoT phi tập trung. **Liên quan đến Phase 2+ của SONIC.**

**[9] WIRS Model (Springer, 2024)** — *"Advancing wireless sensor network security through enhanced intrusion detection techniques"*  
→ Weighted Intrusion Risk Score sử dụng RSSI, distance, và advertisement packets. **Xác nhận RSSI là feature hữu ích cho anomaly detection.**

### 6.4 Physical Layer Security và RSSI-based Detection

**[10] IEEE Access (2024)** — *"A Machine-Learning-Based Technique for False Data Injection Attacks Detection in Industrial IoT"*  
→ Autoencoder khai thác spatial-temporal correlation của sensor data. **Liên quan đến cross-check trong Bước 2.**

**[11] IIoT FDI Dataset (2025)** — *"False data injection attack dataset for classification, identification, and detection for IIoT in Industry 5.0"*  
→ Dataset benchmark cho FDI attacks trong IIoT. Có thể dùng để evaluate SONIC trong future work.

### 6.5 Nền tảng từ nhóm tác giả

**[12] Vu & Nguyen (2025, HUST)** — *"Joint Fragment Dissemination and Edge Fusion for Fast Target Detection in UAV-Assisted Urban IoT"*  
→ Bài báo gốc: Phase 1 (UAV dissemination). SONIC giải quyết prerequisite: trust infrastructure cho ground node network.

**[13] Le, Vu & Nguyen (2025, HUST)** — *"PECEE: Platform for Elastic Clustering and Energy Efficiency"*  
→ Framework phân vùng WSN với hexagon cell và GPS. SONIC kế thừa ý tưởng phân cấp xã hội nhưng không dùng GPS và không phụ thuộc vào pre-configuration.

---

## 7. Câu hỏi Nghiên cứu

**RQ1 — Correctness:** Dưới tỷ lệ Byzantine node $\beta$ trong môi trường không có GPS, Randomized MIS có đảm bảo phân vùng đều và phát hiện được kẻ tấn công không?

**RQ2 — Speed vs. Security trade-off:** $T_{listen}$ và $T_{intro}$ tối ưu là bao nhiêu? Kéo dài hơn → phát hiện tốt hơn nhưng startup chậm. Có thể adaptive theo mức độ anomaly không?

**RQ3 — Clone attack resilience:** Collective RSSI cross-check có đủ để phát hiện clone attack trong môi trường RF phức tạp (multipath, shadowing) của nhà máy công nghiệp không?

**RQ4 — MIS uniformity:** So với hexagon cell (GPS-based), Randomized MIS đạt được bao nhiêu % tính đều về phân bố không gian? Có metric nào đo được không?

**RQ5 — Reputation propagation:** Khi node bị đẩy ra nhóm A và xin vào nhóm B, thông tin "lý lịch" này được truyền như thế nào mà không cần server trung tâm?

---

## 8. Hướng Phát triển tiếp theo

```
Phase 1 (Initialization) ← Đang phát triển
    └── Pure self-organization
    └── GPS-free star-topology grouping
    └── Physical trust bootstrapping
    └── Clone/spoofing detection

Phase 2 (Behavioral Trust Building) ← Chưa phát triển
    └── Receive Trust (RT) xây dựng qua data consistency
    └── Send Trust (ST) xây dựng qua relay behavior
    └── Volatility-aware trust update (DRIFT-inspired)

Phase 3 (Fragment-based Decision Making) ← Từ DRIFT
    └── UAV phát tán fragment
    └── Trust-weighted confidence accumulation
    └── Multi-level hazard classification
    └── Periodic Attack-Rest (PAR) detection
```

---

## 9. Ghi chú Phát triển

- **Thuật ngữ cần tránh:** hexagon cell, Cell Leader (CL), Cell Gateway (CG), Cell Forwarding Tree (CFT) — các khái niệm này thuộc về PECEE
- **Thuật ngữ SONIC:** group, center node, leaf node, physical trust, behavioral trust, RT, ST
- **Simulation platform:** Python (NetworkX) hoặc ns-3, topology grid 20×20 là starting point
- **Key figure cần có:** So sánh spatial uniformity của SONIC vs hexagon cell vs LEACH-style random clustering

---

*Tài liệu này được tổng hợp từ quá trình thảo luận — cập nhật lần cuối: 2026-04-02*