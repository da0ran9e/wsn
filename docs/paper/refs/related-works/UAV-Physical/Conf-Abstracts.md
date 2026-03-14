# Conf-Abstracts — UAV Physical / Air-to-Ground Measurements

Purpose: tập hợp các bài đo kênh và model vật lý (air-to-ground, air-to-air, antenna patterns, PER vs SNR/payload) dùng làm cơ sở cho mô phỏng ns-3.

Status: populated with an initial curated set of measurement and modeling papers (13 entries). Each entry includes citation metadata, a 1‑line relevance note, extracted parameters to consider for ns-3, and a BibTeX record.

Selection criteria:
- Measurement campaigns (air-to-ground / drone-to-ground) prioritized.
- Model papers proposing path-loss / LoS-probability / Doppler models for UAV heights.
- Standard reports (3GPP/ITU) when parameters are derived from standards.
- Papers/datasets with PER/BER vs SNR / payload-size results.

Entries:

1) Title: A Survey of Air-to-Ground Propagation Channel Modeling for Unmanned Aerial Vehicles
   Authors: Wahab Khawaja, Ismail Guvenc, David Matolak, Uwe‑Carsten Fiebig, Nicolas Schneckenberger
   Year: 2018
   Venue / URL: arXiv:1801.01656 — https://arxiv.org/abs/1801.01656
   Why included: comprehensive survey summarizing measurement campaigns and channel models — good starting point for parameter selection.
   Relevant params to extract: summary of path-loss models, LoS criteria, common freq bands.
   BibTeX:
@article{khawaja2018survey,
  title={A Survey of Air-to-Ground Propagation Channel Modeling for Unmanned Aerial Vehicles},
  author={Khawaja, Wahab and Guvenc, Ismail and Matolak, David and Fiebig, Uwe-Carsten and Schneckenberger, Nicolas},
  journal={arXiv preprint arXiv:1801.01656},
  year={2018},
  url={https://arxiv.org/abs/1801.01656},
  doi={10.48550/arXiv.1801.01656}
}

2) Title: 3D Non-Stationary Channel Measurement and Analysis for MaMIMO-UAV Communications
   Authors: Achiel Colpaert, Zhuangzhuang Cui, Evgenii Vinogradov, Sofie Pollin
   Year: 2023
   Venue / URL: arXiv:2310.06579 — https://arxiv.org/abs/2310.06579 (related published DOI: 10.1109/TVT.2023.3340447)
   Why included: CSI / MIMO-focused measurements relevant if using MIMO or massive-MIMO UAV receivers in ns-3.
   Relevant params: 3D channel statistics, angular spreads, stationarity regions.
   BibTeX:
@article{colpaert20233d,
  title={3D Non-Stationary Channel Measurement and Analysis for MaMIMO-UAV Communications},
  author={Colpaert, Achiel and Cui, Zhuangzhuang and Vinogradov, Evgenii and Pollin, Sofie},
  journal={arXiv preprint arXiv:2310.06579},
  year={2023},
  url={https://arxiv.org/abs/2310.06579},
  doi={10.48550/arXiv.2310.06579},
  note={Related (published) DOI: 10.1109/TVT.2023.3340447}
}

3) Title: Measurement-Based Modeling and Analysis of UAV Air-Ground Channels at 1 and 4 GHz
   Authors: Zhuangzhuang Cui, Cesar Briso‑Rodriguez, Ke Guan, Cesar Calvo‑Ramirez, Bo Ai, Zhangdui Zhong
   Year: 2025
   Venue / URL: arXiv:2501.17303 — https://arxiv.org/abs/2501.17303
   Why included: provides measurements at low (1 GHz) and mid (4 GHz) bands — helpful for multi‑band ns-3 scenarios.
   Relevant params: path-loss exponents, shadowing sigma at 1/4 GHz.
   BibTeX:
