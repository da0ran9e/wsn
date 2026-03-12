# Related Work Search Playbook (for UAV2 Scenario5)

## 1) Mục tiêu phần Related Work

Phần related work của bài nên trả lời rõ 3 câu hỏi:

1. Người khác đã làm gì cho **UAV path planning + data dissemination** trong WSN/IoT?
2. Người khác đã làm gì cho **fragment/packet scheduling + threshold/cooperation detection**?
3. Khoảng trống nào còn thiếu để biện minh cho đóng góp của bài này (UAV2 adaptive planning + broadcast policy + mission metric)?

---

## 2) Scope tìm bài (bám IMPLEMENTATION_QUICK_GUIDE)

Từ định hướng hiện tại, ưu tiên 6 cụm chính:

- **Cluster A — UAV path planning**
  - greedy vs coverage-aware vs confidence-aware path
  - mission time minimization, suspicious-region prioritization

- **Cluster B — Fragment dissemination / scheduling**
  - priority broadcast, adaptive interval, information value, novelty-aware delivery

- **Cluster C — Cooperative detection and fusion**
  - threshold-based alert, Bayesian/weighted fusion, intra-cell cooperation overhead

- **Cluster D — Mission completion metrics**
  - single-threshold vs region-level completion, efficiency metrics for dissemination systems

- **Cluster E — Fragment utility + partial reconstruction**
  - chia dữ liệu thành fragment/chunk để xử lý độc lập
  - đánh giá giá trị từng fragment cho AI inference (partial evidence)
  - khôi phục dữ liệu từ tập con fragment (erasure/fountain/network coding)

- **Cluster F — Physical-layer and sensing realism**
  - hình học air-to-ground, contact time, altitude, tốc độ UAV
  - path loss, shadowing, fading, interference, packet loss theo fragment size
  - quality của local sensing: FoV, occlusion, view-dependent utility

Gợi ý phạm vi thời gian:
- Core review: 2019–2026
- Seminal nền tảng: cho phép <2019 nếu thực sự kinh điển

---

## 3) Nguồn tìm kiếm ưu tiên

Ưu tiên theo thứ tự hiệu quả:

1. **IEEE Xplore** (must-have cho conference networking/IoT)
2. **ACM Digital Library**
3. **Scopus / Web of Science** (lọc hệ thống, citation quality)
4. **Google Scholar** (bắt đầu rộng, sau đó lọc bằng venue)
5. **arXiv** (chỉ dùng để phát hiện trend mới, cần đối chiếu bản peer-reviewed)

Venue nên ưu tiên:
- IEEE IoT Journal
- IEEE Transactions on Mobile Computing
- IEEE Transactions on Wireless Communications
- IEEE Communications Letters / IEEE Access (tham khảo nhanh)
- INFOCOM, ICC, GLOBECOM, MASS, SECON, WCNC

---

## 4) Query library (copy-paste)

## 4.1. Cluster A — UAV Path Planning

### Query A1 (general)
("UAV" OR "drone") AND ("path planning" OR "trajectory optimization") AND ("wireless sensor network" OR "IoT") AND ("latency" OR "mission time")

### Query A2 (coverage + priority)
("UAV-assisted" OR "aerial base station") AND ("coverage optimization" OR "priority-aware") AND ("target area" OR "hotspot")

### Query A3 (adaptive / online)
("online replanning" OR "adaptive trajectory") AND ("UAV") AND ("wireless network")

## 4.2. Cluster B — Fragment Broadcast / Scheduling

### Query B1 (fragment/data dissemination)
("fragment" OR "chunk" OR "segmented data") AND ("broadcast scheduling" OR "dissemination") AND ("wireless" OR "IoT")

### Query B2 (value-aware)
("information value" OR "utility-aware" OR "priority scheduling") AND ("broadcast") AND ("UAV" OR "edge network")

### Query B3 (adaptive interval)
("adaptive transmission interval" OR "adaptive rate control") AND ("broadcast") AND ("wireless sensor")

## 4.3. Cluster C — Cooperation + Detection

