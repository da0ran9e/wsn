## Tổng quan: UAV-Assisted Real-Time Wanted Person Recognition in Smart City IoT Networks

**Target Conference**: IEEE ICCE 2026

**Core Problem**: Minimize time-to-detection for wanted person recognition in urban IoT surveillance networks using UAV-assisted fragment distribution with probabilistic matching and cooperative detection.

## Kịch bản: 
UAV mang ảnh/video (file nặng) của đối tượng bị truy nã cần phát hiện nhanh nhất có thể
Một khu vực khả nghi đã được xác định, có mạng IoT gồm các camera/edge nodes với AI recognition capability
UAV bay qua khu vực khả nghi trong thời gian hữu hạn (mission time constraint)
Vì dữ liệu quá nặng, UAV không thể dừng lại để truyền toàn bộ cho từng node → không khả thi về thời gian và năng lượng
Giải pháp: chia dữ liệu thành k mảnh (fragments/blocks), broadcast trong khi bay 

## Key idea: 
UAV phát các mảnh dữ liệu 
Mỗi mảnh có xác suất khớp tại từng node
Intra-cell cooperation để tăng khả năng khớp 
mục tiêu là tiết kiệm thời gian hoàn thành nhiệm vụ

## Mô hình khớp dữ liệu (Fragment-based Probabilistic Detection Model):

**K-block Concept**: Node không nhất thiết phải lấy được $k$ blocks riêng biệt. Thay vào đó, node cần tích lũy **giá trị khớp (confidence)** từ các fragments hiện có đạt ngưỡng nhất định ($\theta_{high}$). Tham số $k$ được sử dụng để ước tính số lượng fragments tối thiểu có thể cần thiết để đạt ngưỡng đó, nhưng giá trị thực tế phụ thuộc vào **chất lượng nội dung, bằng chứng cục bộ, và độ tin cậy tích lũy** từ phép fusion các fragments.

**Per-Fragment Likelihood**: Mỗi node $i$ nhận fragment $f_j$ có xác suất khớp:
$$P(f_j | target, node_i) = \text{function of:}$$
- **Nội dung mảnh** (content quality): góc nhìn, ánh sáng, đặc trưng discriminative (face > clothing > background)
- **Dữ liệu cục bộ** (local evidence): đối tượng có xuất hiện trong camera view của node $i$ không
- **Chất lượng AI** (compute capability): edge processing power, model accuracy

**Bayesian Update Rule**: Node $i$ tích lũy confidence qua các fragments:
$$C_i(t) = P(target | \mathcal{F}_i(t)) = \frac{P(\mathcal{F}_i(t) | target) \cdot P(target)}{P(\mathcal{F}_i(t))}$$
Với $\mathcal{F}_i(t)$ là tập fragments node $i$ có tại thời điểm $t$.

**Fragment Utility Function** (Submodular): 
$$U(f_j | \mathcal{F}_i, node_i)$$ 
đo giá trị thông tin của fragment $f_j$ khi node đã có tập $\mathcal{F}_i$. 
- Tính chất diminishing returns: mảnh đầu tiên (key features) có giá trị cao nhất
- Cần nghiên cứu: information-theoretic utility, submodular maximization

**Detection Thresholds**:
- $\theta_{low} < \theta_{medium} < \theta_{high}$
- Nếu $C_i(t) > \theta_{high}$ → Alert ngay lập tức
- Nếu $C_i(t) > \theta_{medium}$ → Trigger cooperation trong cell
- Nếu $C_i(t) < \theta_{low}$ → Tiếp tục thu thập fragments

**Open Question**: Cần mô hình hóa Bayesian fusion cho multi-fragment person recognition, xử lý correlation giữa các fragments

## Problem Formulation:

**Decision Variables**:
- UAV path $\mathcal{P} = (p_1, p_2, ..., p_m)$ qua các cell targets
- Fragment scheduling $\mathcal{S} = (f_1, f_2, ..., f_k)$ với thứ tự và timing

**Objective**: 
$$\min_{\mathcal{P}, \mathcal{S}} \mathbb{E}[T_{detect}]$$
Minimize expected time-to-first-successful-detection

**Subject to**:
- $P_{D}(T_{max}) \geq \alpha$ (detection probability threshold tại thời điểm deadline)
- $P_{FA} \leq \beta$ (false alarm rate constraint)
- $E_{UAV} \leq E_{budget}$ (UAV energy constraint)
- $T_{flight} \leq T_{max}$ (mission time constraint)
- Intra-cell cooperation cost (latency, bandwidth, energy)

**Key Challenge**: Trade-off giữa tốc độ phát hiện (time) và độ chính xác (accuracy)

## Cơ chế chính:

### 1. UAV Broadcast:
UAV bay theo path $\mathcal{P}$ và phát fragments theo schedule $\mathcal{S}$
Nodes trong coverage radius nhận được fragments
Mỗi fragment có sequence number và metadata (utility hint, priority)

### 2. Realtime Intra-Cell Cooperation (CRITICAL UPDATE):

**Timeline**: Matching và cooperation xảy ra **REALTIME** trong khi UAV đang bay, KHÔNG chờ UAV rời đi.

**Phase A - Immediate Reception & Local Check** (trong khi UAV ở cell):
```
Mỗi node i khi nhận fragment f_j:
1. Cập nhật: C_i ← Bayesian_update(C_i, f_j, local_evidence)
2. Nếu C_i vẫn < θ_medium: tiếp tục nhận fragments khác
3. Nếu C_i > θ_high: ALERT to BS ngay (mission success)
4. Nếu C_i > θ_medium (nhưng < θ_high): Broadcast "need cooperation" trong cell
```

**Phase B - Cooperative Fragment Exchange** (parallel với UAV broadcast):
```
5. Nodes nghe "need cooperation" → gửi manifest (list fragments có)
6. Node requesting → request fragments còn thiếu từ neighbors
7. Nhận fragment từ neighbor → update C_i
8. Nếu C_i vượt θ_high → ALERT to BS
```

