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

private:
  // SpectrumPropagationLossModel override
  virtual Ptr<SpectrumValue> DoCalcRxPowerSpectralDensity(Ptr<const SpectrumSignalParameters> params,
                                                          Ptr<const MobilityModel> a,
                                                          Ptr<const MobilityModel> b) const;
  virtual int64_t DoAssignStreams(int64_t stream);

  double ComputePathLossDb(Ptr<const MobilityModel> txMobility,
                           Ptr<const MobilityModel> rxMobility) const;

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

  // RNGs for shadowing
  mutable Ptr<NormalRandomVariable> m_shadowingLosRng;
  mutable Ptr<NormalRandomVariable> m_shadowingMixedRng;
  mutable Ptr<NormalRandomVariable> m_shadowingNlosRng;
  mutable Ptr<NormalRandomVariable> m_shadowingGroundGroundRng;
};

} // namespace propagation
} // namespace wsn
} // namespace ns3
