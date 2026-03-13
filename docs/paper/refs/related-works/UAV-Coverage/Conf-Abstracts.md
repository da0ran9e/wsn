# Cluster B — UAV Coverage (coverage + priority)

## Query A2 (coverage + priority)
("UAV-assisted" OR "aerial base station") AND ("coverage optimization" OR "priority-aware") AND ("target area" OR "hotspot")

## IEEE Xplore

### Scope of this curated list

Only keep papers that are directly useful for the current paper scope:
- UAV coverage, placement, or trajectory decisions for wireless or IoT service areas
- hotspot, priority-aware, delay-aware, or rescue-oriented service logic
- direct value for suspicious-region coverage, cell prioritization, or waypoint utility design

Removed from the raw query results:
- MAC-layer protocol papers without path or coverage contribution
- MEC or task-offloading papers
- vehicular edge computing variants
- IoMT or airborne fog papers centered on compute architecture

## Kept Papers

### 1. Multi-UAV Assisted Network Coverage Optimization for Rescue Operations using Reinforcement Learning
Omar Sami Oubbati;
Hakim Badis;
Abderrezak Rachedi;
Abderrahmane Lakas;
Pascal Lorenz
2023 IEEE 20th Consumer Communications & Networking Conference (CCNC)
Year: 2023 | Conference Paper | Publisher: IEEE | Cited by: 83

Why keep it:
- Strong fit for emergency or hotspot coverage scenarios.
- Multi-UAV rescue coverage is useful for reasoning about suspicious-region support and future scalability.
- Good reference for RL-based coverage control under disrupted infrastructure.

Use for this paper:
- hotspot or rescue-region coverage motivation
- future multi-UAV extension
- coverage-first baseline discussion

### 2. Energy and Service-Priority aware Trajectory Design for UAV-BSs using Double Q-Learning
Sayed Amir Hoseini;
Ayub Bokani;
Jahan Hassan;
Shavbo Salehi;
Salil S. Kanhere
2021 IEEE 18th Annual Consumer Communications & Networking Conference (CCNC)
Year: 2021 | Conference Paper | Publisher: IEEE | Cited by: 21

Why keep it:
- Explicitly combines service priority and UAV trajectory design.
- Very relevant to the idea of prioritizing suspicious cells instead of maximizing raw geometric coverage.
- Good support for discussing trade-offs among service priority, energy, and travel path.

Use for this paper:
- priority-aware coverage discussion
- cell importance weighting
- heuristic versus learning extension framing

### 3. An Optimized LTE-Based Technique for Drone Base Station Dynamic 3D Placement and Resource Allocation in Delay-Sensitive M2M Networks
Ahmed Fahim;
Yasser Gadallah
IEEE Transactions on Mobile Computing
Year: 2023 | Volume: 22, Issue: 2 | Journal Article | Publisher: IEEE | Cited by: 22

Why keep it:
- Dynamic 3D placement is directly relevant to coverage geometry and delay-sensitive service.
- M2M context is closer to IoT sensing than many MEC-heavy papers.
- Useful for arguing that placement and trajectory should be tied to delay-sensitive demand.

Use for this paper:
- altitude or placement discussion
- delay-aware service region design
- geometric baseline support

### 4. Coverage Optimization for Reliable UAV-Assisted 5G/6G Communication Systems
Bilel Ben Saoud;
Leila Nasraoui
IEEE Systems Journal
Year: 2025 | Volume: 19, Issue: 1 | Journal Article | Publisher: IEEE | Cited by: 11

Why keep it:
- Useful as a generic coverage-radius and reliability reference.
- Helps when you need to justify coverage constraints or probabilistic line-of-sight assumptions.
- Not specific to WSN dissemination, but relevant for coverage modeling.

Use for this paper:
- coverage model assumptions
- reliability-aware coverage argument
- support for UAV broadcast radius analysis

### 5. UAV-Assisted Cellular Communication: Joint Trajectory and Coverage Optimization
Yang Li;
Heli Zhang;
Hong Ji;
Xi Li
2021 IEEE Wireless Communications and Networking Conference (WCNC)
Year: 2021 | Conference Paper | Publisher: IEEE | Cited by: 8

Why keep it:
- Useful for the trajectory-versus-coverage trade-off narrative.
- Although cellular-focused, it still speaks to joint optimization instead of fixed path heuristics.
- Can support the idea that coverage-aware motion planning matters even with mobile aerial BSs.

Use for this paper:
- trajectory and coverage coupling
- baseline background paragraph
- joint objective positioning

### 6. Joint Rate and Coverage Design for UAV-Enabled Wireless Networks with Underlaid D2D Communications
Jiansong Miao;
Qiuqian Liao;
Zhenmin Zhao
2020 IEEE 6th International Conference on Computer and Communications (ICCC)
Year: 2020 | Conference Paper | Publisher: IEEE | Cited by: 5

Why keep it:
- Keeps value mainly because it studies coverage extension through cooperative ground-side communication.
- D2D extension is not your main setting, but it is conceptually relevant to intra-cell cooperation after UAV broadcast.
- Useful as a weak supporting reference, not a core citation.

Use for this paper:
- coverage extension through cooperation
- support paragraph for UAV plus ground cooperation
- future extension note

### 7. Coverage Maximization for Heterogeneous Aerial Networks
Daosen Zhai;
Qiqi Shi;
Ruonan Zhang;
Xiao Tang;
Haotong Cao
IEEE Wireless Communications Letters
Year: 2022 | Volume: 11, Issue: 1 | Journal Article | Publisher: IEEE | Cited by: 18

Why keep it:
- Good reference for pure coverage-maximization thinking.
- Useful as a contrast point: maximizing coverage alone is not enough when region urgency is heterogeneous.
- Helps build the gap statement for suspicious-region or confidence-aware planning.

Use for this paper:
- gap statement against pure coverage objectives
- support for weighted coverage discussion
- future multi-layer aerial network context

### 8. A Novel Multi-Agent RL Approach in Priority-Aware UAV-Assisted Networks for AoI and Energy Consumption Minimization
Xiaodan Zhang;
Xiaoying Fu;
Jiansong Miao;
Yushun Yao;
Junsheng Mu
2025 IEEE 101st Vehicular Technology Conference (VTC2025-Spring)
Year: 2025 | Conference Paper | Publisher: IEEE

Why keep it:
- Despite the broader network framing, it directly combines priority-awareness, AoI, and energy.
- Useful for adaptive coverage or service decisions under urgency-sensitive conditions.
- Better treated as a future-oriented supporting reference than a core baseline.

Use for this paper:
- priority-aware freshness discussion
- adaptive policy extension
- future learning-based coverage control

## Priority Ranking for Writing

### Tier 1 — Most relevant
- Multi-UAV Assisted Network Coverage Optimization for Rescue Operations using Reinforcement Learning
- Energy and Service-Priority aware Trajectory Design for UAV-BSs using Double Q-Learning
- An Optimized LTE-Based Technique for Drone Base Station Dynamic 3D Placement and Resource Allocation in Delay-Sensitive M2M Networks

### Tier 2 — Useful support
- Coverage Optimization for Reliable UAV-Assisted 5G/6G Communication Systems
- UAV-Assisted Cellular Communication: Joint Trajectory and Coverage Optimization
- Coverage Maximization for Heterogeneous Aerial Networks
- A Novel Multi-Agent RL Approach in Priority-Aware UAV-Assisted Networks for AoI and Energy Consumption Minimization

### Tier 3 — Weak but still usable support
- Joint Rate and Coverage Design for UAV-Enabled Wireless Networks with Underlaid D2D Communications