**Cell Leader (CL) Role**:
- Điều phối cooperation để tránh broadcast storm
- Priority scheduling: ưu tiên nodes có $C_i$ gần threshold
- Quản lý Cell Forwarding Tree (CFT) để route alert về BS
- Aggregate confidence từ nhiều nodes (consensus-based validation)

**Cell Gateway (CG) Role**: 
- Forward alert từ CL về BS hoặc neighboring cell
- Không tham gia cooperation trực tiếp (focus on routing)

**Cooperation Cost**:
- Latency: $T_{coop}$ = time for manifest broadcast + request/response
- Energy: intra-cell transmission cost
- Bandwidth: overhead cần tính vào constraints

**Open Question**: Thiết kế protocol cho realtime cooperative detection, cân bằng giữa latency và accuracy

### 3. Fragment Scheduling & UAV Path Planning:
Lập lịch thứ tự phát fragments dựa trên utility
Tối ưu đường bay để coverage cells trong $C_{target}$ với priority weights
Trade-off: tốc độ bay vs dwell time per cell

## Các giai đoạn của giả lập

### Phase 0: Network Deployment & Cell Layer Formation

**Vấn đề**: Cần thiết lập hạ tầng IoT với cấu trúc cell để hỗ trợ hợp tác và routing hiệu quả.

**Chi tiết**:
- **Triển khai nodes**: 100, 400 hoặc 900 nodes IoT (camera + edge compute) phân bố đều/không đều theo cấu trúc đô thị
  - Mỗi node có: GPS/localization, AI inference capability, wireless transceiver (e.g., CC2420), battery
  - Bán kính truyền tin $r_{tx}$ (radio range)
  
- **Hình thành cell layer (Hexagonal Grid)**:
  - Chia vùng giám sát thành hexagonal cells cạnh $a > r_{tx}$ (tránh interference)
  - Mỗi cell: 1 Cell Leader (CL), các Cell Gateway (CG), Cell Forwarding Tree (CFT)
  - TDMA scheduling theo 3-color scheme để tránh collision
  
- **Khởi tạo neighborhood**: Beacon discovery, CL lập topology nội cell, CG chuẩn bị pending links

**Đầu ra**: Network sẵn sàng, chưa có UAV.

---

### Phase 1: Mission Assignment & Optimization Planning

**Vấn đề**: Lập kế hoạch UAV bay qua khu vực khả nghi để phát hiện nhanh nhất với resources hạn chế.

**Chi tiết**:

**1a. Mission Input**:
- BS nhận: đối tượng bị truy nã (ảnh/video full-resolution, size $S_{total}$), khu vực target $C_{target}$, deadline $T_{max}$

**1b. Fragment Planning**:
- **Phân chia file**: Chia $S_{total}$ thành $n$ fragments, mỗi fragment size $s_f$
- **Fragment Value Assignment**: Gán utility $U(f_j)$ dựa trên discriminability (face > clothing > background)
- **Sắp xếp**: Fragments phát theo thứ tự giảm utility (high-value trước)

**1c. UAV Path Planning (Bài toán 1)**:
- **Vấn đề**: Tìm path $\mathcal{P}$ qua cells minimize $\mathbb{E}[T_{detect}]$ subject to flight time, energy
- **Hướng giải**: Prize-Collecting TSP, Traveling Repairman, greedy, genetic algorithm

**1d. Fragment Scheduling (Bài toán 2)**:
- **Vấn đề**: Lập lịch phát fragments theo thứ tự để maximize detection probability
- **Hướng giải**: Greedy (utility order), Restless MAB, Submodular Optimization

**Output**: Path $\mathcal{P}^*$, schedule $\mathcal{S}^*$, dwell time per cell

---

### Phase 2: UAV Flight & Realtime Fragment Broadcast

**Vấn đề**: UAV bay theo đường tối ưu và phát fragments theo lịch trong điều kiện thực tế.

**Chi tiết**:
- **UAV Mobility**: Tuân theo $\mathcal{P}^*$, tốc độ $v$, dwell time $t_c^{\text{dwell}}$ per cell
- **Broadcast Transmission**: Phát fragments theo $\mathcal{S}^*$, mỗi packet có metadata (sequence, utility, priority)
- **Link Model**: Path loss, RSSI, SNR, packet loss $P_{loss}(t)$
- **Adaptive Broadcasting**: Nếu link xấu, repeat hoặc dùng FEC (Forward Error Correction), rateless codes

**Output**: Nodes nhận fragments (subset, có thể bị loss), lưu buffer $\mathcal{F}_i$

---

### Phase 3: Realtime Reception, Local Matching & Intra-Cell Cooperation

**Vấn đề**: Nodes xử lý realtime để phát hiện sớm, hợp tác để cải thiện độ chính xác.

**Chi tiết**:

**3a. Reception & Confidence Update**:
```
Khi node i nhận fragment f_j:
1. Lưu: F_i ← F_i ∪ {f_j}
2. Feature extraction: φ_j ← FeatureExtractor(f_j)
3. Local evidence: s_local = object detection score từ camera
4. Likelihood: P(f_j | target, node_i) = f(φ_j, s_local, AI_quality)
5. Update: C_i(t) ← Bayesian_update(C_i(t-1), f_j, local_evidence)
```

**3b. Threshold Decision**:
```
IF C_i(t) > θ_high: ALERT to BS immediately
ELSE IF C_i(t) > θ_medium: Broadcast cooperation request
ELSE: Wait for more fragments
```

**3c. Intra-Cell Cooperation Protocol**:
- **Phase 3.1**: Node broadcasts manifest (F_i, C_i value)
- **Phase 3.2**: Neighbors reply with their manifests (F_j, F_k, ...)
- **Phase 3.3**: Node requests high-utility fragments từ neighbors
- **Phase 3.4**: Nhận fragments, update $C_i$ lại

