# 1-Month Project Completion Plan
## UAV-Assisted Real-Time Wanted Person Recognition in Smart City IoT Networks

**Target Venue**: IEEE ICCE 2026  
**Start Date**: 3 March 2026  
**Deadline**: 3 April 2026 (31 days)  
**Plan Created**: 3 March 2026

---

## Executive Summary

**Goal**: Complete paper writing + experimental validation for IEEE ICCE 2026 submission within 1 month.

**Strategy**: 
- Week 1-2: Complete core implementation + baseline experiments
- Week 3: Advanced features + full evaluation
- Week 4: Paper writing + final revisions

**Risk Mitigation**: 
- Focus on Direction A (Distributed AI) as primary contribution
- Keep Directions B/C/D as "future work" if time is limited
- Prepare minimal viable paper (6 pages) with option to expand to 8 pages

---

## Week 1: Foundation & Baseline (3-9 March)

### Day 1-2 (Mon-Tue): Complete Core Implementation
**Status**: Cell Forming (Phase 0) is ✅ COMPLETE

**Tasks**:
- ✅ Cell forming implementation verified (3x3, 5x5 grids working)
- 🔄 Complete Phase 1: UAV Fragment Broadcasting
  - Implement `UavMacCc2420` MAC layer for broadcast
  - Add fragment scheduling algorithm
  - Test UAV circular path with waypoint mobility
- 🔄 Complete Phase 2: Node Reception & Local Detection
  - Implement fragment reception at ground nodes
  - Add Bayesian confidence update model
  - Test threshold detection (θ_low, θ_medium, θ_high)

**Deliverable**: Working baseline scenario (UAV broadcasts, nodes detect)

**Files to Complete**:
- `src/wsn/model/uav/uav-mac-cc2420.{h,cc}`
- `src/wsn/model/uav/fragment-packet.{h,cc}`
- `src/wsn/model/uav/detection-model.{h,cc}`
- `scratch/uav-detection-baseline.cc`

**Validation**: 
- Run simulation: `./ns3 run "uav-detection-baseline --gridSize=3 --simTime=60"`
- Check output: time-to-detection metrics, fragment reception stats

---

### Day 3-4 (Wed-Thu): Phase 3 - Cooperative Detection

**Tasks**:
- Implement intra-cell cooperation protocol
  - Cell Leader coordination
  - Fragment exchange between neighbors
  - Cooperative confidence fusion
- Add Cell Forwarding Tree (CFT) for alert routing to BS
- Test cooperation scenarios (nodes share fragments to reach θ_high faster)

**Deliverable**: Cooperative detection working, alerts routed to BS

**Files to Complete**:
- `src/wsn/model/uav/cooperative-detection.{h,cc}`
- Update `cell-forming.cc` to integrate cooperation
- `scratch/uav-detection-cooperation.cc`

**Validation**:
- Cooperation reduces time-to-detection by 20-30% vs independent detection
- False alarm rate < 5%

---

### Day 5-6 (Fri-Sat): Baseline Experiments & Data Collection

**Tasks**:
- Design baseline experiment matrix:
  - Grid sizes: 3x3, 5x5, 7x7 nodes
  - UAV speeds: 30, 50, 70 m/s
  - Fragment counts: k = 3, 5, 8, 10
  - Detection thresholds: θ_high = 0.8, 0.85, 0.9
- Run all baseline experiments (batch script)
- Collect metrics:
  - Time-to-first-detection (T_detect)
  - Detection probability P_D(t)
  - False alarm rate P_FA
  - Fragment delivery ratio
  - Energy consumption (UAV + ground nodes)

**Deliverable**: Complete baseline dataset (CSV files + trace logs)

**Scripts to Create**:
- `scripts/run-baseline-experiments.sh`
- `scripts/parse-metrics.py` (parse ns-3 traces → CSV)
- `scripts/plot-baseline.py` (matplotlib visualizations)

**Validation**:
- Generate 5-10 key plots for paper (T_detect vs k, P_D(t) curves, etc.)

---

### Day 7 (Sun): Week 1 Review & Buffer

**Tasks**:
- Review all code, fix bugs
- Write technical notes for paper (algorithm descriptions, parameter choices)
- Prepare Week 2 plan
- **Buffer day** for catching up on delays

