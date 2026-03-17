# PHY Recent Updates (2026-03)

**Scope**: recent changes affecting PHY realism, packet acceptance, and PHY-facing logging in the UAV-ground WSN stack.

---

## 1. Summary

Recent work added three important PHY-related improvements:

1. **Geometry-aware air-to-ground propagation**
2. **Contact-window prediction for packet viability**
3. **Fast fading support (Ricean / Rayleigh approximation)**

Together, these changes move the simulation from a mostly static distance-based radio approximation toward a
**packet-level, mobility-aware PHY decision path**.

---

## 2. Files Involved

Primary implementation files:

- `src/wsn/model/propagation/cc2420-spectrum-propagation-loss-model.h`
- `src/wsn/model/propagation/cc2420-spectrum-propagation-loss-model.cc`
- `src/wsn/model/radio/cc2420/cc2420-contact-window-model.h`
- `src/wsn/model/radio/cc2420/cc2420-contact-window-model.cc`
- `src/wsn/model/radio/cc2420/cc2420-phy.cc`
- `src/wsn/model/radio/cc2420/cc2420-mac.cc`
- `src/wsn/examples/scenarios/scenario4/scenario4-api.cc`

Supporting design note:

- `src/wsn/docs/design/doppler-fading-analysis.md`

---

## 3. Propagation Model Upgrades

## 3.1 Position-based RX power API

The propagation model now supports direct evaluation from explicit coordinates instead of requiring only live
`MobilityModel` objects.

Added APIs:

- `CalcRxPowerDbmFromPositions(...)`
- `ComputePathLossDbFromPositions(...)`

This is important because the contact-window logic needs to evaluate **future projected positions** over a packet
transmission interval.

### Previous limitation

Before this change, path loss could only be evaluated from the current mobility state. That made it difficult to ask:

- Will the link still be strong enough **0.5 ms later**?
- Will the UAV-ground link remain above sensitivity until the end of the packet?

### Current behavior

The propagation model now classifies links using geometry:

- **Ground-ground**
- **Air-ground LoS**
- **Air-ground Mixed**
- **Air-ground NLoS**

Classification uses:

- 3D distance
- horizontal distance
- elevation angle
- altitude threshold for airborne vs ground nodes

The path loss still follows a log-distance structure, but the exponent and shadowing sigma depend on the profile.

### Implementation impact (what this changes for the system)

- Enables the contact-window logic to evaluate projected receive power over the full packet airtime using explicit
	coordinates instead of relying on the instantaneous MobilityModel state.
- Allows code outside the live mobility stack (MAC pre-scheduling, planning helpers, offline analyzers) to query
	link quality for hypothetical UAV positions and times.
- Improves determinism of gating decisions: contact-window samples are derived from position->pathloss evaluations,
	not from ephemeral object state that may differ across parallel components.

### Influencing factors (inputs that affect results)

- Transmit power `P_tx` and antenna gains (tx/rx).
- 3D distance between transmitter and receiver and elevation angle.
- Environment/profile selection (LoS / Mixed / NLoS / Ground-ground).
- Frequency and reference path-loss at `d0` (implicit in model calibration).
- Shadowing sample (log-normal) drawn per link and per evaluation context.

### Formulas implemented

- Geometry-driven path loss (log-distance model):

	$$PL(d) = PL(d_0) + 10\,n\,\log_{10}\left(\frac{d}{d_0}\right)$$

	where `n` is the profile-dependent path-loss exponent and `PL(d0)` is the reference loss at distance `d0`.

- Slow fading (shadowing) modeled as additive log-normal term:

	$$X_{shadowing}\sim \mathcal{N}(0,\sigma_{profile}^2)\quad\text{(dB)}$$

- Combined per-sample receive power (dBm):

	$$P_{rx}(d) = P_{tx} - PL(d) - X_{shadowing}$$

	(Fast fading is added later in the full receive path; see §7.)

### Citations and mapping

- Al‑Hourani et al., GLOBECOM 2014 — motivates elevation-angle-based LoS modelling and classification used to choose
	`n`/`\sigma` profiles for air-ground links.
- 3GPP TR 38.901 — provides standardized guidance for path-loss parameter ranges and scenario-dependent calibration.
- Goldsmith (Wireless Communications) — canonical reference for log-distance path-loss and shadowing model.

---

## 3.2 Shadowing profiles

The model already included log-normal shadowing and now uses profile-dependent parameters consistently:

| Profile | Path-loss exponent | Shadowing sigma |
|--------|--------------------|-----------------|
| Ground-ground | 3.2 | 7.0 dB |
| Air-ground LoS | 2.0 | 4.0 dB |
| Air-ground Mixed | 2.5 | 6.0 dB |
| Air-ground NLoS | 3.0 | 8.0 dB |

This gives physically more plausible variability than a single fixed propagation profile.

### Implementation impact (what this changes for the system)