**3d. CL Coordination**:
- Collect cooperation requests, prioritize nodes near $\theta_{high}$
- Schedule request-response windows (avoid broadcast storm)
- Aggregate cell-level confidence

**3e. Cooperation Cost**:
- Latency: ~100-150ms per round (manifest + request + tx)
- Energy: intra-cell transmission
- Bandwidth: limited by channel capacity
- Trade-off: cooperation delay vs accuracy gain

**Output**: Staggered alerts from nodes when $C_i > \theta_{high}$

---

### Phase 4: Alert Propagation & Early Termination

**Vấn đề**: Xác nhận phát hiện, báo cho BS và UAV để dừng sớm.

**Chi tiết**:

**4a. Alert Routing**:
```
Node i: C_i > θ_high → ALERT packet
  → Send to CL via CFT
CL: Verify (optional consensus from multiple nodes)
  → Route to BS via CG
BS: Log alert, verify detection
  → Send FEEDBACK to UAV: "Mission complete, return home"
```

**4b. UAV Early Termination**:
```
UAV receives feedback:
  → Skip remaining cells if possible
  → Return to base (minimize remaining energy)
  → Record: cells covered, T_detect
```

**4c. Confirmation & Metrics**:
- Single-node alert: lower confidence
- Multi-node consensus (2+ nodes with $C > \theta_{medium}$): higher confidence
- CL can aggregate before sending alert to BS

**4d. Feedback to Nodes**:
```
CL broadcasts: "MISSION_COMPLETE"
  → Nodes return to idle/standby
  → Save battery for future missions
```

**Output**: Mission completed with $T_{detect}$, alert confirmed, UAV returns.

---

## Metrics for Evaluation:

### Primary Metrics:

**1. Time-to-First-Success** (main objective):
$$T_{detect} = \min_{i \in \text{Nodes}} \{ t : C_i(t) > \theta_{high} \}$$
Thời điểm đầu tiên có bất kỳ node nào đạt ngưỡng detection

**2. Detection Probability at Time t**:
$$P_D(t) = P(\exists i : C_i(t) > \theta_{high} | \text{target present in area})$$
Xác suất phát hiện thành công tại thời điểm $t$

**3. False Alarm Rate**:
$$P_{FA} = P(\exists i : C_i(t) > \theta_{high} | \text{target NOT present})$$
Tỉ lệ báo động nhầm (ví dụ nhận nhầm người khác có giày giống)

**4. Detection Coverage Ratio**:
$$\eta_{coverage} = \frac{|\{c \in C_{target} : \exists i \in c, C_i > \theta_{high}\}|}{|C_{target}|}$$
Tỉ lệ cells phát hiện thành công trong tổng số target cells

### Secondary Metrics:
- **UAV Energy Consumption**: $E_{used}$ vs $E_{budget}$
- **Mission Completion Time**: $T_{mission}$ (có thể < $T_{max}$ nếu early termination)
- **Cooperation Overhead**: số lượng intra-cell transmissions, latency $T_{coop}$
- **Fragment Distribution Fairness**: variance of $|\mathcal{F}_i|$ across nodes
- **Confidence Evolution**: $C_i(t)$ over time per node/cell

### Trade-off Curves (for paper evaluation):
- ROC Curve: $P_D$ vs $P_{FA}$ với các giá trị $\theta$ khác nhau
- Pareto Front: $\mathbb{E}[T_{detect}]$ vs $P_D$ vs $E_{used}$
- Latency vs Accuracy: $T_{detect}$ vs $P_{FA}$ với cooperation overhead

---

## Tóm tắt Các Bài Toán Chính

### Problem 1: UAV Path Planning (Phase 1c)

**Formal Definition**:
$$\mathcal{P}^* = \arg\min_{\mathcal{P}} \mathbb{E}[T_{detect}(\mathcal{P})]$$

Subject to:
- $\text{distance}(\mathcal{P}) \times v^{-1} \leq T_{max}$ (flight time)
- $\text{energy}(\mathcal{P}) \leq E_{budget}$ (energy)
- Coverage: $\{$cells visited$\} \supseteq C_{target}$ (or weighted coverage)

**Characteristics**:
- NP-hard (Prize-Collecting TSP variant)
- Stochastic objective: $\mathbb{E}[T_{detect}]$ depends on:
  - Fragment reception success (link quality)
  - Local evidence (object presence in camera)
  - Intra-cell cooperation dynamics
  
**Approximation Algorithms**:
- Greedy Nearest Neighbor: O(m²) heuristic
- Christofides: 1.5-approximation for TSP
- Genetic Algorithm: population-based metaheuristic
- Dynamic Programming: optimal if m small (m ≤ 15)
- Ant Colony Optimization: pheromone-based heuristic

---

### Problem 2: Fragment Scheduling (Phase 1d)

**Formal Definition**:
$$\mathcal{S}^* = \arg\max_{\mathcal{S}} \Pr[\text{node detects target}]$$

Subject to:
- Total transmission time ≤ $\sum_c t_c^{\text{dwell}}$ (dwell windows)
- Bandwidth constraint: $R_{link}$ (bit rate)
- Coverage fairness: distribute high-value fragments across cells

**Characteristics**:
- Online scheduling: UAV decides next fragment in real-time based on:
  - Current location (cell c)
  - Nodes' current received fragments (if feedback available)
  - Remaining mission time
  
- Restless Multi-Armed Bandit formulation:
  - Each fragment $f_j$ = "arm" with reward $U(f_j)$
  - State space: time, cell, received fragments per node
  - Index policy (Whittle index) for optimal scheduling
  
**Heuristics**:
- **Greedy Utility Order**: Sort fragments by $U(f_j)$ descending, transmit in order
- **Submodular Greedy**: Select fragment maximizing marginal information gain at each cell
- **MAB with Thompson Sampling**: Explore uncertain fragments vs exploit known-good ones
- **Adaptive Scheduling**: Adjust order based on UE feedback (if available) during flight

---

## Các Vấn Đề Kỹ Thuật Cần Giải Quyết

