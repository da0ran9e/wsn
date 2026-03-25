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
#include <fstream>
#include <map>
#include <string>
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
constexpr uint32_t DEFAULT_GRID_SIZE = 20;
constexpr double DEFAULT_SPACING = 20.0;

// Base station location (far from network to avoid interference)
constexpr double BS_POSITION_X = -100.0;
constexpr double BS_POSITION_Y = -100.0;
constexpr double BS_POSITION_Z = 0.0;

// UAV parameters
constexpr uint32_t DEFAULT_NUM_UAVS = 2;
constexpr double DEFAULT_UAV_ALTITUDE = 20.0;
constexpr double DEFAULT_UAV_SPEED = 20.0;  // m/s

// Timing constants
constexpr double STARTUP_PHASE_DURATION = 5.0;  // seconds
constexpr double UAV_PLANNING_DELAY = 0.2;      // seconds after startup
constexpr double FRAGMENT_BROADCAST_INTERVAL = 0.2;  // seconds (fallback/default)
constexpr double SUSPICIOUS_CONFIDENCE_LOG_INTERVAL = 10.0; // seconds

// ===== RADIO AND TRANSMISSION PARAMETERS =====
// CC2420 (IEEE 802.15.4 ZigBee) radio specifications
constexpr uint32_t RADIO_BITRATE_BPS = 250000;  // bits per second (250 kbps)

// ===== ROUTING PARAMETERS =====

// Thresholds
// Reliable reconstruction threshold from fragment-gen.md:
// practical confident range for automated decisions is p_comb >= 0.75..0.9.
// Use 0.75 as cooperation trigger and 0.9 for alert confirmation.
constexpr double COOPERATION_THRESHOLD = 0.3;  // trigger cell cooperation
// Stagger cooperation requests inside the same cell to reduce burst contention.
// Total extra delay = (tree level * COOPERATION_LEVEL_DELAY_STEP) + random_jitter.
constexpr double COOPERATION_LEVEL_DELAY_STEP = 0.02;  // seconds per intra-cell tree hop
constexpr double COOPERATION_RANDOM_JITTER_MAX = 0.015; // seconds, uniform [0, max]
constexpr double ALERT_THRESHOLD = 0.75;        // trigger alert state
constexpr double SUSPICIOUS_COVERAGE_PERCENT = 0.30;  // top 30% nodes

// BS init suspicious-region selection parameters
constexpr double BS_INIT_SUSPICIOUS_TARGET_PERCENT = SUSPICIOUS_COVERAGE_PERCENT;
constexpr uint32_t BS_INIT_SUSPICIOUS_MAX_ITERATIONS = 100;
constexpr uint32_t BS_INIT_SUSPICIOUS_MIN_TARGET_NODES = 1;

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
constexpr double UAV_WAYPOINT_HOVER_TIME = 2.0;  // seconds per waypoint
constexpr double UAV_PATH_MARGIN_METERS = 10.0;  // safety margin around suspicious region
constexpr double UAV_BROADCAST_RADIUS = 50.0;  // meters - UAV communication/broadcast coverage radius

// UAV path planning strategy
constexpr double BS_INIT_UAV_STARTING_ALTITUDE = DEFAULT_UAV_ALTITUDE;
constexpr double BS_INIT_UAV_PATROL_ALTITUDE = DEFAULT_UAV_ALTITUDE;

// UAV1 (Greedy Nearest Neighbor): hover time is computed dynamically from
// master-file transmission time in base-station-node.cc.
constexpr double UAV1_SPEED = 20.0;  // m/s - faster flight speed
constexpr double UAV1_HOVER_TIME = 2.0;  // seconds (fallback/default)

// UAV2 (Greedy Set Cover): Slower speed + no hover time
// UAV2 broadcasts while flying (no hover).
// Inter-fragment spacing is computed dynamically from each fragment TX time
// in node-routing.cc.
constexpr double UAV2_SPEED = 20.0;  // m/s - flight speed
constexpr double UAV2_HOVER_TIME = 0.0;  // seconds - no hovering, broadcasts while flying

