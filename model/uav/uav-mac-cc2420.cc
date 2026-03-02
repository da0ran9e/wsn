/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * UAV MAC Layer Implementation extending CC2420 MAC
 */

#include "uav-mac-cc2420.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/mobility-model.h"

NS_LOG_COMPONENT_DEFINE("UavMacCc2420");

namespace ns3
{
namespace wsn
{

NS_OBJECT_ENSURE_REGISTERED(UavMacCc2420);

TypeId
UavMacCc2420::GetTypeId()
{
    static TypeId tid = TypeId("ns3::wsn::UavMacCc2420")
        .SetParent<Cc2420Mac>()
        .SetGroupName("Wsn")
        .AddConstructor<UavMacCc2420>();
    return tid;
}

UavMacCc2420::UavMacCc2420()
    : Cc2420Mac(),
      m_seqNum(0),
      m_txPowerDbm(10.0),
      m_rxSensitivityDbm(-95.0),
      m_referenceLoss(20.0 * std::log10(40.0 * M_PI / 3e8)),  // ~-37 dB at 1m for 2.4GHz
      m_pathLossExponent(2.0),
      m_referenceDistance(1.0),
      m_totalBroadcasts(0),
      m_totalReceptions(0),
      m_originalUavMac(nullptr)
{
    NS_LOG_FUNCTION(this);

    // Set up RX indication callback to process Phase 1 packets
    SetMcpsDataIndicationCallback(MakeCallback(&UavMacCc2420::ReceiveFromCc2420, this));
}

UavMacCc2420::~UavMacCc2420()
{
    NS_LOG_FUNCTION(this);
    m_broadcastTimer.Cancel();
}

void
UavMacCc2420::Initialize(Ptr<Node> uavNode, NodeContainer groundNodes)
{
    NS_LOG_FUNCTION(this << uavNode);
    m_uavNode = uavNode;
    m_groundNodes = groundNodes;
}

void
UavMacCc2420::StartBroadcast(Time interval, Time stopTime)
{
    NS_LOG_FUNCTION(this << interval << stopTime);
    m_broadcastInterval = interval;
    m_stopTime = stopTime;
    m_broadcastTimer.SetDelay(interval);
    m_broadcastTimer.SetFunction(&UavMacCc2420::DoBroadcast, this);
    m_broadcastTimer.Schedule();
    NS_LOG_INFO("UavMacCc2420: Started broadcast with interval " << interval.GetSeconds() << "s");
}

void
UavMacCc2420::StopBroadcast()
{
    NS_LOG_FUNCTION(this);
    m_broadcastTimer.Cancel();
    NS_LOG_INFO("UavMacCc2420: Stopped broadcast");
}

void
UavMacCc2420::SetTxPower(double txPowerDbm)
{
    NS_LOG_FUNCTION(this << txPowerDbm);
    m_txPowerDbm = txPowerDbm;
}

double
UavMacCc2420::GetTxPower() const
{
    return m_txPowerDbm;
}

void
UavMacCc2420::SetRxSensitivity(double rxSensitivityDbm)
{
    NS_LOG_FUNCTION(this << rxSensitivityDbm);
    m_rxSensitivityDbm = rxSensitivityDbm;
}

uint32_t
UavMacCc2420::GetTotalBroadcasts() const
{
    return m_totalBroadcasts;
}

uint32_t
UavMacCc2420::GetTotalReceptions() const
{
    return m_totalReceptions;
}

void
UavMacCc2420::SetBroadcastCallback(BroadcastCallback cb)
{
    NS_LOG_FUNCTION(this);
    m_broadcastCallback = cb;
}

uint32_t
UavMacCc2420::GetFragmentsSent() const
{
    if (m_originalUavMac) {
        return m_originalUavMac->GetFragmentsSent();
    }
    return 0;
}

void
UavMacCc2420::SetNumFragments(uint32_t numFragments)
{
    NS_LOG_FUNCTION(this << numFragments);
    if (m_originalUavMac) {
        m_originalUavMac->SetNumFragments(numFragments);
    }
}

uint32_t
UavMacCc2420::GetNumFragments() const
{
    if (m_originalUavMac) {
        return m_originalUavMac->GetNumFragments();
    }
    return 0;
}

void
UavMacCc2420::GenerateFragmentSet(uint32_t numFragments, double totalConfidence)
{
    NS_LOG_FUNCTION(this << numFragments << totalConfidence);
    if (m_originalUavMac) {
        m_originalUavMac->GenerateFragmentSet(numFragments, totalConfidence);
    }
}

void
UavMacCc2420::DoBroadcast()
{
    NS_LOG_FUNCTION(this);

    if (Simulator::Now() >= m_stopTime) {
        NS_LOG_DEBUG("UavMacCc2420: Stop time reached");
        return;
    }

    // Create broadcast packet with sequence number and fragment data
    Ptr<Packet> packet = Create<Packet>(256);  // 256 byte payload

    // Get UAV position
    Vector uavPos = Vector(0, 0, 0);
    if (m_uavNode && m_uavNode->GetObject<MobilityModel>()) {
        uavPos = m_uavNode->GetObject<MobilityModel>()->GetPosition();
    }

    // Invoke callback if set
    if (!m_broadcastCallback.IsNull()) {
        m_broadcastCallback(m_seqNum, uavPos, m_txPowerDbm);
    }

    m_seqNum++;
    m_totalBroadcasts++;

    // Send via CC2420 MAC (broadcast)
    Mac16Address broadcast("ff:ff");
    McpsDataRequest(packet, broadcast, false);

    NS_LOG_DEBUG("UavMacCc2420: Broadcast " << m_totalBroadcasts << " at " 
                 << Simulator::Now().GetSeconds() << "s");

    // Reschedule next broadcast
    m_broadcastTimer.Schedule(m_broadcastInterval);
}

double
UavMacCc2420::CalculateRxPower(double distanceMeters)
{
    if (distanceMeters <= 0.0) {
        return m_txPowerDbm;
    }

    // Log distance path loss model: PL(d) = PL(d0) + 10*n*log10(d/d0)
    double pathLoss = m_referenceLoss + 10.0 * m_pathLossExponent * 
                      std::log10(distanceMeters / m_referenceDistance);
    return m_txPowerDbm - pathLoss;
}

void
UavMacCc2420::ReceiveFromCc2420(Ptr<Packet> packet, Mac16Address source, double rssi)
{
    NS_LOG_FUNCTION(this << packet << source << rssi);

    // Track reception statistics
    if (rssi >= m_rxSensitivityDbm) {
        m_totalReceptions++;
        NS_LOG_DEBUG("UavMacCc2420: Received packet with RSSI " << rssi 
                     << " dBm (total receptions: " << m_totalReceptions << ")");
    } else {
        NS_LOG_DEBUG("UavMacCc2420: Received packet below sensitivity threshold (RSSI " 
                     << rssi << " dBm)");
    }
}

} // namespace wsn
} // namespace ns3
