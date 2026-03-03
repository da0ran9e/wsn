/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Scenario3 Node Routing: Network layer routing and scheduling logic
 */

#ifndef SCENARIO3_NODE_ROUTING_H
#define SCENARIO3_NODE_ROUTING_H

#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/mac16-address.h"
#include "ns3/core-module.h"

namespace ns3
{
namespace wsn
{
namespace scenario3
{

/**
 * @brief Initialize routing/network layer for scenario3
 * 
 * Sets up callbacks and statistics tracking for network layer operations.
 * 
 * @param devices Network devices to attach callbacks to
 * @param packetSize Default packet size for transmissions
 */
void InitializeNodeRouting(NetDeviceContainer devices, uint32_t packetSize);

/**
 * @brief Schedule a transmission from a specific node
 * 
 * @param nodes Node container
 * @param srcNodeId Source node ID
 * @param dst Destination MAC address
 * @param packetSize Size of packet to send
 * @param time Time to schedule the transmission
 */
void ScheduleTransmission(NodeContainer nodes,
                         uint32_t srcNodeId,
                         Mac16Address dst,
                         uint32_t packetSize,
                         Time time);

/**
 * @brief Schedule standard scenario3 traffic pattern
 * 
 * Schedules broadcasts from center and four corner nodes at predefined intervals.
 * 
 * @param nodes Nodes to schedule traffic on
 * @param gridSize Grid size (N x N)
 * @param packetSize Packet size in bytes
 * @param firstTxTime Time of first transmission
 */
void ScheduleTrafficPattern(NodeContainer nodes,
                           uint32_t gridSize,
                           uint32_t packetSize,
                           double firstTxTime);

} // namespace scenario3
} // namespace wsn
} // namespace ns3

#endif // SCENARIO3_NODE_ROUTING_H
