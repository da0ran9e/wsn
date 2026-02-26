/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "uav-mac.h"
#include "ground-node-mac.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/double.h"

#include <cmath>
#include <iomanip>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("UavMac");
NS_OBJECT_ENSURE_REGISTERED(UavMac);

TypeId
UavMac::GetTypeId()
{
    static TypeId tid = TypeId("ns3::UavMac")
                            .SetParent<Object>()
                            .SetGroupName("Wsn")
                            .AddConstructor<UavMac>()
                            .AddAttribute("TxPower",
                                          "Transmission power in dBm",
                                          DoubleValue(0.0),
                                          MakeDoubleAccessor(&UavMac::m_txPowerDbm),
                                          MakeDoubleChecker<double>())
                            .AddAttribute("RxSensitivity",
                                          "Receiver sensitivity in dBm",
                                          DoubleValue(-95.0),
                                          MakeDoubleAccessor(&UavMac::m_rxSensitivityDbm),
                                          MakeDoubleChecker<double>())
                            .AddAttribute("PathLossExponent",
                                          "Path loss exponent",
                                          DoubleValue(3.0),
                                          MakeDoubleAccessor(&UavMac::m_pathLossExponent),
                                          MakeDoubleChecker<double>())
                            .AddAttribute("ReferenceLoss",
                                          "Reference loss at 1m in dB (2.4GHz)",
                                          DoubleValue(46.6776),
                                          MakeDoubleAccessor(&UavMac::m_referenceLoss),
                                          MakeDoubleChecker<double>());
    return tid;
}

UavMac::UavMac()
    : m_seqNum(0),
      m_txPowerDbm(0.0),
      m_rxSensitivityDbm(-95.0),
      m_referenceLoss(46.6776),
      m_pathLossExponent(3.0),
      m_referenceDistance(1.0),
      m_totalBroadcasts(0),
      m_totalReceptions(0),
      m_fragmentSet(),
      m_currentFragmentIndex(0),
      m_numFragments(10),
      m_rng(std::random_device{}())
{
    NS_LOG_FUNCTION(this);
    m_broadcastTimer.SetFunction(&UavMac::DoBroadcast, this);
}

UavMac::~UavMac()
{
    NS_LOG_FUNCTION(this);
}

void
UavMac::Initialize(Ptr<Node> uavNode, NodeContainer groundNodes)
{
    NS_LOG_FUNCTION(this << uavNode << groundNodes.GetN());
    m_uavNode = uavNode;
    m_groundNodes = groundNodes;
}

void
UavMac::StartBroadcast(Time interval, Time stopTime)
{
    NS_LOG_FUNCTION(this << interval << stopTime);
    m_broadcastInterval = interval;
    m_stopTime = stopTime;
    m_seqNum = 1;
    
    // Schedule first broadcast
    m_broadcastTimer.Schedule(Seconds(1.0)); // Start after 1s
}

void
UavMac::StopBroadcast()
{
    NS_LOG_FUNCTION(this);
    m_broadcastTimer.Cancel();
}

void
UavMac::DoBroadcast()
{
    NS_LOG_FUNCTION(this);
    
    Time now = Simulator::Now();
    if (now >= m_stopTime)
    {
        NS_LOG_INFO("Broadcast stopped at t=" << now.GetSeconds() << "s");
        return;
    }
    
    m_totalBroadcasts++;
    
    Ptr<MobilityModel> uavMobility = m_uavNode->GetObject<MobilityModel>();
    Vector uavPos = uavMobility->GetPosition();
    
    NS_LOG_INFO(std::fixed << std::setprecision(2)
                << "\n[t=" << now.GetSeconds() << "s] UAV Broadcast #" << m_seqNum);
    NS_LOG_INFO("  UAV Position: (" << uavPos.x << ", " << uavPos.y << ", " << uavPos.z << ")");
    NS_LOG_INFO("  TX Power: " << m_txPowerDbm << " dBm");
    
    // Invoke callback if set
    if (!m_broadcastCallback.IsNull())
    {
        m_broadcastCallback(m_seqNum, uavPos, m_txPowerDbm);
    }
    
    // Get next fragment from pre-generated set (sequential, looping)
    if (m_fragmentSet.empty())
    {
        NS_LOG_ERROR("Fragment set is empty! Call GenerateFragmentSet() first.");
        return;
    }
    
    Fragment frag = m_fragmentSet[m_currentFragmentIndex];
    
    // Update fragment position and timestamp for current broadcast
    frag.broadcastPosition = uavPos;
    frag.timestamp = Simulator::Now().GetNanoSeconds();
    
    NS_LOG_DEBUG("Broadcasting fragment " << m_currentFragmentIndex << "/" << m_numFragments
                 << " (ID: " << frag.fragmentId << ", Conf: " << frag.baseConfidence << ")");
    
    // Move to next fragment (loop back to 0 after last)
    m_currentFragmentIndex = (m_currentFragmentIndex + 1) % m_numFragments;
    
    // Calculate reception for each ground node
    uint32_t successfulReceptions = 0;
    for (uint32_t i = 0; i < m_groundNodes.GetN(); i++)
    {
        Ptr<Node> groundNode = m_groundNodes.Get(i);
        Ptr<MobilityModel> groundMobility = groundNode->GetObject<MobilityModel>();
        Vector groundPos = groundMobility->GetPosition();
        
        double distance = uavMobility->GetDistanceFrom(groundMobility);
        double rxPowerDbm = CalculateRxPower(distance);
        
        // Check if packet can be received
        if (rxPowerDbm >= m_rxSensitivityDbm)
        {
            successfulReceptions++;
            m_totalReceptions++;
            
            // Deliver to ground node MAC
            Ptr<GroundNodeMac> groundMac = groundNode->GetObject<GroundNodeMac>();
            if (groundMac)
            {
                groundMac->ReceivePacket(m_seqNum, uavPos, distance, rxPowerDbm);
                // Also deliver fragment for confidence accumulation
                groundMac->ReceiveFragment(frag, rxPowerDbm);
            }
            
            NS_LOG_INFO("  ✓ Node " << groundNode->GetId() 
                        << " @ (" << groundPos.x << ", " << groundPos.y << ")"
                        << " | Distance: " << std::fixed << std::setprecision(1) << distance << "m"
                        << " | RSSI: " << std::fixed << std::setprecision(1) << rxPowerDbm << " dBm");
        }
        else
        {
            NS_LOG_DEBUG("  ✗ Node " << groundNode->GetId() 
                         << " out of range (RSSI: " << rxPowerDbm << " dBm)");
        }
    }
    
    NS_LOG_INFO("  Reception: " << successfulReceptions << "/" << m_groundNodes.GetN() << " nodes");
    
    m_seqNum++;
    
    // Schedule next broadcast
    m_broadcastTimer.Schedule(m_broadcastInterval);
}

