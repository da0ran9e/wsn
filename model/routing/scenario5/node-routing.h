/*
 * Scenario 4 - Routing Initialization Interface
 */

#ifndef SCENARIO5_NODE_ROUTING_H
#define SCENARIO5_NODE_ROUTING_H

#include <cstdint>

namespace ns3 {
namespace wsn {
namespace scenario5 {
namespace routing {

/**
 * Initialize base station component.
 */
void InitializeBaseStation(uint32_t nodeId);

/**
 * Trigger one BS control tick.
 * BS will pull latest shared topology snapshot if available.
 */
void TickBaseStationControl();

/**
 * Initialize UAV flight - schedule waypoint movements based on planned paths.
 */
void InitializeUavFlight();

/**
 * Initialize UAV fragment broadcast.
 */
void InitializeUavBroadcast();

void InitializeCellCooperationTimeout();

/**
 * Mark UAV2 mission as completed early.
 */
void MarkUav2MissionCompleted(uint32_t triggerNodeId, double triggerConfidence);

/**
 * Check if UAV1 mission already completed.
 */
bool IsUav1MissionCompleted();

/**
 * Check if UAV2 mission already completed.
 */
bool IsUav2MissionCompleted();

/**
 * Get UAV1 mission completion time in seconds, or -1 if incomplete.
 */
double GetUav1MissionCompletedTime();

/**
 * Get UAV2 mission completion time in seconds, or -1 if incomplete.
 */
double GetUav2MissionCompletedTime();


} // namespace routing
} // namespace scenario5
} // namespace wsn
} // namespace ns3

#endif // SCENARIO5_NODE_ROUTING_H