@article{cui2025measurement,
  title={Measurement-Based Modeling and Analysis of UAV Air-Ground Channels at 1 and 4 GHz},
  author={Cui, Zhuangzhuang and Briso-Rodriguez, Cesar and Guan, Ke and Calvo-Ramirez, Cesar and Ai, Bo and Zhong, Zhangdui},
  journal={arXiv preprint arXiv:2501.17303},
  year={2025},
  url={https://arxiv.org/abs/2501.17303},
  doi={10.48550/arXiv.2501.17303}
}

4) Title: An Empirical Air-to-Ground Channel Model Based on Passive Measurements in LTE
   Authors: Xuesong Cai, José Rodríguez‑Piñeiro, Xuefeng Yin, Bo Ai, Gert F. Pedersen, Antonio Pérez‑Yuste
   Year: 2019 (published), arXiv preprint 2019
   Venue / DOI: IEEE Transactions on Vehicular Technology (published) DOI: 10.1109/TVT.2018.2886961 — arXiv:1901.07930
   Why included: passive LTE-based measurements useful when mapping LTE/NB-IoT-like PHY parameters.
   Relevant params: measured path-loss, LoS/NLoS classification, frequency band info.
   BibTeX:
@article{cai2019empirical,
  title={An Empirical Air-to-Ground Channel Model Based on Passive Measurements in LTE},
  author={Cai, Xuesong and Rodr{\'\i}guez-Pi{\~n}eiro, Jos{\'e} and Yin, Xuefeng and Ai, Bo and Pedersen, Gert F. and P{\'e}rez-Yuste, Antonio},
  journal={arXiv preprint arXiv:1901.07930},
  year={2019},
  url={https://arxiv.org/abs/1901.07930},
  doi={10.1109/TVT.2018.2886961}
}

5) Title: Measurement-based fading characteristics analysis and modeling of UAV to vehicles channel
   Authors: Yue Lyu, Wei Wang, Yuzhe Sun, Ibrahim Rashdan, et al.
   Year: 2023
   Venue / DOI: Vehicular Communications, 45 (2023) 100707 — DOI: 10.1016/j.vehcom.2023.100707
   Why included: air-to-vehicle measurements show fading/temporal characteristics that inform Doppler/SP variability.
   Relevant params: fading distributions, Doppler spreads, environment classification.
   BibTeX:
@article{lyu2023uavveh,
  title={Measurement-based fading characteristics analysis and modeling of UAV to vehicles channel},
  author={Lyu, Yue and Wang, Wei and Sun, Yuzhe and Rashdan, Ibrahim and others},
  journal={Vehicular Communications},
  volume={45},
  pages={100707},
  year={2023},
  doi={10.1016/j.vehcom.2023.100707},
  url={https://doi.org/10.1016/j.vehcom.2023.100707}
}

6) Title: Ultra-Wideband Air-to-Ground Propagation Channel Characterization in an Open Area
   Authors: Wahab Khawaja, Özgür Ozdemir, Fatih Erden, Ismail Guvenc, David Matolak
   Year: 2019
   Venue / URL: arXiv:1906.04013 — https://arxiv.org/abs/1906.04013
   Why included: UWB measurements useful for fine-grained multipath / delay-spread modeling.
   Relevant params: power delay profiles, RMS delay spread, UWB-specific behavior.
   BibTeX:
@article{khawaja2019uwb,
  title={Ultra-Wideband Air-to-Ground Propagation Channel Characterization in an Open Area},
  author={Khawaja, Wahab and Ozdemir, {\"O}zg{\"u}r and Erden, Fatih and Guvenc, Ismail and Matolak, David},
  journal={arXiv preprint arXiv:1906.04013},
  year={2019},
  url={https://arxiv.org/abs/1906.04013},
  doi={10.48550/arXiv.1906.04013}
}

7) Title: UWB Air-to-Ground Propagation Channel Measurements and Modeling using UAVs
   Authors: Wahab Khawaja, Özgür Ozdemir, Fatih Erden, Ismail Guvenc, David Matolak
   Year: 2018
   Venue / URL: arXiv:1812.06603 — https://arxiv.org/abs/1812.06603
   Why included: measurement+model pair for UWB; complements narrowband studies.
   Relevant params: UWB-specific path-loss and multipath characterization.
   BibTeX:
@article{khawaja2018uwb2,
  title={UWB Air-to-Ground Propagation Channel Measurements and Modeling using UAVs},
  author={Khawaja, Wahab and Ozdemir, {\"O}zg{\"u}r and Erden, Fatih and Guvenc, Ismail and Matolak, David},
  journal={arXiv preprint arXiv:1812.06603},
  year={2018},
  url={https://arxiv.org/abs/1812.06603},
  doi={10.48550/arXiv.1812.06603}
}

8) Title: Impact of 3D UWB Antenna Radiation Pattern on Air-to-Ground Drone Connectivity
   Authors: Jianlin Chen, Devin Raye, Wahab Khawaja, Priyanka Sinha, Ismail Guvenc
   Year: 2018
   Venue / URL: arXiv:1810.01442 — https://arxiv.org/abs/1810.01442 (VTC-Fall 2018 related)
   Why included: shows importance of antenna pattern / orientation on link budget — relevant for UAV antenna modeling in ns-3.
   Relevant params: 3D antenna pattern, orientation sensitivity, link budget impact.
   BibTeX:
@inproceedings{chen2018impact,
  title={Impact of 3D UWB Antenna Radiation Pattern on Air-to-Ground Drone Connectivity},
  author={Chen, Jianlin and Raye, Devin and Khawaja, Wahab and Sinha, Priyanka and Guvenc, Ismail},
  booktitle={2018 IEEE VTC-Fall (accepted)},
  year={2018},
  url={https://arxiv.org/abs/1810.01442},
  doi={10.48550/arXiv.1810.01442}
}

9) Title: UAV-Assisted Wireless Communications: An Experimental Analysis of A2G and G2A Channels
   Authors: Kamran Shafafi, Eduardo N. Almeida, André Coelho, Helder Fontes, Manuel Ricardo, Rui Campos
   Year: 2023
   Venue / URL: arXiv:2303.16986 — https://arxiv.org/abs/2303.16986 (book chapter DOI: 10.1007/978-3-031-57523-5_19)
   Why included: experimental A2G/G2A study with practical setups — helpful for validation.
   Relevant params: measurement scenarios, link asymmetry observations.
   BibTeX:
@incollection{shafafi2023uavexp,
  title={UAV-Assisted Wireless Communications: An Experimental Analysis of A2G and G2A Channels},
  author={Shafafi, Kamran and Almeida, Eduardo Nuno and Coelho, Andr{\'e} and Fontes, Helder and Ricardo, Manuel and Campos, Rui},
  booktitle={Proceedings / Lecture Notes (conference/volume)},
  year={2023},
  url={https://arxiv.org/abs/2303.16986},
  doi={10.1007/978-3-031-57523-5_19}
}

10) Title: CSI Measurements and Initial Results for Massive MIMO to UAV Communication
    Authors: Zhuangzhuang Cui, Achiel Colpaert, Sofie Pollin
    Year: 2023
    Venue / URL: arXiv:2312.15188 — https://arxiv.org/abs/2312.15188
    Why included: CSI/MIMO measurements useful when modeling spatial channel correlation and beamforming effects.
    Relevant params: CSI traces, spatial correlation, path dynamics.
    BibTeX:
@inproceedings{cui2023csi,
  title={CSI Measurements and Initial Results for Massive MIMO to UAV Communication},
  author={Cui, Zhuangzhuang and Colpaert, Achiel and Pollin, Sofie},
  booktitle={Asilomar Conference Proceedings / arXiv preprint arXiv:2312.15188},
  year={2023},
  url={https://arxiv.org/abs/2312.15188},
  doi={10.48550/arXiv.2312.15188}
}

11) Title: UWB Channel Sounding and Modeling for UAV Air-to-Ground Propagation Channels
    Authors: Wahab Khawaja, Ismail Guvenc, David Matolak
    Year: 2018
    Venue / URL: arXiv:1805.10379 — https://arxiv.org/abs/1805.10379
    Why included: additional UWB sounding results and models.
    Relevant params: impulse responses, delay profiles.
    BibTeX:
@inproceedings{khawaja2018uwb3,
  title={UWB Channel Sounding and Modeling for UAV Air-to-Ground Propagation Channels},
  author={Khawaja, Wahab and Guvenc, Ismail and Matolak, David},
  booktitle={IEEE Globecom / arXiv preprint arXiv:1805.10379},
  year={2018},
  url={https://arxiv.org/abs/1805.10379},
  doi={10.48550/arXiv.1805.10379}
}

12) Title: Line-of-Sight Probability for Outdoor-to-Indoor UAV-Assisted Emergency Networks
    Authors: Gaurav Duggal, R. Michael Buehrer, Nishith Tripathi, Jeffrey H. Reed
    Year: 2023
    Venue / URL: arXiv:2302.14709 — https://arxiv.org/abs/2302.14709 (IEEE ICC 2023 DOI: 10.1109/ICC45041.2023.10279196)
    Why included: LoS probability modeling for outdoor-to-indoor scenarios, helpful for emergency network assumptions.
    Relevant params: LoS probability vs height/distance, indoor penetration considerations.
    BibTeX:
@inproceedings{duggal2023losprob,
  title={Line-of-Sight Probability for Outdoor-to-Indoor UAV-Assisted Emergency Networks},
  author={Duggal, Gaurav and Buehrer, R. Michael and Tripathi, Nishith and Reed, Jeffrey H.},
  booktitle={IEEE ICC 2023 / arXiv preprint arXiv:2302.14709},
  year={2023},
  url={https://arxiv.org/abs/2302.14709},
  doi={10.1109/ICC45041.2023.10279196}
}

13) Representative note: Path-loss / shadowing UWB and narrowband measurement datasets
    Authors: Various (see entries above: Khawaja, Lyu, Cai)
    Why included: use these entries to pick numeric parameters (exponents, sigma) for ns-3; see the URLs above for measurement tables and example parameter values.

---

If you want more entries (to 15+), or prefer publisher-provided BibTeX formatting for DOI items, I can expand and normalize the BibTeX file next.

---

Extracted parameters for ns-3 (formulas and recommended values)

Below are concise formulas and recommended numeric values/ranges to use when configuring ns-3 experiments. Each item lists a representative source from the curated set above — consult the original paper for alternatives or scenario-specific tables.

1) Path-loss (log-distance + shadowing)
  - Formula: PL(d) = PL(d0) + 10 n log10(d/d0) + X_sigma  (d in meters)
    where PL(d0) = 20 log10(4π d0 / λ) for free-space reference, and X_sigma ~ N(0, σ^2) (dB) models shadowing.
  - Recommended (typical ranges from Cai et al. 2019, Khawaja survey):
    - LoS environments: n ≈ 2.0 (nominal), σ ≈ 2.5–4 dB
    - NLoS / cluttered urban: n ≈ 3.0–4.0, σ ≈ 6–9 dB
  - Notes: choose d0 = 1 m or d0 = 10 m depending on measurement tables; convert frequency via λ = c / f.

2) Line‑of‑Sight probability (p_LOS)
  - Use standardized piecewise models (3GPP TR38.901 style) or the Duggal et al. 2023 formulation.
  - Example (3GPP‑style UMi approximation):
    - p_LOS(d) = 1, for d ≤ 18 m
    - p_LOS(d) = 18/d + exp(−d/63)·(1 − 18/d), for d > 18 m
  - Use p_LOS to select stochastic fade model: Rician (LoS) vs Rayleigh (NLoS). (See Duggal et al. 2023; Khawaja 2018)

3) Small‑scale fading and Rician K‑factor
  - Models: Rayleigh (no direct path) or Rician with K factor (linear ratio or dB).
  - Typical K (LoS UAV links, measured ranges): K ≈ 6–12 dB (use lower end in mobile/urban), source: Khawaja survey, measurement papers.

4) Doppler / coherence time
  - Doppler shift: f_d = (v / λ) cos(θ)
  - Max Doppler (approx): f_d,max = v / λ (take cos(θ)=1 worst-case)
  - Coherence time estimate: T_c ≈ 0.423 / f_D,rms (or ≈ 1 / (2 f_d,max) for rough ordering)
  - Use UAV speed v (m/s) and carrier freq f to compute f_d for fade sampling and channel update interval (see Lyu et al. 2023).

5) PER vs SNR and payload size (fragment calculations)
  - BER→PER (uncoded): PER ≈ 1 − (1 − BER)^{N_bits}, where N_bits = payload_bytes × 8.
  - BER examples (AWGN, BPSK): BER = Q(√(2·Eb/N0)); convert SNR to Eb/N0 by Eb/N0 = SNR * (Bandwidth / bit_rate) * (1 / bits_per_symbol) as appropriate.
  - Recommendation: if using ns-3 PHY with SNR→PER curves, supply measured/lookup FER curves when available; otherwise use the BER→PER formula with modulation‑specific BER.
  - Reference: Khawaja survey and Cai et al. measurements for empiric PER/BER trends.

6) Antenna gain, pattern, and orientation
  - Link budget (dBm): P_r = P_t + G_t(θ_t,ϕ_t) + G_r(θ_r,ϕ_r) − PL(d) − L_misc
  - Model antenna pattern with measured 3D gain maps if available (see Chen et al. 2018); otherwise approximate with a simple sector/cos^n pattern.
  - Account for polarization mismatch loss L_pol when TX/RX polarizations differ.

7) Delay spread & coherence bandwidth (useful for PHY selection)
  - Mean delay τ̄ = Σ p_i τ_i / Σ p_i
  - RMS delay spread τ_rms = sqrt( Σ p_i (τ_i − τ̄)^2 / Σ p_i )
  - Coherence bandwidth (approx): B_c ≈ 1 / (5 τ_rms) (50% correlation) — choose mapping according to target metric.
  - Typical τ_rms (UWB / open area): tens to a few hundreds of ns (see Khawaja UWB papers).

8) MIMO / spatial correlation metrics
  - Channel matrix: y = H x + n; capacity (narrowband): C = log2 det( I + (SNR/Nt) H H^H ).
  - Spatial correlation is a function of angular spread; measure or use published angular spread values (α) from Colpaert et al. 2023 for MaMIMO UAV scenarios.

