#pragma once

#include "ns3/spectrum-propagation-loss-model.h"
#include "ns3/spectrum-signal-parameters.h"
#include "ns3/random-variable-stream.h"
#include "ns3/vector.h"

namespace ns3 {
namespace wsn {
namespace propagation {

class Cc2420SpectrumPropagationLossModel : public SpectrumPropagationLossModel
{
public:
  static TypeId GetTypeId();

  Cc2420SpectrumPropagationLossModel();
  virtual ~Cc2420SpectrumPropagationLossModel();

  /**
   * Compute received power (dBm) from TX power and two mobility endpoints.
   * This API is used by CC2420 PHY fast-path evaluation (without full Spectrum RX pipeline).
   */
  double CalcRxPowerDbm(double txPowerDbm,
                        Ptr<const MobilityModel> txMobility,
                        Ptr<const MobilityModel> rxMobility) const;

  /**
   * Compute received power (dBm) from TX power and explicit endpoint positions.
   * Used by contact-window prediction to estimate whether the link will remain
   * receivable long enough to finish the current packet.
   */
  double CalcRxPowerDbmFromPositions(double txPowerDbm,
                                     const Vector& txPosition,
                                     const Vector& rxPosition,
                                     bool includeShadowing = false) const;

private:
  // SpectrumPropagationLossModel override
  virtual Ptr<SpectrumValue> DoCalcRxPowerSpectralDensity(Ptr<const SpectrumSignalParameters> params,
                                                          Ptr<const MobilityModel> a,
                                                          Ptr<const MobilityModel> b) const;
  virtual int64_t DoAssignStreams(int64_t stream);

  double ComputePathLossDb(Ptr<const MobilityModel> txMobility,
                           Ptr<const MobilityModel> rxMobility) const;
  double ComputePathLossDbFromPositions(const Vector& txPosition,
                                        const Vector& rxPosition,
                                        bool includeShadowing) const;

  // Configuration attributes
  double m_refDistM;
  double m_refLossDb;
  double m_groundHeightThresholdM;
  double m_pathLossExpGroundGround;
  double m_shadowingSigmaGroundGroundDb;
  double m_pathLossExpLos;
  double m_pathLossExpMixed;
  double m_pathLossExpNlos;
  double m_shadowingSigmaLosDb;
  double m_shadowingSigmaMixedDb;
  double m_shadowingSigmaNlosDb;
  double m_elevLosThreshDeg;
  double m_elevMixedThreshDeg;
  bool m_enableShadowing;

  // Fast fading (Ricean/Rayleigh per elevation profile)
  // σ_K = 5.57 / sqrt(1 + K) dB  (Rayleigh K=0 → 5.57 dB, strong LoS K=15 → 1.39 dB)
  bool m_enableFastFading;
  double m_kFactorLoS;          // Ricean K-factor for LoS links  (linear, not dB)
  double m_kFactorMixed;        // Ricean K-factor for mixed links
  double m_kFactorNLoS;         // K=0 → Rayleigh
  double m_kFactorGround;       // K=0 → Rayleigh for ground–ground

  // RNGs for shadowing
  mutable Ptr<NormalRandomVariable> m_shadowingLosRng;
  mutable Ptr<NormalRandomVariable> m_shadowingMixedRng;
  mutable Ptr<NormalRandomVariable> m_shadowingNlosRng;
  mutable Ptr<NormalRandomVariable> m_shadowingGroundGroundRng;

  // RNGs for fast fading (one per elevation profile, sampled per packet)
  mutable Ptr<NormalRandomVariable> m_fastFadingLosRng;
  mutable Ptr<NormalRandomVariable> m_fastFadingMixedRng;
  mutable Ptr<NormalRandomVariable> m_fastFadingNlosRng;
  mutable Ptr<NormalRandomVariable> m_fastFadingGroundRng;
};

} // namespace propagation
} // namespace wsn
} // namespace ns3