### Query C1 (threshold/cooperation)
("threshold-based detection" OR "event detection") AND ("cooperative" OR "collaborative") AND ("WSN" OR "IoT")

### Query C2 (Bayesian fusion)
("Bayesian fusion" OR "distributed confidence fusion") AND ("edge" OR "sensor network") AND ("target detection" OR "person detection")

### Query C3 (overhead trade-off)
("cooperation overhead" OR "communication overhead") AND ("detection latency" OR "false alarm") AND ("sensor networks")

## 4.4. Cluster D — Mission Completion / Metrics

### Query D1 (mission completion criteria)
("mission completion" OR "stopping criterion") AND ("UAV" OR "mobile sink") AND ("wireless sensor network")

### Query D2 (efficiency metrics)
("broadcast efficiency" OR "useful delivery ratio" OR "redundancy") AND ("UAV-assisted")

### Query D3 (region-level objectives)
("region coverage threshold" OR "region-level detection") AND ("distributed sensing")

## 4.5. Cluster E — Fragment Utility / Partial Recovery

### Query E1 (fragment utility for AI)
("image fragment" OR "patch" OR "tile") AND ("utility" OR "informativeness" OR "value") AND ("inference" OR "recognition") AND ("edge" OR "distributed")

### Query E2 (independent processing of fragments)
("partial image" OR "cropped region" OR "incomplete data") AND ("independent processing" OR "early inference" OR "partial evidence") AND ("deep learning" OR "computer vision")

### Query E3 (reconstruction from subset)
("data reconstruction" OR "file recovery") AND ("subset of fragments" OR "partial packets" OR "missing chunks") AND ("erasure coding" OR "fountain codes" OR "network coding")

### Query E4 (progressive / layered coding)
("progressive transmission" OR "scalable coding" OR "layered coding") AND (image OR video) AND ("partial decoding" OR "graceful degradation")

### Query E5 (importance-aware transmission)
("importance-aware" OR "content-aware" OR "unequal error protection") AND ("packet" OR "fragment") AND (image OR video OR sensing)

### Query E6 (information-theoretic value)
("value of information" OR "information gain" OR submodular) AND ("sensor" OR "edge") AND ("selection" OR "scheduling")

### Nguồn nên ưu tiên riêng cho Cluster E
- IEEE TMM, TIP, TCSVT (ảnh/video + coding)
- IEEE IoT Journal, TMC, TWC (edge/distributed/network)
- ACM MM, CVPR/ICCV/ECCV workshop papers (để bắt trend, sau đó đối chiếu bản journal)

## 4.6. Cluster F — Physical Layer / Sensing Realism

### Query F1 (air-to-ground propagation)
("UAV" OR drone) AND ("air-to-ground" OR propagation OR channel modeling) AND (IoT OR WSN OR urban)

### Query F2 (contact time / geometry)
("UAV contact time" OR "trajectory contact duration" OR "dwell time") AND (IoT OR sensor network OR data collection)

### Query F3 (packet loss vs size / reliability)
("packet error rate" OR "packet loss") AND (fragment OR packet size) AND (wireless OR UAV OR IoT)

### Query F4 (urban LoS / shadowing)
(UAV OR drone) AND (LoS OR NLoS OR shadowing OR fading) AND (urban OR smart city) AND (communication OR IoT)

### Query F5 (sensing geometry / view quality)
(camera OR vision) AND (field of view OR occlusion OR view quality) AND (distributed inference OR edge inference OR surveillance)

### Query F6 (physical-aware dissemination)
("value of information" OR utility OR confidence) AND (wireless delivery OR dissemination) AND (channel OR SNR OR contact time)

### Query F7 (fragment size vs packet delivery)
("fragment size" OR "packet size") AND (PER OR BER OR reliability OR "delivery probability") AND (wireless OR UAV OR IoT)

### Query F8 (interference during cooperation)
(interference OR contention OR collision OR "shared channel") AND (cooperation OR relay OR manifest exchange) AND (WSN OR IoT OR UAV)