---

## Week 2: Advanced Features & Direction A (10-16 March)

### Day 8-9 (Mon-Tue): Direction A - 3-Tier Distributed AI

**Tasks**:
- Implement Tier 1 (Node-level): Local CNN inference simulation
  - Model fragment processing time: 50ms (INT8) or 150ms (FP32)
  - Update confidence based on CNN output + fragment content
- Implement Tier 2 (Cell-level): Cell Leader fusion
  - Aggregate confidence from multiple nodes: Bayesian, Voting, Weighted Average
  - Cell-level decision making
- Implement Tier 3 (Cloud-level): BS global fusion
  - Combine alerts from multiple cells
  - Adaptive UAV path adjustment (future enhancement)

**Deliverable**: 3-tier fusion working, performance improvement measured

**Files to Complete**:
- `src/wsn/model/uav/cnn-inference-model.{h,cc}`
- `src/wsn/model/uav/cell-fusion.{h,cc}`
- `src/wsn/model/uav/global-fusion.{h,cc}`
- `scratch/uav-detection-3tier.cc`

**Validation**:
- 3-tier fusion reduces T_detect by 30-40% vs baseline
- Improved accuracy (fewer false alarms)

---

### Day 10-11 (Wed-Thu): Direction A Experiments

**Tasks**:
- Design 3-tier fusion experiment matrix:
  - Fusion methods: Bayesian, Voting, Weighted, Dempster-Shafer
  - Heterogeneous compute: Mix of RPi 4 (fast) and RPi 3 (slow) nodes
  - Network delays: 10ms, 50ms, 100ms cooperation latency
- Run all Direction A experiments
- Collect comparative metrics (baseline vs 3-tier)

**Deliverable**: Direction A dataset complete

**Scripts to Update**:
- `scripts/run-direction-a-experiments.sh`
- `scripts/plot-fusion-comparison.py`

**Validation**:
- Generate comparison plots (T_detect improvement, P_D curves, fusion method trade-offs)

---

### Day 12-13 (Fri-Sat): Sensitivity Analysis & Optimization

**Tasks**:
- Sensitivity analysis:
  - Impact of cell radius (100m, 150m, 200m)
  - Impact of UAV altitude (50m, 100m, 150m)
  - Impact of fragment size (50KB, 100KB, 200KB)
  - Impact of node density (sparse vs dense grids)
- Parameter optimization:
  - Find optimal k (fragment count)
  - Find optimal θ_high (detection threshold)
  - Find optimal UAV speed-altitude trade-off
- Statistical validation:
  - Run each scenario 10 times with different seeds
  - Compute confidence intervals (95% CI)
  - ANOVA for significance testing

**Deliverable**: Optimized parameters + statistical validation results

**Scripts to Create**:
- `scripts/sensitivity-analysis.py`
- `scripts/parameter-optimization.py`
- `scripts/statistical-validation.R` (or Python scipy)

**Validation**:
- Identify optimal configuration for paper's main results
- Ensure results are statistically significant (p < 0.05)

---

### Day 14 (Sun): Week 2 Review & Paper Outline

**Tasks**:
- Review all experimental results
- Create paper outline (IEEE format):
  - I. Introduction
  - II. Related Work
  - III. System Model & Problem Formulation
  - IV. Proposed Algorithm (3-tier fusion)
  - V. Performance Evaluation
  - VI. Conclusion
- Identify key figures for paper (6-8 figures max)
- Write preliminary abstract (200 words)
- **Buffer day** for catching up

---

## Week 3: Full Evaluation & Paper Draft (17-23 March)

### Day 15-16 (Mon-Tue): Related Work Analysis

**Tasks**:
- Literature survey (use `/src/wsn/docs/paper/refs/related-works.md` as starting point):
  - UAV-assisted WSN (10-15 papers)
  - Fragment-based data dissemination (5-10 papers)
  - Cooperative detection in IoT (5-10 papers)
  - Distributed AI for edge computing (10-15 papers)
- Write Related Work section (2 pages):
  - Subsection 1: UAV-assisted surveillance
  - Subsection 2: Fragment-based communication
  - Subsection 3: Cooperative detection
  - Subsection 4: Distributed AI
  - Comparison table: Our work vs state-of-the-art
