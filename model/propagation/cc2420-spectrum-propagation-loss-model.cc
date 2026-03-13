#include "cc2420-spectrum-propagation-loss-model.h"

#include "ns3/spectrum-value.h"
#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/type-id.h"

#include <cmath>

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
                            .AddAttribute("PathLossExponentLos",
                                          "Path loss exponent for high-elevation LoS profile",
                                          DoubleValue(2.0),
                                          MakeDoubleAccessor(&Cc2420SpectrumPropagationLossModel::m_pathLossExpLos),
                                          MakeDoubleChecker<double>(1.0))
                            .AddAttribute("PathLossExponentMixed",
                                          "Path loss exponent for mixed profile",
                                          DoubleValue(2.5),
                                          MakeDoubleAccessor(&Cc2420SpectrumPropagationLossModel::m_pathLossExpMixed),
                                          MakeDoubleChecker<double>(1.0))
                            .AddAttribute("PathLossExponentNlos",
                                          "Path loss exponent for low-elevation NLoS profile",
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
                            .AddAttribute("EnableShadowing",
                                          "Enable log-normal shadowing term",
                                          BooleanValue(true),
                                          MakeBooleanAccessor(&Cc2420SpectrumPropagationLossModel::m_enableShadowing),
                                          MakeBooleanChecker());
  return tid;
}

Cc2420SpectrumPropagationLossModel::Cc2420SpectrumPropagationLossModel()
    : m_refDistM(1.0),
      m_refLossDb(40.05),
      m_pathLossExpLos(2.0),
      m_pathLossExpMixed(2.5),
      m_pathLossExpNlos(3.0),
      m_shadowingSigmaLosDb(4.0),
      m_shadowingSigmaMixedDb(6.0),
      m_shadowingSigmaNlosDb(8.0),
      m_elevLosThreshDeg(40.0),
      m_elevMixedThreshDeg(20.0),
      m_enableShadowing(true)
{
  m_shadowingLosRng = CreateObject<NormalRandomVariable>();
  m_shadowingMixedRng = CreateObject<NormalRandomVariable>();
  m_shadowingNlosRng = CreateObject<NormalRandomVariable>();

  m_shadowingLosRng->SetAttribute("Mean", DoubleValue(0.0));
  m_shadowingMixedRng->SetAttribute("Mean", DoubleValue(0.0));
  m_shadowingNlosRng->SetAttribute("Mean", DoubleValue(0.0));

  m_shadowingLosRng->SetAttribute("Variance", DoubleValue(m_shadowingSigmaLosDb * m_shadowingSigmaLosDb));
  m_shadowingMixedRng->SetAttribute("Variance", DoubleValue(m_shadowingSigmaMixedDb * m_shadowingSigmaMixedDb));
  m_shadowingNlosRng->SetAttribute("Variance", DoubleValue(m_shadowingSigmaNlosDb * m_shadowingSigmaNlosDb));
}

Cc2420SpectrumPropagationLossModel::~Cc2420SpectrumPropagationLossModel()
{
}

Ptr<SpectrumValue>
Cc2420SpectrumPropagationLossModel::DoCalcRxPowerSpectralDensity(Ptr<const SpectrumValue> txPsd,
                                                                  Ptr<const MobilityModel> a,
                                                                  Ptr<const MobilityModel> b) const
{
  if (!a || !b || !txPsd)
  {
    return Create<SpectrumValue>(*txPsd);
  }

  const Vector aPos = a->GetPosition();
  const Vector bPos = b->GetPosition();

  const double dx = aPos.x - bPos.x;
  const double dy = aPos.y - bPos.y;
  const double dz = aPos.z - bPos.z;
  const double horizontalDistance = std::sqrt(dx * dx + dy * dy);
  const double distance3D = std::sqrt(horizontalDistance * horizontalDistance + dz * dz);
  const double distanceForLoss = std::max(m_refDistM, distance3D);

  const double kRadToDeg = 180.0 / std::acos(-1.0);
  const double elevDeg = (horizontalDistance > 1e-9) ? (std::atan2(std::abs(dz), horizontalDistance) * kRadToDeg) : 90.0;

  double pathLossExponent = m_pathLossExpNlos;
  Ptr<NormalRandomVariable> shadowingRng = m_shadowingNlosRng;
  double sigmaDb = m_shadowingSigmaNlosDb;

  if (elevDeg >= m_elevLosThreshDeg)
  {
    pathLossExponent = m_pathLossExpLos;
    shadowingRng = m_shadowingLosRng;
    sigmaDb = m_shadowingSigmaLosDb;
  }
  else if (elevDeg >= m_elevMixedThreshDeg)
  {
    pathLossExponent = m_pathLossExpMixed;
    shadowingRng = m_shadowingMixedRng;
    sigmaDb = m_shadowingSigmaMixedDb;
  }

  double shadowingDb = 0.0;
  if (m_enableShadowing && shadowingRng)
  {
    shadowingRng->SetAttribute("Variance", DoubleValue(sigmaDb * sigmaDb));
    shadowingDb = shadowingRng->GetValue();
  }

  const double pathLossDb = m_refLossDb + 10.0 * pathLossExponent * std::log10(distanceForLoss / m_refDistM) + shadowingDb;

  // Scale PSD by path loss (linear factor)
  const double scale = std::pow(10.0, -pathLossDb / 10.0);

  Ptr<SpectrumValue> rv = Create<SpectrumValue>(*txPsd);
  // Multiply each bin by scale
  for (size_t i = 0; i < rv->size(); ++i)
  {
    (*rv)[i] *= scale;
  }

  return rv;
}

} // namespace propagation
} // namespace wsn
} // namespace ns3