### 1. Probabilistic Modeling
- **Challenge**: Model xác suất khớp $P(\text{target} | f_1, ..., f_k)$ với correlation giữa fragments
- **Approach**: 
  - Feature-level fusion: combine CNN embeddings from fragments
  - Bayesian graphical model: capture dependencies
  - Empirical learning: pre-train on labeled dataset

### 2. Real-time Processing
- **Challenge**: Nodes phải update $C_i$ và make decisions ngay lập tức (mili-giây)
- **Approach**:
  - Lightweight AI models: mobile-optimized CNN (e.g., MobileNet)
  - Edge inference: on-device processing
  - Async protocols: don't block waiting for cooperation

### 3. Cooperation Protocol Design
- **Challenge**: Balance latency (fast alert) vs accuracy (wait for more evidence)
- **Approach**:
  - Priority-based queuing: process high-$C_i$ nodes first
  - Time-windowed cooperation: limit wait time
  - Adaptive threshold: decrease $\theta$ as mission time runs out

### 4. Link Reliability
- **Challenge**: Fragile wireless links in urban canyon (interference, fading)
- **Approach**:
  - Rateless codes (LT, Raptor): any k-of-n fragments sufficient for decode
  - FEC (Forward Error Correction): add redundancy
  - Adaptive PHY: switch modulation/coding based on SNR

### 5. Energy Efficiency
- **Challenge**: Nodes have limited battery; cooperation consumes energy
- **Approach**:
  - Sleep scheduling: nodes idle until UAV near
  - Efficient protocols: minimize broadcast storms
  - Early termination: stop mission once detected (save UAV energy)

---

## Bài toán 1: Xây dựng đường bay UAV (Detailed)
Mục tiêu là tối ưu đường bay của UAV để bay qua khu vực khả nghi trong khoảng thời gian yêu cầu
Các yếu tố quyết định bao gồm
Vị trí các node trong khu vực khả nghi (một node có thể đại diện nhận dữ liệu cho cell tương ứng)
Mức độ ưu tiên của từng Cell (ví dụ những khu vực đường giao thông nên có ưu tiên cao hơn những khu vực ít người qua lại)
Bán kính truyền tin của node và UAV
Năng lượng của UAV
Có thể tìm hiểu qua các bài toán liên quan 
Các bài toán TSP, Prize-Collecting TSP
Traveling Repairman, Minimum Latency Problem
Vehicle Routing Problem
Greedy nearest neighbor.
Christofides.
Genetic algorithm.
Dynamic programming cho m không quá lớn
Optimal Control & Continuous Path Planning

## Bài toán 2: Lập lịch phát các mảnh dữ liệu
Mục tiêu là lập lịch hợp lý để phát các mảnh dữ liệu tiết kiệm thời gian nhất (UAV có thể có nhiều bộ phát )
Yếu tố chính:
Độ lớn của mảnh
Thời gian bay qua node hoặc cell
Mức độ ưu tiên của mảnh 
Tìm hiểu các bài liên quan:
Stochastic Scheduling Theory
Multi-Armed Bandit
Submodular Optimization
Influence Maximization Theory
Network Coding Theory
Distributed Detection Theory

## Cơ chế liên quan giao thoa giữa 2 bài toán 
Restless Multi-Armed Bandit
Stochastic Shortest Path 
Markov Decision Process 
Submodular Orienteering
Epidemic Spreading Models

## Open Questions & Considerations:

### 1. Fragment Value Modeling:
- **Vấn đề**: Cần xác định giá trị thông tin của mỗi fragment trong person recognition context
- **Hướng tiếp cận**: Submodular utility function + information-theoretic value
- **Keywords**: "submodular maximization for sensor selection", "mutual information in active learning"

### 2. Bayesian Fusion for Multi-Fragment Detection:
- **Vấn đề**: Cách kết hợp xác suất từ nhiều fragments khi có correlation?
- **Hướng tiếp cận**: Sequential Bayesian update, Chair-Varshney fusion rules
- **Keywords**: "distributed detection with data fusion", "sequential probability ratio test", "Bayesian inference for partial image recognition"

### 3. Realtime Cooperative Protocol:
- **Vấn đề**: Thiết kế protocol cân bằng latency vs accuracy trong cooperation
- **Hướng tiếp cận**: Adaptive request strategy, CL-coordinated scheduling
- **Keywords**: "cooperative spectrum sensing", "distributed hypothesis testing", "consensus-based detection"

### 4. Adaptive Threshold Policy:
- **Vấn đề**: Threshold nên cố định hay thích nghi theo time pressure và cell priority?
- **Hướng tiếp cận**: Dynamic threshold adjustment, Neyman-Pearson optimization
- **Keywords**: "adaptive threshold selection", "ROC curve optimization under constraints"

### 5. Joint Path Planning & Fragment Scheduling:
- **Vấn đề**: Hai bài toán coupling chặt chẽ, cần tích hợp giải pháp
- **Hướng tiếp cận**: Restless MAB + Stochastic Shortest Path, Submodular Orienteering
- **Keywords**: "Whittle index", "MDP with information gathering", "prize-collecting TSP with time windows"

### 6. Coding & Redundancy:
- **Vấn đề**: Nên dùng network coding/rateless codes để tăng độ tin cậy?
- **Hướng tiếp cận**: LT codes, Raptor codes cho broadcast, trade-off redundancy vs latency
- **Keywords**: "rateless codes for UAV broadcast", "network coding in WSN"

## Keyword nghiên cứu: 
UAV-assisted IoT
content distribution
mobile data mule
probabilistic matching 
Bayesian sequential detection
time to first success
distributed cooperative detection 
fragment-based recognition
stochastic scheduling
multi-armed bandit
submodular optimization
epidemic spreading
neural network edge inference
threshold-based decision

---

## Technology Overview

### Hardware Components

#### 1. UAV Platform (Data Mule)

**Vai trò**: UAV đóng vai trò "mobile data mule" bay qua khu vực khả nghi để phân phối fragments đến IoT nodes. Đây là giải pháp cho bài toán "file quá nặng, không thể truyền toàn bộ cho từng node".

