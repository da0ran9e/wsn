# Cluster C — UAV Adaptive / Online Replanning

## Query A3 (adaptive / online)
("online replanning" OR "adaptive trajectory") AND ("UAV") AND ("wireless network")

## IEEE Xplore

### Scope of this curated list

Only keep papers that are directly useful for the current paper scope:
- adaptive or online UAV motion control in wireless or IoT contexts
- papers where trajectory adaptation is central, not incidental
- direct value for feedback-driven replanning, dynamic region priority, or online mission adjustment

Observation from this query:
- This query mostly returns beamforming, spectrum, and resource-allocation papers.
- Most results are not directly useful for the current UAV2 paper, because trajectory is secondary to communication optimization.

## Kept Papers

### 1. Trajectory Design and Beamforming in UAV-Assisted Wireless Networks: A Fine-Tuned M2LLM-Driven DRL-Based Framework
Baolin Yin;
Xuming Fang;
Xianbin Wang;
Li Yan;
Junjie Wu;
Jingyu Wang
IEEE Transactions on Wireless Communications
Year: 2026 | Volume: 25 | Journal Article | Publisher: IEEE

Why keep it:
- It is one of the few results in this query where adaptive trajectory design is genuinely central.
- Useful only as a future-looking reference for environment-aware or model-driven adaptive control.
- Too far from the current first paper to serve as a main baseline.

Use for this paper:
- future work section
- discussion of feedback-driven adaptive control
- evidence that adaptive trajectory is an active research direction

## Removed After Recheck

The remaining raw-query results were removed because they are off-scope for the current paper:
- resource allocation or interference management with pre-defined trajectories
- beamforming or SINR optimization where UAV motion is not the core research object
- multi-UAV communication-control papers without direct relevance to suspicious-region replanning

## Recommendation

This query is not strong for the current paper. Better follow-up query variants would be:

### Better Query C1
("online replanning" OR "adaptive path planning") AND UAV AND ("data collection" OR dissemination OR WSN OR IoT)

### Better Query C2
("feedback-driven" OR "state-aware") AND UAV AND (trajectory OR path planning) AND (IoT OR WSN)

### Better Query C3
("dynamic priority" OR hotspot OR urgency) AND UAV AND (trajectory OR replanning) AND (sensor network OR IoT)