- Cite 30-40 references (IEEE ICCE typical)

**Deliverable**: Related Work section complete (draft)

**Files to Update**:
- `src/wsn/docs/paper/refs/related-works.md` (expand with paper citations)
- `paper-draft/section-2-related-work.tex` (LaTeX)

**Validation**:
- Identify 3-5 key gaps our work addresses
- Highlight unique contributions (3-tier fusion + fragment-based + real-time)

---

### Day 17-18 (Wed-Thu): System Model & Problem Formulation

**Tasks**:
- Write Section III: System Model (1.5 pages)
  - Network model (ground nodes, UAV, BS)
  - Communication model (CC2420, coverage, propagation)
  - Bayesian detection model (fragment likelihood, confidence update)
  - Cooperative detection model (intra-cell fusion)
- Write Section III: Problem Formulation (1 page)
  - Objective: minimize E[T_detect]
  - Constraints: P_D ≥ α, P_FA ≤ β, energy budget, mission time
  - Decision variables: UAV path, fragment schedule, fusion parameters
- Create key figures:
  - Figure 1: System architecture diagram
  - Figure 2: Fragment-based detection model
  - Figure 3: 3-tier fusion architecture

**Deliverable**: Sections III complete (draft)

**Files to Create**:
- `paper-draft/section-3-system-model.tex`
- `paper-draft/figures/system-architecture.pdf`
- `paper-draft/figures/detection-model.pdf`
- `paper-draft/figures/3tier-fusion.pdf`

**Validation**:
- Ensure mathematical notation is consistent
- All variables defined in notation table

---

### Day 19-20 (Fri-Sat): Algorithm Description & Evaluation

**Tasks**:
- Write Section IV: Proposed Algorithm (2 pages)
  - Algorithm 1: UAV fragment broadcast scheduling
  - Algorithm 2: Node-level detection with Bayesian update
  - Algorithm 3: Cell-level cooperative fusion
  - Algorithm 4: Global-level fusion at BS
  - Complexity analysis: O(n) per node, O(k) per cell
- Write Section V: Performance Evaluation (2.5 pages)
  - Subsection A: Simulation setup (ns-3, parameters table)
  - Subsection B: Baseline performance (T_detect, P_D, P_FA)
  - Subsection C: 3-tier fusion performance (improvement metrics)
  - Subsection D: Sensitivity analysis (parameter impact)
  - Subsection E: Comparison with state-of-the-art (if available)
- Create evaluation figures:
  - Figure 4: T_detect vs k (fragment count)
  - Figure 5: P_D(t) curves (baseline vs 3-tier)
  - Figure 6: Fusion method comparison
  - Figure 7: Sensitivity analysis (heatmap)
  - Figure 8: Energy consumption comparison

**Deliverable**: Sections IV-V complete (draft)

**Files to Create**:
- `paper-draft/section-4-algorithm.tex`
- `paper-draft/section-5-evaluation.tex`
- `paper-draft/figures/results-*.pdf` (5-8 figures)

**Validation**:
- All claims supported by experimental data
- Figures have clear captions and are referenced in text

---

### Day 21 (Sun): Introduction & Conclusion

**Tasks**:
- Write Section I: Introduction (1.5 pages)
  - Motivation: Smart city surveillance, wanted person detection
  - Challenges: Large data size, limited time, unreliable links
  - Key idea: UAV fragment broadcast + cooperative detection + 3-tier fusion
  - Contributions (bullet list):
    1. Novel fragment-based detection model with Bayesian fusion
    2. 3-tier distributed AI architecture for real-time detection
    3. Comprehensive evaluation in ns-3 with 30-40% improvement
  - Paper organization
- Write Section VI: Conclusion (0.5 pages)
  - Summary of contributions
  - Key findings (T_detect improvement, optimal parameters)
  - Future work: Directions B/C/D, multi-UAV, real-world deployment
- Write Abstract (200 words)
  - Problem, approach, key results, conclusion

**Deliverable**: Complete paper draft (6-8 pages)

**Files to Create**:
- `paper-draft/section-1-introduction.tex`
- `paper-draft/section-6-conclusion.tex`
- `paper-draft/abstract.tex`
- `paper-draft/main.tex` (assemble all sections)

