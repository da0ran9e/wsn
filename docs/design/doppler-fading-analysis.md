# Doppler và Fast Fading — Phân tích thiết kế cho WSN UAV-Ground

> **Phạm vi**: Áp dụng cho kịch bản UAV-Ground sử dụng CC2420 (2.4 GHz, DSSS-OQPSK, 250 kbps).
> Tài liệu này phân tích lý thuyết, so sánh với các công nghệ khác, và ghi lại quyết định thiết kế
> liên quan đến model truyền dẫn trong `Cc2420SpectrumPropagationLossModel`.

---

## 1. Nền tảng lý thuyết

### 1.1 Doppler Shift

Khi node phát và node thu có chuyển động tương đối với vận tốc $v$ (m/s), sóng vô tuyến bị dịch
tần số một lượng:

$$f_D = \frac{v \cdot \cos\theta}{c} \cdot f_c$$

- $v$: vận tốc tương đối giữa UAV và ground node (m/s)
- $\theta$: góc giữa vectơ vận tốc và đường kết nối TX–RX
- $c = 3 \times 10^8$ m/s
- $f_c$: tần số sóng mang (Hz)

Trường hợp xấu nhất ($\cos\theta = 1$, UAV di chuyển thẳng về phía ground node):

$$f_D^{max} = \frac{v}{c} \cdot f_c$$

**Ví dụ với CC2420**: $f_c = 2.4 \times 10^9$ Hz, $v = 10$ m/s:

$$f_D^{max} = \frac{10}{3 \times 10^8} \times 2.4 \times 10^9 = 80 \text{ Hz}$$

---

### 1.2 Coherence Time

Coherence time $T_c$ là khoảng thời gian mà channel có thể coi là tĩnh (không đổi đáng kể).
Dùng xấp xỉ Clarke:

$$T_c \approx \frac{0.423}{f_D^{max}} = \frac{0.423 \cdot c}{v \cdot f_c}$$

| Tốc độ UAV | $f_D$ (CC2420) | $T_c$ |
|------------|----------------|-------|
| 1 m/s      | 8 Hz           | 52.9 ms |
| 5 m/s      | 40 Hz          | 10.6 ms |
| 10 m/s     | 80 Hz          | **5.3 ms** |
| 20 m/s     | 160 Hz         | **2.6 ms** |
| 30 m/s     | 240 Hz         | **1.8 ms** |

**Airtime của 1 CC2420 MAC frame** (max 127 bytes payload):

$$t_{frame} = \frac{127 \times 8}{250{,}000} = 4.06 \text{ ms}$$

→ Với UAV 10 m/s: $T_c \approx 5.3$ ms $\approx t_{frame}$ — channel có thể thay đổi **trong cùng một packet**.

---

### 1.3 Coherence Bandwidth

Ngoài thời gian, channel còn bị giới hạn bởi **coherence bandwidth** $B_c$, liên quan đến
delay spread $\sigma_\tau$ (đặc trưng của môi trường multipath):

$$B_c \approx \frac{1}{5\sigma_\tau}$$

| Môi trường | $\sigma_\tau$ điển hình | $B_c$ |
|------------|------------------------|-------|
| Outdoor urban | 1–3 µs | 67–200 kHz |
| Suburban | 0.2–1 µs | 200 kHz–1 MHz |
| UAV–Ground LoS | < 0.1 µs | > 2 MHz |

CC2420 chiếm băng thông ~2 MHz. Với LoS UAV–Ground ($B_c > 2$ MHz), channel là
**flat fading** (không có frequency-selective fading đáng kể).

---

## 2. Tác động của Doppler theo công nghệ

### 2.1 Tại sao DSSS (CC2420) gần như miễn nhiễm với Doppler

CC2420 dùng DSSS với chip rate 2 Mchip/s. Doppler shift gây ra lỗi tương quan chỉ khi
$f_D$ đủ lớn so với chip bandwidth:

$$f_D^{critical} \approx \frac{R_{chip}}{10} = \frac{2 \times 10^6}{10} = 200{,}000 \text{ Hz}$$

$$v_{critical} = \frac{f_D^{critical} \cdot c}{f_c} = \frac{200{,}000 \times 3 \times 10^8}{2.4 \times 10^9} \approx 25{,}000 \text{ m/s}$$

→ Không có UAV nào đạt vận tốc này. **Doppler shift không bao giờ trực tiếp gây lỗi bit** trong CC2420.

### 2.2 OFDM (WiFi, LTE, 5G) — Doppler tác động qua ICI

