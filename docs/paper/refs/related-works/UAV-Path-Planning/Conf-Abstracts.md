# Cluster A — UAV Path Planning

### Query A1 (general)
("UAV" OR "drone") AND ("path planning" OR "trajectory optimization") AND ("wireless sensor network" OR "IoT") AND ("latency" OR "mission time")

### Query A2 (coverage + priority)
("UAV-assisted" OR "aerial base station") AND ("coverage optimization" OR "priority-aware") AND ("target area" OR "hotspot")

### Query A3 (adaptive / online)
("online replanning" OR "adaptive trajectory") AND ("UAV") AND ("wireless network")

## IEEE Xplore

### Scope of this curated list

Only keep papers that are directly useful for the current paper scope:
- UAV path planning or deployment in WSN/IoT data collection
- time, latency, AoI, or mission-oriented objectives
- online or adaptive trajectory behavior
- direct value for baseline design, metric design, or gap statement

Removed from the raw query results:
- MEC or task-offloading papers without clear dissemination/path insights
- digital twin, survey, blockchain, federated learning, and DNN assignment papers
- security, jamming, eavesdropping, NOMA, and IRS papers
- marine, satellite, and multi-operator 5G variants
- retracted papers

## Kept Papers

### 1. Offline and Online UAV-Enabled Data Collection in Time-Constrained IoT Networks
Oussama Ghdiri;
Wael Jaafar;
Safwan Alfattani;
Jihene Ben Abderrazak;
Halim Yanikomeroglu
IEEE Transactions on Green Communications and Networking
Year: 2021 | Volume: 5, Issue: 4 | Journal Article | Publisher: IEEE | Cited by: 68

Why keep it:
- Very close to the current problem because it explicitly treats time-constrained IoT data collection.
- Useful for building the baseline narrative: offline planning versus online or adaptive planning.
- Strong citation support, good anchor paper for the related work section.

Use for this paper:
- RQ1 path planning baseline
- motivation for online replanning in UAV2
- comparison point for mission-time objectives

### 2. Holistic Path Planning for Multi-Drone Data Collection
Jonathan Diller;
Peter Hall;
Qi Han
2023 19th International Conference on Distributed Computing in Smart Systems and the Internet of Things (DCOSS-IoT)
Year: 2023 | Conference Paper | Publisher: IEEE | Cited by: 4

Why keep it:
- Directly about path planning for wireless sensor data collection.
- Combines offline planning with online handling of failures and stochastic behavior.
- Useful if you later position UAV2 as robust to network uncertainty.

Use for this paper:
- online strategy reference
- robustness-oriented baseline ideas
- future multi-UAV discussion

### 3. Optimizing UAV-Based Data Collection in IoT Networks With Dynamic Service Time and Buffer-Aware Trajectory Planning
Madhu Donipati;
Ankur Jaiswal;
Abhishek Hazra;
Nabajyoti Mazumdar;
Jagpreet Singh
IEEE Transactions on Network and Service Management
Year: 2025 | Volume: 22, Issue: 2 | Journal Article | Publisher: IEEE | Cited by: 10

Why keep it:
- Strongly relevant to UAV2 execution because it considers dynamic service time, not just geometry.
- Buffer-aware behavior is a useful analogue for fragment backlog, missing-fragment pressure, or cell urgency.
- Good support for arguing against static sequential broadcasting.

Use for this paper:
- service-time-aware scheduling motivation
- waypoint cost function design
- dynamic mission-state modeling

### 4. Flying Path Optimization of Rechargeable UAV for Data Collection in Wireless Sensor Networks
Yuchao Zhu;
Shaowei Wang
IEEE Sensors Letters
Year: 2023 | Volume: 7, Issue: 2 | Journal Article | Publisher: IEEE | Cited by: 25

Why keep it:
- Direct WSN data collection problem with mission-time minimization.
- Energy-constrained flight is relevant even if the current focus is not battery swapping.
- Good baseline for route design under limited endurance.

Use for this paper:
- flight-time constraint discussion
- energy-aware baseline
- route design under endurance limits

### 5. AoI-Optimal Cellular-Connected UAV Trajectory Planning for IoT Data Collection
Amirahmad Chapnevis;
Eyuphan Bulut
2023 IEEE 48th Conference on Local Computer Networks (LCN)
Year: 2023 | Conference Paper | Publisher: IEEE | Cited by: 5

Why keep it:
- Introduces AoI as a timing metric adjacent to mission completion time.
- Useful if you want to argue that pure latency is not the only timing objective.
- Relevant to time-aware visitation order and freshness-sensitive path design.

Use for this paper:
- metric discussion
- timing objective alternatives
- region and time-sensitive waypoint ordering

### 6. Joint Clustering and 3-D UAV Deployment for Delay-Aware UAV-Enabled MTC Data Collection Networks
Lingfeng Shen;
Huanran Zhang;
Ning Wang;
Ying Cui;
Xiang Cheng;
Xiaomin Mu
IEEE Sensors Letters
Year: 2024 | Volume: 8, Issue: 12 | Journal Article | Publisher: IEEE

Why keep it:
- Strong for the deployment side: clustering plus 3-D placement under delay-aware objectives.
- Useful if UAV2 path planning is extended into hotspot or cell prioritization.
- Gives a bridge from geometric deployment to mission-time optimization.

Use for this paper:
- coverage-aware planning discussion
- suspicious-region grouping
- deployment versus trajectory trade-off