**Yêu cầu kỹ thuật**:
- **Platform**: Quadcopter (DJI Matrice, Parrot) hoặc fixed-wing (wingspan) cho flight time dài hơn
  - Quadcopter: linh hoạt hơn, hover tại cell, phù hợp đô thị chật hẹp
  - Fixed-wing: bay xa hơn (1-2 giờ), thích hợp khu vực rộng nhưng ít linh hoạt
  
- **Flight Time**: 20-60 phút (trade-off: battery weight vs payload)
  - Mission time constraint $T_{max}$ phải < flight time - safety margin
  - Energy budget $E_{budget}$ ảnh hưởng trực tiếp đến path planning
  
- **Altitude**: 50-100m (độ cao trung bình của các toà nhà đô thị)
  - **Rationale**: 
    - Tránh collision với buildings, infrastructure
    - Line-of-sight (LOS) với ground nodes → path loss thấp, reliable link
    - Coverage radius tối ưu: $r_{cov} \approx \sqrt{h^2 + r_{tx}^2}$ với $h=50-100m$, $r_{tx}=100-200m$ → coverage ~150-250m radius
  - **Path Loss Model**: Free-space + urban fading
    $$PL(d) = 20\log_{10}(d) + 20\log_{10}(f) + 32.45 + X_{\sigma}$$
    với $X_{\sigma}$ = lognormal shadowing (urban environment)

---

#### **UAV Broadcast Mechanism (Chi tiết)**

**Chức năng**: UAV broadcast fragments **one-way** (unidirectional) tới tất cả nodes trong coverage radius. **KHÔNG cần acknowledgment**, **KHÔNG cần IP layer**.

**Công nghệ Broadcast**:

**A. Layer 2 Broadcast (Physical + MAC only)**

- **Why Layer 2**: 
  - Mục tiêu: phát nhanh, đơn giản, không cần routing/addressing phức tạp
  - Nodes nhận "opportunistically" - ai trong coverage nhận được, không cần register
  - Tiết kiệm overhead: không có IP header (20 bytes), TCP/UDP header (8-20 bytes)
  - Broadcast address: MAC broadcast (`FF:FF:FF:FF:FF:FF` cho 802.15.4)

- **Protocol Stack**:
  ```
  [Application: Fragment Data]
         ↓
  [MAC Layer: 802.15.4 MAC Frame]
         ↓
  [PHY Layer: O-QPSK modulation, 2.4 GHz]
         ↓
  [Antenna: Omni-directional, broadcast]
  ```

**B. 802.15.4 Broadcast Frame Format**:
```
┌──────────┬─────┬────────┬─────────────┬──────────┬─────────┬─────┐
│ Preamble │ SFD │ Length │ Frame Ctrl  │  Seq#    │ Payload │ FCS │
│  4 bytes │ 1B  │  1B    │    2B       │   1B     │ 0-127B  │ 2B  │
└──────────┴─────┴────────┴─────────────┴──────────┴─────────┴─────┘

Frame Control:
  - Frame Type: Data (001)
  - Destination Addressing: Broadcast (11)
  - Source Addressing: Short (10) or None
  
Payload Structure (Fragment Packet):
┌─────────────┬──────────────┬──────────┬───────────────┬──────────┐
│ Fragment ID │ Sequence #   │ Total #  │ Utility Hint  │ Data     │
│    2 bytes  │   2 bytes    │  2 bytes │   1 byte      │ 100-500B │
└─────────────┴──────────────┴──────────┴───────────────┴──────────┘
```

**C. Broadcast Process**:
```
1. UAV enters cell coverage area
2. For each fragment f_j in schedule S*:
   a. Prepare MAC frame: dst_addr = BROADCAST, payload = fragment data
   b. Radio TX: broadcast on channel (e.g., channel 26, 2480 MHz)
   c. Wait t_inter (inter-frame gap, ~10-50 ms)
   d. Next fragment
3. UAV leaves cell, moves to next cell
```

**D. Node Reception** (No ACK, No IP):
```
Node i:
1. Radio RX always ON (or scheduled wake-up when UAV approaches)
2. Receive broadcast frame → check FCS (Frame Check Sequence)
3. If FCS OK:
   - Extract fragment ID, sequence, data
   - Store in buffer F_i
   - Trigger Bayesian update
4. If FCS FAIL: drop packet (no retransmission request)
5. No ACK sent back to UAV
```

**E. Why No IP Layer?**

| **Aspect** | **With IP** | **Without IP (Layer 2 Broadcast)** |
|------------|-------------|------------------------------------|
| **Addressing** | IP address per node, routing table | MAC broadcast, no addressing |
| **Overhead** | 20B (IP) + 8B (UDP) = 28B | 0B (direct MAC frame) |
| **Latency** | IP lookup, routing decision | Direct broadcast, no processing |
| **Complexity** | Need DHCP/static IP, routing protocol | Plug-and-play, no config |
| **Reliability** | Can do ACK at transport layer (TCP) | No ACK, best-effort |
| **Use Case** | Point-to-point, unicast | One-to-many, broadcast |

**Conclusion**: Layer 2 broadcast đủ cho use case này vì:
- UAV không cần biết nodes nào đang nghe (anonymous broadcast)
- Không cần reliability guarantee (nodes sẽ request fragments từ neighbors nếu thiếu)
- Minimize overhead → maximize payload (fragment data)

**F. Broadcast Range & Coverage**:
- **TX Power**: $P_{tx} = 20$ dBm (100 mW) typical cho 802.15.4
- **RX Sensitivity**: $P_{rx}^{min} = -85$ dBm (CC2420)
- **Link Budget**: 
  $$P_{rx} = P_{tx} - PL(d) \geq P_{rx}^{min}$$
  $$20 - PL(d) \geq -85 \Rightarrow PL(d) \leq 105 \text{ dB}$$
