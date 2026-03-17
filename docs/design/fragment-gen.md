## Kích thước ảnh/video thường dùng cho live detection
Thumbnails / embedded detectors: 300×300 → 416×416 px thường dùng (SSD300, YOLOv3/YOLOv4 default 416/608).
Faster‑R‑CNN / two‑stage: ảnh được resize sao cho cạnh lớn ≈ 600–1000 px (tốt cho small objects).
Thực tế live: chọn đầu vào 320–800 px theo trade‑off latency vs. small‑object performance.

## Kích thước nhỏ nhất để nhận diện người/khuôn mặt (thực nghiệm / rule‑of‑thumb)
Khuôn mặt rất nhỏ: reliable detection typically requires face height ≳ 20–40 px (paper “Finding Tiny Faces” và benchmarks). Dưới ~16 px → hiệu năng sụt mạnh.
Người/whole‑body: để nhận dạng/ phát hiện toàn thân đáng tin cậy thường cần object height ≳ 50–100 px.
Lưu ý: con số phụ thuộc detector (mạng nhẹ cần lớn hơn), chất lượng nén, blur, occlusion.
## Nếu chia file theo pixel‑stride (interleaving) hoặc Subsample + shift — khả năng ghép phục hồi từ tập con fragment?
Pixel‑stride (interleave) với stride s (ví dụ s=2): ảnh gốc được tách thành s^2 bộ con bổ sung. Đặc tính:
Để tái tạo chính xác không mất mát cần cả s^2 fragments (ví dụ s=2 → cần 4/4).
Nếu chỉ có k < s^2 fragment, ghép bằng interleave + nội suy cho ra ảnh có nhiều pixel thiếu → aliasing/holes; detection có thể vẫn hoạt động nếu object lớn và detector chịu được blur, nhưng chi tiết mất nhiều.
Subsample + shift (subpixel shifts + low‑pass filter trước khi downsample):
Nếu fragments là các bản downsampled dịch lệch (small controlled shifts), multi‑frame SR / shift‑and‑add có thể phục hồi chi tiết. Số fragments cần thiết phụ thuộc tỉ lệ upsample r: lý tưởng cần ~r^2 “diverse” samples để lấp đầy không gian nyquist; thực tế SR học sâu có thể phục hồi chấp nhận được từ 2–4 khung nếu tín‑nhiễu thấp và chuyển vị hợp lý.
Với fragment ngẫu nhiên (2/10) khả năng tái tạo full‑quality thấp trừ khi fragments chứa lớp thông tin chồng lấp (progressive layers) hoặc có mã hóa/khôi phục (fountain/UEP).
## Confidence của detection thường đo thế nào & ngưỡng “chuẩn”
Score output: detector trả score p ∈ [0,1] (objectness / class probability). Đây là “confidence” thô.
Metric chuẩn: mAP (mean Average Precision) tính bằng precision/recall ở các IoU thresholds (thường IoU≥0.5; COCO dùng [0.5:0.95]).
Ngưỡng nhận diện (practical): score ≥ 0.5 là mặc định nhiều hệ; để giảm FP tăng lên 0.7–0.9 tùy yêu cầu. Với face surveillance hay cảnh báo, người ta đặt threshold cao (0.8–0.95) rồi dùng thêm NMS và lọc theo kích thước.
Kết hợp nhiều fragment: nếu mỗi fragment i cho p_i độc lập, ta có các cách hợp nhất:
Union probability (tốt khi các fragment độc lập): p_union = 1 − ∏_i (1 − p_i).
Log‑odds/Bayesian update: L = Σ logit(p_i), p_comb = sigmoid(L).
Simple voting / averaging: p_mean = mean(p_i) (khi cần đơn giản).
Quy ước “confident”: p_comb ≥ 0.75–0.9 thường được coi là tin cậy cho cảnh báo tự động; tinh chỉnh theo FA/FR yêu cầu của bài.
## ngưỡng tái tạo tin cậy
Interleave: ≳75–100% mảnh để tái tạo tốt.
Subsample+shift, r=2: ≈4 samples (≥40% nếu tổng 10 fragments) để có cải thiện; để gần gốc ≳50–80%.
Deep SR fusion: 2–4 fragments → cải thiện đáng kể; 6–10 → tốt.
Progressive layers: 10–30% đầu → coarse but possibly usable for detection.
Rateless codes: cần ≈100% của k blocks ±5–10% overhead (không phụ thuộc vị trí fragment).