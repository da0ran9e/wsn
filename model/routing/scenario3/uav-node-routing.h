/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Scenario3 UAV Node Routing: Network layer routing and scheduling for multiple UAV nodes
 */

#ifndef SCENARIO3_UAV_NODE_ROUTING_H
#define SCENARIO3_UAV_NODE_ROUTING_H

#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/mac16-address.h"
#include "ns3/core-module.h"

#include <map>
#include <vector>
#include <cstdint>

namespace ns3
{
namespace wsn
{
// Forward declaration of DataFragment
struct DataFragment;

namespace scenario3
{

/**
 * @brief Initialize routing/network layer for UAV nodes
 * 
 * Sets up callbacks and statistics tracking for UAV network operations.
 * 
 * @param devices Network devices to attach callbacks to
 * @param packetSize Default packet size for transmissions
 */
void InitializeUavNodeRouting(NetDeviceContainer devices, uint32_t packetSize);

/**
 * @brief Schedule periodic UAV broadcasts for multi-UAV scenario
 * 
 * Schedules periodic broadcast transmissions from UAV using provided fragments.
 * 
 * @param nodes Node container
 * @param uavNodeId UAV node ID
 * @param fragments Vector of data fragments to broadcast
 * @param packetSize Packet size in bytes
 * @param startTime Start time for broadcasts
 * @param endTime End time for broadcasts
 * @param interval Interval between broadcasts
 * @param uavIndex Index of this UAV (0, 1, 2, ...)
 */
void ScheduleUavPeriodicBroadcasts(NodeContainer nodes,
                                   uint32_t uavNodeId,
                                   const std::vector<ns3::wsn::DataFragment>& fragments,
                                   uint32_t packetSize,
                                   double startTime,
                                   double endTime,
                                   double interval,
                                   uint32_t uavIndex);

/**
 * @brief Get total UAV transmissions
 */
uint32_t GetUavTotalTransmissions();

/**
 * @brief Get total UAV receptions
 */
uint32_t GetUavTotalReceptions();

} // namespace scenario3
} // namespace wsn
} // namespace ns3

#endif // SCENARIO3_UAV_NODE_ROUTING_H