### Query F9 (contact time + data delivery)
("contact time" OR "contact duration" OR "opportunistic contact") AND ("data delivery" OR dissemination OR collection) AND (UAV OR mobile sink)

### Query F10 (altitude / speed trade-off)
(altitude OR speed OR dwell time) AND (UAV) AND (coverage OR throughput OR latency OR packet loss)

### Query F11 (urban blockage / building effects)
(UAV OR drone) AND (urban canyon OR building blockage OR obstruction) AND (channel OR coverage OR packet delivery)

### Query F12 (FoV / occlusion for distributed sensing)
("field of view" OR FoV OR occlusion OR visibility) AND (camera OR surveillance) AND (distributed OR edge) AND (inference OR detection)

### Query F13 (air-to-ground plus sensing)
(UAV OR drone) AND ("air-to-ground") AND (camera OR vision) AND (occlusion OR geometry OR visibility)

### Query F14 (useful delivery under channel uncertainty)
("useful delivery" OR "effective utility" OR "expected utility") AND (channel uncertainty OR fading OR shadowing OR SNR) AND (wireless OR IoT)

### Query F15 (coding/reliability under physical loss)
(FEC OR "forward error correction" OR rateless OR fountain OR erasure) AND (packet loss OR fading OR unreliable channel) AND (UAV OR wireless sensing)

### Query F16 (energy realism for UAV missions)
("propulsion energy" OR "flight energy" OR hover) AND UAV AND (communication OR sensing OR mission time)

### Query F17 (ns-3 oriented search)
(ns-3) AND (UAV OR drone) AND (propagation OR fading OR shadowing OR contact time OR packet loss)

### Query F18 (hybrid system-level physical realism)
(UAV OR drone) AND (trajectory OR dissemination OR data collection) AND (LoS OR SNR OR packet error OR contact time) AND (IoT OR WSN)

### Search strategy for Cluster F
- Bắt đầu với `F18`, `F1`, `F4`, `F10` để lấy paper hệ thống có yếu tố vật lý.
- Dùng `F7`, `F8`, `F15` khi cần paper nối giữa fragment delivery và reliability.
- Dùng `F12`, `F13` khi cần biện minh cho view-dependent utility hoặc local sensing quality.
- Dùng `F17` để tìm paper có khả năng tái hiện được trong ns-3.

### Nguồn nên ưu tiên riêng cho Cluster F
- IEEE Transactions on Wireless Communications
- IEEE Transactions on Mobile Computing
- IEEE IoT Journal
- IEEE Transactions on Vehicular Technology
- IEEE Systems Journal
- INFOCOM, ICC, WCNC, GLOBECOM
- ACM MSWiM, MobiHoc, SenSys (nếu query rộng hơn sang mobile/sensor systems)

---

## 5) Cách tìm hiệu quả (workflow 90 phút / phiên)

## Phase 1 — Wide scan (20 phút)
- Chạy Query A1/B1/C1/D1/E1/F1 trên IEEE + Scholar
- Mỗi query lấy nhanh 20–30 kết quả đầu
- Loại ngay các bài lệch domain (robotics-only, CV-only không có network contribution)

## Phase 2 — Focused filtering (30 phút)
- Lọc theo: năm (2019+), venue tốt, có mô hình/thuật toán/đánh giá định lượng
- Giữ lại ~20 bài “candidate strong”

## Phase 3 — Snowballing (25 phút)
- Backward snowballing: đọc references của 5 bài tốt nhất
- Forward snowballing: xem “cited by” để bắt bài mới
- Mỗi vòng thêm 5–10 bài chất lượng cao

## Phase 4 — Gap mapping (15 phút)
- Điền bảng extraction (mẫu ở mục 7)
- Gắn tag theo RQ1/RQ2/RQ3/RQ4
- Chốt 10–18 bài cốt lõi cho related work chính

---

## 6) Tiêu chí chọn / loại bài

