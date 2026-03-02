/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Scenario1 Ground Node Routing: Network layer routing and scheduling for ground sensor nodes
 */

#ifndef SCENARIO1_GROUND_NODE_ROUTING_H
#define SCENARIO1_GROUND_NODE_ROUTING_H

#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/mac16-address.h"
#include "ns3/core-module.h"

#include <map>

namespace ns3
{
namespace wsn
{
namespace scenario1
{

/**
 * @brief Initialize routing/network layer for ground sensor nodes
 * 
 * Sets up callbacks and statistics tracking for ground node network operations.
 * 
 * @param devices Network devices to attach callbacks to
 * @param packetSize Default packet size for transmissions
 */
void InitializeGroundNodeRouting(NetDeviceContainer devices, uint32_t packetSize);

/**
 * @brief Schedule a transmission from a specific ground node
 * 
 * @param nodes Node container
 * @param srcNodeId Source node ID
 * @param dst Destination MAC address
 * @param packetSize Size of packet to send
 * @param time Time to schedule the transmission
 */
void ScheduleGroundTransmission(NodeContainer nodes,
                                uint32_t srcNodeId,
                                Mac16Address dst,
                                uint32_t packetSize,
                                Time time);

/**
 * @brief Schedule standard scenario1 traffic pattern for ground nodes
 * 
 * Schedules broadcasts from center and four corner nodes at predefined intervals.
 * 
 * @param nodes Node container
 * @param gridSize Grid size (N x N)
 * @param packetSize Packet size in bytes
 * @param firstTxTime Time of first transmission
 */
void ScheduleGroundTrafficPattern(NodeContainer nodes,
                                  uint32_t gridSize,
                                  uint32_t packetSize,
                                  double firstTxTime);

/**
 * @brief Process one logic-only reception event at a ground node
 *
 * This path does not use NetDevice/MAC interactions.
 */
void ProcessGroundLogicReception(uint32_t nodeId,
                                 uint32_t seqNum,
                                 double distance,
                                 double rssiDbm);

/**
 * @brief Process one logic-only fragment at a ground node
 *
 * Implements deduplication + confidence accumulation + alerting.
 */
void ProcessGroundLogicFragment(uint32_t nodeId,
                                uint32_t fragmentId,
                                uint32_t sensorType,
                                double baseConfidence,
                                double rssiDbm);

/**
 * @brief Reset logic-only ground node state
 */
void ResetGroundLogicState();

/**
 * @brief Get confidence of one ground node (logic-only path)
 */
double GetGroundNodeLogicConfidence(uint32_t nodeId);

/**
 * @brief Get number of processed fragments of one ground node (logic-only path)
 */
uint32_t GetGroundNodeLogicFragments(uint32_t nodeId);

/**
 * @brief Check whether one ground node already triggered alert (logic-only path)
 */
bool HasGroundNodeLogicAlerted(uint32_t nodeId);

/**
 * @brief Get confidence of one ground node (network-based path from actual packets)
 */
double GetGroundNodeNetworkConfidence(uint32_t nodeId);

/**
 * @brief Get number of processed fragments of one ground node (network-based path)
 */
uint32_t GetGroundNodeNetworkFragments(uint32_t nodeId);

/**
 * @brief Check whether one ground node triggered alert (network-based path)
 */
bool HasGroundNodeNetworkAlerted(uint32_t nodeId);

/**
 * @brief Get total number of transmissions from ground nodes
 * @return Total TX count
 */
uint32_t GetGroundTotalTransmissions();

/**
 * @brief Get total number of receptions by ground nodes
 * @return Total RX count
 */
uint32_t GetGroundTotalReceptions();

/**
 * @brief Get per-node reception statistics for ground nodes
 * @return Map of node ID to RX count
 */
const std::map<uint32_t, uint32_t>& GetGroundPerNodeReceptions();

/**
 * @brief Reset ground node routing statistics
 */
void ResetGroundRoutingStatistics();

} // namespace scenario1
} // namespace wsn
} // namespace ns3

#endif // SCENARIO1_GROUND_NODE_ROUTING_H
