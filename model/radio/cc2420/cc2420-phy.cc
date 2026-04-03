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
#include "ns3/node.h"
#include "ns3/spectrum-channel.h"
#include "ns3/random-variable-stream.h"

#include <sstream>

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
                      MakeBooleanChecker())
        .AddAttribute("PerfectChannel",
                      "When true, bypass all path-loss/shadowing/BER calculations and always "
                      "receive with a fixed strong RSSI (-50 dBm) and LQI=255",
                      BooleanValue(false),
                      MakeBooleanAccessor(&Cc2420Phy::m_perfectChannel),
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
            m_perfectChannel(false),
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

        m_propagationLossModel = CreateObject<Cc2420SpectrumPropagationLossModel>();

        // Create a default error model (enabled by default)
        m_errorModel = CreateObject<Cc2420ErrorModel>();
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

    if (!params)
    {
        return;
    }

    // Handle the signal start immediately
    ProcessSignalStart(params);

    // Schedule signal end when the duration expires
    Simulator::Schedule(params->duration, &Cc2420Phy::ProcessSignalEnd, this);
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
    // CC2420 has a single integrated antenna; additional antennas are not supported
    NS_LOG_WARN("Cc2420Phy::AddRxAntenna: CC2420 uses single antenna, ignoring");
}

// =============================================================================
// CC2420-Specific Interface
// =============================================================================

void
Cc2420Phy::TransmitPacket(Ptr<Packet> packet, Time duration)
{
    NS_LOG_FUNCTION(this << packet << duration);
    EmitDebugTrace("TransmitPacket", packet);

    if (m_currentState == PHY_TX)
    {
        NS_LOG_WARN("TransmitPacket called while already TX");
        return;
    }

    DoStateChange(PHY_TX);

    m_txCompleteEvent = Simulator::Schedule(duration,
                                            &Cc2420Phy::TxComplete, this);
}

bool
Cc2420Phy::SetState(PhyState newState)
{
    NS_LOG_FUNCTION(this << GetStateName(newState));

    // Validate transition: TX/RX can only be interrupted via SWITCHING
    if (m_currentState == PHY_TX && newState != PHY_SWITCHING && newState != PHY_IDLE)
    {
        NS_LOG_WARN("Invalid state transition: TX → " << GetStateName(newState));
        return false;
    }
    if (m_currentState == PHY_RX && newState != PHY_SWITCHING && newState != PHY_IDLE)
    {
        NS_LOG_WARN("Invalid state transition: RX → " << GetStateName(newState));
        return false;
    }

    DoStateChange(newState);
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

    // Channel is busy if total received power is above CCA threshold
    const bool channelClear = (m_totalPowerDbm < m_ccaThresholdDbm);
    NS_LOG_DEBUG("CCA: totalPower=" << m_totalPowerDbm
                 << " dBm, threshold=" << m_ccaThresholdDbm
                 << " dBm → " << (channelClear ? "CLEAR" : "BUSY"));

    if (!m_plmeCcaConfirmCallback.IsNull())
    {
        m_plmeCcaConfirmCallback(channelClear ? 0 : 1);
    }

    return channelClear;
}

