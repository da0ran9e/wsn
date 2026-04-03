# Tổng hợp nghiên cứu về Trust Management trong WSN/IoT (2020–2025) và khoảng trống nghiên cứu

**Không có bất kỳ framework thống nhất nào từ 2020 đến 2025 kết hợp đồng thời cả 6 chủ đề nghiên cứu được đề cập.** Các công trình hiện tại chỉ bao phủ tối đa 3 trong 6 yếu tố, với ba yếu tố hiếm nhất là progressive trust building, RSSI-based physical layer trust, và data quality cross-validation giữa các node. Đây là một khoảng trống nghiên cứu rõ ràng và có giá trị — một framework hợp nhất tất cả sẽ là đóng góp mới hoàn toàn. Dưới đây là tổng hợp chi tiết từng chủ đề cùng đánh giá mức độ chồng lấn giữa chúng.

---

## 1. Trust-aware routing: nền tảng phổ biến nhất trong trust management

Trust-aware routing là chủ đề được nghiên cứu nhiều nhất, với hầu hết các công trình tập trung vào việc tích hợp trust score vào quyết định chọn đường trong WSN. **Pathak, Al-Anbagi & Hamilton (2022)** đề xuất thuật toán LSR trong *IEEE IoT Journal* (vol. 9, no. 23, pp. 23826–23840), sử dụng Ant Colony Optimization kết hợp mô hình bảo mật thích ứng dựa trên direct và indirect trust, cho hiệu suất vượt trội về trust convergence, network lifetime và packet delivery ratio.

**Hu, Han, Yao & Song (2022)** trình bày TBSEER trong *IEEE Access* (vol. 10, pp. 10585–10596) — một giao thức tính comprehensive trust value qua **adaptive direct trust, indirect trust và energy trust**, kết hợp cơ chế phạt thích ứng để nhanh chóng nhận diện node độc hại, chống được black hole, selective forwarding, sinkhole và hello flood attacks. **Bin-Yahya, Alhussein & Shen (2022)** đề xuất TSW trong *IEEE IoT Journal* (vol. 9, no. 22, pp. 22230–22245), thiết kế trust phân tách cho **control traffic và data traffic riêng biệt** trong software-defined WSN — đây là một trong số ít bài phân loại trust theo loại lưu lượng.

Các công trình bổ sung đáng chú ý gồm: **Han, Hu & Guo (2022)** với TAGA trong *IEEE Access* sử dụng adaptive genetic algorithm; **Muzammal, Murugesan & Jhanjhi (2021)** cung cấp survey toàn diện trong *IEEE IoT Journal* (vol. 8, pp. 4186–4210) phân loại các giải pháp trust-based routing cho RPL/IoT; **Kim, Ko & Chung (2022)** đề xuất PITrust trong *IEEE Wireless Communications Letters* (vol. 11, no. 5) dùng RSSI-based physical identification cho trust path routing chống Sybil attack trong RPL; và **Sharma, Beniwal & Kumar (2024)** với ML-HSOR trong *Journal of Supercomputing* (Springer, vol. 80) sử dụng Markov model với adaptive weighting cho multi-level trust routing.

---

## 2. Multi-dimensional trust: phân tách trust thành nhiều chiều vẫn còn thiếu sự chuẩn hóa

Nghiên cứu về multi-dimensional trust tập trung vào việc kết hợp nhiều yếu tố đánh giá thay vì dùng một trust score đơn nhất. **Souissi, Ben Azzouna & Ben Said (2019)** trong *Computer Networks* (Elsevier) đưa ra framework phân tách trust thành ba tầng: **Data Perception Trust, Communication Trust, và Data Fusion Trust** — đây là công trình nền tảng được trích dẫn rộng rãi trong giai đoạn 2020–2025, dù xuất bản trước 2020.

**Wei, Yang, Wu, Long & Li (2022)** cung cấp survey toàn diện trong *IEEE IoT Journal* (vol. 9, no. 10, pp. 7664–7679) phân tích các chiều trust gồm trust definition, composition, aggregation, và computation. **Guo, Liu, Ota, Dong, Deng & Xiong (2022)** đề xuất ITCN trong *IEEE Trans. Network Science and Engineering* (vol. 9, no. 1, pp. 203–218), tích hợp **data trust và behavioral trust** với AI-based analytics cho IoT data collection. Hệ thống sử dụng active and verifiable trust evaluation kết hợp misbehavior components, data trust và energy trust.

