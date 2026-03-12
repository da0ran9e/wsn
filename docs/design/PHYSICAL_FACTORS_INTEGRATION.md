# Physical Factors Integration for UAV2 Experiments

**Target**: `scenario5` and follow-up UAV2 studies  
**Focus**: add physically meaningful wireless, mobility, sensing, and energy effects without overcomplicating the first conference paper

---

## 1. Purpose

Current scenario logic already has a strong system-level story:
- UAV2 flies over suspicious regions
- UAV2 broadcasts fragments
- ground nodes receive, infer, cooperate, and trigger alerts

What is still simplified is the **physical realism** of delivery:
- who can actually receive a fragment
- how long a node stays in useful coverage
- how fragment size and channel quality affect packet success
- how cooperation traffic interferes with ongoing UAV broadcast

This document proposes a practical path to add these effects into ns-3 in layers.

---

## 2. Design Goal

The objective is **not** to build a full 6G digital twin.

The objective is to replace binary or overly ideal assumptions with a small set of physically meaningful variables that materially affect:
- `T_detect`
- fragment delivery ratio
- cooperation overhead
- useful confidence gain per unit time

The simulation should move from:
- `coverage radius -> fragment received`

to:
- `geometry + channel + packet size + contention + sensing context -> useful fragment delivery`

---

## 3. Recommended Physical Factors

## 3.1. Air-to-Ground Geometry

Model explicitly:
- UAV altitude `h`
- horizontal offset `d_h`
- 3D distance `d_3D = sqrt(d_h^2 + h^2)`
- elevation angle `theta = arctan(h / d_h)`

Why it matters:
- path loss is not constant inside a cell
- elevation angle changes LoS probability
- contact time depends on path geometry, not only cell membership

Minimum implementation:
- compute `d_3D` and `theta` per UAV-ground pair at each transmission event

---

## 3.2. Contact Time Instead of Binary Coverage

A node should not be modeled as simply in-range or out-of-range.

Useful quantity:

$$
T_{contact,i} \approx \frac{2\sqrt{R_{eff}^2 - d_{\perp,i}^2}}{v}
$$

Where:
- `R_eff` is the effective coverage radius under current PHY settings
- `d_perp,i` is node distance to UAV path centerline
- `v` is UAV speed

Why it matters:
- determines how many fragments a node can realistically receive
- directly couples mobility and dissemination quality

Minimum implementation:
- derive contact windows from the waypoint path and UAV speed
- log contact time per node and per cell

---

## 3.3. Air-to-Ground Path Loss, Shadowing, and Fading

The first paper does not need a perfect model, but it should avoid a fixed range assumption.

Recommended hierarchy:

### MVP
- Friis or LogDistance path loss
- log-normal shadowing

### Better urban model
- LoS probability as a function of elevation angle
- separate LoS and NLoS loss
- Nakagami or Rician fading for link variability

Why it matters:
- fragment reception becomes location-dependent
- neighboring nodes may share correlated bad links in urban canyon situations

---

## 3.4. Packet Error as a Function of Fragment Size

Fragment utility should depend on both semantic value and physical deliverability.

Relevant variables:
- fragment size `s_f`
- MCS or data rate
- SNR / SINR
- packet error rate `PER(s_f, SNR)`

Why it matters:
- large fragments may be semantically strong but physically fragile
- small fragments may survive more often but carry less evidence

Minimum implementation:
- use a simplified PER model derived from bit error probability or an empirical threshold curve
- log success probability by fragment class

---

## 3.5. Interference and Cooperation Contention

Realtime cooperation is not free.

Physical effect:
- nodes receiving UAV broadcast may also contend for manifest exchange and fragment relays
- this creates collision, queuing, and half-duplex penalties

Minimum implementation:
- assign separate time windows or TDMA slots for cooperation
- or simulate shared-channel contention explicitly if the PHY stack supports it

Why it matters:
- otherwise cooperation gain will be systematically overestimated

---

## 3.6. Sensing Geometry for Fragment Utility

In this scenario, a fragment is useful only if the node has a compatible local view.

Useful physical variables:
- camera orientation or viewing sector
- target distance to camera
- occlusion probability
- lighting / blur proxy

Why it matters:
- a face-rich fragment is not equally useful to all nodes
- fragment utility should include a local sensing term, not only content category

Minimum implementation:
- approximate each camera by a field-of-view sector
- assign `viewQuality_i` to each node or region
- incorporate `viewQuality_i` into confidence update

---

## 3.7. Propulsion Energy and Hover Trade-offs

UAV transmit energy is usually not the dominant cost.

Important split:
- propulsion energy
- communication energy
- onboard compute energy if modeled later

Why it matters:
- dwell longer improves fragment delivery but burns mission budget
- re-plan and hover decisions should not be treated as free

Minimum implementation:
- track hover time and flight distance separately
- use a coarse propulsion model proportional to flight time and speed regime

---

## 4. Ns-3 Modules That Can Be Reused

## 4.1. `mobility-module`

Use for:
- UAV waypoints
- node positions
- path geometry
- contact-time calculation

Useful classes:
- `WaypointMobilityModel`
- `ConstantPositionMobilityModel`
- mobility trace hooks

Current fit:
- already aligned with the current UAV and ground-node architecture

---

## 4.2. `propagation` module

Use for:
- path loss
- shadowing
- fading
- air-to-ground channel customization

