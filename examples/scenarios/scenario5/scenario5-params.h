/*
 * Scenario 5 - Centralized Parameters
 *
 * All scenario and routing parameters are defined here for easy tuning.
 */

#ifndef SCENARIO5_PARAMS_H
#define SCENARIO5_PARAMS_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace ns3 {
namespace wsn {
namespace scenario5 {
namespace params {

extern std::map<uint32_t, std::map<int32_t, uint32_t>> g_intraCellRoutingTree;
extern std::map<int32_t, std::map<int32_t, std::vector<uint32_t>>> g_cellGatewayPairs;

constexpr uint32_t DEFAULT_GRID_SIZE = 20;
constexpr double DEFAULT_SPACING = 20.0;

constexpr double BS_POSITION_X = -100.0;
constexpr double BS_POSITION_Y = -100.0;
constexpr double BS_POSITION_Z = 0.0;

constexpr uint32_t DEFAULT_NUM_UAVS = 2;
constexpr double DEFAULT_UAV_ALTITUDE = 20.0;
constexpr double DEFAULT_UAV_SPEED = 20.0;

constexpr double STARTUP_PHASE_DURATION = 5.0;
constexpr double UAV_PLANNING_DELAY = 0.2;
constexpr double FRAGMENT_BROADCAST_INTERVAL = 0.2;

constexpr double COOPERATION_THRESHOLD = 0.35;
constexpr double ALERT_THRESHOLD = 0.75;
constexpr double SUSPICIOUS_COVERAGE_PERCENT = 0.30;

constexpr double BS_INIT_SUSPICIOUS_TARGET_PERCENT = SUSPICIOUS_COVERAGE_PERCENT;
constexpr uint32_t BS_INIT_SUSPICIOUS_MAX_ITERATIONS = 100;
constexpr uint32_t BS_INIT_SUSPICIOUS_MIN_TARGET_NODES = 1;

constexpr double TX_POWER_DBM = 0.0;
constexpr double RX_SENSITIVITY_DBM = -95.0;
constexpr uint32_t PACKET_SIZE = 100;
constexpr double HEX_CELL_RADIUS = 80.0;
constexpr double NEIGHBOR_DISCOVERY_RADIUS = HEX_CELL_RADIUS;

constexpr uint32_t HEX_GRID_OFFSET_BASE = 10000;
constexpr uint32_t HEX_GRID_OFFSET_MULTIPLIER = 100;
constexpr uint32_t HEX_GRID_OFFSET_EXTRA = 1000;

constexpr double UAV_PATH_ALTITUDE = DEFAULT_UAV_ALTITUDE;
constexpr double UAV_PATH_SPEED = DEFAULT_UAV_SPEED;
constexpr double UAV_WAYPOINT_HOVER_TIME = 2.0;
constexpr double UAV_PATH_MARGIN_METERS = 10.0;
constexpr double UAV_BROADCAST_RADIUS = 50.0;

constexpr double BS_INIT_UAV_STARTING_ALTITUDE = DEFAULT_UAV_ALTITUDE;
constexpr double BS_INIT_UAV_PATROL_ALTITUDE = DEFAULT_UAV_ALTITUDE;

constexpr double UAV1_SPEED = 80.0;
constexpr double UAV1_HOVER_TIME = 2.0;
constexpr double UAV2_SPEED = 80.0;
constexpr double UAV2_HOVER_TIME = 0.0;

inline int32_t
ComputeDefaultHexGridOffset(uint32_t nodeCount)
{
	const uint32_t gridSizeApprox =
		static_cast<uint32_t>(std::lround(std::sqrt(static_cast<double>(nodeCount))));
	return static_cast<int32_t>(std::max(
		HEX_GRID_OFFSET_BASE,
		gridSizeApprox * HEX_GRID_OFFSET_MULTIPLIER + HEX_GRID_OFFSET_EXTRA));
}

constexpr uint32_t DEFAULT_NUM_FRAGMENTS = 10;
constexpr double MIN_FRAGMENT_CONFIDENCE = 0.1;
constexpr double MAX_FRAGMENT_CONFIDENCE = 0.9;
constexpr uint32_t DEFAULT_FRAGMENT_SIZE_BYTES = 1024;
constexpr uint32_t DEFAULT_MASTER_FILE_SIZE_BYTES = 2 * 1024 * 1024;
constexpr double DEFAULT_MASTER_FILE_CONFIDENCE = 0.95;
constexpr double FRAGMENT_WEIGHT_MIN = 0.5;
constexpr double FRAGMENT_WEIGHT_MAX = 2.0;
constexpr uint32_t BS_INIT_FRAGMENT_GENERATION_COUNT = DEFAULT_NUM_FRAGMENTS;

extern std::ofstream* g_resultFileStream;

} // namespace params
} // namespace scenario5
} // namespace wsn
} // namespace ns3

#endif // SCENARIO5_PARAMS_H
