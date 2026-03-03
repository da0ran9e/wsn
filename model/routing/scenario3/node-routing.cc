/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Scenario3 Node Routing: Network layer routing and scheduling logic
 */

#include "node-routing.h"
#include "ground-node-routing.h"
#include "uav-node-routing.h"

#include "ns3/log.h"

namespace ns3
{
namespace wsn
{
namespace scenario3
{

NS_LOG_COMPONENT_DEFINE("Scenario3NodeRouting");

void
InitializeNodeRouting(NetDeviceContainer devices, uint32_t packetSize)
{
    // Initialize ground node routing (inherited from ground-node-routing.h)
    InitializeGroundNodeRouting(devices, packetSize);
}

void
ScheduleTransmission(NodeContainer nodes,
                    uint32_t srcNodeId,
                    Mac16Address dst,
                    uint32_t packetSize,
                    Time time)
{
    // Delegate to ground node routing for now
    ScheduleGroundTransmission(nodes, srcNodeId, dst, packetSize, time);
}

void
ScheduleTrafficPattern(NodeContainer nodes,
                      uint32_t gridSize,
                      uint32_t packetSize,
                      double firstTxTime)
{
    // This function is deprecated - network-level scheduling moved to scenario3.cc
    // For backward compatibility, just schedule transmissions for individual nodes
    NS_LOG_WARN("ScheduleTrafficPattern is deprecated. Use ScheduleScenario3GroundTrafficPattern in scenario3.cc");
    
    // Simple fallback: just delegate to per-node transmission scheduling
    ScheduleGroundTransmission(nodes, 0, Mac16Address("FF:FF"), 64, Seconds(firstTxTime));
}

} // namespace scenario3
} // namespace wsn
} // namespace ns3