// UAV2 coverage path strategy (Greedy Max-Coverage with Cost)
constexpr bool UAV2_USE_NEW_ALGO = true;
constexpr bool UAV2_USE_CENTROIDS = true;
constexpr uint32_t UAV2_NUM_CENTROIDS_MAX = 8;
constexpr uint32_t UAV2_KMEANS_MAX_ITER = 20;
constexpr double UAV2_ALPHA = 1.0;
constexpr double UAV2_SCORE_EPS = 1e-6;
constexpr bool UAV2_USE_TRAVEL_TIME = true;

// Enable/disable GMC cell-aware expansion (when a candidate covers > threshold
// fraction of a cell's suspicious nodes, expand coverage to the whole cell).
constexpr bool COOP_GMC = true;

// Enable/disable Perfect Channel mode for the CC2420 radio.
// When true, all path-loss, shadowing and BER/PER calculations are bypassed:
// every packet is received with a fixed strong RSSI (-50 dBm) and LQI=255.
// This restores the simple ideal-range model used in Phase 1.
constexpr bool PERFECT_CHANNEL = true;

inline int32_t
ComputeDefaultHexGridOffset(uint32_t nodeCount)
{
	const uint32_t gridSizeApprox =
		static_cast<uint32_t>(std::lround(std::sqrt(static_cast<double>(nodeCount))));
	return static_cast<int32_t>(std::max(
		HEX_GRID_OFFSET_BASE,
		gridSizeApprox * HEX_GRID_OFFSET_MULTIPLIER + HEX_GRID_OFFSET_EXTRA));
}

// Fragment parameters - image pixel-region fragments
// An image of DEFAULT_IMAGE_WIDTH x DEFAULT_IMAGE_HEIGHT is partitioned into
// DEFAULT_NUM_FRAGMENTS fragments via pixel-stride interleaving.
constexpr uint32_t DEFAULT_NUM_FRAGMENTS = 10;
constexpr double MIN_FRAGMENT_CONFIDENCE = 0.1;
constexpr double MAX_FRAGMENT_CONFIDENCE = 0.9;
// Image input dimensions (YOLOv4 / SSD default 416x416 px input)
constexpr uint32_t DEFAULT_IMAGE_WIDTH    = 416;   // pixels
constexpr uint32_t DEFAULT_IMAGE_HEIGHT   = 416;   // pixels
constexpr uint32_t DEFAULT_BYTES_PER_PIXEL = 3;    // RGB
// Minimum recognizable target size (whole-body rule-of-thumb from fragment-gen.md: 50-100 px).
// Use the conservative lower bound 50 px and map it to minimum fragment area.
constexpr uint32_t MIN_RECOGNIZABLE_OBJECT_HEIGHT_PX = 50;
constexpr uint32_t MIN_RECOGNIZABLE_FRAGMENT_PIXELS =
	MIN_RECOGNIZABLE_OBJECT_HEIGHT_PX * MIN_RECOGNIZABLE_OBJECT_HEIGHT_PX;
// Base detection confidence for the full unpartitioned image.
// Individual fragment confidence is derived so that union probability
// over all N fragments recovers exactly this value.
constexpr double DEFAULT_MASTER_FILE_CONFIDENCE = 0.90;
constexpr uint32_t BS_INIT_FRAGMENT_GENERATION_COUNT = DEFAULT_NUM_FRAGMENTS;

// ===== GLOBAL RESULT FILE STREAM =====
// Global file stream for logging all scenario4 results to a single file
// Example4.cc will open/close this stream
// Other files can write directly: if (g_resultFileStream) *g_resultFileStream << "text";
extern std::ofstream* g_resultFileStream;

} // namespace params
} // namespace scenario4
} // namespace wsn
} // namespace ns3

#endif // SCENARIO4_PARAMS_H