Useful models to inspect or reuse conceptually:
- `FriisPropagationLossModel`
- `LogDistancePropagationLossModel`
- `NakagamiPropagationLossModel`
- chained propagation-loss models

Current fit:
- best place to improve receive realism without rewriting all logic first

---

## 4.3. `spectrum-module`

Use for:
- interference-aware reception
- concurrent signal modeling
- PHY-level power accounting

Why it matters:
- the current CC2420 implementation summary indicates simplified PHY behavior and stubbed interference logic
- this module is the right long-term path if you want stronger RX realism

Current fit:
- medium to high complexity
- not required for MVP, but recommended if the paper later emphasizes contention/interference claims

---

## 4.4. `buildings-module`

Use for:
- urban obstruction
- indoor/outdoor distinction
- LoS/NLoS realism in city-like layouts

Why it matters:
- very useful if the paper claims urban smart-city realism

Current fit:
- good upgrade once basic channel realism is already in place

---

## 4.5. `energy-module`

Use for:
- node battery tracking
- radio energy consumption
- UAV energy abstraction if extended

Current fit:
- already consistent with the current architecture summary
- should be used at least for ground-node radio cost and coarse UAV budget tracking

---

## 4.6. `wifi`, `lr-wpan`, and current CC2420 stack

Use for:
- practical MAC/PHY experimentation path
- comparing richer built-in models against the custom CC2420 implementation

Recommendation:
- keep the current custom CC2420 path for scenario continuity
- borrow ideas or selected behavior from built-in stacks when needed
- do not attempt a full PHY rewrite before the experiment design is stable

---

## 5. Integration Points in Current Codebase

Based on the current implementation summary, the natural hook points are:

### `WaypointMobilityModel` on UAV
Use for:
- contact time
- path-dependent link quality
- dwell time accounting

### `Cc2420Phy`
Add or improve:
- path-loss based receive power
- SNR estimation
- PER decision
- interference aggregation later

### `UavMacCc2420` / broadcast path
Add:
- fragment-size aware transmission timing
- adaptive repeat or FEC hooks
- logging of useful versus failed deliveries

### ground-node receive path
Add:
- per-fragment receive quality
- local sensing quality term
- receive reason logging: success, weak signal, collision, out-of-contact-window

### cooperation scheduling
Add:
- explicit channel occupancy or slot cost for manifest/request/response
- separation between UAV broadcast window and intra-cell exchange window

---

## 6. Recommended Implementation Roadmap

## Phase P1 — Minimal Physical Realism

Add first:
- altitude-aware 3D distance
- log-distance path loss
- shadowing term
- contact time logging
- PER as a function of fragment size and SNR

Expected output:
- physically meaningful fragment reception statistics
- enough realism for a solid conference paper

Complexity:
- low to medium

---

## Phase P2 — Cooperation Cost Realism

Add:
- cooperation slot budget or contention model
- explicit latency for manifest exchange and fragment relay
- shared-channel cost accounting

Expected output:
- more credible comparison between “broadcast only” and “broadcast + cooperation”

Complexity:
- medium

---

## Phase P3 — Sensing-Aware Utility

Add:
- node-specific view quality
- field-of-view approximation
- fragment utility weighted by local observation geometry

Expected output:
- utility function becomes jointly semantic + physical

Complexity:
- medium

---

## Phase P4 — Advanced Urban Realism

Add:
- buildings-based obstruction
- correlated NLoS zones
- stronger interference via spectrum modeling

Expected output:
- stronger urban-realism claims

Complexity:
- high

---

## 7. Suggested Experiment Matrix

For the first paper, vary only a few physical knobs:

### Geometry
- UAV altitude: `50 / 100 / 150 m`
- UAV speed: `10 / 20 / 30 m/s`

### Channel
- shadowing sigma: `0 / 4 / 8 dB`
- fading model: `none / Nakagami`

### Fragmentation
- fragment size: `small / medium / large`
- coding mode: `none / repeat / FEC-light`

### Cooperation
- no cooperation
- delayed cooperation
- realtime cooperation with bounded slot budget

### Sensing
- uniform view quality
- heterogeneous view quality by cell

Core metrics:
- `T_detect`
- fragment reception ratio
- useful fragment delivery ratio
- cooperation overhead
- energy per successful mission

---

## 8. Practical Advice on Scope

Do not try to add all physical factors at once.

Best conference-scope package:
- geometry-aware contact time
- simple air-to-ground channel
- fragment-size dependent PER
- bounded cooperation overhead

That set is already enough to materially improve the credibility of the study.

Leave these for later unless the paper specifically targets PHY/6G reviewers:
- full spectrum interference
- detailed MIMO or beamforming
- full urban building maps
- advanced UAV aerodynamics

---

## 9. What to Claim Carefully

Safe claims:
- physically informed dissemination
- geometry-aware fragment delivery
- contact-time-aware scheduling
- recovery-aware and utility-aware broadcast

Avoid unless fully validated:
- realistic 6G NTN stack
- accurate city-scale radio propagation
- production-grade UAV energy model
- exact computer-vision utility model from real datasets

---

## 10. Deliverables for the Codebase

Recommended near-term deliverables:

1. A configuration block for physical parameters
2. A reusable air-to-ground link-quality function
3. Fragment-size aware receive-success model
4. Logs for contact time, SNR proxy, packet loss reason, and useful delivery
5. One ablation experiment showing how physical realism changes the ranking of UAV2 policies

If these five items are completed, the simulation becomes substantially more defensible for publication.