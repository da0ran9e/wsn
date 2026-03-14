#include "cc2420-spectrum-propagation-loss-model.h"

#include "ns3/spectrum-value.h"
#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/type-id.h"

#include <cmath>
#include <algorithm>

namespace ns3 {
namespace wsn {
namespace propagation {

NS_LOG_COMPONENT_DEFINE("Cc2420SpectrumPropagationLossModel");

TypeId
Cc2420SpectrumPropagationLossModel::GetTypeId()
{
  static TypeId tid = TypeId("ns3::wsn::propagation::Cc2420SpectrumPropagationLossModel")
                            .SetParent<SpectrumPropagationLossModel>()
                            .SetGroupName("Cc2420")
                            .AddConstructor<Cc2420SpectrumPropagationLossModel>()
                            .AddAttribute("PathLossReferenceDistance",
                                          "Reference distance d0 for log-distance model (m)",
                                          DoubleValue(1.0),
                                          MakeDoubleAccessor(&Cc2420SpectrumPropagationLossModel::m_refDistM),
                                          MakeDoubleChecker<double>(0.1))
                            .AddAttribute("PathLossReferenceLoss",
                                          "Reference path loss PL0 at d0 (dB)",
                                          DoubleValue(40.05),
                                          MakeDoubleAccessor(&Cc2420SpectrumPropagationLossModel::m_refLossDb),
                                          MakeDoubleChecker<double>(0.0))
                            .AddAttribute("GroundHeightThreshold",
                                          "Altitude threshold (m) to classify node as airborne",
                                          DoubleValue(2.0),
                                          MakeDoubleAccessor(&Cc2420SpectrumPropagationLossModel::m_groundHeightThresholdM),
                                          MakeDoubleChecker<double>(0.0))
                            .AddAttribute("PathLossExponentGroundGround",
                                          "Path loss exponent for ground-ground links",
                                          DoubleValue(3.2),
                                          MakeDoubleAccessor(&Cc2420SpectrumPropagationLossModel::m_pathLossExpGroundGround),
                                          MakeDoubleChecker<double>(1.0))
                            .AddAttribute("ShadowingSigmaGroundGround",
                                          "Shadowing sigma for ground-ground links (dB)",
                                          DoubleValue(7.0),
                                          MakeDoubleAccessor(&Cc2420SpectrumPropagationLossModel::m_shadowingSigmaGroundGroundDb),
                                          MakeDoubleChecker<double>(0.0))
                            .AddAttribute("PathLossExponentLos",
                                          "Path loss exponent for air-ground high-elevation LoS profile",
                                          DoubleValue(2.0),
                                          MakeDoubleAccessor(&Cc2420SpectrumPropagationLossModel::m_pathLossExpLos),
                                          MakeDoubleChecker<double>(1.0))
                            .AddAttribute("PathLossExponentMixed",
                                          "Path loss exponent for air-ground mixed profile",
                                          DoubleValue(2.5),
                                          MakeDoubleAccessor(&Cc2420SpectrumPropagationLossModel::m_pathLossExpMixed),
                                          MakeDoubleChecker<double>(1.0))
                            .AddAttribute("PathLossExponentNlos",
                                          "Path loss exponent for air-ground low-elevation NLoS profile",
                                          DoubleValue(3.0),
                                          MakeDoubleAccessor(&Cc2420SpectrumPropagationLossModel::m_pathLossExpNlos),
                                          MakeDoubleChecker<double>(1.0))
                            .AddAttribute("ShadowingSigmaLos",
                                          "Shadowing sigma for LoS profile (dB)",
                                          DoubleValue(4.0),
                                          MakeDoubleAccessor(&Cc2420SpectrumPropagationLossModel::m_shadowingSigmaLosDb),
                                          MakeDoubleChecker<double>(0.0))
                            .AddAttribute("ShadowingSigmaMixed",
                                          "Shadowing sigma for mixed profile (dB)",
                                          DoubleValue(6.0),
                                          MakeDoubleAccessor(&Cc2420SpectrumPropagationLossModel::m_shadowingSigmaMixedDb),
                                          MakeDoubleChecker<double>(0.0))
                            .AddAttribute("ShadowingSigmaNlos",
                                          "Shadowing sigma for NLoS profile (dB)",
                                          DoubleValue(8.0),
                                          MakeDoubleAccessor(&Cc2420SpectrumPropagationLossModel::m_shadowingSigmaNlosDb),
                                          MakeDoubleChecker<double>(0.0))
                            .AddAttribute("ElevationLosThreshold",
                                          "Elevation threshold for LoS profile (deg)",
                                          DoubleValue(40.0),
                                          MakeDoubleAccessor(&Cc2420SpectrumPropagationLossModel::m_elevLosThreshDeg),
                                          MakeDoubleChecker<double>(0.0, 90.0))
                            .AddAttribute("ElevationMixedThreshold",
                                          "Elevation threshold for mixed profile (deg)",
                                          DoubleValue(20.0),
                                          MakeDoubleAccessor(&Cc2420SpectrumPropagationLossModel::m_elevMixedThreshDeg),
                                          MakeDoubleChecker<double>(0.0, 90.0))
                            .AddAttribute("EnableStochasticLos",
                                          "Enable stochastic LoS selection using pLoS(elevation).",
                                          BooleanValue(false),
                                          MakeBooleanAccessor(&Cc2420SpectrumPropagationLossModel::m_enableStochasticLos),
                                          MakeBooleanChecker())
                            .AddAttribute("LosProbabilityA",
                                          "Logistic pLoS parameter 'a' for pLoS(theta)=1/(1+a*exp(-b*(theta-a))).",
                                          DoubleValue(9.61),
                                          MakeDoubleAccessor(&Cc2420SpectrumPropagationLossModel::m_losProbA),
                                          MakeDoubleChecker<double>(1e-6))
                            .AddAttribute("LosProbabilityB",
                                          "Logistic pLoS parameter 'b' for pLoS(theta)=1/(1+a*exp(-b*(theta-a))).",
                                          DoubleValue(0.16),
                                          MakeDoubleAccessor(&Cc2420SpectrumPropagationLossModel::m_losProbB),
                                          MakeDoubleChecker<double>(1e-6))
                            .AddAttribute("EnableShadowing",
                                          "Enable log-normal shadowing term",
                                          BooleanValue(true),
                                          MakeBooleanAccessor(&Cc2420SpectrumPropagationLossModel::m_enableShadowing),
                                          MakeBooleanChecker())
                            .AddAttribute("EnableFastFading",
                                          "Enable per-packet Ricean/Rayleigh fast fading term",
                                          BooleanValue(true),
                                          MakeBooleanAccessor(&Cc2420SpectrumPropagationLossModel::m_enableFastFading),
                                          MakeBooleanChecker())
                            .AddAttribute("KFactorLoS",
                                          "Ricean K-factor (linear) for air-ground LoS links (elev > threshold)",
                                          DoubleValue(15.0),
                                          MakeDoubleAccessor(&Cc2420SpectrumPropagationLossModel::m_kFactorLoS),
                                          MakeDoubleChecker<double>(0.0))
                            .AddAttribute("KFactorMixed",
                                          "Ricean K-factor (linear) for air-ground mixed links",
                                          DoubleValue(6.0),
                                          MakeDoubleAccessor(&Cc2420SpectrumPropagationLossModel::m_kFactorMixed),
                                          MakeDoubleChecker<double>(0.0))
                            .AddAttribute("KFactorNLoS",
                                          "Ricean K-factor (linear) for NLoS links; 0 = Rayleigh",
                                          DoubleValue(0.0),
                                          MakeDoubleAccessor(&Cc2420SpectrumPropagationLossModel::m_kFactorNLoS),
                                          MakeDoubleChecker<double>(0.0))
                            .AddAttribute("KFactorGround",
                                          "Ricean K-factor (linear) for ground-ground links; 0 = Rayleigh",
                                          DoubleValue(0.0),
                                          MakeDoubleAccessor(&Cc2420SpectrumPropagationLossModel::m_kFactorGround),
                                          MakeDoubleChecker<double>(0.0))
                            .AddAttribute("EnableHeadingPenalty",
                                          "Enable lightweight heading-mismatch penalty (proxy for orientation effects).",
                                          BooleanValue(false),
                                          MakeBooleanAccessor(&Cc2420SpectrumPropagationLossModel::m_enableHeadingPenalty),
                                          MakeBooleanChecker())
                            .AddAttribute("HeadingPenaltyMaxDb",
                                          "Maximum additional loss for severe heading mismatch (dB).",
                                          DoubleValue(3.0),
                                          MakeDoubleAccessor(&Cc2420SpectrumPropagationLossModel::m_headingPenaltyMaxDb),
                                          MakeDoubleChecker<double>(0.0))
                            .AddAttribute("HeadingPenaltyMinSpeedMps",
                                          "Minimum TX speed to activate heading penalty (m/s).",
                                          DoubleValue(0.5),
                                          MakeDoubleAccessor(&Cc2420SpectrumPropagationLossModel::m_headingPenaltyMinSpeedMps),
                                          MakeDoubleChecker<double>(0.0));
  return tid;
}

Cc2420SpectrumPropagationLossModel::Cc2420SpectrumPropagationLossModel()
    : m_refDistM(1.0),
      m_refLossDb(40.05),
  m_groundHeightThresholdM(2.0),
  m_pathLossExpGroundGround(3.2),
  m_shadowingSigmaGroundGroundDb(7.0),
      m_pathLossExpLos(2.0),
      m_pathLossExpMixed(2.5),
      m_pathLossExpNlos(3.0),
      m_shadowingSigmaLosDb(4.0),
      m_shadowingSigmaMixedDb(6.0),
      m_shadowingSigmaNlosDb(8.0),
      m_elevLosThreshDeg(40.0),
      m_elevMixedThreshDeg(20.0),
      m_enableStochasticLos(false),
      m_losProbA(9.61),
      m_losProbB(0.16),
      m_enableShadowing(true),
      m_enableFastFading(true),
      m_kFactorLoS(15.0),
      m_kFactorMixed(6.0),
      m_kFactorNLoS(0.0),
      m_kFactorGround(0.0),
      m_enableHeadingPenalty(false),
      m_headingPenaltyMaxDb(3.0),
      m_headingPenaltyMinSpeedMps(0.5)
{
  m_shadowingLosRng = CreateObject<NormalRandomVariable>();
  m_shadowingMixedRng = CreateObject<NormalRandomVariable>();
  m_shadowingNlosRng = CreateObject<NormalRandomVariable>();
  m_shadowingGroundGroundRng = CreateObject<NormalRandomVariable>();
  m_losSelectorRng = CreateObject<UniformRandomVariable>();

  m_shadowingLosRng->SetAttribute("Mean", DoubleValue(0.0));
  m_shadowingMixedRng->SetAttribute("Mean", DoubleValue(0.0));
  m_shadowingNlosRng->SetAttribute("Mean", DoubleValue(0.0));
  m_shadowingGroundGroundRng->SetAttribute("Mean", DoubleValue(0.0));

  m_shadowingLosRng->SetAttribute("Variance", DoubleValue(m_shadowingSigmaLosDb * m_shadowingSigmaLosDb));
  m_shadowingMixedRng->SetAttribute("Variance", DoubleValue(m_shadowingSigmaMixedDb * m_shadowingSigmaMixedDb));
  m_shadowingNlosRng->SetAttribute("Variance", DoubleValue(m_shadowingSigmaNlosDb * m_shadowingSigmaNlosDb));
  m_shadowingGroundGroundRng->SetAttribute("Variance", DoubleValue(m_shadowingSigmaGroundGroundDb * m_shadowingSigmaGroundGroundDb));

  // Fast fading RNGs — variance is set per-call based on K-factor.
  m_fastFadingLosRng    = CreateObject<NormalRandomVariable>();
  m_fastFadingMixedRng  = CreateObject<NormalRandomVariable>();
  m_fastFadingNlosRng   = CreateObject<NormalRandomVariable>();
  m_fastFadingGroundRng = CreateObject<NormalRandomVariable>();

  m_fastFadingLosRng->SetAttribute("Mean",    DoubleValue(0.0));
  m_fastFadingMixedRng->SetAttribute("Mean",  DoubleValue(0.0));
  m_fastFadingNlosRng->SetAttribute("Mean",   DoubleValue(0.0));
  m_fastFadingGroundRng->SetAttribute("Mean", DoubleValue(0.0));
}

Cc2420SpectrumPropagationLossModel::~Cc2420SpectrumPropagationLossModel()
{
}

double
Cc2420SpectrumPropagationLossModel::ComputePathLossDb(Ptr<const MobilityModel> txMobility,
                                                       Ptr<const MobilityModel> rxMobility) const
{
  if (!txMobility || !rxMobility)
  {
    return 1e9;
  }

  const Vector txPos = txMobility->GetPosition();
  const Vector rxPos = rxMobility->GetPosition();

  double pathLossDb = ComputePathLossDbFromPositions(txPos, rxPos, true);

  // Optional heading penalty (lightweight proxy, not full 3D antenna model):
  // adds loss when TX velocity vector points away from LOS direction.
  if (m_enableHeadingPenalty)
  {
    const Vector txVel = txMobility->GetVelocity();
    const double txSpeed = std::sqrt(txVel.x * txVel.x + txVel.y * txVel.y + txVel.z * txVel.z);
    if (txSpeed >= m_headingPenaltyMinSpeedMps)
    {
      const Vector los(rxPos.x - txPos.x, rxPos.y - txPos.y, rxPos.z - txPos.z);
      const double losNorm = std::sqrt(los.x * los.x + los.y * los.y + los.z * los.z);
      if (losNorm > 1e-9)
      {
        const double cosPsi = (txVel.x * los.x + txVel.y * los.y + txVel.z * los.z) / (txSpeed * losNorm);
        const double cosClamped = std::max(-1.0, std::min(1.0, cosPsi));
        const double mismatch = (1.0 - cosClamped) * 0.5; // 0: aligned, 1: opposite
        pathLossDb += mismatch * m_headingPenaltyMaxDb;
      }
    }
  }

  return pathLossDb;
}

double
Cc2420SpectrumPropagationLossModel::ComputePathLossDbFromPositions(const Vector& txPos,
                                                                   const Vector& rxPos,
                                                                   bool includeShadowing) const
{

  const double dx = txPos.x - rxPos.x;
  const double dy = txPos.y - rxPos.y;
  const double dz = txPos.z - rxPos.z;
  const double horizontalDistance = std::sqrt(dx * dx + dy * dy);
  const double distance3D = std::sqrt(horizontalDistance * horizontalDistance + dz * dz);
  const double distanceForLoss = std::max(m_refDistM, distance3D);

  const bool txAirborne = std::abs(txPos.z) > m_groundHeightThresholdM;
  const bool rxAirborne = std::abs(rxPos.z) > m_groundHeightThresholdM;
  const bool isGroundGround = !txAirborne && !rxAirborne;

  const double kRadToDeg = 180.0 / std::acos(-1.0);
  const double elevDeg =
      (horizontalDistance > 1e-9)
          ? (std::atan2(std::abs(dz), horizontalDistance) * kRadToDeg)
          : 90.0;

  enum class LinkProfile
  {
    GROUND,
    LOS,
    MIXED,
    NLOS
  };

  LinkProfile profile = LinkProfile::NLOS;
  if (isGroundGround)
  {
    profile = LinkProfile::GROUND;
  }
  else
  {
    const bool deterministicLos = (elevDeg >= m_elevLosThreshDeg);
    const bool deterministicMixed = (elevDeg >= m_elevMixedThreshDeg);

    if (m_enableStochasticLos && m_losSelectorRng)
    {
      // Research-inspired A2G logistic LoS probability model (angle-based form):
      // pLoS(theta)=1/(1+a*exp(-b*(theta-a))).
      const double pLosRaw = 1.0 / (1.0 + m_losProbA * std::exp(-m_losProbB * (elevDeg - m_losProbA)));
      const double pLos = std::max(0.0, std::min(1.0, pLosRaw));

      if (m_losSelectorRng->GetValue() < pLos)
      {
        profile = LinkProfile::LOS;
      }
      else
      {
        profile = deterministicMixed ? LinkProfile::MIXED : LinkProfile::NLOS;
      }
    }
    else
    {
      if (deterministicLos)
      {
        profile = LinkProfile::LOS;
      }
      else if (deterministicMixed)
      {
        profile = LinkProfile::MIXED;
      }
      else
      {
        profile = LinkProfile::NLOS;
      }
    }
  }

  double pathLossExponent = m_pathLossExpNlos;
  Ptr<NormalRandomVariable> shadowingRng = m_shadowingNlosRng;
  double sigmaDb = m_shadowingSigmaNlosDb;

  switch (profile)
  {
  case LinkProfile::GROUND:
    pathLossExponent = m_pathLossExpGroundGround;
    shadowingRng = m_shadowingGroundGroundRng;
    sigmaDb = m_shadowingSigmaGroundGroundDb;
    break;
  case LinkProfile::LOS:
    pathLossExponent = m_pathLossExpLos;
    shadowingRng = m_shadowingLosRng;
    sigmaDb = m_shadowingSigmaLosDb;
    break;
  case LinkProfile::MIXED:
    pathLossExponent = m_pathLossExpMixed;
    shadowingRng = m_shadowingMixedRng;
    sigmaDb = m_shadowingSigmaMixedDb;
    break;
  case LinkProfile::NLOS:
  default:
    pathLossExponent = m_pathLossExpNlos;
    shadowingRng = m_shadowingNlosRng;
    sigmaDb = m_shadowingSigmaNlosDb;
    break;
  }

  double shadowingDb = 0.0;
  if (includeShadowing && m_enableShadowing && shadowingRng)
  {
    shadowingRng->SetAttribute("Variance", DoubleValue(sigmaDb * sigmaDb));
    shadowingDb = shadowingRng->GetValue();
  }

  // Fast fading — Ricean (K > 0) or Rayleigh (K = 0) modelled as Gaussian in dB.
  // σ_K = 5.57 / sqrt(1 + K):  K=0  → 5.57 dB (Rayleigh),
  //                              K=6  → 2.10 dB (partial LoS),
  //                              K=15 → 1.39 dB (strong LoS).
  // Not applied on the contact-window prediction path (includeShadowing == false).
  double fastFadingDb = 0.0;
  if (includeShadowing && m_enableFastFading)
  {
    const double kFactor = (profile == LinkProfile::GROUND) ? m_kFactorGround
               : (profile == LinkProfile::LOS)    ? m_kFactorLoS
               : (profile == LinkProfile::MIXED)  ? m_kFactorMixed
                                 : m_kFactorNLoS;

    Ptr<NormalRandomVariable> fastRng = (profile == LinkProfile::GROUND) ? m_fastFadingGroundRng
                      : (profile == LinkProfile::LOS)    ? m_fastFadingLosRng
                      : (profile == LinkProfile::MIXED)  ? m_fastFadingMixedRng
                                        : m_fastFadingNlosRng;

    if (fastRng)
    {
      const double sigmaFastDb = 5.57 / std::sqrt(1.0 + kFactor);
      fastRng->SetAttribute("Variance", DoubleValue(sigmaFastDb * sigmaFastDb));
      fastFadingDb = fastRng->GetValue();
    }
  }

  return m_refLossDb + 10.0 * pathLossExponent * std::log10(distanceForLoss / m_refDistM) +
         shadowingDb + fastFadingDb;
}

double
Cc2420SpectrumPropagationLossModel::CalcRxPowerDbm(double txPowerDbm,
                                                    Ptr<const MobilityModel> txMobility,
                                                    Ptr<const MobilityModel> rxMobility) const
{
  const double pathLossDb = ComputePathLossDb(txMobility, rxMobility);
  if (pathLossDb > 1e8)
  {
    return -1e9;
  }
  return txPowerDbm - pathLossDb;
}

double
Cc2420SpectrumPropagationLossModel::CalcRxPowerDbmFromPositions(double txPowerDbm,
                                                                const Vector& txPosition,
                                                                const Vector& rxPosition,
                                                                bool includeShadowing) const
{
  const double pathLossDb = ComputePathLossDbFromPositions(txPosition, rxPosition, includeShadowing);
  if (pathLossDb > 1e8)
  {
    return -1e9;
  }
  return txPowerDbm - pathLossDb;
}

Ptr<SpectrumValue>
Cc2420SpectrumPropagationLossModel::DoCalcRxPowerSpectralDensity(Ptr<const SpectrumSignalParameters> params,
                                                                  Ptr<const MobilityModel> a,
                                                                  Ptr<const MobilityModel> b) const
{
  if (!params)
  {
    return nullptr;
  }

  Ptr<const SpectrumValue> txPsd = params->psd;
  if (!txPsd)
  {
    return nullptr;
  }

  if (!a || !b)
  {
    return Create<SpectrumValue>(*txPsd);
  }
  const double pathLossDb = ComputePathLossDb(a, b);

  // Scale PSD by path loss (linear factor)
  const double scale = std::pow(10.0, -pathLossDb / 10.0);

  Ptr<SpectrumValue> rv = Create<SpectrumValue>(*txPsd);
  (*rv) *= scale;

  return rv;
}

int64_t
Cc2420SpectrumPropagationLossModel::DoAssignStreams(int64_t stream)
{
  if (m_shadowingLosRng)
  {
    m_shadowingLosRng->SetStream(stream++);
  }
  if (m_shadowingMixedRng)
  {
    m_shadowingMixedRng->SetStream(stream++);
  }
  if (m_shadowingNlosRng)
  {
    m_shadowingNlosRng->SetStream(stream++);
  }
  if (m_shadowingGroundGroundRng)
  {
    m_shadowingGroundGroundRng->SetStream(stream++);
  }
  if (m_losSelectorRng)
  {
    m_losSelectorRng->SetStream(stream++);
  }
  if (m_fastFadingLosRng)
  {
    m_fastFadingLosRng->SetStream(stream++);
  }
  if (m_fastFadingMixedRng)
  {
    m_fastFadingMixedRng->SetStream(stream++);
  }
  if (m_fastFadingNlosRng)
  {
    m_fastFadingNlosRng->SetStream(stream++);
  }
  if (m_fastFadingGroundRng)
  {
    m_fastFadingGroundRng->SetStream(stream++);
  }
  return 9;
}

} // namespace propagation
} // namespace wsn
} // namespace ns3
