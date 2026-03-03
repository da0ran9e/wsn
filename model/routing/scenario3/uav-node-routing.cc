/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Scenario3 UAV Node Routing: Network layer routing and scheduling for multiple UAV nodes
 */

#include "uav-node-routing.h"
#include "ground-node-routing.h"
#include "fragment.h"

#include "ns3/cc2420-net-device.h"
#include "ns3/cc2420-mac.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"

#include <map>
#include <vector>
#include <random>
#include <cmath>

namespace ns3
{
namespace wsn
{
namespace scenario3
{

NS_LOG_COMPONENT_DEFINE("Scenario3UavNodeRouting");

namespace
{
uint32_t g_uavTotalTx = 0;
uint32_t g_uavTotalRx = 0;
std::map<uint32_t, uint32_t> g_uavRxPerNode;

struct UavLogicFragment
{
    uint32_t fragmentId;
    uint32_t sensorType;
    double baseConfidence;
};

// Per-UAV logic fragments
std::map<uint32_t, std::vector<UavLogicFragment>> g_logicFragments;
std::map<uint32_t, uint32_t> g_logicFragmentIndex;
uint32_t g_logicBroadcasts = 0;
uint32_t g_logicReceptions = 0;

struct UavNetworkFragment
{
    uint32_t fragmentId;
    uint32_t sensorType;
    double baseConfidence;
};

// Per-UAV network fragments
std::map<uint32_t, std::vector<UavNetworkFragment>> g_networkFragments;
std::map<uint32_t, uint32_t> g_networkFragmentIndex;
std::map<uint32_t, uint32_t> g_networkSequenceNumber;

constexpr double kReferenceDistance = 1.0;
constexpr double kReferenceLoss = 46.6776;
constexpr double kPathLossExponent = 3.0;
} // anonymous namespace

/**
 * @brief Callback for received packets at UAV node network layer
 */
static void
OnUavNodeReceivePacket(uint32_t nodeId, Ptr<Packet> packet, Mac16Address src, double rssiDbm)
{
    g_uavTotalRx++;
    g_uavRxPerNode[nodeId]++;

    NS_LOG_INFO("t=" << Simulator::Now().GetSeconds() << "s UAV Node " << nodeId 
                     << " RX from " << src << " size=" << packet->GetSize()
                     << " RSSI=" << rssiDbm << " dBm");
}

/**
 * @brief Attach RX callback to a UAV node network device
 */
static void
AttachUavRxCallback(Ptr<NetDevice> dev, uint32_t nodeId)
{
    Ptr<Cc2420NetDevice> cc2420Dev = DynamicCast<Cc2420NetDevice>(dev);
    if (cc2420Dev)
    {
        Ptr<Cc2420Mac> mac = cc2420Dev->GetMac();
        if (mac)
        {
            mac->SetMcpsDataIndicationCallback(MakeBoundCallback(&OnUavNodeReceivePacket, nodeId));
        }
    }
}

/**
 * @brief Internal function to send packet from a UAV node
 */
static void
SendUavPacketFrom(NodeContainer nodes, uint32_t srcNodeId, Mac16Address dst, uint32_t packetSize, uint32_t uavIndex)
{
    Ptr<Node> srcNode = nodes.Get(srcNodeId);
    Ptr<Cc2420NetDevice> dev = DynamicCast<Cc2420NetDevice>(srcNode->GetDevice(0));
    if (!dev)
    {
        NS_LOG_WARN("UAV Node " << srcNodeId << " has no Cc2420NetDevice");
        return;
    }

    // Get next fragment from network fragment set for this UAV (if available)
    Ptr<Packet> p;
    if (!g_networkFragments[uavIndex].empty())
    {
        const UavNetworkFragment& frag = g_networkFragments[uavIndex][g_networkFragmentIndex[uavIndex]];
        g_networkFragmentIndex[uavIndex] = (g_networkFragmentIndex[uavIndex] + 1) % g_networkFragments[uavIndex].size();
        g_networkSequenceNumber[uavIndex]++;
        
        // Get TX power from device PHY
        Ptr<Cc2420Phy> phy = dev->GetPhy();
        double txPowerDbm = phy ? phy->GetTxPower() : 0.0;
        
        // Create fragment data and header (with UAV source ID)
        FragmentData fragData(frag.fragmentId, frag.sensorType, frag.baseConfidence,
                             g_networkSequenceNumber[uavIndex], static_cast<float>(txPowerDbm), uavIndex);
        FragmentHeader fragHeader(fragData);
        
        // Create packet with header
        uint32_t payloadSize = (packetSize > fragHeader.GetSerializedSize()) ? 
                               (packetSize - fragHeader.GetSerializedSize()) : 0;
        p = Create<Packet>(payloadSize);
        p->AddHeader(fragHeader);
        
        NS_LOG_DEBUG("UAV " << uavIndex << " sending fragment #" << frag.fragmentId 
                    << " (seq=" << g_networkSequenceNumber[uavIndex]
                    << ", type=" << frag.sensorType 
                    << ", conf=" << frag.baseConfidence << ")");
    }
    else
    {
        // No fragments configured - send plain packet
        p = Create<Packet>(packetSize);
    }

    if (dev->Send(p, dst, 0))
    {
        g_uavTotalTx++;
        NS_LOG_DEBUG("UAV Node " << srcNodeId << " (index=" << uavIndex << ") TX to " << dst << " size=" << p->GetSize());
    }
}

/**
 * @brief Internal function for periodic UAV broadcast
 */
static void
SendUavPeriodicBroadcast(NodeContainer nodes,
                        uint32_t uavNodeId,
                        Mac16Address dst,
                        uint32_t packetSize,
                        double currentTime,
                        double endTime,
                        double interval,
                        uint32_t uavIndex)
{
    // Send current broadcast
    SendUavPacketFrom(nodes, uavNodeId, dst, packetSize, uavIndex);

    // Schedule next broadcast if within time range
    double nextTime = currentTime + interval;
    if (nextTime < endTime)
    {
        Simulator::Schedule(Seconds(interval),
                          &SendUavPeriodicBroadcast,
                          nodes,
                          uavNodeId,
                          dst,
                          packetSize,
                          nextTime,
                          endTime,
                          interval,
                          uavIndex);
    }
}

void
InitializeUavNodeRouting(NetDeviceContainer devices, uint32_t packetSize)
{
    NS_LOG_FUNCTION_NOARGS();

    // Reset statistics
    ResetUavRoutingStatistics();

    // Attach RX callbacks to all UAV devices
    for (uint32_t i = 0; i < devices.GetN(); ++i)
    {
        AttachUavRxCallback(devices.Get(i), i);
    }

    NS_LOG_INFO("UAV node routing initialized for " << devices.GetN() << " devices");
}

void
ScheduleUavTransmission(NodeContainer nodes,
                       uint32_t srcNodeId,
                       Mac16Address dst,
                       uint32_t packetSize,
                       Time time)
{
    NS_LOG_FUNCTION(srcNodeId << dst << packetSize << time);

    // For basic transmission scheduling (not per-UAV), use index 0
    Simulator::Schedule(time, &SendUavPacketFrom, nodes, srcNodeId, dst, packetSize, 0);
}

void
ScheduleUavPeriodicBroadcasts(NodeContainer nodes,
                              uint32_t uavNodeId,
                              uint32_t packetSize,
                              double startTime,
                              double endTime,
                              double interval,
                              uint32_t uavIndex)
{
    NS_LOG_FUNCTION(uavNodeId << packetSize << startTime << endTime << interval << uavIndex);

    Mac16Address broadcast = Mac16Address("FF:FF");

    // Schedule first broadcast
    Simulator::Schedule(Seconds(startTime),
                       &SendUavPeriodicBroadcast,
                       nodes,
                       uavNodeId,
                       broadcast,
                       packetSize,
                       startTime,
                       endTime,
                       interval,
                       uavIndex);

    uint32_t numBroadcasts = static_cast<uint32_t>((endTime - startTime) / interval) + 1;
    NS_LOG_INFO("UAV #" << (uavIndex + 1) << " periodic broadcasts scheduled: " << numBroadcasts 
                << " broadcasts from t=" << startTime << "s to t=" << endTime << "s");
}

void
ResetUavLogicState(uint32_t uavIndex)
{
    g_logicFragments[uavIndex].clear();
    g_logicFragmentIndex[uavIndex] = 0;
}

void
GenerateUavLogicFragments(uint32_t numFragments, double totalConfidence, uint32_t uavIndex)
{
    NS_LOG_FUNCTION(numFragments << totalConfidence << uavIndex);
    
    g_logicFragments[uavIndex].clear();
    g_logicFragmentIndex[uavIndex] = 0;

    if (numFragments == 0)
    {
        return;
    }

    std::mt19937 rng(std::random_device{}() + uavIndex);  // Different seed per UAV
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    std::uniform_int_distribution<uint32_t> sensorDist(0, 3);

    std::vector<double> values(numFragments);
    double sum = 0.0;
    for (uint32_t i = 0; i < numFragments; ++i)
    {
        values[i] = dist(rng);
        sum += values[i];
    }

    for (uint32_t i = 0; i < numFragments; ++i)
    {
        UavLogicFragment f;
        f.fragmentId = uavIndex * 1000 + i;  // Unique fragment IDs per UAV
        f.sensorType = sensorDist(rng);
        f.baseConfidence = (values[i] / sum) * totalConfidence;
        g_logicFragments[uavIndex].push_back(f);
    }
}

double
CalculateUavLogicRxPower(double distanceMeters, double txPowerDbm)
{
    const double d = std::max(distanceMeters, kReferenceDistance);
    const double pathLossDb =
        kReferenceLoss + 10.0 * kPathLossExponent * std::log10(d / kReferenceDistance);
    return txPowerDbm - pathLossDb;
}

void
SimulateUavLogicBroadcast(uint32_t seqNum,
                          const Vector& uavPosition,
                          double txPowerDbm,
                          double rxSensitivityDbm,
                          const std::vector<Vector>& groundPositions,
                          uint32_t uavIndex)
{
    if (g_logicFragments[uavIndex].empty())
    {
        return;
    }

    const UavLogicFragment& frag = g_logicFragments[uavIndex][g_logicFragmentIndex[uavIndex]];
    g_logicFragmentIndex[uavIndex] = (g_logicFragmentIndex[uavIndex] + 1) % g_logicFragments[uavIndex].size();
    g_logicBroadcasts++;

    for (uint32_t i = 0; i < groundPositions.size(); ++i)
    {
        const Vector& gp = groundPositions[i];
        const double dx = uavPosition.x - gp.x;
        const double dy = uavPosition.y - gp.y;
        const double dz = uavPosition.z - gp.z;
        const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        const double rxPowerDbm = CalculateUavLogicRxPower(distance, txPowerDbm);

        if (rxPowerDbm >= rxSensitivityDbm)
        {
            g_logicReceptions++;
            ProcessGroundLogicReception(i, seqNum, distance, rxPowerDbm);
            ProcessGroundLogicFragment(i,
                                       frag.fragmentId,
                                       frag.sensorType,
                                       frag.baseConfidence,
                                       rxPowerDbm);
        }
    }
}

uint32_t
GetUavLogicTotalBroadcasts()
{
    return g_logicBroadcasts;
}

uint32_t
GetUavLogicTotalReceptions()
{
    return g_logicReceptions;
}

uint32_t
GetUavTotalTransmissions()
{
    return g_uavTotalTx;
}

uint32_t
GetUavTotalReceptions()
{
    return g_uavTotalRx;
}

void
ResetUavRoutingStatistics()
{
    g_uavTotalTx = 0;
    g_uavTotalRx = 0;
    g_uavRxPerNode.clear();
}

void
GenerateUavNetworkFragments(uint32_t numFragments, double totalConfidence, uint32_t uavIndex)
{
    NS_LOG_FUNCTION(numFragments << totalConfidence << uavIndex);
    
    g_networkFragments[uavIndex].clear();
    g_networkFragmentIndex[uavIndex] = 0;
    g_networkSequenceNumber[uavIndex] = 0;

    if (numFragments == 0)
    {
        NS_LOG_WARN("GenerateUavNetworkFragments called with numFragments=0");
        return;
    }

    std::mt19937 rng(std::random_device{}() + uavIndex);  // Different seed per UAV
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    std::uniform_int_distribution<uint32_t> sensorDist(0, 3);

    std::vector<double> values(numFragments);
    double sum = 0.0;
    for (uint32_t i = 0; i < numFragments; ++i)
    {
        values[i] = dist(rng);
        sum += values[i];
    }

    for (uint32_t i = 0; i < numFragments; ++i)
    {
        UavNetworkFragment f;
        f.fragmentId = uavIndex * 1000 + i;  // Unique fragment IDs per UAV
        f.sensorType = sensorDist(rng);
        f.baseConfidence = (values[i] / sum) * totalConfidence;
        g_networkFragments[uavIndex].push_back(f);
        
        NS_LOG_INFO("UAV " << uavIndex << " network fragment " << i << ": type=" << f.sensorType 
                    << " conf=" << f.baseConfidence);
    }
    
    NS_LOG_INFO("Generated " << numFragments << " network fragments for UAV " << uavIndex 
                << " with total confidence " << totalConfidence);
}

} // namespace scenario3
} // namespace wsn
} // namespace ns3