**Alghofaili & Rassam (2022)** trong *Sensors* (MDPI, vol. 22, no. 2, Article 634) đề xuất multi-criteria decision-making trust model dùng SMART + Shannon entropy cho **dynamic weight calculation** giữa nhiều trust factors, kết hợp Deep LSTM cho trust prediction. **Pathak, Singh, Khan et al. (2024)** giới thiệu NATURE model trong *Scientific Reports* (Springer, vol. 14, Art. 28162) với multi-level clustered trust kết hợp **residual energy** vào đánh giá trust — nhận ra rằng node hết năng lượng không thể được coi là đáng tin dù có behavioral history tốt.

Một điểm quan trọng: hầu hết các mô hình kết hợp direct trust + indirect trust + energy, nhưng **rất ít bài formalize cụ thể bộ ba communication trust, data trust và energy trust** như các chiều riêng biệt. Công trình TSW của Bin-Yahya et al. phân tách control/data trust là ngoại lệ đáng chú ý.

---

## 3. Incremental/progressive trust building: chủ đề hiếm và chưa được khai thác đầy đủ

Progressive trust building — nơi trust được xây dựng qua nhiều giai đoạn từ bootstrapping đến đánh giá hành vi đến data quality — là một trong những chủ đề hiếm nhất. **Su, Sfar, Natalizio, Moyal & Song (2021)** đề xuất PDTM trong *ICCCN 2021* (IEEE), cấu trúc trust thành **4 pha rõ ràng**: access control phase, pre-selection phase, service interaction phase, và post-evaluation phase. Đặc biệt, PDTM không gán default trust score cố định cho node mới mà tiến hóa trust qua các giai đoạn — đây là công trình trực tiếp nhất về progressive trust.

**Wang, Cai, Seo & Li (2023)** đề xuất TMETA trong *IEEE IoT Journal* (vol. 10, no. 24, pp. 21337–21348), trực tiếp giải quyết **cold-start trust problem** bằng digital twin + blockchain để mô phỏng IoT entities trong không gian số và quản lý trust evolution. **Bampatsikos, Politis & Xenakis (2021)** trong *ARES '21* (ACM) đề xuất cơ chế xác định initial trust score cho new devices bằng blockchain-based remote attestation kết hợp device properties và communication context — giải quyết vấn đề gán trust tùy ý cho node mới có thể phá hủy hệ thống.

**Bampatsikos, Politis, Ioannidis & Xenakis (2025)** mở rộng nghiên cứu trước trong *IEEE Trans. Consumer Electronics* (vol. 71, no. 1, pp. 862–882), đề xuất **dynamic trust score lifecycle management** dùng MADM cho initial trust + Markov chains cho trust evolution qua toàn bộ vòng đời thiết bị. **Marche & Nitti (2021)** trong *IEEE Trans. Network and Service Management* (vol. 18, no. 3, pp. 3297–3308) trình bày trust model ML-based cho Social IoT, nơi trust tiến hóa qua iterative transactions.

Tuy nhiên, **không có bài nào kết hợp progressive trust building với trust-aware routing và physical layer trust** trong một framework thống nhất.

---

## 4. Self-organizing hierarchical network: trust-based clustering chiếm ưu thế nhưng thiếu tự tổ chức thực sự

Phần lớn nghiên cứu về chủ đề này tập trung vào **trust-based cluster head (CH) selection** hơn là self-organization thực sự. **Yang, Yu, Yang, Chakraborty, Lu & Guo (2022)** trong *IEEE Trans. Industrial Informatics* (vol. 18, no. 12, pp. 8864–8875) đề xuất intelligent trust cloud management cho secure clustering trong 5G IoMT, dùng fuzzy trust inferring để hình thành trust clouds cho thiết bị.

**Fang, Zhang, Yang et al. (2021)** đề xuất LEACH-TM trong *Digital Communications and Networks* (vol. 7, no. 4, pp. 470–478) — tích hợp trust management vào LEACH clustering protocol, nơi node tự tổ chức thành cluster phân cấp với CH selection dựa trên residual energy, neighbor density và trust, **không yêu cầu GPS**. **Das & Dash (2023)** trong *Journal of Reliable Intelligent Environments* (Springer, vol. 9, pp. 27–48) đề xuất LS-EATO dùng harmonic search genetic algorithm cho CH selection dựa trên energy, trust, distance và density.

