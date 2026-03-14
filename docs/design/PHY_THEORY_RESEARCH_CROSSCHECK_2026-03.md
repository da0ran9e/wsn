# PHY Theory → Implementation → Research Cross-check (2026-03)

Mục tiêu tài liệu này:
1) Viết lại danh sách **lý thuyết liên quan** dựa trên những gì đã triển khai trong code.
2) Đối chiếu các lý thuyết đó với **tài liệu nghiên cứu thật** (measurement/model papers).
3) Chỉ ra phần nào đang lệch, phần nào còn thiếu.

> Ghi chú: tài liệu này **không sửa** nội dung trích dẫn trong `docs/paper/refs/...`; chỉ tạo bản đối chiếu kỹ thuật phục vụ triển khai.

---

## A. Danh sách lý thuyết liên quan (theo đúng phần đã triển khai)

## A1) Large-scale propagation: log-distance path loss theo profile hình học

Lý thuyết:
- Path loss dạng log-distance
- Phân profile theo điều kiện hình học (ground-ground, LoS/mixed/NLoS)

Trong code:
- `Cc2420SpectrumPropagationLossModel`
- Phân profile theo altitude threshold + elevation threshold
- Exponent theo profile:
  - ground-ground: 3.2
  - LoS: 2.0
  - mixed: 2.5
  - NLoS: 3.0

Mức triển khai: **Đã có**

---

## A2) Slow fading: log-normal shadowing

Lý thuyết:
- Shadowing Gaussian theo dB, sigma phụ thuộc môi trường/profile

Trong code:
- `EnableShadowing`
- Sigma theo profile:
  - ground-ground: 7 dB
  - LoS: 4 dB
  - mixed: 6 dB
  - NLoS: 8 dB

Mức triển khai: **Đã có**

---

## A3) Small-scale fading: Ricean/Rayleigh (xấp xỉ)

Lý thuyết:
- LoS thường Ricean (K > 0), NLoS thường Rayleigh (K = 0)
- Fading ảnh hưởng xác suất lỗi gói ngay cả khi RSSI vượt sensitivity

Trong code:
- `EnableFastFading`
- `KFactorLoS`, `KFactorMixed`, `KFactorNLoS`, `KFactorGround`
- Xấp xỉ dB Gaussian theo K:
  - $\sigma_{fast} \approx 5.57/\sqrt{1+K}$
- Lấy mẫu per-packet trong propagation model

Mức triển khai: **Đã có (xấp xỉ nhẹ, chưa time-correlated)**

---

## A4) BER/PER theo SNR và kích thước packet

Lý thuyết:
- BER từ SNR (O-QPSK DSSS), PER từ BER và số bit gói:
  - $PER = 1 - (1-BER)^{8L}$

Trong code:
- `Cc2420ErrorModel`:
  - `GetBer(snrDb)` dùng erfc O-QPSK + processing gain
  - `GetPer(ber, packetSizeBytes)`
  - `PacketIsLost(per)` stochastic Bernoulli
- Được gọi trong `Cc2420Phy::EvaluateReceptionFrom()`

Mức triển khai: **Đã có**

---

## A5) Contact-time viability / mobility-aware packet viability

Lý thuyết:
- Link “đủ điều kiện nhận” không chỉ theo vị trí tức thời, mà theo thời gian tiếp xúc trong lúc truyền gói

Trong code:
- `Cc2420ContactWindowModel::HasContactForPacket()`
- Dự báo vị trí TX/RX theo vận tốc trong `[0, airtime + guard]`
- Sample theo `SampleStepSeconds`
- Kiểm tra ngưỡng RSSI tại từng mẫu

Mức triển khai: **Đã có**

---

## A6) Doppler / coherence-time

Lý thuyết:
- Doppler và coherence-time quyết định tốc độ biến thiên kênh

