/*
 * Scenario 4 - Centralized Parameters
 * 
 * All scenario and routing parameters are defined here for easy tuning.
 */

#ifndef SCENARIO4_PARAMS_H
#define SCENARIO4_PARAMS_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <vector>

namespace ns3 {
namespace wsn {
namespace scenario4 {
namespace params {

// ===== ROUTING TREE STRUCTURE =====
// Global intra-cell routing tree: routing[nodeId][targetCellId] = nexthopNodeId
// Used for data aggregation and multi-hop communication within cell
extern std::map<uint32_t, std::map<int32_t, uint32_t>> g_intraCellRoutingTree;

// Per-cell gateway pairs: cellGateways[cellId][neighborCellId] = {gw1, gw2}
// Stores gateway node IDs for cross-cell communication
extern std::map<int32_t, std::map<int32_t, std::vector<uint32_t>>> g_cellGatewayPairs;

// ===== SCENARIO PARAMETERS =====

// Grid parameters
constexpr uint32_t DEFAULT_GRID_SIZE = 10;
constexpr double DEFAULT_SPACING = 20.0;

// Base station location (far from network to avoid interference)
constexpr double BS_POSITION_X = -1000.0;
constexpr double BS_POSITION_Y = -1000.0;
constexpr double BS_POSITION_Z = 0.0;

// UAV parameters
constexpr uint32_t DEFAULT_NUM_UAVS = 2;
constexpr double DEFAULT_UAV_ALTITUDE = 20.0;
constexpr double DEFAULT_UAV_SPEED = 20.0;  // m/s

// Timing constants
constexpr double STARTUP_PHASE_DURATION = 5.0;  // seconds
constexpr double UAV_PLANNING_DELAY = 0.2;      // seconds after startup
constexpr double FRAGMENT_BROADCAST_INTERVAL = 1.0;  // seconds

// ===== ROUTING PARAMETERS =====

// Thresholds
constexpr double COOPERATION_THRESHOLD = 0.35;  // trigger cell cooperation
constexpr double ALERT_THRESHOLD = 0.75;        // trigger alert state
constexpr double SUSPICIOUS_COVERAGE_PERCENT = 0.30;  // top 30% nodes

// Network parameters
constexpr double TX_POWER_DBM = 0.0;
constexpr double RX_SENSITIVITY_DBM = -95.0;
constexpr uint32_t PACKET_SIZE = 100;  // bytes
constexpr double HEX_CELL_RADIUS = 80.0;  // meters
constexpr double NEIGHBOR_DISCOVERY_RADIUS = HEX_CELL_RADIUS;  // meters

// Hex grid indexing parameters (cellId = q + r * gridOffset)
constexpr uint32_t HEX_GRID_OFFSET_BASE = 10000;
constexpr uint32_t HEX_GRID_OFFSET_MULTIPLIER = 100;
constexpr uint32_t HEX_GRID_OFFSET_EXTRA = 1000;

// UAV flight planning defaults
constexpr double UAV_PATH_ALTITUDE = DEFAULT_UAV_ALTITUDE;
constexpr double UAV_PATH_SPEED = DEFAULT_UAV_SPEED;

inline int32_t
ComputeDefaultHexGridOffset(uint32_t nodeCount)
{
	const uint32_t gridSizeApprox =
		static_cast<uint32_t>(std::lround(std::sqrt(static_cast<double>(nodeCount))));
	return static_cast<int32_t>(std::max(
		HEX_GRID_OFFSET_BASE,
		gridSizeApprox * HEX_GRID_OFFSET_MULTIPLIER + HEX_GRID_OFFSET_EXTRA));
}

// Fragment parameters
constexpr uint32_t DEFAULT_NUM_FRAGMENTS = 10;
constexpr double MIN_FRAGMENT_CONFIDENCE = 0.1;
constexpr double MAX_FRAGMENT_CONFIDENCE = 0.9;

} // namespace params
} // namespace scenario4
} // namespace wsn
} // namespace ns3

#endif // SCENARIO4_PARAMS_H