**Osamy, Khedr, Vijayan et al. (2023)** trình bày TACTIRSO trong *Journal of Supercomputing* (Springer, vol. 79, pp. 5962–6016) dùng improved rat swarm optimizer cho trusted CH selection trong intelligent transportation. **Jiang, Zhu, Han, Guizani & Shu (2020)** trong *IEEE Trans. Vehicular Technology* (vol. 69, no. 8, pp. 9031–9040) đề xuất trust evaluation dựa trên C4.5 decision tree cho **underwater WSN** — môi trường GPS-denied điển hình. **Naghibi, Barati & Barati (2025)** trong *Computing* (Springer) đề xuất dynamic trust-based clustering cho IoT với genetic algorithm optimization.

**Shahraki, Taherkordi, Haugen & Eliassen (2021)** cung cấp survey toàn diện trong *IEEE Trans. Network and Service Management* (vol. 18, no. 2, pp. 2242–2274) về clustering techniques từ WSN đến IoT, xác nhận trust là yếu tố then chốt trong CH election và decentralized clustering.

Điểm quan trọng: **không có bài nào kết hợp self-organizing hierarchy dựa trên trust với physical layer trust để bootstrap** — hai yếu tố này vẫn tách biệt hoàn toàn trong literature.

---

## 5. Physical layer trust và RSSI-based trust: lĩnh vực trưởng thành nhưng bị cô lập khỏi trust management

Physical layer authentication (PLA) là một lĩnh vực nghiên cứu sôi động nhưng hầu như **không kết nối với trust management frameworks** ở tầng cao hơn. **Xie, Li & Tan (2021)** cung cấp survey đầu tiên toàn diện về PLA trong *IEEE Communications Surveys & Tutorials* (vol. 23, no. 1, pp. 282–310), phân loại thành passive (RF fingerprints, CSI/RSSI) và active (tag-embedding).

**Lei, Pang, Wen, Hou & Li (2023)** trong *IEEE Trans. Industrial Informatics* đề xuất **physical layer enhanced zero-trust security** cho wireless IIoT — framework 3 bước: (1) security zone formation dùng channel characteristics, (2) device authentication dùng physical fingerprints, (3) cryptographic key negotiation qua physical-layer key distribution. Đây là bài gần nhất với ý tưởng dùng physical layer để bootstrap trust.

Về RF fingerprinting, **Zhang, Shen, Saad & Chowdhury (2023)** trong *IEEE Communications Magazine* (vol. 61, no. 10) review RFFI cho IoT device authentication, và **Shen, Zhang, Marshall & Cavallaro (2022)** trong *IEEE Trans. Information Forensics and Security* (vol. 17, pp. 774–787) đề xuất scalable channel-robust RFFI cho LoRa dùng deep metric learning — quan trọng cho việc enroll new devices (liên quan đến cold-start problem).

Về RSSI-based attack detection, **Yan, Jiang, Lin et al. (2023)** trong *EURASIP JWCN* (Springer) đề xuất Sybil detection cho mobile IoT dùng RSSI two-round protocol, và **Wang, Zhao, Li & Liu (2021)** trong *Springer LNDECT* kết hợp coding method với RSSI-based location cho Sybil detection. **Wang & Fu (2022)** trong *IEEE IoT Journal* (vol. 9, no. 10, pp. 7731–7745) đề xuất channel-prediction one-class authentication cho mobile IoT, và **Liao, Wen, Chen et al. (2020)** trong *IEEE IoT Journal* (vol. 7, no. 3, pp. 2077–2088) giải quyết multiuser PLA dùng deep learning.

Phát hiện then chốt: **PLA research hoạt động như một "ốc đảo" riêng biệt** — có nhiều kỹ thuật mạnh mẽ để xác thực thiết bị qua tín hiệu vật lý, nhưng gần như không bài nào feed kết quả PLA vào multi-dimensional trust model cho routing decisions.

---

## 6. Data quality trust: cross-validation giữa sensor nodes là yếu tố hiếm nhất

Trust dựa trên chất lượng dữ liệu cảm biến tồn tại trong literature nhưng hiếm khi được tích hợp vào trust-aware routing. **Aboelwafa, Seddik, Eldefrawy, Gadallah & Gidlund (2020)** trong *IEEE IoT Journal* (vol. 7, pp. 8462–8471) dùng **denoising autoencoders khai thác spatiotemporal correlation** để phát hiện false data injection — trực tiếp nhất về cross-validation trong không gian và thời gian.

