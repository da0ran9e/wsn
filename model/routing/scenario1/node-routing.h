/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Scenario1 Node Routing: Network layer routing and scheduling logic
 */

#ifndef SCENARIO1_NODE_ROUTING_H
#define SCENARIO1_NODE_ROUTING_H

#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/mac16-address.h"
#include "ns3/core-module.h"

namespace ns3
{
namespace wsn
{
namespace scenario1
{

/**
 * @brief Initialize routing/network layer for scenario1
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
 * @brief Schedule standard scenario1 traffic pattern
 * 
 * Schedules broadcasts from center and four corner nodes at predefined intervals.
 * 
 * @param nodes Node container
 * @param gridSize Grid size (N x N)
 * @param packetSize Packet size in bytes
 * @param firstTxTime Time of first transmission
 */
void ScheduleScenario1TrafficPattern(NodeContainer nodes,
                                    uint32_t gridSize,
                                    uint32_t packetSize,
                                    double firstTxTime);

/**
 * @brief Get total number of transmissions
 * @return Total TX count
 */
uint32_t GetTotalTransmissions();

/**
 * @brief Get total number of receptions
 * @return Total RX count
 */
uint32_t GetTotalReceptions();

/**
 * @brief Get per-node reception statistics
 * @return Map of node ID to RX count
 */
const std::map<uint32_t, uint32_t>& GetPerNodeReceptions();

/**
 * @brief Reset all routing statistics
 */
void ResetRoutingStatistics();

} // namespace scenario1
} // namespace wsn
} // namespace ns3

#endif // SCENARIO1_NODE_ROUTING_H
