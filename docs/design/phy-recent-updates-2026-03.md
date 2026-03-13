# PHY Recent Updates (2026-03)

**Scope**: recent changes affecting PHY realism, packet acceptance, and PHY-facing logging in the UAV-ground WSN stack.

---

## 1. Summary

Recent work added four important PHY-related improvements:

1. **Geometry-aware air-to-ground propagation**
2. **Contact-window prediction for packet viability**
3. **PHY/MAC drop tracing into scenario output**
4. **Fast fading support (Ricean / Rayleigh approximation)**

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
- **Air-ground mixed**
- **Air-ground NLoS**

Classification uses:

- 3D distance
- horizontal distance
- elevation angle
- altitude threshold for airborne vs ground nodes

The path loss still follows a log-distance structure, but the exponent and shadowing sigma depend on the profile.

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

## 5. PHY Drop Tracing

The PHY now emits structured debug trace events for receive-side rejection.

Added receive rejection reasons include:

- `RxDropBelowSensitivity`
- `RxDropBer`

The MAC also emits PHY-adjacent rejection reasons such as:

- `DropPhyReject`
- `DropContactWindow`
- `DropContactWindowSummary`

These events are forwarded through the unified net-device callback path and logged by scenario code into the shared
result stream.

### Why this matters

Previously, a packet might silently fail due to low signal or BER without a clear event in the result file.
Now the scenario has a consistent hook to record why PHY delivery failed.

This is useful for:

- debugging propagation assumptions
- validating contact-window behavior
- explaining why a UAV broadcast did not reach a set of receivers

---

## 6. Aggregated Contact-Drop Logging

To avoid extremely heavy logs during broadcast, contact-window drops are now aggregated per transmission attempt.

### Old behavior

One UAV broadcast to many peers could generate one drop line per destination.
This became noisy and expensive in result files.

### New behavior

The MAC accumulates all destinations dropped due to insufficient contact time and emits a **single summary event**:

- source node
- dropped destination count
- destination list

This keeps PHY-related diagnostics usable in large broadcasts.

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
- structured PHY drop visibility
- lightweight fast fading

This is a good intermediate design point for UAV-ground dissemination studies where packet success should depend on
**where the UAV is, how fast it moves, how long the packet takes, and how stable the link is**.
