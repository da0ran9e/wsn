/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ground-node-mac.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#include <limits>
#include <cmath>
#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("GroundNodeMac");
NS_OBJECT_ENSURE_REGISTERED(GroundNodeMac);

TypeId
GroundNodeMac::GetTypeId()
{
    static TypeId tid = TypeId("ns3::GroundNodeMac")
                            .SetParent<Object>()
                            .SetGroupName("Wsn")
                            .AddConstructor<GroundNodeMac>();
    return tid;
}

GroundNodeMac::GroundNodeMac()
    : m_packetsReceived(0),
      m_rssiSum(0.0),
      m_minDistance(std::numeric_limits<double>::max()),
      m_receivedFragments(),
      m_receivedFragmentIds(),
      m_confidence(0.0),
      m_fragmentsProcessed(0),
      m_alerted(false),
      m_confidenceThreshold(0.75),
      m_sensorTypeSeen()
{
    NS_LOG_FUNCTION(this);
}

GroundNodeMac::~GroundNodeMac()
{
    NS_LOG_FUNCTION(this);
}

void
GroundNodeMac::ReceivePacket(uint32_t seqNum, Vector uavPos, double distance, double rssiDbm)
{
    NS_LOG_FUNCTION(this << seqNum << distance << rssiDbm);
    
    m_packetsReceived++;
    m_rssiSum += rssiDbm;
    
    if (distance < m_minDistance)
    {
        m_minDistance = distance;
    }
    
    // Invoke callback if set
    if (!m_receptionCallback.IsNull())
    {
        m_receptionCallback(seqNum, distance, rssiDbm);
    }
    
    NS_LOG_DEBUG("Ground node received packet #" << seqNum 
                 << " | Distance: " << distance << "m"
                 << " | RSSI: " << rssiDbm << " dBm");
}

uint32_t
GroundNodeMac::GetPacketsReceived() const
{
    return m_packetsReceived;
}

double
GroundNodeMac::GetAverageRssi() const
{
    if (m_packetsReceived == 0)
    {
        return 0.0;
    }
    return m_rssiSum / m_packetsReceived;
}

double
GroundNodeMac::GetMinDistance() const
{
    return m_minDistance;
}

void
GroundNodeMac::ResetStatistics()
{
    NS_LOG_FUNCTION(this);
    m_packetsReceived = 0;
    m_rssiSum = 0.0;
    m_minDistance = std::numeric_limits<double>::max();
    m_receivedFragments.clear();
    m_receivedFragmentIds.clear();
    m_confidence = 0.0;
    m_fragmentsProcessed = 0;
    m_alerted = false;
    m_sensorTypeSeen.clear();
}

void
GroundNodeMac::SetReceptionCallback(ReceptionCallback cb)
{
    m_receptionCallback = cb;
}

void
GroundNodeMac::ReceiveFragment(const Fragment& fragment, double rssiDbm)
{
    NS_LOG_FUNCTION(this << fragment.fragmentId << rssiDbm);
    
    // Check if fragment already received (deduplication)
    if (m_receivedFragmentIds.count(fragment.fragmentId) > 0)
    {
        NS_LOG_DEBUG("Fragment #" << fragment.fragmentId << " already received, ignoring duplicate");
        return;
    }
    
    // Mark fragment as received
    m_receivedFragmentIds.insert(fragment.fragmentId);
    
    // Store fragment
    m_receivedFragments.push_back(fragment);
    m_fragmentsProcessed++;
    
    // Evaluate confidence contribution
    double delta = EvaluateConfidenceFromFragment(fragment, rssiDbm);
    m_confidence = std::min(m_confidence + delta, 1.0);
    
    // Track sensor type diversity
    m_sensorTypeSeen.insert(fragment.sensorType);
    
    NS_LOG_DEBUG("Fragment #" << fragment.fragmentId 
                 << " processed | Confidence delta: " << delta 
                 << " | Total confidence: " << m_confidence);
    
    // Check alert condition
    if (m_confidence >= m_confidenceThreshold && !m_alerted)
    {
        m_alerted = true;
        NS_LOG_WARN("ALERT TRIGGERED at t=" << Simulator::Now().GetSeconds() 
                    << "s | Node confidence: " << m_confidence 
                    << " | Fragments: " << m_fragmentsProcessed);
    }
}

double
GroundNodeMac::EvaluateConfidenceFromFragment(const Fragment& frag, double rssi)
{
    // Use fragment's baseConfidence directly
    // (fragments are pre-partitioned to sum to 1.0)
    return frag.baseConfidence;
}

double
GroundNodeMac::GetConfidence() const
{
    return m_confidence;
}

uint32_t
GroundNodeMac::GetFragmentsReceived() const
{
    return m_fragmentsProcessed;
}

bool
GroundNodeMac::HasAlerted() const
{
    return m_alerted;
}
} // namespace ns3