## Inclusion criteria
- Có ít nhất 1 yếu tố liên quan trực tiếp tới A/B/C/D/E/F
- Có mô tả thuật toán rõ ràng
- Có thực nghiệm định lượng (simulation/testbed/real trace)
- Có metric gần với bài của bạn: latency, success rate, overhead, energy, coverage
- Với Cluster E: có đánh giá fragment-level (utility/importance/recovery) hoặc partial-decoding behavior
- Với Cluster F: có mô hình hóa rõ geometry/channel/sensing effect và chỉ ra ảnh hưởng tới system metrics

## Exclusion criteria
- Bài chỉ nói kiến trúc ý tưởng, không có evaluation
- Bài thuần CV model accuracy nhưng không có network/system layer
- Bài chỉ tối ưu 1 điểm quá xa scenario5 (ví dụ route planner cho delivery logistics)
- Bài chỉ nói nén/codec thuần media mà không có khía cạnh fragment selection/recovery/inference
- Bài chỉ nói PHY rất chi tiết nhưng không có liên hệ tới dissemination/detection/performance ở mức hệ thống

---

## 7) Bảng trích xuất bằng chứng (dán vào note hoặc Excel)

| ID | Citation | Cluster | Problem | Method | Metrics | Key Result | Limitation | Relevance (High/Med/Low) | Maps to RQ |
|---|---|---|---|---|---|---|---|---|---|
| P01 | ... | A | ... | ... | ... | ... | ... | High | RQ1 |
| P02 | ... | B | ... | ... | ... | ... | ... | Med | RQ2 |

Gợi ý field bắt buộc:
- Baseline mà paper so sánh với gì
- Có xử lý adaptive/feedback hay không
- Có định nghĩa stopping criterion / mission completion không
- Có mô tả fragment utility / importance metric không
- Có ngưỡng recovery tối thiểu (k-of-n, decoding success curve) không
- Có mô hình air-to-ground/contact-time/LoS-NLoS/sensing geometry không
- Physical effect đó có làm thay đổi ranking của policy hoặc metric chính không

---

## 8) Khung viết Related Work cho paper của bạn

## 8.1. Cấu trúc 6 đoạn

1. **UAV trajectory and coverage-aware dissemination** (Cluster A)
2. **Fragment/priority broadcast in constrained wireless networks** (Cluster B)
3. **Fragment utility, partial inference, and data recovery from subsets** (Cluster E)
4. **Physical-layer and sensing realism in UAV-assisted delivery** (Cluster F)
5. **Cooperative detection and confidence fusion in WSN/edge systems** (Cluster C)
6. **Research gap & our positioning** (Cluster D + synthesis)

## 8.2. Mẫu câu gap (có thể tái dùng)

- Existing studies optimize trajectory or dissemination separately; few jointly model path planning and fragment-value-aware broadcast under strict mission-time constraints.
- Many UAV-assisted studies evaluate high-level delivery or coverage gains, but rely on simplified physical assumptions that ignore contact time, fragment-size dependent loss, and view-dependent utility.
- Prior cooperative detection works often use fixed threshold stopping rules, while region-level mission completion and dissemination efficiency are underexplored.
- Feedback-driven adaptation between ground confidence state and UAV broadcast control remains limited in current WSN-oriented designs.

---

## 9) Mapping trực tiếp từ related work sang đóng góp bài của bạn

Sau khi đọc đủ bài, bạn nên chứng minh 3 đóng góp có bằng chứng gap:

1. **Contribution C1 (Planner):** coverage/confidence-aware UAV2 planning cho suspicious region.
2. **Contribution C2 (Scheduler):** priority + adaptive broadcast policy dựa trên fragment utility/deficit.
3. **Contribution C3 (Recovery-aware design):** mô hình hóa partial evidence + khả năng khôi phục từ subset fragment.
4. **Contribution C4 (Physical-aware delivery):** tích hợp geometry/channel/sensing effects vào expected useful fragment delivery.
5. **Contribution C5 (Metric):** mission completion criterion đa điều kiện + efficiency metrics beyond completion time.

Mỗi contribution cần gắn tối thiểu 2–3 related papers để chỉ ra “khác gì” và “vì sao cần thiết”.

