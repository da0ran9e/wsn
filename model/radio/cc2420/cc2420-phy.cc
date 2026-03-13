/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "cc2420-phy.h"
#include "../../propagation/cc2420-spectrum-propagation-loss-model.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-value.h"
#include "ns3/mobility-model.h"
#include "ns3/antenna-model.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/net-device.h"
#include "ns3/spectrum-channel.h"
#include "ns3/random-variable-stream.h"

namespace ns3
{
namespace wsn
{

NS_LOG_COMPONENT_DEFINE("Cc2420Phy");
NS_OBJECT_ENSURE_REGISTERED(Cc2420Phy);

// =============================================================================
// Cc2420Phy Implementation
// =============================================================================

TypeId
Cc2420Phy::GetTypeId()
{
    static TypeId tid = TypeId("ns3::wsn::Cc2420Phy")
        .SetParent<SpectrumPhy>()
        .SetGroupName("Cc2420")
        .AddConstructor<Cc2420Phy>()
        .AddAttribute("TxPower",
                      "Transmission power in dBm",
                      DoubleValue(0.0),
                      MakeDoubleAccessor(&Cc2420Phy::m_txPowerDbm),
                      MakeDoubleChecker<double>())
        .AddAttribute("RxSensitivity",
                      "Reception sensitivity in dBm",
                      DoubleValue(-95.0),
                      MakeDoubleAccessor(&Cc2420Phy::m_rxSensitivityDbm),
                      MakeDoubleChecker<double>())
        .AddAttribute("NoiseFloor",
                      "Noise floor in dBm",
                      DoubleValue(-100.0),
                      MakeDoubleAccessor(&Cc2420Phy::m_noiseFloorDbm),
                      MakeDoubleChecker<double>())
        .AddAttribute("CCAThreshold",
                      "CCA threshold in dBm",
                      DoubleValue(-77.0),
                      MakeDoubleAccessor(&Cc2420Phy::m_ccaThresholdDbm),
                      MakeDoubleChecker<double>())
        .AddAttribute("PathLossReferenceDistance",
                      "Reference distance d0 for log-distance model (m)",
                      DoubleValue(1.0),
                      MakeDoubleAccessor(&Cc2420Phy::m_pathLossRefDistM),
                      MakeDoubleChecker<double>(0.1))
        .AddAttribute("PathLossReferenceLoss",
                      "Reference path loss PL0 at d0 (dB)",
                      DoubleValue(40.05),
                      MakeDoubleAccessor(&Cc2420Phy::m_pathLossRefLossDb),
                      MakeDoubleChecker<double>(0.0))
        .AddAttribute("PathLossExponentLos",
                      "Path loss exponent for high-elevation LoS profile",
                      DoubleValue(2.0),
                      MakeDoubleAccessor(&Cc2420Phy::m_pathLossExpLos),
                      MakeDoubleChecker<double>(1.0))
        .AddAttribute("PathLossExponentMixed",
                      "Path loss exponent for mixed elevation profile",
                      DoubleValue(2.5),
                      MakeDoubleAccessor(&Cc2420Phy::m_pathLossExpMixed),
                      MakeDoubleChecker<double>(1.0))
        .AddAttribute("PathLossExponentNlos",
                      "Path loss exponent for low-elevation NLoS profile",
                      DoubleValue(3.0),
                      MakeDoubleAccessor(&Cc2420Phy::m_pathLossExpNlos),
                      MakeDoubleChecker<double>(1.0))
        .AddAttribute("ShadowingSigmaLos",
                      "Shadowing sigma for LoS profile (dB)",
                      DoubleValue(4.0),
                      MakeDoubleAccessor(&Cc2420Phy::m_shadowingSigmaLosDb),
                      MakeDoubleChecker<double>(0.0))
        .AddAttribute("ShadowingSigmaMixed",
                      "Shadowing sigma for mixed profile (dB)",
                      DoubleValue(6.0),
                      MakeDoubleAccessor(&Cc2420Phy::m_shadowingSigmaMixedDb),
                      MakeDoubleChecker<double>(0.0))
        .AddAttribute("ShadowingSigmaNlos",
                      "Shadowing sigma for NLoS profile (dB)",
                      DoubleValue(8.0),
                      MakeDoubleAccessor(&Cc2420Phy::m_shadowingSigmaNlosDb),
                      MakeDoubleChecker<double>(0.0))
        .AddAttribute("ElevationLosThreshold",
                      "Elevation threshold for LoS profile (deg)",
                      DoubleValue(40.0),
                      MakeDoubleAccessor(&Cc2420Phy::m_elevLosThreshDeg),
                      MakeDoubleChecker<double>(0.0, 90.0))
        .AddAttribute("ElevationMixedThreshold",
                      "Elevation threshold for mixed profile (deg)",
                      DoubleValue(20.0),
                      MakeDoubleAccessor(&Cc2420Phy::m_elevMixedThreshDeg),
                      MakeDoubleChecker<double>(0.0, 90.0))
        .AddAttribute("EnableShadowing",
                      "Enable log-normal shadowing term",
                      BooleanValue(true),
                      MakeBooleanAccessor(&Cc2420Phy::m_enableShadowing),
                      MakeBooleanChecker());
    return tid;
}

Cc2420Phy::Cc2420Phy()
    : m_txPowerDbm(0.0),
      m_rxSensitivityDbm(-95.0),
      m_noiseFloorDbm(-100.0),
      m_ccaThresholdDbm(-77.0),
            m_pathLossRefDistM(1.0),
            m_pathLossRefLossDb(40.05),
            m_pathLossExpLos(2.0),
            m_pathLossExpMixed(2.5),
            m_pathLossExpNlos(3.0),
            m_shadowingSigmaLosDb(4.0),
            m_shadowingSigmaMixedDb(6.0),
            m_shadowingSigmaNlosDb(8.0),
            m_elevLosThreshDeg(40.0),
            m_elevMixedThreshDeg(20.0),
            m_enableShadowing(true),
      m_currentState(PHY_SLEEP),
      m_pendingState(PHY_SLEEP),
      m_totalPowerDbm(-100.0),
      m_lastSignalChange(Seconds(0)),
      m_stateStartTime(Seconds(0)),
      m_previousState(PHY_SLEEP)
{
    NS_LOG_FUNCTION(this);

        m_shadowingLosRng = CreateObject<NormalRandomVariable>();
        m_shadowingMixedRng = CreateObject<NormalRandomVariable>();
        m_shadowingNlosRng = CreateObject<NormalRandomVariable>();

        m_shadowingLosRng->SetAttribute("Mean", DoubleValue(0.0));
        m_shadowingMixedRng->SetAttribute("Mean", DoubleValue(0.0));
        m_shadowingNlosRng->SetAttribute("Mean", DoubleValue(0.0));

        m_shadowingLosRng->SetAttribute("Variance", DoubleValue(m_shadowingSigmaLosDb * m_shadowingSigmaLosDb));
        m_shadowingMixedRng->SetAttribute("Variance", DoubleValue(m_shadowingSigmaMixedDb * m_shadowingSigmaMixedDb));
        m_shadowingNlosRng->SetAttribute("Variance", DoubleValue(m_shadowingSigmaNlosDb * m_shadowingSigmaNlosDb));

        m_propagationLossModel = CreateObject<propagation::Cc2420SpectrumPropagationLossModel>();
}

Cc2420Phy::~Cc2420Phy()
{
    NS_LOG_FUNCTION(this);
}

// =============================================================================
// SpectrumPhy Interface
// =============================================================================

void
Cc2420Phy::SetMobility(Ptr<MobilityModel> m)
{
    m_mobility = m;
}

Ptr<MobilityModel>
Cc2420Phy::GetMobility() const
{
    return m_mobility;
}

void
Cc2420Phy::SetAntenna(Ptr<AntennaModel> a)
{
    m_antenna = a;
}

void
Cc2420Phy::SetChannel(Ptr<SpectrumChannel> c)
{
    m_channel = c;
}

Ptr<Object>
Cc2420Phy::GetAntenna() const
{
    return m_antenna;
}

void
Cc2420Phy::StartRx(Ptr<SpectrumSignalParameters> params)
{
    NS_LOG_FUNCTION(this << params);
    EmitDebugTrace("StartRx", nullptr);
    // TODO: Implement RX signal processing
}

Ptr<NetDevice>
Cc2420Phy::GetDevice() const
{
    return m_netDevice;
}

void
Cc2420Phy::SetDevice(Ptr<NetDevice> d)
{
    m_netDevice = d;
}

Ptr<const SpectrumModel>
Cc2420Phy::GetRxSpectrumModel() const
{
    return m_rxSpectrumModel;
}

void
Cc2420Phy::AddRxAntenna(Ptr<AntennaModel> a)
{
    // TODO: Handle multiple RX antennas if needed
}

// =============================================================================
// CC2420-Specific Interface
// =============================================================================

void
Cc2420Phy::TransmitPacket(Ptr<Packet> packet, Time duration)
{
    NS_LOG_FUNCTION(this << packet << duration);
    EmitDebugTrace("TransmitPacket", packet);
    // TODO: Implement TX
}

bool
Cc2420Phy::SetState(PhyState newState)
{
    NS_LOG_FUNCTION(this << GetStateName(newState));
    // TODO: Implement state validation and transition
    return true;
}

PhyState
Cc2420Phy::GetState() const
{
    return m_currentState;
}

std::string
Cc2420Phy::GetStateName(PhyState state)
{
    switch (state)
    {
    case PHY_SLEEP:
        return "SLEEP";
    case PHY_IDLE:
        return "IDLE";
    case PHY_RX:
        return "RX";
    case PHY_TX:
        return "TX";
    case PHY_CCA:
        return "CCA";
    case PHY_SWITCHING:
        return "SWITCHING";
    default:
        return "UNKNOWN";
    }
}

bool
Cc2420Phy::PerformCCA()
{
    NS_LOG_FUNCTION(this);
    // TODO: Implement CCA logic
    return true;
}

double
Cc2420Phy::GetRSSI() const
{
    // TODO: Calculate RSSI from received signals
    return m_totalPowerDbm;
}

void
Cc2420Phy::SetTxPower(double powerDbm)
{
    m_txPowerDbm = powerDbm;
}

double
Cc2420Phy::GetTxPower() const
{
    return m_txPowerDbm;
}

void
Cc2420Phy::SetRxSensitivity(double sensitivityDbm)
{
    m_rxSensitivityDbm = sensitivityDbm;
}

double
Cc2420Phy::GetRxSensitivity() const
{
    return m_rxSensitivityDbm;
}

void
Cc2420Phy::SetPropagationLossModel(Ptr<propagation::Cc2420SpectrumPropagationLossModel> model)
{
    m_propagationLossModel = model;
}

Ptr<propagation::Cc2420SpectrumPropagationLossModel>
Cc2420Phy::GetPropagationLossModel() const
{
    return m_propagationLossModel;
}

bool
Cc2420Phy::EvaluateReceptionFrom(Ptr<Cc2420Phy> txPhy, double& rssiDbm, uint8_t& lqi) const
{
    // Default outputs for safety
    rssiDbm = m_noiseFloorDbm;
    lqi = 0;

    if (!txPhy || !m_mobility || !txPhy->GetMobility())
    {
        return false;
    }

    if (m_propagationLossModel)
    {
        rssiDbm = m_propagationLossModel->CalcRxPowerDbm(
            txPhy->GetTxPower(), txPhy->GetMobility(), m_mobility);
    }
    else
    {
        // Fallback to previous internal model if module is not attached.
        const Vector txPos = txPhy->GetMobility()->GetPosition();
        const Vector rxPos = m_mobility->GetPosition();

        const double dx = txPos.x - rxPos.x;
        const double dy = txPos.y - rxPos.y;
        const double dz = txPos.z - rxPos.z;
        const double horizontalDistance = std::sqrt(dx * dx + dy * dy);
        const double distance3D = std::sqrt(horizontalDistance * horizontalDistance + dz * dz);
        const double distanceForLoss = std::max(m_pathLossRefDistM, distance3D);

        const double kRadToDeg = 180.0 / std::acos(-1.0);
        const double elevDeg =
            (horizontalDistance > 1e-9)
                ? (std::atan2(std::abs(dz), horizontalDistance) * kRadToDeg)
                : 90.0;

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

        const double pathLossDb =
            m_pathLossRefLossDb +
            10.0 * pathLossExponent * std::log10(distanceForLoss / m_pathLossRefDistM) +
            shadowingDb;

        rssiDbm = txPhy->GetTxPower() - pathLossDb;
    }
    if (rssiDbm < m_rxSensitivityDbm)
    {
        return false;
    }

    const double snrDb = rssiDbm - m_noiseFloorDbm;
    const double snrClamped = std::max(0.0, std::min(30.0, snrDb));
    lqi = static_cast<uint8_t>(std::round((snrClamped / 30.0) * 255.0));
    return true;
}

// =============================================================================
// Callback Setup
// =============================================================================

void
Cc2420Phy::SetPdDataIndicationCallback(PdDataIndicationCallback callback)
{
    m_pdDataIndicationCallback = callback;
}

void
Cc2420Phy::SetPdDataConfirmCallback(PdDataConfirmCallback callback)
{
    m_pdDataConfirmCallback = callback;
}

void
Cc2420Phy::SetPlmeCcaConfirmCallback(PlmeCcaConfirmCallback callback)
{
    m_plmeCcaConfirmCallback = callback;
}

void
Cc2420Phy::SetStateChangeCallback(StateChangeCallback callback)
{
    m_stateChangeCallback = callback;
}

void
Cc2420Phy::SetDebugPacketTraceCallback(DebugPacketTraceCallback callback)
{
    m_debugPacketTraceCallback = callback;
}

// =============================================================================
// Private Helper Methods
// =============================================================================

void
Cc2420Phy::DoStateChange(PhyState newState)
{
    NS_LOG_FUNCTION(this << GetStateName(newState));
    // TODO: Implement state change logic
}

void
Cc2420Phy::TxComplete()
{
    NS_LOG_FUNCTION(this);
    // TODO: Handle TX completion
}

void
Cc2420Phy::RxComplete()
{
    NS_LOG_FUNCTION(this);
    // TODO: Handle RX completion
}

void
Cc2420Phy::ProcessSignalStart(Ptr<SpectrumSignalParameters> params)
{
    NS_LOG_FUNCTION(this << params);
    EmitDebugTrace("ProcessSignalStart", nullptr);
    // TODO: Implement signal start processing
}

void
Cc2420Phy::ProcessSignalEnd()
{
    NS_LOG_FUNCTION(this);
    EmitDebugTrace("ProcessSignalEnd", nullptr);
    // TODO: Implement signal end processing
}

void
Cc2420Phy::UpdateInterference()
{
    NS_LOG_FUNCTION(this);
    // TODO: Update interference levels
}

double
Cc2420Phy::CalculateSNR(const ReceivedSignal& signal) const
{
    // SNR = signal power - interference
    return signal.powerDbm - signal.currentInterference;
}

bool
Cc2420Phy::IsPacketDestroyed(const ReceivedSignal& signal) const
{
    // SIMPLE_COLLISION_MODEL:
    // If any other signal is within 6dB of sensitivity, packet is destroyed
    if (signal.maxInterference > (m_rxSensitivityDbm - 6.0))
        return true;
    return false;
}

void
Cc2420Phy::EmitDebugTrace(const std::string& eventName, Ptr<const Packet> packet) const
{
    if (!m_debugPacketTraceCallback.IsNull())
    {
        m_debugPacketTraceCallback(eventName, packet);
    }
}

} // namespace cc2420
} // namespace ns3
