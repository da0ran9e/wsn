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

namespace ns3
{
namespace wsn
{
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
 * @brief Schedule a transmission from a UAV node
 * 
 * @param nodes Node container
 * @param srcNodeId Source UAV node ID
 * @param dst Destination MAC address
 * @param packetSize Size of packet to send
 * @param time Time to schedule the transmission
 */
void ScheduleUavTransmission(NodeContainer nodes,
                             uint32_t srcNodeId,
                             Mac16Address dst,
                             uint32_t packetSize,
                             Time time);

/**
 * @brief Schedule periodic UAV broadcasts for multi-UAV scenario
 * 
 * Schedules periodic broadcast transmissions from UAV during its flight.
 * Supports multiple UAVs with separate fragment sets per UAV.
 * 
 * @param nodes Node container
 * @param uavNodeId UAV node ID
 * @param packetSize Packet size in bytes
 * @param startTime Start time for broadcasts
 * @param endTime End time for broadcasts
 * @param interval Interval between broadcasts
 * @param uavIndex Index of this UAV (0, 1, 2, ...)
 */
void ScheduleUavPeriodicBroadcasts(NodeContainer nodes,
                                   uint32_t uavNodeId,
                                   uint32_t packetSize,
                                   double startTime,
                                   double endTime,
                                   double interval,
                                   uint32_t uavIndex);

/**
 * @brief Reset logic-only UAV state for a specific UAV
 */
void ResetUavLogicState(uint32_t uavIndex);

/**
 * @brief Generate logic-only fragment set (confidence sum = totalConfidence) for a UAV
 */
void GenerateUavLogicFragments(uint32_t numFragments, 
                               double totalConfidence, 
                               uint32_t uavIndex);

/**
 * @brief Simulate one UAV logic-only broadcast over ground node positions
 */
void SimulateUavLogicBroadcast(uint32_t broadcastId,
                               Vector uavPosition,
                               double txPowerDbm,
                               double rxSensitivityDbm,
                               const std::vector<Vector>& groundPositions,
                               uint32_t uavIndex);

/**
 * @brief Get total logic broadcasts from all UAVs
 */
uint32_t GetUavLogicTotalBroadcasts();

/**
 * @brief Get total logic receptions from all UAVs
 */
uint32_t GetUavLogicTotalReceptions();

/**
 * @brief Reset routing statistics (for new scenario runs)
 */
void ResetUavRoutingStatistics();

/**
 * @brief Get total UAV transmissions
 */
uint32_t GetUavTotalTransmissions();

/**
 * @brief Get total UAV receptions
 */
uint32_t GetUavTotalReceptions();

/**
 * @brief Generate network fragments for a UAV
 */
void GenerateUavNetworkFragments(uint32_t numFragments, 
                                 double totalConfidence,
                                 uint32_t uavIndex);

} // namespace scenario3
} // namespace wsn
} // namespace ns3

#endif // SCENARIO3_UAV_NODE_ROUTING_H