**Validation**:
- Compile LaTeX → PDF, check formatting (IEEE conference template)
- Page count: 6-8 pages (ICCE limit)

---

## Week 4: Paper Refinement & Submission (24-31 March)

### Day 22-23 (Mon-Tue): First Draft Review & Revision

**Tasks**:
- Complete first draft compilation
- Self-review checklist:
  - ✅ All sections complete
  - ✅ All figures captioned and referenced
  - ✅ All tables formatted (IEEE style)
  - ✅ All equations numbered and explained
  - ✅ References formatted (IEEEtran.bst)
  - ✅ Grammar check (Grammarly, LanguageTool)
  - ✅ Plagiarism check (Turnitin, iThenticate)
- Identify weak sections for rewriting
- Check figure quality (300 dpi minimum for IEEE)
- Verify all experimental claims with data

**Deliverable**: First complete draft ready for feedback

**Files to Finalize**:
- `paper-draft/main.pdf`
- `paper-draft/references.bib`

**Validation**:
- PDF compiles without errors
- Meets IEEE ICCE formatting requirements

---

### Day 24-25 (Wed-Thu): Revisions & Improvements

**Tasks**:
- Address self-review issues:
  - Rewrite unclear sections
  - Improve figure quality (re-plot if needed)
  - Add missing citations
  - Fix grammar and typos
- Strengthen contributions section
- Add comparison table with related work
- Improve abstract (make it more compelling)
- Polish conclusion (emphasize impact)

**Deliverable**: Revised draft (version 2)

**Validation**:
- Re-read entire paper for flow and clarity
- Ensure logical progression from intro to conclusion

---

### Day 26 (Fri): Experimental Validation Double-Check

**Tasks**:
- Verify all experimental results are reproducible:
  - Re-run key experiments (spot checks)
  - Verify figure data matches CSV files
  - Check statistical significance claims
- Add error bars to all performance plots
- Document all simulation parameters in paper appendix (or supplementary)
- Create reproducibility package:
  - Code: `uav-detection-sim.tar.gz` (ns-3 module + examples)
  - Data: `experimental-results.tar.gz` (CSV files + traces)
  - Scripts: `analysis-scripts.tar.gz` (plotting + statistics)
  - README: `REPRODUCE.md` (step-by-step instructions)

**Deliverable**: Reproducibility package ready

**Files to Create**:
- `paper-draft/supplementary/REPRODUCE.md`
- `paper-draft/supplementary/code-data-scripts.tar.gz`

**Validation**:
- Test reproducibility on clean ns-3 installation
- Ensure all data points in paper can be regenerated

---

### Day 27 (Sat): Co-author Review & Feedback

**Tasks**:
- Share draft with co-authors (if any)
- Request feedback on:
  - Technical accuracy
  - Clarity of presentation
  - Strength of contributions
  - Completeness of evaluation
- Hold review meeting/discussion
- Collect revision suggestions

**Deliverable**: Feedback from co-authors collected

**Buffer**: If working solo, use this day for additional self-review or catch-up

---

### Day 28 (Sun): Incorporate Feedback & Final Revisions

**Tasks**:
- Address all co-author feedback
- Make final revisions:
  - Technical corrections
  - Presentation improvements
  - Add missing details
- Final proofreading
- Check page limit (6-8 pages max)
- Final figure adjustments

**Deliverable**: Camera-ready draft (version 3)

**Validation**:
- All feedback addressed
- Paper reads smoothly from start to finish

---

### Day 29 (Mon): Final Checks & Submission Preparation

**Tasks**:
- IEEE ICCE submission checklist:
  - ✅ PDF compiles (IEEE template)
  - ✅ Author information (names, affiliations, emails)
  - ✅ Copyright form prepared
  - ✅ Conflict of interest statement
  - ✅ Plagiarism check passed (< 15% similarity)
  - ✅ Page count within limit (6-8 pages)
  - ✅ References formatted (IEEE style)
  - ✅ Figures high-resolution (300 dpi)
  - ✅ Supplementary materials ready (if allowed)
- Test PDF on different devices (ensure no rendering issues)
- Create submission package:
  - main.pdf
  - source.zip (LaTeX files + figures)
  - supplementary.zip (code + data, if allowed)