- Profile-dependent shadowing makes some air-ground links significantly more stable (LoS, small \sigma) while others
	become more bursty (NLoS, large \sigma). This changes which receivers pass the contact-window test and therefore
	which nodes are selected as targets during UAV broadcasts.
- Using distinct `\sigma` values per profile also improves reproducibility of experiments when comparing environment
	assumptions (e.g., suburban vs urban maps).

### Influencing factors

- Propagation profile selection (based on elevation angle and geometry).
- Choice of `\sigma` per profile (calibrated from measurements or standards).
- Correlation model: current implementation samples shadowing per link independently (no spatial correlation).

### Formulas implemented

- Shadowing term applied as an additive Gaussian in dB (log-normal power variation):

	$$X_{shadowing} \sim \mathcal{N}(0,\sigma_{profile}^2)\quad\text{(dB)}$$

- Effective received power (combining path loss and shadowing):

	$$P_{rx} = P_{tx} - PL(d) - X_{shadowing}$$

	(This is the sample used by `CalcRxPowerDbmFromPositions` when evaluating a projected position.)

### Citations and mapping

- 3GPP TR 38.901 — recommended shadowing sigma values and profile-dependent parameter guidance.
- Al‑Hourani et al. (2014) — supports different treatment for LoS vs NLoS in low-altitude aerial channels.
- Goldsmith (2005) — background on log-normal shadowing and its interpretation.

### 3.2.1 Mapping to 3GPP recommendations

The profile values above were chosen to be consistent with parametric ranges in 3GPP TR 38.901 (UMi/UMa measurement envelopes) and with low-altitude air-to-ground summaries (Al‑Hourani). The table below shows the doc's working values and their 3GPP-inspired mapping/justification.

| Profile | Working n | Working \sigma | 3GPP-inspired mapping / justification |
|--------|-----------:|---------------:|----------------------------------------|
| Ground-ground | 3.2 | 7.0 dB | Maps to urban/suburban ground scenarios (3GPP UMi/NLoS range), higher shadowing due to street clutter.
| Air-ground LoS | 2.0 | 4.0 dB | Consistent with 3GPP LoS urban-macro/low-altitude LoS exponents (~2.0) and small shadowing (3–4 dB).
| Air-ground Mixed | 2.5 | 6.0 dB | Hybrid between LoS and NLoS; use intermediate exponent and sigma to represent partial obstruction scenarios.
| Air-ground NLoS | 3.0 | 8.0 dB | Aligned with 3GPP NLoS exponent ranges and larger sigma for deep shadowing / cluttered low-altitude NLoS.

Notes:
- 3GPP TR 38.901 provides scenario-specific PL exponents and shadowing sigma ranges (UMi, UMa, RMa). We adapt those ranges conservatively for low-altitude UAV links: LoS ~2.0, NLoS in the 3.0–3.5 range, shadowing sigma range ≈ 3–8 dB depending on environment.
- Use `KFactor` defaults (see §7) informed by measurement summaries (TR36.777 and TR38.901 guidance): LoS high K (Ricean), NLoS K near 0 (Rayleigh-like).
- If you want exact per-scenario 3GPP numbers (UMi LoS, UMi NLoS, UMa LoS/NLoS), I can extract the table entries from TR 38.901 and insert a second detailed mapping table.

---

## 4. Contact Window Model

A new PHY-facing helper, `Cc2420ContactWindowModel`, was added to estimate whether a link stays receivable long enough
for a packet to finish transmission.

### Core idea

A packet should not be forwarded to a receiver if the receiver is about to move out of viable link conditions before
packet completion.

The model evaluates the link over:

- packet airtime
- guard time
- discrete sampling intervals along projected motion

If any projected RX power sample falls below the required threshold, the packet is rejected before normal receive flow.

### Why this matters

This adds a missing physical effect:

- not just **"in range now"**
- but **"will still be receivable for the full packet duration"**

This is especially relevant for UAV broadcast with:

- large packet sizes
- high UAV speed
- edge-of-coverage receivers

### Current usage

The MAC calls `HasContactForPacket(...)` before dispatching to each peer.
If the contact window is insufficient, the transmission to that peer is skipped.

This is logically a PHY-informed gating decision implemented at the MAC dispatch point.

---

## 7. Fast Fading Support

Fast fading has now been added to the propagation model as a per-packet random term.

### Model form

Received power now follows:

$$
P_{rx} = P_{tx} - PL(d) - X_{shadowing} - X_{fast}
$$

Where:

- `PL(d)` = geometry-driven path loss
- `X_shadowing` = log-normal slow fading
- `X_fast` = Ricean/Rayleigh-style fast fading approximation in dB

### Profile-dependent K-factors

The model now exposes attributes:

- `EnableFastFading`
- `KFactorLoS`
- `KFactorMixed`
- `KFactorNLoS`
- `KFactorGround`

Default values:

| Profile | K-factor |
|--------|----------|
| LoS | 15 |
| Mixed | 6 |
| NLoS | 0 |
| Ground-ground | 0 |

Interpretation:

- `K > 0` → Ricean-like fading
- `K = 0` → Rayleigh-like fading

