# WSN Simulation Technical Report (CC2420 + UAV)

## 1) Mục tiêu hệ thống và phạm vi

Hệ thống mô phỏng bài toán WSN có UAV hỗ trợ thu thập/phân phối thông tin trong vùng nghi vấn:

- Ground nodes bố trí theo lưới 2D.
- Base Station (BS) điều phối, chọn suspicious region.
- UAV1 là baseline (đi từng điểm nghi vấn theo nearest-neighbor).
- UAV2 là phương án tối ưu hóa broadcast fragment (coverage-aware + cost-aware).

Điểm nhấn kỹ thuật của phiên bản hiện tại:

1. Hoàn thiện khung radio CC2420 trong ns-3 (PHY/MAC/NetDevice).
2. Hoàn thiện quy trình kịch bản nhiều phase (setup, routing, broadcast, cooperation).
3. Thêm mô hình vật lý truyền tin (path loss, shadowing, fast fading, contact-window, BER/PER).

---

## 2) Tiến độ theo giai đoạn

## Giai đoạn 1 — Triển khai Radio CC2420

Thành phần đã có:

- PHY state machine cơ bản (`SLEEP`, `RX_ON`, `TX_ON`, ...).
- MAC khung truyền/nhận và callback lên tầng trên.
- `Cc2420NetDevice` để tích hợp vào node ns-3.
- Header/trailer gói tin.
- Khung energy model (chưa mô hình tiêu hao chi tiết theo trạng thái).

Kết quả:

- Có thể chạy broadcast test nhiều node.
- Xác thực đường truyền callback end-to-end giữa các tầng.

## Giai đoạn 2 — Khung bài toán Scenario 4

Đã triển khai pipeline:

- Global setup: cell mapping, neighbor discovery, cell leader election, intra-cell routing.
- BS chọn vùng nghi vấn + sinh fragment từ file gốc.
- UAV1/UAV2 thực hiện nhiệm vụ theo chiến lược khác nhau.
- Ground nodes tích lũy confidence và kích hoạt chia sẻ fragment theo ngưỡng.

### 2.1) Mô hình confidence của fragment

Gọi $S$ là kích thước file gốc, $s_f$ là kích thước fragment $f$, và $C_\text{master}$ là confidence tổng của file gốc.

Confidence của từng fragment:

$$
c_f = \frac{s_f}{S} \cdot C_\text{master}
$$

Confidence tích lũy của node $i$ tại thời điểm $t$:

$$
C_i(t) = \sum_{f \in \mathcal{F}_i(t)} c_f
$$

Điều kiện trigger cooperation (đơn giản hóa):

$$
C_i(t) \ge \tau_\text{coop}
$$

Điều kiện alert:

$$
C_i(t) \ge \tau_\text{alert}
$$

Trong cấu hình hiện tại: $\tau_\text{coop}=0.35$, $\tau_\text{alert}=0.75$.

### 2.2) Điều kiện hoàn thành nhiệm vụ

- UAV1: hoàn thành khi đã đi qua toàn bộ waypoint baseline và giao dữ liệu theo kế hoạch.
- UAV2: thành công khi node tại suspicious point đạt ngưỡng confidence yêu cầu.

### 2.3) Kết quả benchmark giai đoạn 2 (100 rounds)

- `earlierRate = 0.947`
- `uav1MeanCompletionTime = 97.8543 s`
- `uav2MeanCompletionTime = 17.4891 s`
- `averageEarlierTime = 75.3947 s`

=> UAV2 tốt hơn baseline ở phần lớn seed, nhưng khi đó kênh truyền vẫn gần-ideal (chưa đủ yếu tố vật lý).

## Giai đoạn 3 — Heuristic bay UAV2 + PHY thực tế hơn

Hai nâng cấp lớn:

1. Nâng thuật toán lập kế hoạch UAV2 sang Greedy Max-Coverage with Cost.
2. Thêm mô hình truyền vô tuyến có shadowing/fading/contact-window/BER-PER.

---

## 3) Thuật toán UAV2: Greedy Max-Coverage with Cost

## 3.1) Tập candidate waypoint

Cho tập điểm nghi vấn $\mathcal{P}$, tập candidate $\mathcal{C}$ gồm:

- Toàn bộ điểm trong $\mathcal{P}$.
- (Tùy chọn) thêm centroid từ k-means để tạo waypoint “ở giữa cụm”.

K-means cập nhật centroid vòng lặp:

$$
\mu_j \leftarrow \frac{1}{|\mathcal{S}_j|} \sum_{p \in \mathcal{S}_j} p
$$

với $\mathcal{S}_j$ là tập điểm gán vào cụm $j$.

## 3.2) Coverage set

Với mỗi candidate $c \in \mathcal{C}$:

$$
CoverageSet(c)=\{p \in \mathcal{P} \mid d(c,p) \le R_b\}
$$

Trong đó $R_b$ là broadcast radius.

## 3.3) Hàm chọn greedy

Tại vị trí hiện tại $x_t$, chọn candidate tối ưu:

$$
c^* = \arg\max_{c \in \mathcal{C}} \frac{\text{gain}(c)}{\text{cost}(x_t,c)^\alpha + \varepsilon}
$$

Trong đó:

$$
gain(c)=\left|CoverageSet(c)\setminus Covered\right|
$$

Cost theo thời gian bay:

$$
cost(x_t,c)=\frac{d(x_t,c)}{v_{uav}}
$$

Tie-break: score cao hơn > gain cao hơn > quãng đường ngắn hơn.

Ý nghĩa tham số:

- $\alpha=0$: ưu tiên phủ tối đa, gần như bỏ qua cost.
- $\alpha>1$: phạt mạnh waypoint xa.
- $\varepsilon$ nhỏ để tránh chia 0 khi candidate sát vị trí hiện tại.

## 3.4) Chỉ số đánh giá

- Coverage ratio: $|\text{Covered}|/|\mathcal{P}|$.
- Tổng quãng đường: $\sum_k d(w_k,w_{k+1})$.
- Thời gian hoàn thành: gồm thời gian bay + hover time.

---

## 4) Mô hình truyền vô tuyến CC2420 (giai đoạn 3)

## 4.1) Log-distance path loss theo profile

Với khoảng cách hiệu dụng $d=\max(d_0, d_{3D})$:

$$
PL(d)=PL_0 + 10n\log_{10}\left(\frac{d}{d_0}\right) + X_\sigma + X_f + L_h
$$

Trong đó:

- $PL_0$: suy hao tham chiếu tại $d_0$.
- $n$: path loss exponent theo profile (ground, LoS, mixed, NLoS).
- $X_\sigma \sim \mathcal{N}(0,\sigma^2)$: log-normal shadowing.
- $X_f$: fast fading (dB-domain Gaussian approximation).
- $L_h$: heading penalty (tùy chọn).

Profile được chọn theo elevation angle $\theta$ hoặc theo pLoS ngẫu nhiên.

## 4.2) Stochastic LoS (tùy chọn)

Khi bật `EnableStochasticLos`, xác suất LoS theo logistic:

$$
p_\text{LoS}(\theta)=\frac{1}{1+a\exp[-b(\theta-a)]}
$$

Mặc định: $a=9.61$, $b=0.16$.

## 4.3) Fast fading và K-factor

Mô hình sử dụng nhiễu Gaussian trong miền dB, với độ lệch chuẩn phụ thuộc K-factor:

$$
\sigma_f = \frac{5.57}{\sqrt{1+K}} \; (\text{dB})
$$

Ví dụ:

- $K=0$ (Rayleigh): $\sigma_f\approx 5.57$ dB.
- $K=6$: $\sigma_f\approx 2.10$ dB.
- $K=15$: $\sigma_f\approx 1.39$ dB.

## 4.4) Heading penalty (tùy chọn)

Cho $\psi$ là góc giữa vector vận tốc TX và hướng LOS tới RX:

$$
m=\frac{1-\cos\psi}{2}, \quad L_h = m\cdot L_{h,\max}
$$

Khi TX bay ngược hướng RX ($\psi \to \pi$), penalty gần cực đại.

## 4.5) Contact-window model

Mục tiêu: kiểm tra gói có đủ “thời gian tiếp xúc khả dụng” hay không.

Với gói kích thước $L$ bytes, data rate $R$ (bps):