**Deliverable**: Submission-ready package

**Files to Finalize**:
- `submission/icce2026-main.pdf`
- `submission/icce2026-source.zip`
- `submission/icce2026-supplementary.zip`
- `submission/cover-letter.pdf`

**Validation**:
- Final check with IEEE PDF eXpress (if required)
- Ensure file sizes within limits

---

### Day 30 (Tue): Submission Day

**Tasks**:
- Log into IEEE ICCE 2026 submission portal
- Upload all required files:
  - Main paper PDF
  - Source files
  - Supplementary materials (if allowed)
- Fill out submission form:
  - Title, abstract, keywords
  - Authors and affiliations
  - Track selection: "Special Session on Distributed AI across Edge-Cloud Continuum"
  - Conflict of interest
- Submit and receive confirmation email
- Download submission receipt

**Deliverable**: Paper submitted successfully! 🎉

**Post-Submission**:
- Archive all materials: `archive/icce2026-submission-YYYYMMDD.tar.gz`
- Update project status: "SUBMITTED"
- Plan for potential revisions (if accepted with changes)

---

### Day 31 (Wed): Buffer & Future Planning

**Tasks**:
- Review entire 1-month journey
- Document lessons learned
- Plan future work:
  - Direction B (6G NTN): 2-month plan
  - Direction C (Advanced Comms): 2-month plan
  - Direction D (Edge ML): 1.5-month plan
  - Direction E (Hybrid): 4-month plan for IEEE Transactions
- Prepare presentation slides (for conference, if accepted)
- Update GitHub repository with clean code

**Deliverable**: Project archived, future work planned

---

## Contingency Plans

### If Week 1 Delays (Implementation Issues)

**Plan B**: Simplify baseline scenario
- Use simpler detection model (threshold-based without Bayesian)
- Skip energy modeling initially
- Focus on time-to-detection as primary metric
- Add complexity in Week 2 if time permits

### If Week 2 Delays (Direction A Complex)

**Plan C**: Narrow scope to 2-tier fusion
- Skip Tier 3 (global fusion) initially
- Focus on Tier 1 (node) + Tier 2 (cell) only
- Mention Tier 3 as "future work"
- Still shows distributed AI concept

### If Week 3 Delays (Paper Writing Slow)

**Plan D**: Reduce paper length
- Target 6 pages (minimum for ICCE)
- Reduce related work section (1 page instead of 2)
- Combine system model + problem formulation (1.5 pages)
- Shorter evaluation section (focus on key results only)

### If Week 4 Delays (Submission Deadline Tight)

**Plan E**: Rush submission with caveats
- Submit with known minor issues (note in cover letter)
- Plan for "minor revisions" response
- Have revision-ready materials prepared

---

## Key Milestones & Checkpoints

| Milestone | Date | Checkpoint Criteria |
|-----------|------|---------------------|
| **M1: Baseline Working** | 6 Mar | UAV broadcasts, nodes detect, cooperation works |
| **M2: Baseline Data Complete** | 9 Mar | All baseline experiments run, CSV data collected |
| **M3: Direction A Working** | 13 Mar | 3-tier fusion implemented, performance improvement shown |
| **M4: Direction A Data Complete** | 16 Mar | All Direction A experiments run, comparative analysis done |
| **M5: Paper First Draft** | 23 Mar | All sections written, compiled PDF ready |
| **M6: Final Draft Ready** | 30 Mar | All revisions complete, submission-ready PDF |
| **M7: Submission Complete** | 31 Mar | Paper submitted to IEEE ICCE 2026 portal |

---

## Resource Allocation

### Time Budget (31 days)
- Implementation: 7 days (Week 1)
- Experimentation: 7 days (Week 1-2)
- Analysis: 3 days (Week 2)
- Paper Writing: 10 days (Week 3-4)
- Review & Revision: 4 days (Week 4)

### Compute Resources
- Workstation: ns-3 simulations (batch runs)
- RAM: 16 GB minimum (for large grid simulations)
- Storage: 50 GB for traces + results
- Plotting: Python + matplotlib (local)
- LaTeX: Overleaf or local installation

### Reference Materials
- IEEE ICCE 2026 call for papers
- IEEE conference paper template (LaTeX)
- Related works database (30-40 papers)
- ns-3 documentation
- Project docs: `/src/wsn/docs/paper/`