Trong code:
- **Chưa có Doppler tường minh** (không có symbol-level Doppler process)
- Đã đưa coherence-time vào contact gating dưới dạng margin penalty xấp xỉ:
  - $f_{D,max} \approx (v_{rel}/c)\,f_c$
  - $T_c \approx 0.423/f_{D,max}$
  - nếu $t_{airtime}/T_c > 1$ thì cộng penalty margin (có cap)

Mức triển khai: **Một phần (gián tiếp), chưa explicit time-correlated Doppler model**

---

## B. Đối chiếu với tài liệu nghiên cứu thật

## B1) Nguồn nghiên cứu đã kiểm tra

1. Khawaja et al., *A Survey of Air-to-Ground Propagation Channel Modeling for UAVs*, arXiv:1801.01656, 2018, DOI: 10.48550/arXiv.1801.01656  
   URL: https://arxiv.org/abs/1801.01656

2. Cai et al., *An Empirical Air-to-Ground Channel Model Based on Passive Measurements in LTE*, arXiv:1901.07930 / IEEE TVT, DOI: 10.1109/TVT.2018.2886961  
   URL: https://arxiv.org/abs/1901.07930

3. Duggal et al., *Line-of-Sight Probability for Outdoor-to-Indoor UAV-Assisted Emergency Networks*, arXiv:2302.14709 / IEEE ICC 2023, DOI: 10.1109/ICC45041.2023.10279196  
   URL: https://arxiv.org/abs/2302.14709

4. Shafafi et al., *UAV-Assisted Wireless Communications: An Experimental Analysis of A2G and G2A Channels*, arXiv:2303.16986 / LNCS chapter, DOI: 10.1007/978-3-031-57523-5_19  
   URL: https://arxiv.org/abs/2303.16986

5. Lyu et al., *Measurement-based fading characteristics analysis and modeling of UAV to vehicles channel*, Vehicular Communications 45 (2023) 100707, DOI: 10.1016/j.vehcom.2023.100707  
   DOI URL: https://doi.org/10.1016/j.vehcom.2023.100707

---

## B2) Mapping theory ↔ implementation ↔ evidence từ paper

| Lý thuyết | Trạng thái triển khai | Đối chiếu paper |
|---|---|---|
| Log-distance path loss + profile theo môi trường | Đã có | Phù hợp với survey/measurement hướng large-scale (Khawaja 2018; Cai 2019) |
| Log-normal shadowing | Đã có | Phù hợp với hướng stochastic large-scale trong các campaign (Khawaja 2018; Cai 2019) |
| Fast fading (Ricean/Rayleigh) | Đã có (xấp xỉ per-packet) | Cùng tinh thần measurement về fading variability, nhưng chưa time-correlation đầy đủ (Cai 2019; Lyu 2023) |
| PER theo SNR và payload | Đã có | Phù hợp với nhu cầu đo/đánh giá reliability theo packet-level; hiện dùng analytic model thay vì curve-fit đo đạc |
| Contact-time viability theo chuyển động | Đã có | Là triển khai kỹ thuật rất hữu ích cho UAV mobility; không mâu thuẫn với literature A2G |
| LoS probability model $p_{LoS}(h,d)$ | Đã có mode tùy chọn (stochastic theo elevation) + mode deterministic cũ | Bám theo hướng LoS-probability của literature; dùng logistic approximation để giữ nhẹ (Duggal 2023; Khawaja 2018) |
| Antenna heading/pattern effect | Đã có bản nhẹ (optional heading-mismatch penalty); chưa có full 3D pattern | Phù hợp hướng thực nghiệm về ảnh hưởng heading, ở mức proxy đơn giản (Shafafi 2023) |
| Doppler explicit/time-correlated fading | Chưa có | Measurement papers có phân tích Doppler spread/time variation (Cai 2019; Lyu 2023) |

---

## C. Các điểm lệch chính giữa lý thuyết và phần đã triển khai