- **Max Distance** (với altitude 50-100m):
  - Free-space: ~400-600m radius
  - Urban (shadowing): ~150-250m effective radius
  - **Implication**: UAV phải bay qua mỗi cell để ensure coverage

**G. Adaptive Transmission Power** (optional):
- UAV có thể điều chỉnh $P_{tx}$ dựa trên altitude, node density
- Lower altitude → lower $P_{tx}$ (tiết kiệm năng lượng)
- Higher density → higher $P_{tx}$ (ensure far nodes receive)

---

#### **Communication Links Summary**:

1. **Control Link** (BS ↔ UAV):
   - **Technology**: WiFi 802.11ac hoặc LTE
   - **Function**: Telemetry, command & control, mission updates
   - **Direction**: Bidirectional
   - **Layer**: Full TCP/IP stack
   - **Reliability**: High (ACK, retransmission)

2. **Broadcast Link** (UAV → Nodes):
   - **Technology**: 802.15.4 PHY/MAC (Layer 2 only)
   - **Function**: Fragment distribution
   - **Direction**: Unidirectional (no ACK)
   - **Layer**: Physical + MAC only (no IP)
   - **Reliability**: Best-effort (nodes cooperate to fill gaps)
  
- **Edge Compute**: NVIDIA Jetson Nano/TX2
  - **Why**: Adaptive scheduling - UAV có thể điều chỉnh fragment order dựa trên feedback realtime
  - Compute utility values, adjust path nếu nhận early alert
  - Trade-off: thêm compute → thêm trọng lượng + năng lượng

**Design Rationale**:
- UAV giải quyết vấn đề "không thể dừng lại truyền full data" bằng broadcast-on-the-fly
- Mobility cho phép coverage dynamic area, không cần cơ sở hạ tầng cố định
- Edge compute cho adaptive mission - không phụ thuộc hoàn toàn vào pre-computed plan

---

#### 2. IoT Nodes (Static Surveillance)

**Vai trò**: Nodes là "static observers" với camera + edge AI, nhận fragments từ UAV, chạy detection realtime, hợp tác trong cell để tăng confidence.

**Yêu cầu kỹ thuật**:

**Giả định quan trọng**: Nodes được **cấp nguồn AC** (AC-powered, không dùng battery) → **KHÔNG cần lo năng lượng**. Đây là giả định hợp lý cho smart-city deployment (camera giám sát đô thị thường có nguồn điện).

**Implications**:
- Radio có thể **always ON** (không cần sleep scheduling)
- Compute có thể chạy **continuous inference** (không lo battery drain)
- **Cooperation overhead không phải concern** về năng lượng
- Có thể dùng **high-power radio** (WiFi) thay vì low-power (802.15.4) nếu cần bandwidth cao

---

**a) Wireless Transceiver** (AC-powered scenario):

- **Preferred Option: WiFi 802.11ac (5 GHz)**: 
  - **High bandwidth**: ~100-300 Mbps (real-world)
  - **Moderate range**: ~50-150m (đủ cho urban cell)
  - **Why chọn WiFi khi AC-powered**:
    - Fragment size lớn (500KB - 2MB) → transmission nhanh (~10-100 ms/fragment)
    - Không lo power consumption (200-500 mW TX không là vấn đề)
    - Infrastructure đã có sẵn (WiFi APs trong đô thị)
    - Nodes có thể dual-role: surveillance + WiFi hotspot
  - **PHY/MAC**: OFDM modulation, CSMA/CA với RTS/CTS
  
- **Fallback Option: 802.15.4 (CC2420)**: 
  - **Low bandwidth**: ~250 kbps
  - **Why vẫn xem xét**: Compatibility với WSN hardware đã triển khai
  - **Challenge**: Fragment nhỏ (100-500 bytes) → cần nhiều fragments → transmission time lâu
  - **Use case**: Khi cần interoperability với existing sensor network
  
- **Not Recommended: LoRaWAN**: 
  - Data rate quá thấp (300 bps - 50 kbps) → không phù hợp realtime detection
  - Range dài (10km) không cần thiết trong urban cell (50-200m)
  - **Challenge**: Năng lượng cao (~200-500 mW TX), cần nhiều cells

**Trade-off Selection**: 
- **Chọn 802.15.4** làm baseline vì cân bằng: power, range, data rate phù hợp WSN
- WiFi cho scenario "high-bandwidth, AC-powered nodes"
- LoRa cho scenario "ultra wide-area, battery-constrained"

**b) Camera/Sensor**:
- USB camera hoặc CSI camera (Raspberry Pi compatible)
- **Vai trò**: Cung cấp "local evidence" - node kiểm tra xem đối tượng có thực sự xuất hiện trong camera view không
- **Why critical**: Bayesian update $P(\text{target} | f_j, \text{local evidence})$ - local evidence giảm false positive
- Pre-captured video (alternative): giả lập scenario "đã có footage trước đó"

**c) Compute Board**:
- **Raspberry Pi 4** (ARM, 4GB RAM): 
  - **Why**: Đủ mạnh chạy lightweight CNN (MobileNet), Linux OS cho flexibility
  - Power: ~3-5W (idle), ~6-8W (compute) → battery drains nhanh nếu continuous inference
  
- **Arduino MKR / Cortex-M4** (alternative):
  - Lower power (~100-500 mW), nhưng compute limited
  - Phù hợp cho "feature matching only", không chạy full CNN
  
**d) AI Model**:
- **Lightweight CNN**: MobileNet, SqueezeNet, EfficientNet-Lite
  - **Why**: Optimized cho mobile/edge (depthwise separable convolutions, quantization)
  - Model size: 5-20 MB (fit in RAM)
  - Inference time: 50-200 ms per frame (acceptable cho realtime)
  - Accuracy: ~70-85% (trade-off với speed)
  
- **Quantization**: 8-bit integer thay vì float32
  - Speedup ~2-4x, memory giảm 4x
  - Accuracy drop ~1-3%