---

## Success Criteria

### Must-Have (P0)
- ✅ Core implementation working (baseline + cooperation)
- ✅ Baseline experiments complete with statistical validation
- ✅ Direction A (3-tier fusion) implemented and evaluated
- ✅ Paper draft complete (6-8 pages, IEEE format)
- ✅ Reproducibility package prepared
- ✅ Submitted to IEEE ICCE 2026 by deadline

### Nice-to-Have (P1)
- Optimal parameter selection via grid search
- Comparison with 2-3 related works (if implementations available)
- High-quality figures (publication-ready)
- Supplementary video demo (simulation visualization)

### Optional (P2)
- Directions B/C/D (mention in future work)
- Real-world dataset integration (if available)
- Multi-UAV extension (for future paper)
- Code release on GitHub (after acceptance)

---

## Daily Time Allocation Template

```
Morning (3 hours):
- Implementation/Experimentation (hands-on coding/simulation)

Afternoon (3 hours):
- Analysis/Writing (data processing, paper writing)

Evening (2 hours):
- Review/Planning (check progress, plan next day)

Weekends:
- Sat: Catch-up + buffer for delays
- Sun: Weekly review + plan next week
```

---

## Risk Assessment & Mitigation

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Implementation bugs | High | High | Daily testing, modular design, unit tests |
| ns-3 build issues | Medium | Medium | Use stable ns-3.46, avoid exotic modules |
| Experiment runtime too long | Medium | Medium | Optimize scenarios, use smaller grids for testing |
| Data analysis bottleneck | Low | Medium | Prepare scripts early, automate plotting |
| Writer's block | Medium | Medium | Use outline, write rough draft first, polish later |
| Co-author delays | Low | High | Start solo, incorporate feedback asynchronously |
| Submission portal issues | Low | High | Submit 1-2 days early, have backups ready |

---

## Next Actions (Immediate)

**Today (3 March, Day 1)**:
1. Review cell forming implementation: `src/wsn/model/uav/cell-forming.cc` ✅
2. Start UAV MAC layer: Create `src/wsn/model/uav/uav-mac-cc2420.{h,cc}`
3. Design fragment packet structure: `src/wsn/model/uav/fragment-packet.h`
4. Test UAV waypoint mobility: `scratch/test-uav-mobility.cc`
5. Set up experiment directory: `experiments/baseline/`

**This Week (3-9 March)**:
- Complete Phase 1-2 implementation (UAV broadcast + node detection)
- Test baseline scenario end-to-end
- Start baseline experiments on Friday

**This Month (March 2026)**:
- Follow weekly plan above
- Track progress in this document (update checkboxes)
- Adjust timeline if needed (use contingency plans)

---

## Progress Tracking

**Update this section daily**:

- [ ] Day 1 (3 Mar): _____
- [ ] Day 2 (4 Mar): _____
- [ ] Day 3 (5 Mar): _____
- [ ] ...
- [ ] Day 31 (31 Mar): _____

**Weekly Status**:
- Week 1: _____
- Week 2: _____
- Week 3: _____
- Week 4: _____

---

## References for Planning

- **Idea**: `/src/wsn/docs/paper/idea.md` (core concept, problem formulation)
- **Implementation**: `/src/wsn/docs/paper/implementation.md` (Phase 0 complete)
- **Directions**: `/src/wsn/docs/paper/DIRECTIONS_SUMMARY.md` (5 development paths)
- **Related Works**: `/src/wsn/docs/paper/refs/related-works.md` (8 key areas)
- **Conference**: `/src/wsn/docs/paper/conference-scope.md` (IEEE ICCE 2026 tracks)
- **Quick Guide**: `/src/wsn/docs/paper/IMPLEMENTATION_QUICK_GUIDE.md` (coding reference)

---

## Contact & Support

If you need help during this 1-month sprint:
- **ns-3**: ns-3-users mailing list, official documentation
- **Research**: Co-authors, advisors, research group meetings
- **Writing**: IEEE Author Center, Grammarly, academic writing resources
- **LaTeX**: Overleaf templates, TeX Stack Exchange

---

**Good luck! Let's make this happen in 31 days! 🚀**