**Yang, Lu, Yang, Guo & Liang (2021)** đề xuất SCFTO trong *IEEE Trans. Industrial Informatics* (vol. 17, no. 7, pp. 4837–4847) dùng interval type-2 fuzzy logic cho trust estimation kết hợp DBSCAN outlier detection để cô lập node độc hại. **He, Han, Jiang, Wang & Martínez-García (2022)** trong *IEEE Trans. Mobile Computing* (vol. 21, no. 3, pp. 811–821) đề xuất reinforcement learning-based trust update cho underwater acoustic sensor networks, nơi đánh giá data quality là yếu tố sống còn.

**Shen, Liu, Huang, Xiong & Lu (2021)** trình bày ATTDC trong *IEEE IoT Journal* (vol. 8, no. 8, pp. 6437–6453) — active and traceable trust-based data collection cho IIoT smart cities với traceability mechanisms kiểm tra data provenance. **Alwan et al. (2023)** cung cấp systematic review trong *ACM Computing Surveys* (vol. 55) về data quality trong CPS/IoT, xác nhận data trustworthiness là yếu tố cốt lõi.

**Abid, El Khediri & Kachouri (2021)** trong *Computing* (Springer, vol. 103, pp. 2275–2292) so sánh DBSCAN và OPTICS cho outlier detection dùng dữ liệu thực từ Intel Berkeley Lab. Phát hiện quan trọng: **cross-validation giữa neighboring sensor nodes như một cơ chế trust được tích hợp vào routing là yếu tố gần như vắng mặt** trong các framework hiện tại.

---

## Đánh giá khoảng trống: sự kết hợp 6 yếu tố là đóng góp hoàn toàn mới

Phân tích chéo giữa các chủ đề cho thấy một bức tranh rõ ràng về mức độ bao phủ:

| Yếu tố | Mức phổ biến | Bài tiêu biểu gần nhất |
|---------|-------------|----------------------|
| (1) Trust-aware routing | **Rất phổ biến** | Pathak et al. 2022; Hu et al. 2022 |
| (2) Multi-dimensional trust | **Phổ biến trung bình** | ITCN 2022; Souissi et al. 2019 |
| (3) Progressive trust building | **Hiếm** | PDTM (Su et al. 2021); TMETA 2023 |
| (4) Self-organizing hierarchy | **Phổ biến trung bình** | LEACH-TM 2021; TACTIRSO 2023 |
| (5) Physical layer / RSSI trust | **Phong phú riêng lẻ, chưa tích hợp** | Lei et al. 2023; Zhang et al. 2023 |
| (6) Data quality cross-validation | **Rất hiếm trong trust frameworks** | Aboelwafa et al. 2020 |

Các framework bao phủ nhiều yếu tố nhất — TEAHR (2025), ML-HSOR (Sharma et al. 2024), và NATURE (Pathak et al. 2024) — đều chỉ đạt **tối đa 3/6 yếu tố**, tập trung vào trust-aware routing + multi-dimensional trust + hierarchical structure. Ba yếu tố còn lại — **progressive trust building, physical layer trust bootstrap, và data quality cross-validation** — tạo thành một tam giác khoảng trống chưa được lấp đầy.

Đặc biệt đáng chú ý là sự **phân mảnh giữa physical layer research và trust management research**. PLA community phát triển nhiều kỹ thuật mạnh (RF fingerprinting, RSSI-based authentication, channel-based verification) nhưng không kết nối output vào trust scoring systems. Ngược lại, trust management community giả định node đã được xác thực và tập trung vào behavioral observation — bỏ qua câu hỏi "làm sao thiết lập trust ban đầu khi chưa có interaction history."

## Kết luận: cơ hội cho một framework cross-layer thống nhất

Khoảng trống nghiên cứu này không chỉ là sự thiếu vắng đơn thuần — nó phản ánh một **vấn đề cấu trúc** trong cách cộng đồng nghiên cứu tiếp cận trust. Physical layer trust, behavioral trust, data quality trust, và routing decisions hiện được nghiên cứu bởi các subcommunities khác nhau với ít giao thoa. Một framework thống nhất sẽ cần: (a) dùng RSSI/physical signals để bootstrap trust ban đầu cho cold-start nodes, (b) progressive elevation qua behavioral observation và data quality validation, (c) multi-dimensional trust scores feed trực tiếp vào adaptive routing và self-organizing hierarchy formation, và (d) continuous data cross-validation để duy trì và cập nhật trust. Sự kết hợp này tạo ra một **vòng lặp trust khép kín từ physical layer đến application layer** — chưa từng xuất hiện trong literature 2020–2025 và đại diện cho một hướng nghiên cứu có tiềm năng đóng góp đáng kể.