**e) Power**:
- **AC-Powered** (110-240V mains → 5V/12V DC adapter)
- **Power Budget**: KHÔNG giới hạn (assume reliable mains power)
  - Idle: ~3-5W (RPi 4 + camera + radio idle)
  - Active (inference + RX): ~8-12W (full compute + radio RX)
  - Cooperation (TX): ~10-15W (CPU + WiFi TX)
  - **Total**: ~15W peak → negligible cost (~$0.01/day per node)
  
- **Backup Power** (optional): UPS (Uninterruptible Power Supply) với 1-2 giờ battery backup cho power outage

- **Why AC-powered realistic for smart-city**:
  - Surveillance cameras đã có sẵn nguồn điện từ building/streetlight
  - Không cần bảo trì battery (thay battery mỗi 6-12 tháng rất tốn kém)
  - Infrastructure cost thấp hơn (không cần solar panel, charge controller)
  
- **Impact on Design**:
  - Radio **always ON** → không miss broadcasts từ UAV
  - Continuous inference → **realtime detection**, không cần wait for fragments đủ
  - Frequent cooperation → **lower latency**, nodes trao đổi ngay khi cần

**Design Rationale**:
- Edge inference thay vì cloud → giảm latency, không cần reliable backhaul
- Local evidence fusion với fragments → tăng accuracy, giảm false alarm
- Power-aware design → mission có thể kéo dài vài giờ, nodes phải survive

---

#### **2.1. Advanced Communication Techniques**

##### **A. MIMO (Multiple-Input Multiple-Output) Enhancement**

**Motivation**: Cải thiện link budget và coverage trong urban canyon với shadowing/fading.

**UAV Transmit MIMO**:
- **Antenna Array**: UAV mang 2-4 antennas (patch antennas, 2.4 GHz) tại horizontal boom
  - Spacing: ≥ λ/2 ≈ 6 cm (wavelength tại 2.4 GHz)
  - Weight: ~100-200g (lightweight)
- **Spatial Diversity**: Phát fragments qua multiple antennas với phase offsets → Alamouti coding
  - **Benefit**: Frequency-selective fading diversity → SNR improvement ~3-6 dB
  - **Link budget**: 105 dB → 108-110 dB (extra 3-5 dB margin)
  - **Coverage**: 250 m (urban) → 300-350 m (better penetration)

**Node Receive MIMO**:
- **Dual-antenna reception**: Nodes mang 2 antennas (diversity/MIMO-capable boards)
  - Maximum Ratio Combining (MRC) → improved SNR
  - Valuable cho **intra-cell cooperation**: nodes với poor UAV links

**Trade-offs**:
- **Pro**: +3-5 dB link budget, better coverage
- **Con**: +200-300g weight (UAV), +$100-150 cost
- **Recommendation**: Enhancement cho production; baseline dùng single antenna

---

##### **B. OFDM (Orthogonal Frequency Division Multiplexing) Options**

**WiFi 802.11ac Baseline** (current choice):
- **256-point OFDM**, 20/40/80/160 MHz bandwidth
  - 48-52 data subcarriers, adaptive per-subcarrier modulation
  - **Throughput**: 100-300 Mbps realtime → fragments trong 10-100 ms
  - **Chắn ISI** trong frequency-selective fading

**802.15.4 (Non-OFDM) Baseline**:
- **DSSS** (Direct Sequence Spread Spectrum), 250 kbps fixed rate
  - Simpler, lower power (~100-200 mW TX)
  - **Challenge**: 500B fragment × k fragments = tích tụ transmission time
  - Example: 20 fragments → 320 ms (marginal nếu T_max ngắn)

**Upgrade: 802.15.4g/OFDM**:
- **Multi-band OFDM**, 250 kbps - 2 Mbps configurable
  - **4-8x throughput** improvement vs classic 802.15.4
  - Same power consumption, range 150-200m
  - Trade-off: slight complexity (OFDM sync)

**Recommendation**:
- **Baseline**: WiFi 802.11ac (OFDM implicit, proven)
- **Alternative**: 802.15.4 (simple) for legacy
- **Future**: 802.15.4g/OFDM if fragment throughput bottleneck

---

##### **C. Heterogeneous Networks (HetNet) - Multi-Technology Support**

**Problem**: Urban networks không đồng nhất (WiFi, 802.15.4, LTE nodes). UAV chỉ phát một technology → some nodes miss.

**Option A: Gateway Nodes**
- Strategic nodes là gateways với multi-radio (802.15.4 + WiFi + LTE)
- Relay fragments từ UAV tech sang khác tech
- Overhead: +30-50 ms relaying latency
- CL (Cell Leader) plays gateway role

**Option B: Sequential Broadcast**
- UAV 2 passes: Pass 1 (802.15.4) → Pass 2 (WiFi)
- Zero relaying latency, nhưng 2x mission time
- Use case: high-value targets, strict accuracy

**Option C: Multi-Channel UAV**
- Dual-radio modules (802.15.4 + WiFi) → phát song song
- Full parallelism, zero staging latency
- Con: +100-150W power, +300-500g weight, +$200-300 cost
- Feasible cho swarm UAVs

**Option D: Mesh Relay with Tech Bridge** (PREFERRED)
- Nodes tự organize (blind to specific technology)
- Fragments có metadata tags → convertible across techs
- Manifest broadcast: `[node_id, F_i, tech_mask]` every 100-500 ms
- Request/relay: AODV/OLSR tuned cho scale này
- Example: Node A (WiFi) nhận từ UAV → relay via 802.15.4 tới Node B
  - Latency: +50-100 ms (1-2 hops), acceptable

**Advantages** (Option D):
- Zero additional hardware (software only)
- Automatic heterogeneity handling
- Resilient (tech failure → reroute)
- Scales to new technologies

**Disadvantages**:
- +50-100 ms relay latency per hop
- Protocol complexity (manifest, request, relay)
- Encoding/FEC overhead

