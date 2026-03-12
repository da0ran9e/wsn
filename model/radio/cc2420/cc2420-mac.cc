/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "cc2420-mac.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/random-variable-stream.h"
#include "ns3/net-device.h"
#include "ns3/node.h"

#include <algorithm>
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

        // PHY decides link viability and reports RSSI/LQI.
        double rssiDbm = -80.0;
        uint8_t lqi = 255;

        if (!(m_phy && peer->m_phy && peer->m_phy->EvaluateReceptionFrom(m_phy, rssiDbm, lqi)))
        {
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
    // TODO: Handle CCA result
}

void
Cc2420Mac::TxConfirmCallback(int status)
{
    NS_LOG_FUNCTION(this << status);
    // TODO: Handle TX completion
}

// =============================================================================
// CSMA-CA Algorithm
// =============================================================================

void
Cc2420Mac::StartCSMACA()
{
    NS_LOG_FUNCTION(this);
    // TODO: Initialize CSMA-CA parameters
    // NB = 0, BE = macMinBE, CW = 1
}

void
Cc2420Mac::BackoffExpired()
{
    NS_LOG_FUNCTION(this);
    // TODO: Handle backoff expiration
}

void
Cc2420Mac::DoCCA()
{
    NS_LOG_FUNCTION(this);
    // TODO: Request CCA from PHY
}

void
Cc2420Mac::HandleCCAResult(int result)
{
    NS_LOG_FUNCTION(this << result);
    // TODO: Process CCA result
}

void
Cc2420Mac::AttemptTransmission()
{
    NS_LOG_FUNCTION(this);
    // TODO: Send current packet
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
    // Unslotted CSMA-CA: random(0, 2^BE - 1) unit backoff periods
    // Unit backoff period = 20 symbols = 320 microseconds (for 2.4 GHz)
    // TODO: Implement random backoff calculation
    return MilliSeconds(1);
}

void
Cc2420Mac::HandleAckPacket(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this << packet);
    // TODO: Implement ACK handling
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

} // namespace cc2420
} // namespace ns3