How to use these in ns-3 (practical checklist):
- Path-loss: implement `LogDistancePropagationLossModel` with `Exponent = n` and `Shadowing` (Normal) using σ from papers.
- LoS selection: implement or reuse a `RandomVariable` rule guided by `p_LOS(d)`; when LoS choose Rician fading with K, else Rayleigh.
- Fading update rate: set channel update interval ≈ fraction of `T_c` computed from UAV speed.
- PER mapping: either load empirical SNR→PER tables into the `ErrorModel` or use a computed BER→PER as fallback.
- Antenna: use `AntennaModel` with gain pattern files if available; else set fixed gains and add orientation-dependent attenuation.

References for parameter choices: use the individual papers listed above (Cai et al. 2019; Khawaja 2018/2019; Lyu 2023; Colpaert/Cui 2023; Duggal 2023; Chen et al. 2018). For standardized LoS probability, consult 3GPP TR38.901.



---

BibTeX

@article{khawaja2018survey,
  title={A Survey of Air-to-Ground Propagation Channel Modeling for Unmanned Aerial Vehicles},
  author={Khawaja, Wahab and Guvenc, Ismail and Matolak, David and Fiebig, Uwe-Carsten and Schneckenberger, Nicolas},
  journal={arXiv preprint arXiv:1801.01656},
  year={2018},
  url={https://arxiv.org/abs/1801.01656},
  doi={10.48550/arXiv.1801.01656}
}

@article{colpaert20233d,
  title={3D Non-Stationary Channel Measurement and Analysis for MaMIMO-UAV Communications},
  author={Colpaert, Achiel and Cui, Zhuangzhuang and Vinogradov, Evgenii and Pollin, Sofie},
  journal={arXiv preprint arXiv:2310.06579},
  year={2023},
  url={https://arxiv.org/abs/2310.06579},
  doi={10.48550/arXiv.2310.06579},
  note={Related (published) DOI: 10.1109/TVT.2023.3340447}
}

@article{cui2025measurement,
  title={Measurement-Based Modeling and Analysis of UAV Air-Ground Channels at 1 and 4 GHz},
  author={Cui, Zhuangzhuang and Briso-Rodriguez, Cesar and Guan, Ke and Calvo-Ramirez, Cesar and Ai, Bo and Zhong, Zhangdui},
  journal={arXiv preprint arXiv:2501.17303},
  year={2025},
  url={https://arxiv.org/abs/2501.17303},
  doi={10.48550/arXiv.2501.17303}
}

@article{cai2019empirical,
  title={An Empirical Air-to-Ground Channel Model Based on Passive Measurements in LTE},
  author={Cai, Xuesong and Rodr{\'\i}guez-Pi{\~n}eiro, Jos{\'e} and Yin, Xuefeng and Ai, Bo and Pedersen, Gert F. and P{\'e}rez-Yuste, Antonio},
  journal={arXiv preprint arXiv:1901.07930},
  year={2019},
  url={https://arxiv.org/abs/1901.07930},
  doi={10.1109/TVT.2018.2886961}
}

@article{lyu2023uavveh,
  title={Measurement-based fading characteristics analysis and modeling of UAV to vehicles channel},
  author={Lyu, Yue and Wang, Wei and Sun, Yuzhe and Rashdan, Ibrahim and others},
  journal={Vehicular Communications},
  volume={45},
  pages={100707},
  year={2023},
  doi={10.1016/j.vehcom.2023.100707},
  url={https://doi.org/10.1016/j.vehcom.2023.100707}
}

@article{khawaja2019uwb,
  title={Ultra-Wideband Air-to-Ground Propagation Channel Characterization in an Open Area},
  author={Khawaja, Wahab and Ozdemir, {\"O}zg{\"u}r and Erden, Fatih and Guvenc, Ismail and Matolak, David},
  journal={arXiv preprint arXiv:1906.04013},
  year={2019},
  url={https://arxiv.org/abs/1906.04013},
  doi={10.48550/arXiv.1906.04013}
}

@article{khawaja2018uwb2,
  title={UWB Air-to-Ground Propagation Channel Measurements and Modeling using UAVs},
  author={Khawaja, Wahab and Ozdemir, {\"O}zg{\"u}r and Erden, Fatih and Guvenc, Ismail and Matolak, David},
  journal={arXiv preprint arXiv:1812.06603},
  year={2018},
  url={https://arxiv.org/abs/1812.06603},
  doi={10.48550/arXiv.1812.06603}
}

@inproceedings{chen2018impact,
  title={Impact of 3D UWB Antenna Radiation Pattern on Air-to-Ground Drone Connectivity},
  author={Chen, Jianlin and Raye, Devin and Khawaja, Wahab and Sinha, Priyanka and Guvenc, Ismail},
  booktitle={2018 IEEE VTC-Fall (accepted)},
  year={2018},
  url={https://arxiv.org/abs/1810.01442},
  doi={10.48550/arXiv.1810.01442}
}

@incollection{shafafi2023uavexp,
  title={UAV-Assisted Wireless Communications: An Experimental Analysis of A2G and G2A Channels},
  author={Shafafi, Kamran and Almeida, Eduardo Nuno and Coelho, Andr{\'e} and Fontes, Helder and Ricardo, Manuel and Campos, Rui},
  booktitle={Proceedings / Lecture Notes (conference/volume)},
  year={2023},
  url={https://arxiv.org/abs/2303.16986},
  doi={10.1007/978-3-031-57523-5_19}
}

@inproceedings{cui2023csi,
  title={CSI Measurements and Initial Results for Massive MIMO to UAV Communication},
  author={Cui, Zhuangzhuang and Colpaert, Achiel and Pollin, Sofie},
  booktitle={Asilomar Conference Proceedings / arXiv preprint arXiv:2312.15188},
  year={2023},
  url={https://arxiv.org/abs/2312.15188},
  doi={10.48550/arXiv.2312.15188}
}

@inproceedings{khawaja2018uwb3,
  title={UWB Channel Sounding and Modeling for UAV Air-to-Ground Propagation Channels},
  author={Khawaja, Wahab and Guvenc, Ismail and Matolak, David},
  booktitle={IEEE Globecom / arXiv preprint arXiv:1805.10379},
  year={2018},
  url={https://arxiv.org/abs/1805.10379},
  doi={10.48550/arXiv.1805.10379}
}

@inproceedings{duggal2023losprob,
  title={Line-of-Sight Probability for Outdoor-to-Indoor UAV-Assisted Emergency Networks},
  author={Duggal, Gaurav and Buehrer, R. Michael and Tripathi, Nishith and Reed, Jeffrey H.},
  booktitle={IEEE ICC 2023 / arXiv preprint arXiv:2302.14709},
  year={2023},
  url={https://arxiv.org/abs/2302.14709},
  doi={10.1109/ICC45041.2023.10279196}
}


