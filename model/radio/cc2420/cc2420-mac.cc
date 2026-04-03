/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "cc2420-mac.h"
#include "cc2420-contact-window-model.h"
#include "../../propagation/cc2420-spectrum-propagation-loss-model.h"

#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/simulator.h"
#include "ns3/random-variable-stream.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/vector.h"

#include <algorithm>
#include <limits>
#include <sstream>
#include <vector>

namespace ns3
{
namespace wsn
{

namespace
{
std::vector<Cc2420Mac*> g_allMacs;
}

NS_LOG_COMPONENT_DEFINE("Cc2420Mac");
NS_OBJECT_ENSURE_REGISTERED(Cc2420Mac);

// =============================================================================
// Cc2420Mac Implementation
// =============================================================================

TypeId
Cc2420Mac::GetTypeId()
{
    static TypeId tid = TypeId("ns3::wsn::Cc2420Mac")
        .SetParent<Object>()
        .SetGroupName("Cc2420")
        .AddConstructor<Cc2420Mac>();
    return tid;
}

Cc2420Mac::Cc2420Mac()
    : m_macState(MAC_IDLE),
      m_NB(0),
      m_BE(3),
      m_CW(1),
      m_retries(0),
      m_sequenceNumber(0),
      m_txCount(0),
      m_rxCount(0),
      m_txFailureCount(0)
{
    NS_LOG_FUNCTION(this);

    m_contactWindowModel = CreateObject<Cc2420ContactWindowModel>();

    // Initialize MAC config with defaults
    m_config.panId = 0;
    m_config.shortAddress = Mac16Address();
    m_config.macMinBE = 3;
    m_config.macMaxBE = 5;
    m_config.macMaxCSMABackoffs = 4;
    m_config.macMaxFrameRetries = 3;
    m_config.txAckRequest = true;
    m_config.rxOnWhenIdle = true;

    g_allMacs.push_back(this);
}

Cc2420Mac::~Cc2420Mac()
{
    NS_LOG_FUNCTION(this);

    auto it = std::find(g_allMacs.begin(), g_allMacs.end(), this);
    if (it != g_allMacs.end())
    {
        g_allMacs.erase(it);
    }
}

// =============================================================================
// Initialization
// =============================================================================

void
Cc2420Mac::SetPhy(Ptr<Cc2420Phy> phy)
{
    m_phy = phy;
}

Ptr<Cc2420Phy>
Cc2420Mac::GetPhy() const
{
    return m_phy;
}

void
Cc2420Mac::SetMacConfig(const MacConfig& config)
{
    m_config = config;
}

MacConfig
Cc2420Mac::GetMacConfig() const
{
    return m_config;
}

void
Cc2420Mac::Start()
{
    NS_LOG_FUNCTION(this);
    m_macState = MAC_IDLE;
}

// =============================================================================
// Data Transmission Interface
// =============================================================================

bool
Cc2420Mac::McpsDataRequest(Ptr<Packet> packet, Mac16Address destAddr, bool requestAck)
{
    NS_LOG_FUNCTION(this << packet << destAddr << requestAck);
    EmitDebugTrace("McpsDataRequest", packet);

    if (!packet)
    {
        return false;
    }

    // Minimal functional MAC path: send through CC2420 MAC and dispatch to peers.
    // This keeps all traffic traversing cc2420-mac while PHY is still skeleton.
    m_txCount++;

    const bool isBroadcast = (destAddr == Mac16Address("FF:FF"));
    const Mac16Address src = m_config.shortAddress;

    auto getNodeIdFromPhy = [](Ptr<Cc2420Phy> phy) -> uint32_t {
        if (!phy || !phy->GetDevice() || !phy->GetDevice()->GetNode())
        {
            return 0;
        }
        return phy->GetDevice()->GetNode()->GetId();
    };

    const uint32_t srcNodeId = getNodeIdFromPhy(m_phy);

    // Accumulate contact-window drops; emit a single summary after the peer loop.
    std::vector<uint32_t> contactDropDsts;
    std::vector<uint32_t> contactDropDstsGoodRssi;

    auto computeStartRssi = [&](Cc2420Mac* peer, double& outRssiDbm, double& outRxSensitivityDbm) -> bool {
        outRssiDbm = -std::numeric_limits<double>::infinity();
        outRxSensitivityDbm = -std::numeric_limits<double>::infinity();

        if (!m_phy || !peer || !peer->m_phy)
        {
            return false;
        }

        Ptr<MobilityModel> txMob = m_phy->GetMobility();
        Ptr<MobilityModel> rxMob = peer->m_phy->GetMobility();
        Ptr<Cc2420SpectrumPropagationLossModel> propagation =
            peer->m_phy->GetPropagationLossModel();
        if (!txMob || !rxMob || !propagation)
        {
            return false;
        }

        const Vector txPos = txMob->GetPosition();
        const Vector rxPos = rxMob->GetPosition();
        outRssiDbm = propagation->CalcRxPowerDbmFromPositions(
            m_phy->GetTxPower(), txPos, rxPos, false);
        outRxSensitivityDbm = peer->m_phy->GetRxSensitivity();
        return true;
    };

    for (Cc2420Mac* peer : g_allMacs)
    {
        if (peer == nullptr || peer == this)
        {
            continue;
        }

        MacConfig peerCfg = peer->GetMacConfig();
        if (!isBroadcast && peerCfg.shortAddress != destAddr)
        {
            continue;
        }

        const uint32_t dstNodeId = getNodeIdFromPhy(peer->m_phy);

        auto makeDropEvent = [&](const std::string& reason) {
            std::ostringstream oss;
            oss << srcNodeId << "-D-" << dstNodeId
                << "|" << reason
                << "|srcAddr=" << src
                << "|dstAddr=" << peerCfg.shortAddress;
            return oss.str();
        };

        if (m_contactWindowModel &&
            !(m_phy && m_phy->GetPerfectChannel()) &&
            !m_contactWindowModel->HasContactForPacket(m_phy, peer->m_phy, packet->GetSize()))
        {
            contactDropDsts.push_back(dstNodeId);

            double startRssiDbm = -std::numeric_limits<double>::infinity();
            double rxSensitivityDbm = -std::numeric_limits<double>::infinity();
            if (computeStartRssi(peer, startRssiDbm, rxSensitivityDbm) &&
                startRssiDbm >= rxSensitivityDbm)
            {
                contactDropDstsGoodRssi.push_back(dstNodeId);
            }

            continue;
        }

        // PHY decides link viability and reports RSSI/LQI.
        double rssiDbm = -80.0;
        uint8_t lqi = 255;

        if (!(m_phy && peer->m_phy &&
              peer->m_phy->EvaluateReceptionFrom(m_phy, rssiDbm, lqi, packet->GetSize())))
        {
            EmitDebugTrace(makeDropEvent("DropPhyReject"), packet);
            continue;
        }

        Ptr<Packet> rxCopy = packet->Copy();
        uint32_t rxContext = Simulator::NO_CONTEXT;
        if (peer->m_phy && peer->m_phy->GetDevice() && peer->m_phy->GetDevice()->GetNode())
        {
            rxContext = peer->m_phy->GetDevice()->GetNode()->GetId();
        }

        auto rxDispatch = [peer, rxCopy, src, rssiDbm, lqi]() {
            peer->EmitDebugTrace("RxDispatchFromPeer", rxCopy);
            peer->FrameReceptionCallback(rxCopy, rssiDbm, lqi);
            if (!peer->m_mcpsDataIndicationCallback.IsNull())
            {
                peer->EmitDebugTrace("McpsDataIndication", rxCopy);
                peer->m_mcpsDataIndicationCallback(rxCopy, src, rssiDbm);
            }
        };

        if (rxContext != Simulator::NO_CONTEXT)
        {
            Simulator::ScheduleWithContext(rxContext, Seconds(0), rxDispatch);
        }
        else
        {
            Simulator::ScheduleNow(rxDispatch);
        }
    }

    // Emit a single aggregated summary event for all contact-window drops.
    if (!contactDropDsts.empty())
    {
        std::ostringstream oss;
        oss << srcNodeId << "-D-*|DropContactWindowSummary"
            << "|srcAddr=" << src
            << "|dropCount=" << contactDropDsts.size()
            << "|dsts=";
        for (std::size_t i = 0; i < contactDropDsts.size(); ++i)
        {
            if (i > 0)
            {
                oss << ',';
            }
            oss << contactDropDsts[i];
        }
        EmitDebugTrace(oss.str(), packet);
    }

    // Emit filtered summary: packets that had sufficient start RSSI but still
    // dropped due to contact-time insufficiency.
    if (!contactDropDstsGoodRssi.empty())
    {
        std::ostringstream oss;
        oss << srcNodeId << "-D-*|DropContactWindowGoodRssiSummary"
            << "|srcAddr=" << src
            << "|dropCount=" << contactDropDstsGoodRssi.size()
            << "|dsts=";
        for (std::size_t i = 0; i < contactDropDstsGoodRssi.size(); ++i)
        {
            if (i > 0)
            {
                oss << ',';
            }
            oss << contactDropDstsGoodRssi[i];
        }
        EmitDebugTrace(oss.str(), packet);
    }

    if (!m_mcpsDataConfirmCallback.IsNull())
    {
        Simulator::ScheduleNow([this]() { m_mcpsDataConfirmCallback(0); });
    }

    return true;
}

// =============================================================================
// Frame Reception (from PHY)
// =============================================================================

void
Cc2420Mac::FrameReceptionCallback(Ptr<Packet> packet, double rssi, uint8_t lqi)
{
    NS_LOG_FUNCTION(this << packet << rssi << (uint16_t)lqi);
    EmitDebugTrace("FrameReceptionCallback", packet);
    m_rxCount++;
}

void
Cc2420Mac::CcaConfirmCallback(int result)
{
    NS_LOG_FUNCTION(this << result);
    // Called asynchronously by PHY after PerformCCA() when CCA runs in non-blocking mode.
    // In the current design PerformCCA() is synchronous, so this callback is a fallback
    // for future spectrum-based CCA integration.
    if (m_macState == MAC_CCA)
    {
        HandleCCAResult(result);
    }
}

void
Cc2420Mac::TxConfirmCallback(int status)
{
    NS_LOG_FUNCTION(this << status);
    // Called by PHY when a TransmitPacket duration expires (TxComplete → PdDataConfirmCallback).
    // AttemptTransmission already schedules its own completion; this handles the PHY-driven path.
    if (m_macState == MAC_SENDING)
    {
        if (status == 0) // success
        {
            m_txCount++;
            EmitDebugTrace("TxConfirm:Success", m_currentPacket);
        }
        else
        {
            m_txFailureCount++;
            EmitDebugTrace("TxConfirm:Failure", m_currentPacket);
        }
        ClearCurrentPacket();
        if (!m_mcpsDataConfirmCallback.IsNull())
        {
            m_mcpsDataConfirmCallback(status);
        }
    }
}

// =============================================================================
// CSMA-CA Algorithm (Unslotted IEEE 802.15.4)
// =============================================================================

void
Cc2420Mac::StartCSMACA()
{
    NS_LOG_FUNCTION(this);

    m_NB = 0;
    m_BE = m_config.macMinBE;
    m_CW = 1;
    m_macState = MAC_CSMA_BACKOFF;

    m_backoffEvent = Simulator::Schedule(CalculateBackoffDelay(),
                                         &Cc2420Mac::BackoffExpired, this);
}

void
Cc2420Mac::BackoffExpired()
{
    NS_LOG_FUNCTION(this);
    DoCCA();
}

void
Cc2420Mac::DoCCA()
{
    NS_LOG_FUNCTION(this);
    m_macState = MAC_CCA;

    if (m_phy)
    {
        // CCA result is returned synchronously (or via callback)
        const bool clear = m_phy->PerformCCA();
        HandleCCAResult(clear ? 0 : 1);
    }
    else
    {
        // No PHY: assume clear
        HandleCCAResult(0);
    }
}

void
Cc2420Mac::HandleCCAResult(int result)
{
    NS_LOG_FUNCTION(this << result);

    const bool channelClear = (result == 0);

    if (channelClear)
    {
        // Channel clear: transmit immediately
        AttemptTransmission();
    }
    else
    {
        // Channel busy: back off and retry
        m_NB++;
        m_BE = std::min(static_cast<uint8_t>(m_BE + 1), m_config.macMaxBE);

        if (m_NB > m_config.macMaxCSMABackoffs)
        {
            NS_LOG_DEBUG("CSMA-CA: exceeded max backoffs, drop packet");
            m_txFailureCount++;
            if (!m_mcpsDataConfirmCallback.IsNull())
            {
                m_mcpsDataConfirmCallback(1); // failure
            }
            ClearCurrentPacket();
            return;
        }

        m_macState = MAC_CSMA_BACKOFF;
        m_backoffEvent = Simulator::Schedule(CalculateBackoffDelay(),
                                             &Cc2420Mac::BackoffExpired, this);
    }
}

void
Cc2420Mac::AttemptTransmission()
{
    NS_LOG_FUNCTION(this);

    if (!m_currentPacket)
    {
        NS_LOG_WARN("AttemptTransmission: no current packet");
        return;
    }

    m_macState = MAC_SENDING;
    EmitDebugTrace("AttemptTransmission", m_currentPacket);

    // Calculate transmission duration: (size * 8 bits) / 250 kbps
    const uint32_t totalBytes = m_currentPacket->GetSize();
    const double dataRateBps = 250000.0;
    const Time txDuration = Seconds(static_cast<double>(totalBytes * 8u) / dataRateBps);

    if (m_phy)
    {
        m_phy->TransmitPacket(m_currentPacket, txDuration);
    }

    // Confirm TX success after air time
    m_txEvent = Simulator::Schedule(txDuration, [this]() {
        m_txCount++;
        EmitDebugTrace("TxDone", m_currentPacket);
        ClearCurrentPacket();
        if (!m_mcpsDataConfirmCallback.IsNull())
        {
            m_mcpsDataConfirmCallback(0); // success
        }
    });
}

// =============================================================================
// Callback Setup
// =============================================================================

void
Cc2420Mac::SetMcpsDataIndicationCallback(McpsDataIndicationCallback callback)
{
    m_mcpsDataIndicationCallback = callback;
}

void
Cc2420Mac::SetMcpsDataConfirmCallback(McpsDataConfirmCallback callback)
{
    m_mcpsDataConfirmCallback = callback;
}

void
Cc2420Mac::SetDebugPacketTraceCallback(DebugPacketTraceCallback callback)
{
    m_debugPacketTraceCallback = callback;
}

// =============================================================================
// Private Helper Methods
// =============================================================================

Time
Cc2420Mac::CalculateBackoffDelay()
{
    // Unslotted CSMA-CA: random(0, 2^BE - 1) × aUnitBackoffPeriod
    // aUnitBackoffPeriod = 20 symbols = 320 µs at 250 kbps / 4 bits-per-symbol
    const uint32_t maxBackoff = (1u << m_BE) - 1u;
    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
    const uint32_t backoffUnits = static_cast<uint32_t>(rng->GetInteger(0, maxBackoff));
    return MicroSeconds(backoffUnits * 320);
}

void
Cc2420Mac::HandleAckPacket(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this << packet);

    if (m_macState != MAC_ACK_PENDING)
    {
        return;
    }

    m_ackWaitEvent.Cancel();
    EmitDebugTrace("AckReceived", packet);
    m_txCount++;

    ClearCurrentPacket();

    if (!m_mcpsDataConfirmCallback.IsNull())
    {
        m_mcpsDataConfirmCallback(0); // success
    }
}

void
Cc2420Mac::ClearCurrentPacket()
{
    m_currentPacket = nullptr;
    m_macState = MAC_IDLE;
    m_retries = 0;
}

void
Cc2420Mac::EmitDebugTrace(const std::string& eventName, Ptr<const Packet> packet) const
{
    if (!m_debugPacketTraceCallback.IsNull())
    {
        m_debugPacketTraceCallback(eventName, packet);
    }
}

} // namespace wsn
} // namespace ns3
