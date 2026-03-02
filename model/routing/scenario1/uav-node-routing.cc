/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Scenario1 UAV Node Routing: Network layer routing and scheduling for UAV nodes
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
namespace scenario1
{

NS_LOG_COMPONENT_DEFINE("Scenario1UavNodeRouting");

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

std::vector<UavLogicFragment> g_logicFragments;
uint32_t g_logicFragmentIndex = 0;
uint32_t g_logicBroadcasts = 0;
uint32_t g_logicReceptions = 0;

struct UavNetworkFragment
{
    uint32_t fragmentId;
    uint32_t sensorType;
    double baseConfidence;
};

std::vector<UavNetworkFragment> g_networkFragments;
uint32_t g_networkFragmentIndex = 0;
uint32_t g_networkSequenceNumber = 0;

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
SendUavPacketFrom(NodeContainer nodes, uint32_t srcNodeId, Mac16Address dst, uint32_t packetSize)
{
    Ptr<Node> srcNode = nodes.Get(srcNodeId);
    Ptr<Cc2420NetDevice> dev = DynamicCast<Cc2420NetDevice>(srcNode->GetDevice(0));
    if (!dev)
    {
        NS_LOG_WARN("UAV Node " << srcNodeId << " has no Cc2420NetDevice");
        return;
    }

    // Get next fragment from network fragment set (if available)
    Ptr<Packet> p;
    if (!g_networkFragments.empty())
    {
        const UavNetworkFragment& frag = g_networkFragments[g_networkFragmentIndex];
        g_networkFragmentIndex = (g_networkFragmentIndex + 1) % g_networkFragments.size();
        g_networkSequenceNumber++;
        
        // Get TX power from device PHY
        Ptr<Cc2420Phy> phy = dev->GetPhy();
        double txPowerDbm = phy ? phy->GetTxPower() : 0.0;
        
        // Create fragment data and header
        FragmentData fragData(frag.fragmentId, frag.sensorType, frag.baseConfidence,
                             g_networkSequenceNumber, static_cast<float>(txPowerDbm));
        FragmentHeader fragHeader(fragData);
        
        // Create packet with header
        uint32_t payloadSize = (packetSize > fragHeader.GetSerializedSize()) ? 
                               (packetSize - fragHeader.GetSerializedSize()) : 0;
        p = Create<Packet>(payloadSize);
        p->AddHeader(fragHeader);
        
        NS_LOG_DEBUG("UAV sending fragment #" << frag.fragmentId 
                    << " (seq=" << g_networkSequenceNumber
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
        NS_LOG_DEBUG("UAV Node " << srcNodeId << " TX to " << dst << " size=" << p->GetSize());
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
                        double interval)
{
    // Send current broadcast
    SendUavPacketFrom(nodes, uavNodeId, dst, packetSize);

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
                          interval);
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

    Simulator::Schedule(time, &SendUavPacketFrom, nodes, srcNodeId, dst, packetSize);
}

void
ScheduleUavPeriodicBroadcasts(NodeContainer nodes,
                              uint32_t uavNodeId,
                              uint32_t packetSize,
                              double startTime,
                              double endTime,
                              double interval)
{
    NS_LOG_FUNCTION(uavNodeId << packetSize << startTime << endTime << interval);

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
                       interval);

    uint32_t numBroadcasts = static_cast<uint32_t>((endTime - startTime) / interval) + 1;
    NS_LOG_INFO("UAV periodic broadcasts scheduled: " << numBroadcasts 
                << " broadcasts from t=" << startTime << "s to t=" << endTime << "s");
}

void
ResetUavLogicState()
{
    g_logicFragments.clear();
    g_logicFragmentIndex = 0;
    g_logicBroadcasts = 0;
    g_logicReceptions = 0;
}

void
GenerateUavLogicFragments(uint32_t numFragments, double totalConfidence)
{
    g_logicFragments.clear();
    g_logicFragmentIndex = 0;

    if (numFragments == 0)
    {
        return;
    }

    std::mt19937 rng(std::random_device{}());
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
        f.fragmentId = i;
        f.sensorType = sensorDist(rng);
        f.baseConfidence = (values[i] / sum) * totalConfidence;
        g_logicFragments.push_back(f);
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
                          const std::vector<Vector>& groundPositions)
{
    if (g_logicFragments.empty())
    {
        return;
    }

    const UavLogicFragment& frag = g_logicFragments[g_logicFragmentIndex];
    g_logicFragmentIndex = (g_logicFragmentIndex + 1) % g_logicFragments.size();
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

const std::map<uint32_t, uint32_t>&
GetUavPerNodeReceptions()
{
    return g_uavRxPerNode;
}

void
ResetUavRoutingStatistics()
{
    g_uavTotalTx = 0;
    g_uavTotalRx = 0;
    g_uavRxPerNode.clear();
}

void
GenerateUavNetworkFragments(uint32_t numFragments, double totalConfidence)
{
    NS_LOG_FUNCTION(numFragments << totalConfidence);
    
    g_networkFragments.clear();
    g_networkFragmentIndex = 0;
    g_networkSequenceNumber = 0;

    if (numFragments == 0)
    {
        NS_LOG_WARN("GenerateUavNetworkFragments called with numFragments=0");
        return;
    }

    std::mt19937 rng(std::random_device{}());
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
        f.fragmentId = i;
        f.sensorType = sensorDist(rng);
        f.baseConfidence = (values[i] / sum) * totalConfidence;
        g_networkFragments.push_back(f);
        
        NS_LOG_INFO("Network fragment " << i << ": type=" << f.sensorType 
                    << " conf=" << f.baseConfidence);
    }
    
    NS_LOG_INFO("Generated " << numFragments << " network fragments with total confidence " 
                << totalConfidence);
}

void
ResetUavNetworkFragments()
{
    g_networkFragments.clear();
    g_networkFragmentIndex = 0;
    g_networkSequenceNumber = 0;
}

} // namespace scenario1
} // namespace wsn
} // namespace ns3
