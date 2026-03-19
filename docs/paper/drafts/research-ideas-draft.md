# Research Ideas Draft

This draft collects short research notes and ideas encountered during development and literature reading. Sections follow a typical conference structure; each section contains a table of notes with columns: `ID | Keyword | Specification | Link`.

---

## Abstract

| ID | Keyword | Specification | Link |
|----|---------|---------------|------|
| A1 | GMC summary | One-sentence claim of Greedy Max-Coverage with Cost performance and % improvement over NN baseline | [paper draft](src/wsn/docs/paper/version1/paper.tex#L1-L120) |
| A2 | PHY realism | Mention CC2420 realistic stack: shadowing, fast-fading, contact-window, BER/PER mapping | [cc2420 error model](src/wsn/model/radio/cc2420/cc2420-error-model.cc#L1-L260) |

---

## Introduction

| ID | Keyword | Specification | Link |
|----|---------|---------------|------|
| I1 | Motivation | Scale of urban IoT & need for fast fragment dissemination | [paper draft](src/wsn/docs/paper/version1/paper.tex#L1-L80) |
| I2 | Broadcast mule | UAV as broadcast ferry vs unicast base station | [related works](src/wsn/docs/paper/refs/related-works/UAV-Path-Planning) |
|..| ..| In the next section, we discuss previous related works. Section III presents the proposed algorithms. Section IV evaluates the proposed algorithm through extensive simulations. Section V concludes this paper and gives some future work suggestions. |..|

---

## Related Work

| ID | Keyword | Specification | Link |
|----|---------|---------------|------|
| R1 | A2G pLoS models | Use logistic elevation‑based pLoS (Al‑Hourani) to classify link profile | [phy crosscheck](src/wsn/docs/design/PHY_THEORY_RESEARCH_CROSSCHECK_2026-03.md#L1-L40) |
| R2 | Antenna pattern | References on 3D antenna effects for UAVs and orientation penalty proxy | [Conf-Abstracts](src/wsn/docs/paper/refs/related-works/UAV-Physical/Conf-Abstracts.md#L130-L140) |

---

## System Model

| ID | Keyword | Specification | Link |
|----|---------|---------------|------|
| S1 | Fragment model | File split into K fragments; partial confidence accumulation model | [paper draft](src/wsn/docs/paper/version1/paper.tex#L100-L170) |
| S2 | Cell leaders | Cell leader aggregation + intra-cell cooperation protocol summary | [paper draft](src/wsn/docs/paper/version1/paper.tex#L170-L220) |

---

## Problem Formulation

| ID | Keyword | Specification | Link |
|----|---------|---------------|------|
| P1 | Coverage constraint | Set-cover tour formulation: cover all suspicious nodes with waypoints | [paper draft](src/wsn/docs/paper/version1/paper.tex#L220-L290) |
| P2 | Objective | Minimize mission completion time (travel + broadcast) | [paper draft](src/wsn/docs/paper/version1/paper.tex#L290-L330) |

---

## Algorithm (GMC)

| ID | Keyword | Specification | Link |
|----|---------|---------------|------|
| G1 | Candidate generation | Nodes ∪ optional k-means centroids; k_max parameter | [UAV2 design](src/wsn/docs/design/UAV2_GreedyMaxCoverageWithCost.md#L1-L120) |
| G2 | Score function | score = gain / (cost^alpha + eps); tie-breakers: gain then distance | [base-station code](src/wsn/model/routing/scenario4/base-station-node/base-station-node.cc#L1-L200) |

---

## Physical Layer

| ID | Keyword | Specification | Link |
|----|---------|---------------|------|
| PHY1 | Path loss profiles | LoS/Mixed/NLoS exponents and sigma table; elevation thresholds | [profiles table](src/wsn/docs/paper/version1/paper.tex#L320-L380) |
| PHY2 | Contact-window | Sample PRx across airtime + guard; reject if any < sensitivity | [contact model](src/wsn/docs/paper/version1/paper.tex#L400-L460) |
| PHY3 | BER→PER | BER via erfc(Eb/N0 with DSSS gain) → PER = 1-(1-BER)^{8L} | [cc2420 error model](src/wsn/model/radio/cc2420/cc2420-error-model.cc#L1-L260) |
| PHY4 | Heading penalty | Lightweight orientation penalty based on tx velocity vs LOS (mismatch * maxDb) | [spectrum model](src/wsn/model/propagation/cc2420-spectrum-propagation-loss-model.cc#L220-L260) |

---

## Implementation / Reproducibility

| ID | Keyword | Specification | Link |
|----|---------|---------------|------|
| IM1 | ns-3 modules | Custom CC2420 radio stack files and helpers | [cc2420 dir](src/wsn/model/radio/cc2420) |
| IM2 | Scripts | Extraction, experiment runner, seeds list | [pdf extractor script](src/wsn/docs/paper/refs/pdf_extract.py) |
| IM3 | Reproducibility note | README + assign streams for RNG `AssignStreams()` usage | [paper reproducibility section](src/wsn/docs/paper/version1/paper.tex#L420-L480) |

---

## Evaluation / Results

| ID | Keyword | Specification | Link |
|----|---------|---------------|------|
| E1 | Metrics | Mission completion time, win rate, PDR, confidence thresholds | [paper draft](src/wsn/docs/paper/version1/paper.tex#L500-L640) |
| E2 | Parameter sweep | Seeds 101–120; alpha sweep; grid sizes | [paper params](src/wsn/docs/paper/version1/paper.tex#L540-L600) |

---

## Discussion / Limitations

| ID | Keyword | Specification | Link |
|----|---------|---------------|------|
| D1 | Fast-fading fidelity | Gaussian-in-dB approx vs Ricean sampling; trade-offs | [phy recent updates](src/wsn/docs/design/phy-recent-updates-2026-03.md#L1-L120) |
| D2 | Shadowing policy | Per-eval vs per-link sampling; spatial correlation missing | [phy crosscheck](src/wsn/docs/design/PHY_THEORY_RESEARCH_CROSSCHECK_2026-03.md#L1-L80) |

---

## Conclusion / Future Work

| ID | Keyword | Specification | Link |
|----|---------|---------------|------|
| F1 | Multi-UAV | Multi-UAV coordination / load balancing as next step | [paper draft](src/wsn/docs/paper/version1/paper.tex#L700-L740) |
| F2 | 3D propagation | Urban canyon 3D models & measured CC2420 calibration | [phy recent updates](src/wsn/docs/design/phy-recent-updates-2026-03.md#L1-L80) |

---

## Appendix / Notes

| ID | Keyword | Specification | Link |
|----|---------|---------------|------|
| APP1 | Code pointers | Quick list of source files used during research (routing, phy, helpers) | [src/wsn/model](src/wsn/model) |
| APP2 | Papers collected | `third-party-papers/sources.bib` and downloaded PDFs | [third-party-papers](third-party-papers) |

---

*End of draft.*