$$
T_\text{air} = \frac{8L}{R}, \quad T_\text{req}=T_\text{air}+T_g
$$

Tại mỗi mẫu thời gian $t\in[0,T_\text{req}]$, dự đoán vị trí TX/RX:

$$
\mathbf{p}_{tx}(t)=\mathbf{p}_{tx}(0)+\mathbf{v}_{tx}t,\qquad
\mathbf{p}_{rx}(t)=\mathbf{p}_{rx}(0)+\mathbf{v}_{rx}t
$$

Gói hợp lệ khi:

$$
P_{rx}(t) \ge P_{\min}, \; \forall t \in [0,T_\text{req}]
$$

với:

$$
P_{\min}=P_{sens}+M_{req}+M_{vel}
$$

Velocity-aware margin (tùy chọn):

$$
f_D \approx \frac{v_{rel}}{c}f_c,\quad T_c\approx\frac{0.423}{f_D},\quad
M_{vel}=\min\left(M_{cap},\max\left(0,\frac{T_{air}}{T_c}-1\right)\cdot s\right)
$$

Ghi chú triển khai hiện tại: `EnableVelocityAwareMargin` đang để mặc định `false` để tránh quá bảo thủ với payload lớn.

## 4.6) BER/PER từ SNR

SNR theo dB:

$$
SNR_{dB}=P_{rx,dBm}-N_{floor,dBm}
$$

Với DSSS processing gain $G_p$:

$$
\gamma_b = \text{SNR}_{lin}\cdot G_{p,lin}
$$

Mô hình BER (O-QPSK approximation):

$$
BER = \frac{1}{2}\,\operatorname{erfc}(\sqrt{\gamma_b})
$$

PER cho gói $L$ bytes:

$$
PER = 1-(1-BER)^{8L}
$$

Quyết định mất gói theo Bernoulli trial:

$$
u\sim U(0,1),\; \text{drop nếu } u<PER
$$

---

## 5) Tham số mặc định quan trọng (snapshot hiện tại)

- CC2420: $f_c=2.4$ GHz, `TxPower=0 dBm`, `RxSensitivity=-95 dBm`.
- Path loss reference: $d_0=1$ m, $PL_0=40.05$ dB.
- Exponent: ground=3.2, LoS=2.0, mixed=2.5, NLoS=3.0.
- Shadowing sigma: ground=7, LoS=4, mixed=6, NLoS=8 (dB).
- Elevation threshold: LoS 40°, mixed 20°.
- Contact-window: `DataRate=250 kbps`, `Guard=2 ms`, `SampleStep=1 ms`.

---

## 6) Kết quả kiểm thử gần nhất (sau nâng cấp UAV2)

Đo với nhiều seed và nhiều cấu hình (`example4`), UAV2 (`GreedyMaxCoverageCost`) thắng UAV1 ở hầu hết trường hợp thử nghiệm, một số cấu hình đạt 100% seed thắng.

Ví dụ cấu hình mạnh cho UAV2:

- `gridSize=10`, `gridSpacing=30`, `suspiciousPercent=0.20`.

Quan sát thực nghiệm: heuristic coverage+cost giảm rõ tổng quãng đường và thời gian hoàn thành so với baseline đi tuần tự waypoint.

---

## 7) Điểm còn hạn chế / hướng mở rộng

1. Chưa có full 3D antenna pattern (hiện dùng heading penalty đơn giản).
2. Fast fading hiện là mẫu độc lập theo packet (chưa time-correlated fading).
3. Contact-window và BER/PER có thể cần calibration thêm theo đo đạc thực tế phần cứng.
4. Chưa mô hình tiêu hao năng lượng chi tiết theo state machine của CC2420.

---

## 8) Kết luận

Sau 3 giai đoạn, dự án đã chuyển từ khung WSN logic sang mô phỏng có ý nghĩa vật lý: vừa có cơ chế ra quyết định ở tầng ứng dụng/routing (confidence-based cooperation), vừa có cơ chế suy hao/lỗi thực tế ở PHY.

Nâng cấp UAV2 bằng Greedy Max-Coverage with Cost là bước hiệu quả nhất ở lớp điều phối đường bay, đồng thời các mô hình PHY mới giúp kết quả đánh giá gần thực tế hơn so với kênh ideal.