double
Cc2420Phy::GetRSSI() const
{
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
Cc2420Phy::SetPropagationLossModel(Ptr<Cc2420SpectrumPropagationLossModel> model)
{
    m_propagationLossModel = model;
}

Ptr<Cc2420SpectrumPropagationLossModel>
Cc2420Phy::GetPropagationLossModel() const
{
    return m_propagationLossModel;
}

void
Cc2420Phy::SetErrorModel(Ptr<Cc2420ErrorModel> model)
{
    m_errorModel = model;
}

Ptr<Cc2420ErrorModel>
Cc2420Phy::GetErrorModel() const
{
    return m_errorModel;
}

void
Cc2420Phy::SetPerfectChannel(bool enable)
{
    m_perfectChannel = enable;
}

bool
Cc2420Phy::GetPerfectChannel() const
{
    return m_perfectChannel;
}

bool
Cc2420Phy::EvaluateReceptionFrom(Ptr<Cc2420Phy> txPhy,
                                  double& rssiDbm,
                                  uint8_t& lqi,
                                  uint32_t packetSizeBytes)
{
    // Default outputs for safety
    rssiDbm = m_noiseFloorDbm;
    lqi = 0;

    // When PerfectChannel is enabled we still compute distance-based
    // path-loss (so RSSI scales with range) but we bypass stochastic
    // impairments: shadowing and BER/PER are disabled.  This preserves
    // a simple range-based attenuation model while removing small-scale
    // variability and packet corruption.

    if (!txPhy || !m_mobility || !txPhy->GetMobility())
    {
        return false;
    }

    auto getNodeIdFromPhy = [](Ptr<const Cc2420Phy> phy) -> uint32_t {
        if (!phy || !phy->GetDevice() || !phy->GetDevice()->GetNode())
        {
            return 0;
        }
        return phy->GetDevice()->GetNode()->GetId();
    };

    const uint32_t srcNodeId = getNodeIdFromPhy(txPhy);
    const uint32_t dstNodeId = getNodeIdFromPhy(this);

    auto emitDrop = [&](const std::string& reason, const std::string& meta) {
        std::ostringstream oss;
        oss << srcNodeId << "-D-" << dstNodeId << "|" << reason;
        if (!meta.empty())
        {
            oss << "|" << meta;
        }
        EmitDebugTrace(oss.str(), nullptr);
    };

    if (!m_perfectChannel && m_propagationLossModel)
    {
        // Normal mode: delegate to external propagation model
        rssiDbm = m_propagationLossModel->CalcRxPowerDbm(
            txPhy->GetTxPower(), txPhy->GetMobility(), m_mobility);
    }
    else
    {
        // Fallback/internal log-distance path-loss model.
        // When PerfectChannel==true we use the same distance-based loss but
        // explicitly disable shadowing (shadowingDb == 0).
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
        if (!m_perfectChannel && m_enableShadowing && shadowingRng)
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
        std::ostringstream meta;
        meta << "rssiDbm=" << rssiDbm
             << "|rxSensitivityDbm=" << m_rxSensitivityDbm
             << "|snrDb=" << (rssiDbm - m_noiseFloorDbm)
             << "|noiseFloorDbm=" << m_noiseFloorDbm;
        emitDrop("RxDropBelowSensitivity", meta.str());
        return false;
    }

    const double snrDb = rssiDbm - m_noiseFloorDbm;
    const double snrClamped = std::max(0.0, std::min(30.0, snrDb));
    lqi = static_cast<uint8_t>(std::round((snrClamped / 30.0) * 255.0));

    // ── BER / PER stochastic drop ────────────────────────────────────────────
    // Even though RSSI is above sensitivity, the channel can still corrupt
    // individual bits.  At low SNR a packet large enough to carry an image
    // fragment has a non-trivial probability of containing at least one bad bit.
    //
    // packetSizeBytes == 0 means the caller did not provide size; skip the check
    // to preserve backward compatibility.
    if (!m_perfectChannel && m_errorModel && m_errorModel->IsEnabled() && packetSizeBytes > 0)
    {
        const double ber = m_errorModel->GetBer(snrDb);
        const double per = m_errorModel->GetPer(ber, packetSizeBytes);
        if (m_errorModel->PacketIsLost(per))
        {
            NS_LOG_DEBUG("[ErrorModel] packet lost: SNR=" << snrDb
                         << " dB, BER=" << ber
                         << ", PER=" << per
                         << ", size=" << packetSizeBytes << " B");
            std::ostringstream meta;
            meta << "snrDb=" << snrDb
                 << "|ber=" << ber
                 << "|per=" << per
                 << "|packetSize=" << packetSizeBytes;
            emitDrop("RxDropBer", meta.str());
            lqi = 0;
            return false;
        }
    }

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
    NS_LOG_FUNCTION(this << GetStateName(m_currentState) << "->" << GetStateName(newState));

    PhyState oldState = m_currentState;
    m_previousState = oldState;
    m_currentState = newState;
    m_stateStartTime = Simulator::Now();

    if (!m_stateChangeCallback.IsNull())
    {
        m_stateChangeCallback(oldState, newState);
    }

    NS_LOG_DEBUG("[PHY] state: " << GetStateName(oldState)
                 << " → " << GetStateName(newState)
                 << " @ " << Simulator::Now().GetSeconds() << "s");
}

void
Cc2420Phy::TxComplete()
{
    NS_LOG_FUNCTION(this);
    EmitDebugTrace("TxComplete", nullptr);

    DoStateChange(PHY_IDLE);

    if (!m_pdDataConfirmCallback.IsNull())
    {
        m_pdDataConfirmCallback(0); // 0 = success
    }
}

void
Cc2420Phy::RxComplete()
{
    NS_LOG_FUNCTION(this);
    EmitDebugTrace("RxComplete", nullptr);

    // Find the primary received signal (highest power)
    if (m_receivedSignals.empty())
    {
        DoStateChange(PHY_IDLE);
        return;
    }

    auto bestIt = m_receivedSignals.begin();
    for (auto it = m_receivedSignals.begin(); it != m_receivedSignals.end(); ++it)
    {
        if (it->powerDbm > bestIt->powerDbm)
        {
            bestIt = it;
        }
    }

    const ReceivedSignal& sig = *bestIt;

    if (IsPacketDestroyed(sig))
    {
        NS_LOG_DEBUG("[PHY] RxComplete: packet destroyed by interference");
        DoStateChange(PHY_IDLE);
        return;
    }

    if (sig.powerDbm < m_rxSensitivityDbm)
    {
        NS_LOG_DEBUG("[PHY] RxComplete: signal below sensitivity");
        DoStateChange(PHY_IDLE);
        return;
    }

    const double snrDb = sig.powerDbm - m_noiseFloorDbm;
    const double snrClamped = std::max(0.0, std::min(30.0, snrDb));
    const uint8_t lqi = static_cast<uint8_t>(std::round((snrClamped / 30.0) * 255.0));

    DoStateChange(PHY_IDLE);

    // Forward to MAC via callback — packet reference not tracked in skeleton
    if (!m_pdDataIndicationCallback.IsNull())
    {
        m_pdDataIndicationCallback(nullptr, sig.powerDbm, lqi);
    }
}

void
Cc2420Phy::ProcessSignalStart(Ptr<SpectrumSignalParameters> params)
{
    NS_LOG_FUNCTION(this << params);
    EmitDebugTrace("ProcessSignalStart", nullptr);

    if (!params)
    {
        return;
    }

    // Compute received power from spectrum values
    double rxPowerW = 0.0;
    if (params->psd)
    {
        for (auto it = params->psd->ConstBandsBegin(); it != params->psd->ConstBandsEnd(); ++it)
        {
            const std::size_t idx = it - params->psd->ConstBandsBegin();
            rxPowerW += (*(params->psd))[idx] * (it->fh - it->fl);
        }
    }

    // Convert to dBm
    const double rxPowerDbm = (rxPowerW > 0.0)
        ? (10.0 * std::log10(rxPowerW * 1000.0))
        : m_noiseFloorDbm;

    ReceivedSignal sig;
    sig.sourceNodeId = 0;
    sig.powerDbm = rxPowerDbm;
    sig.currentInterference = 0.0;
    sig.maxInterference = 0.0;
    sig.bitErrors = 0;
    sig.startTime = Simulator::Now();

    m_receivedSignals.push_back(sig);
    UpdateInterference();

    if (m_currentState == PHY_IDLE && rxPowerDbm >= m_rxSensitivityDbm)
    {
        DoStateChange(PHY_RX);
    }
}

void
Cc2420Phy::ProcessSignalEnd()
{
    NS_LOG_FUNCTION(this);
    EmitDebugTrace("ProcessSignalEnd", nullptr);

    if (!m_receivedSignals.empty())
    {
        m_receivedSignals.erase(m_receivedSignals.begin());
    }
    UpdateInterference();

    if (m_currentState == PHY_RX && m_receivedSignals.empty())
    {
        RxComplete();
    }
}

void
Cc2420Phy::UpdateInterference()
{
    NS_LOG_FUNCTION(this);

    if (m_receivedSignals.empty())
    {
        m_totalPowerDbm = m_noiseFloorDbm;
        return;
    }

    // Sum all received power linearly then convert back to dBm
    double totalPowerW = 0.0;
    for (const auto& sig : m_receivedSignals)
    {
        totalPowerW += std::pow(10.0, (sig.powerDbm - 30.0) / 10.0); // dBm → W
    }

    m_totalPowerDbm = (totalPowerW > 0.0)
        ? (10.0 * std::log10(totalPowerW) + 30.0)  // W → dBm
        : m_noiseFloorDbm;

    // Update interference for each signal: interference = total - own signal
    for (auto& sig : m_receivedSignals)
    {
        const double ownPowerW = std::pow(10.0, (sig.powerDbm - 30.0) / 10.0);
        const double intfPowerW = std::max(0.0, totalPowerW - ownPowerW);
        sig.currentInterference = (intfPowerW > 0.0)
            ? (10.0 * std::log10(intfPowerW) + 30.0)
            : (m_noiseFloorDbm - 40.0);
        sig.maxInterference = std::max(sig.maxInterference, sig.currentInterference);
    }
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

} // namespace wsn
} // namespace ns3
