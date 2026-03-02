/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Scenario1 UAV Node Routing: Network layer routing and scheduling for UAV nodes
 */

#ifndef SCENARIO1_UAV_NODE_ROUTING_H
#define SCENARIO1_UAV_NODE_ROUTING_H

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
namespace scenario1
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
 * @brief Schedule periodic UAV broadcasts
 * 
 * Schedules periodic broadcast transmissions from UAV during its flight.
 * 
 * @param nodes Node container
 * @param uavNodeId UAV node ID
 * @param packetSize Packet size in bytes
 * @param startTime Start time for broadcasts
 * @param endTime End time for broadcasts
 * @param interval Interval between broadcasts
 */
void ScheduleUavPeriodicBroadcasts(NodeContainer nodes,
                                   uint32_t uavNodeId,
                                   uint32_t packetSize,
                                   double startTime,
                                   double endTime,
                                   double interval);

/**
 * @brief Reset logic-only UAV state
 */
void ResetUavLogicState();

/**
 * @brief Generate logic-only fragment set (confidence sum = totalConfidence)
 */
void GenerateUavLogicFragments(uint32_t numFragments, double totalConfidence = 1.0);

/**
 * @brief Simulate one UAV logic-only broadcast over ground node positions
 */
void SimulateUavLogicBroadcast(uint32_t seqNum,
                               const Vector& uavPosition,
                               double txPowerDbm,
                               double rxSensitivityDbm,
                               const std::vector<Vector>& groundPositions);

/**
 * @brief Log-distance RX power model used in logic-only simulation
 */
double CalculateUavLogicRxPower(double distanceMeters, double txPowerDbm);

/**
 * @brief Number of logic-only UAV broadcasts
 */
uint32_t GetUavLogicTotalBroadcasts();

/**
 * @brief Number of logic-only successful receptions across all ground nodes
 */
uint32_t GetUavLogicTotalReceptions();

/**
 * @brief Get total number of transmissions from UAV nodes
 * @return Total UAV TX count
 */
uint32_t GetUavTotalTransmissions();

/**
 * @brief Get total number of receptions by UAV nodes
 * @return Total UAV RX count
 */
uint32_t GetUavTotalReceptions();

/**
 * @brief Get per-UAV reception statistics
 * @return Map of UAV node ID to RX count
 */
const std::map<uint32_t, uint32_t>& GetUavPerNodeReceptions();

/**
 * @brief Reset UAV node routing statistics
 */
void ResetUavRoutingStatistics();

/**
 * @brief Generate network fragment set for UAV broadcasts
 * 
 * Creates a set of fragments with random confidence values that sum to totalConfidence.
 * These fragments will be sent sequentially in UAV broadcasts with FragmentHeader.
 * 
 * @param numFragments Number of fragments to generate
 * @param totalConfidence Total confidence to distribute (default 1.0)
 */
void GenerateUavNetworkFragments(uint32_t numFragments, double totalConfidence = 1.0);

/**
 * @brief Reset network fragment state
 */
void ResetUavNetworkFragments();

} // namespace scenario1
} // namespace wsn
} // namespace ns3

#endif // SCENARIO1_UAV_NODE_ROUTING_H
