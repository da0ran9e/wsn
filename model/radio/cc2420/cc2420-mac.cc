/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "cc2420-mac.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/random-variable-stream.h"

namespace ns3
{
namespace wsn
{

NS_LOG_COMPONENT_DEFINE("Cc2420Mac");
NS_OBJECT_ENSURE_REGISTERED(Cc2420Mac);

// =============================================================================
// Cc2420Mac Implementation
// =============================================================================

TypeId
Cc2420Mac::GetTypeId()
{
    static TypeId tid = TypeId("ns3::cc2420::Cc2420Mac")
        .SetParent<Object>()
        .SetGroupName("Cc2420")
        .AddConstructor<Cc2420Mac>()
        .AddAttribute("MinBE",
                      "Minimum Backoff Exponent",
                      UintegerValue(3),
                      MakeUintegerAccessor(&Cc2420Mac::m_config.macMinBE),
                      MakeUintegerChecker<uint8_t>())
        .AddAttribute("MaxBE",
                      "Maximum Backoff Exponent",
                      UintegerValue(5),
                      MakeUintegerAccessor(&Cc2420Mac::m_config.macMaxBE),
                      MakeUintegerChecker<uint8_t>())
        .AddAttribute("MaxCSMABackoffs",
                      "Maximum CSMA-CA Backoffs",
                      UintegerValue(4),
                      MakeUintegerAccessor(&Cc2420Mac::m_config.macMaxCSMABackoffs),
                      MakeUintegerChecker<uint8_t>())
        .AddAttribute("MaxFrameRetries",
                      "Maximum Frame Retries",
                      UintegerValue(3),
                      MakeUintegerAccessor(&Cc2420Mac::m_config.macMaxFrameRetries),
                      MakeUintegerChecker<uint8_t>());
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
}

Cc2420Mac::~Cc2420Mac()
{
    NS_LOG_FUNCTION(this);
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
    // TODO: Start MAC operations
    // Set PHY to RX mode if configured
    // Initialize sequence number
}

// =============================================================================
// Data Transmission Interface
// =============================================================================

bool
Cc2420Mac::McpsDataRequest(Ptr<Packet> packet, Mac16Address destAddr, bool requestAck)
{
    NS_LOG_FUNCTION(this << packet << destAddr << requestAck);
    // TODO: Implement MCPS-DATA.request
    return true;
}

// =============================================================================
// Frame Reception (from PHY)
// =============================================================================

void
Cc2420Mac::FrameReceptionCallback(Ptr<Packet> packet, double rssi, uint8_t lqi)
{
    NS_LOG_FUNCTION(this << packet << rssi << (uint16_t)lqi);
    // TODO: Implement frame reception
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
    // TODO: Reset current packet and related state
}

} // namespace cc2420
} // namespace ns3
