/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Scenario3 Ground Node Routing: Network layer routing and scheduling for ground sensor nodes
 */

#include "ground-node-routing.h"
#include "fragment.h"
#include "ground-node-routing/global-startup-phase.h"
#include "packet-header.h"

#include "ns3/cc2420-net-device.h"
#include "ns3/cc2420-mac.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"

#include <map>
#include <set>
#include <limits>
#include <algorithm>

namespace ns3
{
namespace wsn
{
namespace scenario3
{

NS_LOG_COMPONENT_DEFINE("Scenario3GroundNodeRouting");

namespace
{
uint32_t g_groundTotalTx = 0;
uint32_t g_groundTotalRx = 0;
std::map<uint32_t, uint32_t> g_groundRxPerNode;

struct GroundLogicState
{
    uint32_t packetsReceived = 0;
    double rssiSum = 0.0;
    double minDistance = std::numeric_limits<double>::max();
    std::set<uint32_t> receivedFragmentIds;
    double confidence = 0.0;
    uint32_t fragmentsProcessed = 0;
    bool alerted = false;
    double confidenceThreshold = 0.75;
};

std::map<uint32_t, GroundLogicState> g_groundLogicPerNode;

struct GroundNetworkState
{
    std::set<uint32_t> receivedFragmentIds;
    double confidence = 0.0;
    uint32_t fragmentsProcessed = 0;
    bool alerted = false;
    double confidenceThreshold = 0.75;
};

std::map<uint32_t, GroundNetworkState> g_groundNetworkPerNode;

static double
EvaluateConfidenceFromFragment(double baseConfidence, double rssiDbm)
{
    (void) rssiDbm;
    return baseConfidence;
}
} // anonymous namespace

/**
 * @brief Callback for received packets at ground node network layer
 */
static void
OnGroundNodeReceivePacket(uint32_t nodeId, Ptr<Packet> packet, Mac16Address src, double rssiDbm)
{
    g_groundTotalRx++;
    g_groundRxPerNode[nodeId]++;

    // Try to extract main packet header first
    Scenario3PacketHeader mainHeader;
    Ptr<Packet> copy = packet->Copy();
    uint32_t headerRemoved = copy->RemoveHeader(mainHeader);

    if (headerRemoved > 0)
    {
        // Check if this is a Global Setup Phase packet
        if (mainHeader.GetMessageType() == MSG_TYPE_GLOBAL_SETUP_PHASE)
        {
            uint32_t sourceNodeId = mainHeader.GetSourceNodeId();
            // NS_LOG_INFO("t=" << Simulator::Now().GetSeconds() << "s Ground Node " << nodeId
            //             << " received GLOBAL SETUP PHASE packet from node " << sourceNodeId
            //             << " (size=" << packet->GetSize() << " bytes)");

            // Delegate to Global Setup Phase handler
            HandleGlobalStartupPhasePacket(nodeId, packet, sourceNodeId, rssiDbm);
            return;  // Don't process further for setup phase packets
        }
    }

    // Try to extract fragment header for non-setup-phase packets
    FragmentHeader fragHeader;
    Ptr<Packet> fragCopy = packet->Copy();
    uint32_t removed = fragCopy->RemoveHeader(fragHeader);
    
    if (removed > 0)
    {
        FragmentData fragData = fragHeader.GetFragmentData();
        
        // Process fragment with deduplication and confidence accumulation
        GroundNetworkState& state = g_groundNetworkPerNode[nodeId];
        
        if (state.receivedFragmentIds.count(fragData.fragmentId) == 0)
        {
            // New fragment - add it
            state.receivedFragmentIds.insert(fragData.fragmentId);
            state.fragmentsProcessed++;
            
            double delta = EvaluateConfidenceFromFragment(fragData.baseConfidence, rssiDbm);
            state.confidence = std::min(1.0, state.confidence + delta);
            
            // NS_LOG_DEBUG("Ground Node " << nodeId << " processed fragment #" << fragData.fragmentId
            //              << " from UAV " << fragData.uavSourceId
            //              << " (type=" << fragData.sensorType << ", conf=" << fragData.baseConfidence
            //              << ") | Total confidence: " << state.confidence
            //              << " | Fragments: " << state.fragmentsProcessed);
            
            // Check alert condition
            if (!state.alerted && state.confidence >= state.confidenceThreshold)
            {
                state.alerted = true;
                NS_LOG_WARN("*** ALERT *** Ground Node " << nodeId
                           << " at t=" << Simulator::Now().GetSeconds() << "s"
                           << " | Confidence: " << state.confidence
                           << " | Fragments: " << state.fragmentsProcessed);
            }
        }
        else
        {
            NS_LOG_DEBUG("Ground Node " << nodeId << " ignored duplicate fragment #" 
                        << fragData.fragmentId);
        }
    }

    // NS_LOG_INFO("t=" << Simulator::Now().GetSeconds() << "s Ground Node " << nodeId 
    //                  << " RX from " << src << " size=" << packet->GetSize()
    //                  << " RSSI=" << rssiDbm << " dBm");
}

/**
 * @brief Attach RX callback to a ground node network device
 */
static void
AttachGroundRxCallback(Ptr<NetDevice> dev, uint32_t nodeId)
{
    Ptr<Cc2420NetDevice> cc2420Dev = DynamicCast<Cc2420NetDevice>(dev);
    if (cc2420Dev)
    {
        Ptr<Cc2420Mac> mac = cc2420Dev->GetMac();
        if (mac)
        {
            mac->SetMcpsDataIndicationCallback(MakeBoundCallback(&OnGroundNodeReceivePacket, nodeId));
        }
    }
}

/**
 * @brief Internal function to send packet from a ground node
 */
static void
SendGroundPacketFrom(NodeContainer nodes, uint32_t srcNodeId, Mac16Address dst, uint32_t packetSize)
{
    Ptr<Node> srcNode = nodes.Get(srcNodeId);
    Ptr<Cc2420NetDevice> dev = DynamicCast<Cc2420NetDevice>(srcNode->GetDevice(0));
    if (!dev)
    {
        NS_LOG_WARN("Ground Node " << srcNodeId << " has no Cc2420NetDevice");
        return;
    }

    Ptr<Packet> p = Create<Packet>(packetSize);
    if (dev->Send(p, dst, 0))
    {
        g_groundTotalTx++;
        NS_LOG_DEBUG("Ground Node " << srcNodeId << " TX to " << dst << " size=" << packetSize);
    }
}

void
InitializeGroundNodeRouting(NetDeviceContainer devices, uint32_t packetSize)
{
    NS_LOG_FUNCTION_NOARGS();

    // Reset statistics
    ResetGroundRoutingStatistics();

    // Attach RX callbacks to all ground node devices
    for (uint32_t i = 0; i < devices.GetN(); ++i)
    {
        AttachGroundRxCallback(devices.Get(i), i);
    }

    //NS_LOG_INFO("Ground node routing initialized for " << devices.GetN() << " devices");
}

void
ScheduleGroundTransmission(NodeContainer nodes,
                          uint32_t srcNodeId,
                          Mac16Address dst,
                          uint32_t packetSize,
                          Time time)
{
    NS_LOG_FUNCTION(srcNodeId << dst << packetSize << time);

    Simulator::Schedule(time, &SendGroundPacketFrom, nodes, srcNodeId, dst, packetSize);
}

// ScheduleGroundTrafficPattern moved to scenario3.cc for proper architectural separation

uint32_t
GetGroundTotalTransmissions()
{
    return g_groundTotalTx;
}

uint32_t
GetGroundTotalReceptions()
{
    return g_groundTotalRx;
}

void
ResetGroundRoutingStatistics()
{
    g_groundTotalTx = 0;
    g_groundTotalRx = 0;
    g_groundRxPerNode.clear();
}

void
ProcessGroundLogicReception(uint32_t nodeId,
                            uint32_t seqNum,
                            double distance,
                            double rssiDbm)
{
    (void) seqNum;

    GroundLogicState& state = g_groundLogicPerNode[nodeId];
    state.packetsReceived++;
    state.rssiSum += rssiDbm;
    if (distance >= 0.0 && distance < state.minDistance)
    {
        state.minDistance = distance;
    }
}

void
ProcessGroundLogicFragment(uint32_t nodeId,
                           uint32_t fragmentId,
                           uint32_t sensorType,
                           double baseConfidence,
                           double rssiDbm)
{
    (void) sensorType;

    GroundLogicState& state = g_groundLogicPerNode[nodeId];
    if (state.receivedFragmentIds.count(fragmentId) > 0)
    {
        return;
    }

    state.receivedFragmentIds.insert(fragmentId);
    state.fragmentsProcessed++;

    const double delta = EvaluateConfidenceFromFragment(baseConfidence, rssiDbm);
    state.confidence = std::min(1.0, state.confidence + delta);

    if (!state.alerted && state.confidence >= state.confidenceThreshold)
    {
        state.alerted = true;
        // NS_LOG_INFO("t=" << Simulator::Now().GetSeconds() << "s Ground Node " << nodeId
        //                   << " ALERT confidence=" << state.confidence
        //                   << " fragments=" << state.fragmentsProcessed);
    }
}

void
ResetGroundLogicState()
{
    g_groundLogicPerNode.clear();
}

double
GetGroundNodeLogicConfidence(uint32_t nodeId)
{
    const auto it = g_groundLogicPerNode.find(nodeId);
    if (it == g_groundLogicPerNode.end())
    {
        return 0.0;
    }
    return it->second.confidence;
}

uint32_t
GetGroundNodeLogicFragments(uint32_t nodeId)
{
    const auto it = g_groundLogicPerNode.find(nodeId);
    if (it == g_groundLogicPerNode.end())
    {
        return 0;
    }
    return it->second.fragmentsProcessed;
}

bool
HasGroundNodeLogicAlerted(uint32_t nodeId)
{
    const auto it = g_groundLogicPerNode.find(nodeId);
    if (it == g_groundLogicPerNode.end())
    {
        return false;
    }
    return it->second.alerted;
}

double
GetGroundNodeNetworkConfidence(uint32_t nodeId)
{
    const auto it = g_groundNetworkPerNode.find(nodeId);
    if (it == g_groundNetworkPerNode.end())
    {
        return 0.0;
    }
    return it->second.confidence;
}

uint32_t
GetGroundNodeNetworkFragments(uint32_t nodeId)
{
    const auto it = g_groundNetworkPerNode.find(nodeId);
    if (it == g_groundNetworkPerNode.end())
    {
        return 0;
    }
    return it->second.fragmentsProcessed;
}

bool
HasGroundNodeNetworkAlerted(uint32_t nodeId)
{
    const auto it = g_groundNetworkPerNode.find(nodeId);
    if (it == g_groundNetworkPerNode.end())
    {
        return false;
    }
    return it->second.alerted;
}

} // namespace scenario3
} // namespace wsn
} // namespace ns3