### 7. UAV as a Data Ferry for a Sparse Adaptive WSN
Pejman A. Karegar;
Adnan Al-Anbuky
2022 27th Asia Pacific Conference on Communications (APCC)
Year: 2022 | Conference Paper | Publisher: IEEE | Cited by: 5

Why keep it:
- Conceptually close because the UAV is treated as a mobile delivery or data-ferry agent rather than only a compute server.
- Sparse WSN setting is relevant to dissemination efficiency and path-dependent delivery.
- Useful for framing UAV2 as a mobile information carrier in suspicious regions.

Use for this paper:
- conceptual framing
- WSN-grounded motivation
- data-ferry baseline language

### 8. Collaborative Data Acquisition for UAV-Aided IoT Based on Time-Balancing Scheduling
Mingyuan Ren;
Xiuwen Fu;
Pasquale Pace;
Gianluca Aloi;
Giancarlo Fortino
IEEE Internet of Things Journal
Year: 2024 | Volume: 11, Issue: 8 | Journal Article | Publisher: IEEE | Cited by: 29

Why keep it:
- Relevant to low-latency data acquisition in sparse IoT deployments.
- Time-balancing scheduling is a useful analogue for balancing waypoint service across suspicious cells.
- Strong support against purely distance-greedy visitation.

Use for this paper:
- scheduling-aware trajectory discussion
- fairness or time-balancing motivation
- cell-priority service logic

### 9. Low-AoI Data Collection for UAV-Assisted IoT With Dynamic Geohazard Importance Levels
Xiuwen Fu;
Tianle Wang;
Pasquale Pace;
Gianluca Aloi;
Giancarlo Fortino
IEEE Internet of Things Journal
Year: 2025 | Volume: 12, Issue: 11 | Journal Article | Publisher: IEEE | Cited by: 7

Why keep it:
- The dynamic importance concept is close to suspicious-region prioritization in UAV2.
- Helps justify adaptive waypoint ordering when region urgency changes over time.
- The domain is geohazard monitoring, but the optimization logic is transferable.

Use for this paper:
- dynamic priority modeling
- urgency-aware path planning
- hotspot-aware replanning justification

### 10. Green Mobility Management in UAV-Assisted IoT Based on Dueling DQN
Wenqi Liu;
Pengbo Si;
Enchang Sun;
Meng Li;
Chao Fang;
Yanhua Zhang
ICC 2019 - 2019 IEEE International Conference on Communications (ICC)
Year: 2019 | Conference Paper | Publisher: IEEE | Cited by: 19

Why keep it:
- Relevant as a learning-based mobility reference if you later discuss RL-based UAV2 planning.
- Not a primary baseline for the first paper, but useful as future-work support.
- Shows that mobility control in UAV-assisted IoT has already moved beyond static heuristics.

Use for this paper:
- future work section
- heuristic versus learning contrast
- mobility-control extension paragraph

### 11. Deep Reinforcement Learning for Trajectory Path Planning and Distributed Inference in Resource-Constrained UAV Swarms
Marwan Abdou Dhuheir;
Emna Baccour;
Aiman Erbad;
Sinan Sabeeh Al-Obaidi;
Mounir Hamdi
IEEE Internet of Things Journal
Year: 2023 | Volume: 10, Issue: 9 | Journal Article | Publisher: IEEE | Cited by: 55

Why keep it:
- Important because it links trajectory planning with distributed inference, which is structurally close to the broader direction A.
- More relevant for future integrated scope than for the narrow first baseline.
- Good support for arguing that path and inference should not always be optimized independently.

Use for this paper:
- broader systems positioning
- future hybrid direction
- path plus inference co-design discussion

### 12. Energy-Efficient Multi-Agent UAV Path Planning for Green IoT Systems
Md. Najmul Mowla;
Davood Asadi;
Khaled Rabie;
Xingwang Li
2025 IEEE 36th International Symposium on Personal, Indoor and Mobile Radio Communications (PIMRC)
Year: 2025 | Conference Paper | Publisher: IEEE

Why keep it:
- Relevant for large-scale or multi-agent extension, though not core to the current single-UAV2 first paper.
- Good secondary reference for scalability and coordination.
- Supports the argument that path planning must consider dynamic network conditions.

Use for this paper:
- future multi-UAV extension
- scalability note
- dynamic adaptation discussion

## Priority Ranking for Writing

### Tier 1 — Most relevant
- Offline and Online UAV-Enabled Data Collection in Time-Constrained IoT Networks
- Holistic Path Planning for Multi-Drone Data Collection
- Optimizing UAV-Based Data Collection in IoT Networks With Dynamic Service Time and Buffer-Aware Trajectory Planning
- Flying Path Optimization of Rechargeable UAV for Data Collection in Wireless Sensor Networks

### Tier 2 — Useful support
- AoI-Optimal Cellular-Connected UAV Trajectory Planning for IoT Data Collection
- Joint Clustering and 3-D UAV Deployment for Delay-Aware UAV-Enabled MTC Data Collection Networks
- UAV as a Data Ferry for a Sparse Adaptive WSN
- Collaborative Data Acquisition for UAV-Aided IoT Based on Time-Balancing Scheduling
- Low-AoI Data Collection for UAV-Assisted IoT With Dynamic Geohazard Importance Levels

### Tier 3 — Keep for future or hybrid framing
- Green Mobility Management in UAV-Assisted IoT Based on Dueling DQN
- Deep Reinforcement Learning for Trajectory Path Planning and Distributed Inference in Resource-Constrained UAV Swarms
- Energy-Efficient Multi-Agent UAV Path Planning for Green IoT Systems