---

## 10) Checklist trước khi chốt Related Work section

- [ ] Có ít nhất 10 bài mạnh (core) và 5–10 bài phụ (context)
- [ ] Mỗi cụm A/B/C/D/E/F có ít nhất 2–3 bài đại diện
- [ ] Có bảng so sánh rõ baseline vs proposed direction
- [ ] Không overclaim novelty (chỉ claim ở đúng phần mình cải tiến)
- [ ] Gap statement khớp hoàn toàn với phần Method + Evaluation của paper

---

## 11) Kế hoạch thực thi ngắn (3 ngày)

## Day 1
- Hoàn thành scan + filter Cluster A, B, E, F
- Chốt 8–12 bài đầu tiên

## Day 2
- Hoàn thành scan + filter Cluster C, D
- Điền bảng extraction, gắn RQ tags

## Day 3
- Viết Related Work draft 1.0 (6 đoạn)
- Viết Gap paragraph + Contribution mapping
- Soát consistency với Method và Evaluation plan

---

## 12) Research Extension for Physical Factors

## 12.1. Mục tiêu học thuật

Cluster F không chỉ nhằm “làm mô phỏng thật hơn”, mà nhằm trả lời câu hỏi nghiên cứu:

- khi thêm yếu tố vật lý, thứ hạng của các planner/broadcast policy có thay đổi không?
- fragment utility có còn giữ nguyên khi channel và sensing geometry thay đổi không?
- cooperation có thực sự giúp nếu contention và contact-time budget được tính đúng?

## 12.2. Physical factors nên đọc tài liệu theo nhóm

### F-A. Geometry and contact time
- altitude
- 3D distance
- elevation angle
- dwell time / contact duration

### F-B. Wireless delivery realism
- path loss
- LoS/NLoS
- shadowing and fading
- packet loss as a function of fragment size and SNR

### F-C. Local sensing realism
- camera field-of-view
- occlusion
- distance to target
- view-dependent fragment usefulness

### F-D. System coupling
- cooperation contention
- half-duplex cost
- propulsion-vs-communication trade-off

## 12.3. Research questions for Cluster F

### RQ-F1
How does contact-time-aware modeling change the effectiveness of UAV path planning and fragment scheduling?

### RQ-F2
Does fragment utility remain stable under realistic channel loss, or should utility be redefined as semantic value times delivery probability?

### RQ-F3
When interference and cooperation overhead are modeled explicitly, does realtime cooperation still dominate delayed cooperation?

### RQ-F4
How much does local sensing geometry alter node-level confidence gain from the same fragment?

## 12.4. Working hypotheses

- **H-F1**: Contact-time-aware modeling changes the relative ranking of greedy, coverage-aware, and confidence-aware UAV planners.
- **H-F2**: A fragment with high semantic value but low delivery probability is not globally optimal; effective utility must combine semantic and physical delivery terms.
- **H-F3**: Cooperation gains shrink when shared-channel contention and relay latency are modeled explicitly.
- **H-F4**: View-dependent sensing quality makes fragment utility node-specific rather than globally fixed.

## 12.5. Suggested theoretical framing

Instead of treating fragment utility as purely semantic, define:

$$
U_{effective}(f_j, i) = U_{semantic}(f_j) \cdot P_{deliver}(f_j, i) \cdot Q_{view}(i)
$$

Where:
- $U_{semantic}(f_j)$ is content importance
- $P_{deliver}(f_j, i)$ depends on channel/contact time/fragment size
- $Q_{view}(i)$ captures local sensing quality of node $i$

This framing creates a direct bridge between AI, communication, and physical realism.

## 12.6. What this adds to the paper's novelty

If used carefully, Cluster F supports a stronger claim:
- the paper does not only optimize fragment dissemination logically
- it optimizes **expected useful fragment delivery** under geometry, channel, and sensing constraints

That claim is substantially stronger than a pure coverage or pure scheduling paper, but still realistic enough for a conference paper if the physical model stays moderate.
