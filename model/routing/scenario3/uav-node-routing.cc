/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Scenario3 UAV Node Routing: Network layer routing for UAV nodes
 */

#include "uav-node-routing.h"

#include "fragment.h"
#include "../../../examples/scenarios/scenario3.h"
#include "ns3/cc2420-net-device.h"
#include "ns3/cc2420-mac.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/mac16-address.h"

#include <map>
#include <vector>

namespace ns3
{
namespace wsn
{
namespace scenario3
{

NS_LOG_COMPONENT_DEFINE("Scenario3UavNodeRouting");

namespace
{
// UAV statistics
uint32_t g_uavTotalTransmissions = 0;
uint32_t g_uavTotalReceptions = 0;

// Per-UAV fragment round-robin index
std::map<uint32_t, uint32_t> g_fragmentIndex;
std::map<uint32_t, uint32_t> g_sequenceNumber;

} // anonymous namespace

/**
 * @brief Initialize UAV node routing
 * 
 * Sets up statistics tracking and prepares the network layer for UAV operations.
 * 
 * @param devices Network devices to initialize
 * @param packetSize Default packet size
 */
void
InitializeUavNodeRouting(NetDeviceContainer devices, uint32_t packetSize)
{
    NS_LOG_FUNCTION_NOARGS();

    // Reset statistics
    g_uavTotalTransmissions = 0;
    g_uavTotalReceptions = 0;

    NS_LOG_INFO("UAV node routing initialized for " << devices.GetN() << " devices");
}

/**
 * @brief Schedule periodic UAV broadcasts
 * 
 * Schedules fragment transmissions from a UAV node at regular intervals.
 * 
 * @param nodes Node container
 * @param uavNodeId UAV node ID
 * @param fragments Vector of data fragments to broadcast
 * @param packetSize Packet size in bytes
 * @param startTime Start time in seconds
 * @param endTime End time in seconds
 * @param interval Interval between broadcasts in seconds
 * @param uavIndex Index of this UAV
 */
void
ScheduleUavPeriodicBroadcasts(NodeContainer nodes,
                              uint32_t uavNodeId,
                              const std::vector<ns3::wsn::DataFragment>& fragments,
                              uint32_t packetSize,
                              double startTime,
                              double endTime,
                              double interval,
                              uint32_t uavIndex)
{
    NS_LOG_FUNCTION(uavNodeId << fragments.size() << packetSize << startTime << endTime << interval << uavIndex);

    if (uavNodeId >= nodes.GetN())
    {
        NS_LOG_WARN("Invalid UAV node ID: " << uavNodeId);
        return;
    }

    if (interval <= 0.0)
    {
        NS_LOG_WARN("Invalid broadcast interval: " << interval);
        return;
    }

    if (fragments.empty())
    {
        NS_LOG_WARN("No fragments provided for UAV periodic broadcasts");
        return;
    }

    Mac16Address broadcast = Mac16Address("FF:FF");
    uint32_t numBroadcasts = static_cast<uint32_t>((endTime - startTime) / interval) + 1;

    NS_LOG_INFO("UAV #" << (uavIndex + 1) << " periodic broadcasts scheduled: "
                << numBroadcasts << " broadcasts from t=" << startTime << "s to t=" 
                << endTime << "s, interval=" << interval << "s, fragments=" << fragments.size());

    // Initialize fragment index for this UAV
    g_fragmentIndex[uavIndex] = 0;
    g_sequenceNumber[uavIndex] = 0;

    // Make a copy of fragments for the lambda to capture by value
    std::vector<ns3::wsn::DataFragment> fragmentsCopy = fragments;

    // Schedule periodic broadcasts at regular intervals
    for (uint32_t i = 0; i < numBroadcasts; ++i)
    {
        double txTime = startTime + (i * interval);
        
        if (txTime > endTime)
            break;

        Simulator::Schedule(Seconds(txTime), 
            [nodes, uavNodeId, broadcast, packetSize, uavIndex, fragmentsCopy]() {
            Ptr<Node> uavNode = nodes.Get(uavNodeId);
            if (!uavNode)
            {
                NS_LOG_WARN("UAV node " << uavNodeId << " not found");
                return;
            }
            Ptr<Cc2420NetDevice> dev = DynamicCast<Cc2420NetDevice>(uavNode->GetDevice(0));
            if (!dev)
            {
                NS_LOG_WARN("UAV node " << uavNodeId << " has no CC2420 device");
                return;
            }

            // Select next fragment in round-robin fashion
            const ns3::wsn::DataFragment& frag = fragmentsCopy[g_fragmentIndex[uavIndex]];
            g_fragmentIndex[uavIndex] = (g_fragmentIndex[uavIndex] + 1) % fragmentsCopy.size();
            g_sequenceNumber[uavIndex]++;

            // Create FragmentData from DataFragment
            FragmentData fragData;
            fragData.fragmentId = frag.fragmentId;
            fragData.sensorType = 0;  // Placeholder sensor type
            fragData.baseConfidence = frag.priority;  // Use priority as confidence
            fragData.sequenceNumber = g_sequenceNumber[uavIndex];
            fragData.txPowerDbm = 0.0f;  // Will be set by PHY layer
            fragData.uavSourceId = uavIndex;

            // Create packet with FragmentHeader
            Ptr<Packet> packet = Create<Packet>(packetSize);
            FragmentHeader fragHeader;
            fragHeader.SetFragmentData(fragData);
            packet->AddHeader(fragHeader);
            
            NS_LOG_INFO("UAV " << uavIndex << " TX fragment " << frag.fragmentId
                        << " seq=" << g_sequenceNumber[uavIndex]
                        << " size=" << frag.fragmentSize << " bytes"
                        << " confidence=" << frag.priority);

            // Send broadcast
            if (dev->Send(packet, broadcast, 0))
            {
                g_uavTotalTransmissions++;
                NS_LOG_INFO("UAV node " << uavNodeId << " (index=" << uavIndex 
                            << ") TX broadcast, size=" << packet->GetSize() << " bytes");
            }
        });
    }
}

/**
 * @brief Get total UAV transmissions
 * 
 * @return Total number of transmissions from all UAVs
 */
uint32_t
GetUavTotalTransmissions()
{
    return g_uavTotalTransmissions;
}

/**
 * @brief Get total UAV receptions
 * 
 * @return Total number of receptions by UAVs
 */
uint32_t
GetUavTotalReceptions()
{
    return g_uavTotalReceptions;
}

} // namespace scenario3
} // namespace wsn
} // namespace ns3