OFDM chia spectrum thành các subcarrier khoảng cách $\Delta f_{sub}$. Doppler shift $f_D$
phá vỡ tính trực giao giữa các subcarrier → gây **Inter-Carrier Interference (ICI)**:

$$\text{SINR}_{ICI} \propto \left(\frac{\Delta f_{sub}}{f_D}\right)^2$$

Khi $f_D / \Delta f_{sub} > 0.1$, BER tăng đột biến không thể bù bằng coding.

Ngưỡng tốc độ gây lỗi:

$$v_{critical} = \frac{0.1 \cdot \Delta f_{sub} \cdot c}{f_c}$$

| Công nghệ | $f_c$ | $\Delta f_{sub}$ | $v_{critical}$ | UAV 10 m/s | UAV 30 m/s | UAV 100 m/s |
|-----------|-------|-----------------|----------------|:----------:|:----------:|:-----------:|
| WiFi 802.11n 2.4G | 2.4 GHz | 312 kHz | 39 m/s | ✅ | ⚠️ cận | ❌ |
| WiFi 802.11ac 5G | 5 GHz | 312 kHz | 18.7 m/s | ⚠️ cận | ❌ | ❌ |
| WiFi 802.11ax 5G | 5 GHz | 78 kHz | 4.7 m/s | ❌ | ❌ | ❌ |
| 4G LTE | 1.8 GHz | 15 kHz | 833 m/s | ✅ | ✅ | ✅ |
| 5G NR µ=1 | 3.5 GHz | 30 kHz | 857 m/s | ✅ | ✅ | ✅ |
| 5G NR FR2 mmWave | 28 GHz | 120 kHz | 429 m/s | ✅ | ✅ | ✅ |
| LoRa SF7 | 868 MHz | 125 kHz | 14,400 m/s | ✅ | ✅ | ✅ |
| **CC2420 DSSS** | **2.4 GHz** | **chip 2 MHz** | **>25,000 m/s** | **✅✅** | **✅✅** | **✅✅** |

> **Ghi chú quan trọng**: LTE/5G có $\Delta f_{sub}$ nhỏ hơn WiFi nhưng chịu Doppler tốt hơn
> vì có **pilot density cao** (CRS/DMRS rải đều theo thời gian–tần số) để tracking channel liên tục.
> WiFi ít pilot hơn → channel estimation stale khi channel thay đổi nhanh.

---

## 3. Vấn đề thực sự với CC2420 UAV: Fast Fading

Dù Doppler không trực tiếp gây lỗi bit, **coherence time ngắn** ($T_c \approx t_{frame}$) dẫn đến
hiện tượng channel fading biến đổi nhanh (fast fading) — đây mới là nguồn lỗi chính.

### 3.1 Phân loại fading theo link type

| Link type | Elevation | Model thích hợp | K-factor | $\sigma_{rice,dB}$ |
|-----------|-----------|-----------------|----------|--------------------|
| UAV–Ground LoS cao | > 40° | **Ricean** | 10–20 dB | ~2.6 dB |
| UAV–Ground mixed | 20°–40° | Ricean | 3–10 dB | ~3–5 dB |
| UAV–Ground NLoS | < 20° | **Rayleigh** ($K=0$) | 0 | ~5.6 dB |
| Ground–Ground | — | Rayleigh | 0 | ~5.6 dB |

Phân phối Ricean mô tả amplitude fading khi có một thành phần LoS dominant (K lớn → fading ít),
Rayleigh là trường hợp đặc biệt $K=0$ khi không có LoS.

### 3.2 Ricean fading và BER

Với modulation OQPSK và Ricean fading, BER xấp xỉ:

$$\text{BER}_{Ricean} \approx Q_1\!\left(\sqrt{2K}, \sqrt{\frac{2(K+1) \cdot E_b/N_0}{1 + E_b/N_0}}\right)$$

Ảnh hưởng thực tế trên link margin:

| Scenario | Channel | Link margin cần thêm |
|----------|---------|---------------------|
| UAV 30m LoS, elev 60° | Ricean K=15 | +3 dB |
| UAV 30m mixed, elev 30° | Ricean K=6 | +6 dB |
| UAV 30m NLoS, elev 10° | Rayleigh | +10–15 dB |
| Ground–Ground | Rayleigh | +10–15 dB |

### 3.3 Tác động của vận tốc UAV đến BER qua coherence time

Khi $T_c < t_{frame}$, channel thay đổi **trong khi đang nhận packet** → các symbol cuối frame
nhận được trên channel khác với channel ở đầu frame → **không có equalizer nào bù được**:

