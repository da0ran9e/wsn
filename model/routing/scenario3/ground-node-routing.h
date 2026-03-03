/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Scenario3 Ground Node Routing: Network layer routing and scheduling for ground sensor nodes
 */

#ifndef SCENARIO3_GROUND_NODE_ROUTING_H
#define SCENARIO3_GROUND_NODE_ROUTING_H

#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/mac16-address.h"
#include "ns3/core-module.h"

#include "ground-node-routing/global-startup-phase.h"

#include <map>

namespace ns3
{
namespace wsn
{
namespace scenario3
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
 * @brief Reset all ground node logic state
 */
void ResetGroundLogicState();

/**
 * @brief Get total ground node transmissions
 */
uint32_t GetGroundTotalTransmissions();

/**
 * @brief Get total ground node receptions
 */
uint32_t GetGroundTotalReceptions();

/**
 * @brief Reset routing statistics (for new scenario runs)
 */
void ResetGroundRoutingStatistics();

/**
 * @brief Get confidence value for a node from logic-only simulation
 */
double GetGroundNodeLogicConfidence(uint32_t nodeId);

/**
 * @brief Get number of fragments for a node from logic-only simulation
 */
uint32_t GetGroundNodeLogicFragments(uint32_t nodeId);

/**
 * @brief Check if a node has alerted in logic-only simulation
 */
bool HasGroundNodeLogicAlerted(uint32_t nodeId);

/**
 * @brief Get confidence value for a node from network simulation
 */
double GetGroundNodeNetworkConfidence(uint32_t nodeId);

/**
 * @brief Get number of fragments for a node from network simulation
 */
uint32_t GetGroundNodeNetworkFragments(uint32_t nodeId);

/**
 * @brief Check if a node has alerted in network simulation
 */
bool HasGroundNodeNetworkAlerted(uint32_t nodeId);

} // namespace scenario3
} // namespace wsn
} // namespace ns3

#endif // SCENARIO3_GROUND_NODE_ROUTING_H