double
UavMac::CalculateRxPower(double distanceMeters)
{
    // Log distance path loss: PL(d) = PL(d0) + 10*n*log10(d/d0)
    if (distanceMeters < m_referenceDistance)
    {
        distanceMeters = m_referenceDistance;
    }
    
    double pathLossDb = m_referenceLoss + 
                        10.0 * m_pathLossExponent * std::log10(distanceMeters / m_referenceDistance);
    double rxPowerDbm = m_txPowerDbm - pathLossDb;
    
    return rxPowerDbm;
}

void
UavMac::SetTxPower(double txPowerDbm)
{
    NS_LOG_FUNCTION(this << txPowerDbm);
    m_txPowerDbm = txPowerDbm;
}

double
UavMac::GetTxPower() const
{
    return m_txPowerDbm;
}

void
UavMac::SetRxSensitivity(double rxSensitivityDbm)
{
    NS_LOG_FUNCTION(this << rxSensitivityDbm);
    m_rxSensitivityDbm = rxSensitivityDbm;
}

uint32_t
UavMac::GetTotalBroadcasts() const
{
    return m_totalBroadcasts;
}

uint32_t
UavMac::GetTotalReceptions() const
{
    return m_totalReceptions;
}

uint32_t
UavMac::GetFragmentsSent() const
{
    return m_totalBroadcasts;
}

void
UavMac::GenerateFragmentSet(uint32_t numFragments, double totalConfidence)
{
    NS_LOG_FUNCTION(this << numFragments << totalConfidence);
    
    m_fragmentSet.clear();
    m_numFragments = numFragments;
    m_currentFragmentIndex = 0;
    
    if (numFragments == 0)
    {
        NS_LOG_ERROR("numFragments must be > 0");
        return;
    }
    
    // Generate random confidence values that sum to totalConfidence
    std::vector<double> confidences(numFragments);
    double sum = 0.0;
    
    // Generate random values
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (uint32_t i = 0; i < numFragments; i++)
    {
        confidences[i] = dist(m_rng);
        sum += confidences[i];
    }
    
    // Normalize to sum to totalConfidence
    for (uint32_t i = 0; i < numFragments; i++)
    {
        confidences[i] = (confidences[i] / sum) * totalConfidence;
    }
    
    // Create fragments with random sensor types
    std::uniform_int_distribution<uint32_t> sensorDist(0, 3);
    
    for (uint32_t i = 0; i < numFragments; i++)
    {
        uint32_t sensorType = sensorDist(m_rng);
        Fragment frag(i, sensorType, confidences[i], Vector(0, 0, 0), 0, m_txPowerDbm);
        m_fragmentSet.push_back(frag);
        
        NS_LOG_INFO("Generated fragment " << i << ": Type=" << sensorType 
                    << ", Conf=" << std::fixed << std::setprecision(4) << confidences[i]);
    }
    
    // Verify sum
    double actualSum = 0.0;
    for (const auto& f : m_fragmentSet)
    {
        actualSum += f.baseConfidence;
    }
    NS_LOG_INFO("Fragment set created: " << numFragments << " fragments, total confidence: " 
                << std::fixed << std::setprecision(4) << actualSum);
}

void
UavMac::SetNumFragments(uint32_t numFragments)
{
    m_numFragments = numFragments;
    GenerateFragmentSet(numFragments);
}

uint32_t
UavMac::GetNumFragments() const
{
    return m_numFragments;
}

void
UavMac::SetBroadcastCallback(BroadcastCallback cb)
{
    m_broadcastCallback = cb;
}

} // namespace ns3
