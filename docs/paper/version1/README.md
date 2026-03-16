# Paper — Version 1

## Title
**Coverage-Aware UAV Path Planning for Fragment-Based Cooperative Detection
in Smart City IoT Networks**

## Target Venue
IEEE ICCE 2026

---

## How to Compile

### Requirements
- TeX distribution: **TeX Live 2022+** or **MacTeX 2022+**
- Required packages: `IEEEtran`, `amsmath`, `amssymb`, `algorithmic`,
  `algorithm`, `graphicx`, `booktabs`, `url`, `xcolor`, `cite`
  (all included in a standard TeX Live installation)

### Compile
```bash
cd src/wsn/docs/paper/version1
make          # copies IEEEtran.cls + runs pdflatex twice
```
Or manually:
```bash
cp ../template/IEEE_Conference_Template/IEEEtran.cls .
pdflatex paper.tex
pdflatex paper.tex   # second pass to resolve cross-references
```

### Clean
```bash
make clean      # remove auxiliary files
make cleanall   # also remove paper.pdf and IEEEtran.cls
```

---

## File Structure

| File | Description |
|------|-------------|
| `paper.tex` | Main LaTeX source (complete paper, v1 draft) |
| `Makefile` | Compilation helper |
| `README.md` | This file |
| `fig_sysmodel.pdf` | **TODO** — System architecture figure (placeholder) |
| `fig_alpha_sweep.pdf` | **TODO** — Alpha sensitivity plot (placeholder) |

---

## Paper Structure

| Section | Content |
|---------|---------|
| Abstract | 150 words — problem, approach, key result |
| I. Introduction | Motivation, challenges, 4 contributions |
| II. System Model | Grid topology, fragment confidence model, intra-cell cooperation |
| III. Problem Formulation | Coverage-tour minimization objective |
| IV. GMC Algorithm | k-means augmentation, coverage sets, greedy loop, α analysis |
| V. Physical Layer | Log-distance PL, fast fading, contact-window, BER/PER |
| VI. Evaluation | Completion time table, win-rate table, α sweep, discussion |
| VII. Related Work | UAV data mules, submodular orienteering, rateless codes |
| VIII. Conclusion | Summary + future work |
| References | 12 entries |

---

## Key Results (v1 numbers)

| Metric | Value |
|--------|-------|
| UAV1 (Nearest Neighbor) mean completion time | 97.85 s |
| UAV2 (GMC proposed) mean completion time | 17.49 s |
| Reduction | ≈ 82 % |
| GMC win rate (6 configs × 20 seeds) | **100 %** |

---

## TODO Before Submission

- [ ] Fill in author names, affiliations, emails
- [ ] Add funding acknowledgment (if applicable)
- [ ] Generate `fig_sysmodel.pdf` — system architecture diagram
- [ ] Generate `fig_alpha_sweep.pdf` — α sensitivity curve
- [ ] Run more seeds (100 seeds) for tighter confidence intervals
- [ ] Add min/max columns to Table III (timing table)
- [ ] Verify all reference details (volume, pages, year)
- [ ] Proof-read abstract (no symbols, no math — IEEE rule)
- [ ] Check page count (target: 6–8 pages in IEEE format)
- [ ] Remove placeholder figure references if figures not ready