### Approximation used

The implementation uses a Gaussian-in-dB approximation with profile-dependent variance:

$$
\sigma_{fast} \approx \frac{5.57}{\sqrt{1 + K}} \; \text{dB}
$$

This yields:

| Profile | Approx. sigma |
|--------|---------------|
| LoS (K=15) | 1.39 dB |
| Mixed (K=6) | 2.10 dB |
| Rayleigh (K=0) | 5.57 dB |

### Design tradeoff

This is not a full time-correlated Ricean channel simulator.
It is a lightweight per-packet approximation suitable for current ns-3 integration and scenario studies.

---

## 8. Important Design Choice: Contact Prediction Is Still Deterministic

Fast fading is **not** applied to the contact-window prediction path.

That means:

- full PHY receive evaluation uses path loss + shadowing + fast fading
- contact-window prediction uses a more deterministic geometry-based estimate

### Reason

Applying per-sample fast fading directly inside contact prediction would make the gating decision unstable and overly
random, especially for long packets or dense sampling.

The current design intentionally uses:

- deterministic prediction for **link continuity**
- stochastic fading for **actual PHY receive success**

This separation keeps the model practical and interpretable.

---

## 9. What Changed in System Behavior

These PHY-related updates change simulation behavior in several ways:

### 9.1 More realistic air-ground link evaluation

Receivers no longer depend only on static current distance.
Geometry and elevation now affect link profile and path loss.

### 9.2 Mobility now matters during packet transmission

A receiver near the edge of coverage may be rejected because the contact duration is too short, even if the link is
momentarily good at the start of transmission.

### 9.3 Packet success becomes more variable

Fast fading introduces realistic per-packet variation, especially for:

- low-elevation air-ground links
- ground-ground links
- edge-of-threshold receptions

### 9.4 Debugging PHY failures is easier

Scenario output can now explain packet rejection with explicit PHY-facing reasons instead of silent non-delivery.

---

## 10. Current Limitations

Despite these upgrades, the PHY model is still simplified.

Not yet modeled in detail:

- time-correlated fading across adjacent packets
- symbol-level Doppler effects
- frequency-selective fading across channel bandwidth
- full SNR-to-BER tables for CC2420 modulation
- correlated shadowing across nearby receivers
- interference accumulation from simultaneous transmitters

So the current implementation should be viewed as:

- **much better than static range-based delivery**
- but **still lighter than a full waveform-faithful PHY**

---

## 11. Recommended Next PHY Steps

If PHY realism is extended further, the next useful upgrades are:

1. **Time-correlated fast fading** across consecutive packets
2. **Velocity-aware margin** inside `Cc2420ContactWindowModel`
3. **Better BER/PER mapping** for CC2420 OQPSK under fading
4. **Optional correlated fading/shadowing** for nearby ground nodes
5. **Scenario-level counters** for PHY reject causes

---

## 12. Bottom Line

The recent PHY-related changes significantly improve realism without turning the simulator into a full waveform model.

The key advances are:

- geometry-aware propagation
- packet-duration-aware contact gating
- lightweight fast fading

This is a good intermediate design point for UAV-ground dissemination studies where packet success should depend on
**where the UAV is, how fast it moves, how long the packet takes, and how stable the link is**.

---

## References (selected)

- Al‑Hourani, A., Kandeepan, S., & Lardner, S. (2014). Modeling air-to-ground path loss for low altitude platforms. IEEE Global Communications Conference (GLOBECOM), 2014.

	- Use: justification for altitude-dependent LoS probability, mixed/NLoS profiling, and centroid-based coverage reasoning in urban/suburban topologies.

- 3GPP TR 38.901 (2017) — Study on channel model for frequencies from 0.5 to 100 GHz.

	- Use: standardized parametric models for LoS probability, path-loss exponents, and K-factor guidance for air-ground links (see UMi/UMa/LOS/NLOS variants).

- 3GPP TR 36.777 (2017) — Study on enhanced LTE support for aerial vehicles.

	- Use: UAV-specific measurement summaries and parameter recommendations for handover and link stability tradeoffs relevant to contact-window thresholds.

- Goldsmith, A. (2005). Wireless Communications. Cambridge University Press.

	- Use: canonical derivations for log-distance path-loss, shadowing (log-normal) modeling, and Rice/Rayleigh fading basics used in the fast-fading approximation.

- Chvátal, V. (1979). A greedy heuristic for the set-covering problem. Mathematics of Operations Research, 4(3), 233–235.

	- Use: theoretical justification for the greedy coverage step used in waypoint selection (see design doc `UAV2_GreedyMaxCoverageWithCost.md`).

---

Notes:
- If you want full BibTeX entries (DOIs, pages) I can fetch and insert them into `src/wsn/docs/paper/refs/related-works/UAV-Physical/UAV-Physical.bib` or a dedicated `phy-updates.bib`.
- I intentionally referenced standards (3GPP) because they provide concrete parameter recommendations that can be mapped to the profile tables above.