$$T_c < t_{frame} \Rightarrow \text{BER}_{\text{effective}} \approx \text{BER}_{\text{worst-case over fading cycle}}$$

| UAV speed | $T_c$ | So với $t_{frame}$ = 4ms | Hậu quả |
|-----------|-------|--------------------------|---------|
| < 5 m/s | > 10 ms | $T_c > 2t_{frame}$ | Flat fading per packet, stable |
| 5–10 m/s | 5–10 ms | $T_c \approx t_{frame}$ | Channel đổi cuối packet, ~2–3 dB penalty |
| 10–20 m/s | 2.5–5 ms | $T_c < t_{frame}$ | Fast fading rõ rệt, ~5 dB penalty |
| > 30 m/s | < 1.8 ms | $T_c \ll t_{frame}$ | Severe fading, retransmit nhiều |

---

## 4. Trạng thái hiện tại của model

### 4.1 Model đang được implement

File: `src/wsn/model/propagation/cc2420-spectrum-propagation-loss-model.cc`

```
P_rx = P_tx - PL(d) - X_σ
```

Trong đó:
- `PL(d)` = log-distance path loss với exponent phụ thuộc elevation angle
- `X_σ` = log-normal shadowing (slow fading), `N(0, σ²)`, $\sigma$ = 4–8 dB tùy profile

**Chưa có**:
- Fast fading (Ricean/Rayleigh)
- Doppler-induced channel variation over packet duration

### 4.2 Shadowing đang dùng

| Profile | Exponent $n$ | Sigma $\sigma$ |
|---------|--------------|----------------|
| Ground–Ground | 3.2 | 7.0 dB |
| Air–Ground LoS (elev > 40°) | 2.0 | 4.0 dB |
| Air–Ground Mixed (20°–40°) | 2.5 | 6.0 dB |
| Air–Ground NLoS (< 20°) | 3.0 | 8.0 dB |

### 4.3 Contact Window Model

File: `src/wsn/model/radio/cc2420/cc2420-contact-window-model.cc`

Hiện tại `HasContactForPacket()` sample path loss theo projected positions — **không tính fast fading**.
Dự đoán link chỉ dựa vào path loss + shadowing mean. Đây là conservative estimate (underestimates
viability khi có Ricean gain, overestimates khi có Rayleigh deep fade).

---

## 5. Khuyến nghị cho các bước tiếp theo

### 5.1 Bổ sung Ricean fast fading vào propagation model

Thêm số hạng fast fading $X_{rice}$ (dB) vào `ComputePathLossDbFromPositions`:

$$P_{rx} = P_{tx} - PL(d) - X_\sigma - X_{rice}$$

Xấp xỉ Ricean trong dB (Beaulieu 2004):

$$X_{rice} \sim \mathcal{N}\!\left(\mu_K,\, \sigma_K^2\right) \quad \text{với} \quad
\sigma_K \approx \frac{4.34}{\sqrt{K+1}} \text{ dB}, \quad \mu_K \approx -10\log_{10}(K+1) + 10 \cdot \frac{K}{K+1}$$

Attributes cần thêm: `KFactorLoS`, `KFactorMixed`, `KFactorNLoS` (giá trị tuyến tính).

### 5.2 Contact Window với fast fading margin

Trong `Cc2420ContactWindowModel::HasContactForPacket()`, nên thêm margin phụ thuộc $T_c$:

```
if (T_c < t_frame):
    required_margin += fast_fading_penalty(v_uav, elevation)
```

### 5.3 Với UAV tốc độ cao (> 20 m/s)

Cân nhắc model **time-varying channel per packet**: thay vì sample một giá trị fading per packet,
sample $N_{samples} = t_{frame} / T_c$ giá trị và lấy worst-case hoặc average BER.

---

## 6. Tóm tắt

| Hiện tượng | Ảnh hưởng với CC2420 | Cần model không? |
|------------|---------------------|-----------------|
| Doppler shift (ICI) | **Không** — DSSS miễn nhiễm | Không |
| Coherence time | **Có** — fast fading khi UAV > 10 m/s | Có (5.1, 5.2) |
| Fast fading (Ricean) | **Có** — 2–6 dB margin penalty theo elevation | Có (5.1) |
| Slow fading (shadowing) | **Có** — đã implement | ✅ Done |
| Frequency-selective fading | **Không** — LoS link, $B_c > 2$ MHz | Không |

> **Kết luận**: Với CC2420 UAV scenario, ưu tiên bổ sung **Ricean fast fading** theo elevation angle
> và **dynamic margin** trong contact window model theo vận tốc UAV, thay vì lo ngại Doppler ICI.