**Recommended Integration**:
1. **Primary**: Option D (Mesh Relay) default architecture
2. **Fallback**: Option A (Gateway) if overhead high
3. **Research**: Option C (Multi-UAV swarm)

**Open Issues**:
- Relay latency tail ($p_{95}$) affects $\\mathbb{E}[T_{detect}]$ distribution
- Cross-tech encoding tradeoff: accuracy vs latency
- Prefer same-tech or any-tech neighbors? (topology-dependent)

---

#### 3. Base Station (Mission Control)

**Vai trò**: BS là "brain" của hệ thống, chịu trách nhiệm mission planning (path + schedule optimization), nhận alerts, điều khiển UAV.

**Yêu cầu kỹ thuật**:
- **Hardware**: Server (cloud hoặc local workstation/laptop)
  - CPU: multi-core (Intel i7, AMD Ryzen) cho optimization algorithms
  - RAM: 8-16 GB cho path planning, simulation
  - Storage: SSD cho fast logging
  
- **Responsibilities**:
  1. **Pre-mission Planning**:
     - Fragment generation: chia target image thành $n$ fragments
     - Utility assignment: compute $U(f_j)$ dựa trên feature importance
     - Path optimization: solve TSP/Prize-Collecting variant cho UAV path $\mathcal{P}^*$
     - Schedule optimization: MAB/greedy cho fragment order $\mathcal{S}^*$
     
  2. **Mission Monitoring**:
     - UAV telemetry: position, battery, link status
     - Alert reception: nodes → CL → CG → BS
     - Real-time adjustment (optional): nếu early detection, command UAV return
     
  3. **Post-mission Analysis**:
     - Log metrics: $T_{detect}$, $P_D(t)$, $P_{FA}$, energy consumption
     - Generate reports, visualizations

**Design Rationale**:
- Centralized planning giúp tối ưu global (toàn bộ mission), không chỉ local
- BS có compute power mạnh → chạy complex optimization (genetic algorithm, dynamic programming)
- Logging + analysis → iterative improvement cho missions tiếp theo

---

#### 4. Network Infrastructure

**Cellular Structure (Hexagonal Grid)**:
- **Why Hexagonal**: Coverage đều nhất, minimize overlap/gap, mathematical elegance
- **Cell Radius** $a > r_{tx}$: Tránh interference giữa non-adjacent cells
- **3-Color TDMA**: Minimize collision - cells cùng màu không kề nhau, transmit cùng time slot
  - Red cells: slot 0 (0-50 ms)
  - Green cells: slot 1 (50-100 ms)
  - Yellow cells: slot 2 (100-150 ms)
  - Frame duration: 150 ms → repeat

**Cell Roles**:
- **Cell Leader (CL)**: Node gần tâm cell, điều phối cooperation, route alerts về BS
  - **Election**: Closest to center (minimize intra-cell path length)
  - **Responsibility**: Cooperation queue, priority scheduling, consensus validation
  
- **Cell Gateway (CG)**: Nodes ở biên cell, forward data tới cells kế cận hoặc BS
  - **Pending Links**: Pre-established paths tới CGs của neighboring cells
  - **Routing**: Alert từ CL → CG → BS (multi-hop nếu cần)
  
- **Cell Forwarding Tree (CFT)**: Tree topology trong cell, root = CL hoặc CG
  - **Purpose**: Efficient routing cho alerts và cooperation messages
  - **Construction**: Shortest path tree từ root

**Design Rationale**:
- Cell structure giúp scalability - không cần all-to-all communication
- TDMA giảm collision → reliable transmission
- Hierarchy (CL/CG) giảm overhead coordination

---

### Design Trade-offs Summary

| Component | Option A | Option B | Option C | Trade-off |
|-----------|----------|----------|----------|-------------|
| **UAV Type** | Quadcopter | Fixed-wing | - | Maneuverability vs Flight time |
| **UAV Broadcast** | 802.15.4 (simple) | WiFi 802.11ac (fast) | Dual-channel (multi-radio) | Simplicity vs Coverage |
| **MIMO Support** | Baseline (single antenna) | Dual-antenna diversity | 4x4 MIMO | Cost/Weight vs Link budget |
| **Modulation** | 802.15.4 DSSS (250 kbps) | 802.15.4g OFDM (2 Mbps) | WiFi OFDM (100-300 Mbps) | Complexity vs Throughput |
| **Multi-tech Support** | Gateway relay (A) | Sequential broadcast (B) | Mesh bridge (D) | Simplicity vs Universality |
| **Node Radio** | 802.15.4 (low-power) | WiFi 802.11ac (high-BW) | - | Power consumption vs Throughput |
| **Node Compute** | RPi 4 (powerful) | Arduino (efficient) | - | AI capability vs Power |
| **AI Model** | MobileNet (accurate) | Quantized (fast) | - | Accuracy vs Speed/Power |
| **Power** | Battery only | AC-powered | AC + backup UPS | Mission duration vs Deployment cost |
| **Cell Scheduling** | TDMA (deterministic) | CSMA-CA (random) | - | Reliability vs Simplicity |

**Key Insight**: Hệ thống thiết kế theo nguyên tắc **"Good enough"** - không cần perfection, cần balance giữa cost, power, performance cho smart-city deployment thực tế.

---

### Communication Stack

**802.15.4 PHY/MAC** (low-power WSN):
- Modulation: O-QPSK, Bandwidth: 2 MHz, Data rate: 250 kbps, Frequency: 2.4 GHz ISM band
- MAC: CSMA-CA hoặc TDMA (3-color cell scheduling)

**Fragment Format**: [Header: 4B | Fragment ID: 2B | Sequence: 1B | Payload: 100-500B]

**Coding (optional)**: Rateless codes (LT, Raptor) cho robustness, overhead ~1-10%

### Simulation Platform

**ns-3**: Discrete-event simulator với accurate PHY layer modeling (propagation, interference, fading), existing 802.15.4/energy/mobility models.

**Chi tiết implementation**: Xem [implementation.md](implementation.md) cho detailed setup, modules, scenarios, và testing.