1) **LoS selection**
- Lý thuyết/measurement: thường stochastic hoặc tham số hóa theo geometry
- Hiện tại: có cả hai mode
  - deterministic threshold (`ElevationLosThreshold`, `ElevationMixedThreshold`) — mặc định
  - stochastic pLoS theo logistic elevation model (`EnableStochasticLos`, `LosProbabilityA`, `LosProbabilityB`)

2) **Fading dynamics theo thời gian**
- Lý thuyết/measurement: có non-stationary/time-correlation, Doppler spread
- Hiện tại: fast fading lấy mẫu per-packet, chưa có time-correlated process

3) **Antenna orientation**
- Lý thuyết/measurement: heading/pattern ảnh hưởng lớn
- Hiện tại: đã có bản nhẹ theo heading-mismatch penalty (tùy chọn), nhưng chưa phải full 3D gain pattern

4) **PER calibration theo đo đạc**
- Lý thuyết thực nghiệm: có thể fit trực tiếp SNR→PER theo campaign
- Hiện tại: dùng mô hình analytic BER→PER (ưu điểm: nhẹ, dễ tái lập)

---

## D. Kết luận đối chiếu ngắn gọn

- Stack hiện tại đã bao phủ tốt các trụ cột có giá trị cao cho mô phỏng UAV-WSN:
  - path loss theo profile hình học,
  - shadowing,
  - fast fading (xấp xỉ),
  - BER/PER theo packet size,
  - contact-time gating.
- So với literature, phần còn thiếu có giá trị khoa học cao nhất là:
  - stochastic LoS probability,
  - time-correlated fading/Doppler dynamics,
  - antenna heading/pattern effects.

---

## E. Trạng thái triển khai nhanh (high value, low effort)

1) **Optional stochastic LoS mode** ✅
- Giữ mode deterministic hiện tại làm default
- Đã thêm mode `p_LOS` theo công thức logistic (tùy chọn)

2) **Contact-window margin theo vận tốc** ✅
- Đã bổ sung penalty margin khi coherence-time nhỏ hơn packet airtime

3) **Trace PHY chi tiết hơn** ✅
- Đã ghi `snrDb`, `ber`, `per` (và ngưỡng liên quan) vào PHY drop trace

4) **Antenna heading scalar penalty (bản nhẹ)** ✅
- Đã thêm mode tùy chọn: suy hao phụ thuộc mismatch giữa vector vận tốc TX và hướng LOS
- Full 3D antenna pattern vẫn là bước nâng cao tiếp theo

---

## G. Công thức nghiên cứu đã đưa trực tiếp vào triển khai

1) **LoS probability (A2G logistic approximation)**

Theo hướng của các nghiên cứu A2G về xác suất LoS theo hình học/elevation, dùng dạng:

$$
p_{LoS}(\theta) = \frac{1}{1 + a\exp\left(-b(\theta-a)\right)}
$$

Trong code:
- `EnableStochasticLos`
- `LosProbabilityA`, `LosProbabilityB`

2) **Doppler/coherence-time driven margin**

Theo quan hệ Doppler và coherence-time (Clarke-style approximation):

$$
f_{D,max} \approx \frac{v_{rel}}{c}f_c, \qquad T_c \approx \frac{0.423}{f_{D,max}}
$$

Và khi:

$$
\frac{t_{airtime}}{T_c} > 1
$$

thì cộng thêm margin penalty trong contact-window gate (bounded bởi cap).

3) **BER/PER packet reliability**

Giữ nguyên hướng analytic đã có trong code:

$$
PER = 1 - (1-BER)^{8L}
$$

và dùng stochastic Bernoulli để quyết định drop ở PHY receive path.

---

## F. Nguồn code đã đối chiếu trực tiếp

- `src/wsn/model/propagation/cc2420-spectrum-propagation-loss-model.cc`
- `src/wsn/model/radio/cc2420/cc2420-contact-window-model.cc`
- `src/wsn/model/radio/cc2420/cc2420-error-model.cc`
- `src/wsn/model/radio/cc2420/cc2420-phy.cc`

