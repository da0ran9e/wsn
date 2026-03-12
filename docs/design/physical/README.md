# Physical Layer Upgrade — Analysis Files

Thư mục này chứa phân tích chi tiết cho từng hướng nâng cấp tầng vật lý của module `src/wsn`, tập trung vào Scenario5 và các thí nghiệm UAV2.

---

## Danh sách hướng

| File | Hướng | Pha | Ưu tiên |
|---|---|---|---|
| [P1A-3D-GEOMETRY.md](P1A-3D-GEOMETRY.md) | 3D distance + elevation angle | P1 | ★★★★★ |
| [P1B-PATH-LOSS-MODEL.md](P1B-PATH-LOSS-MODEL.md) | Log-distance path loss + shadowing | P1 | ★★★★★ |
| [P1C-PER-DECISION.md](P1C-PER-DECISION.md) | PER filter — physics-based receive decision | P1 | ★★★★★ |
| [P1D-CONTACT-TIME.md](P1D-CONTACT-TIME.md) | Contact time per node per pass | P1 | ★★★☆☆ |
| [P2-COOPERATION-CONTENTION.md](P2-COOPERATION-CONTENTION.md) | Cooperation slot budget | P2 | ★★★☆☆ |
| [P3-VIEW-QUALITY.md](P3-VIEW-QUALITY.md) | Sensing geometry — view quality per node | P3 | ★★☆☆☆ |
| [P4-URBAN-OBSTRUCTION.md](P4-URBAN-OBSTRUCTION.md) | Buildings / NLoS obstruction | P4 | ★☆☆☆☆ |

---

## Ghi chú chung

- **P1A + P1B + P1C** là package tối thiểu để claim "physically-informed fragment dissemination".
- **P1D + P2** thêm vào để claim "contact-time-aware scheduling with bounded cooperation overhead".
- **P3** nhỏ nhưng tạo heterogeneity có giá trị cho research claim về utility-aware broadcast.
- **P4** không nên làm trước khi P1–P3 ổn định.
- Không nên cố wiring ns-3 propagation pipeline (SpectrumChannel) vào scenario5 routing layer — thay vào đó, tạo utility layer độc lập và migrate sau khi CC2420 PHY hoàn thiện.
