/*
 * Scenario 4 - Centralized Parameters
 * 
 * All scenario and routing parameters are defined here for easy tuning.
 */

#ifndef SCENARIO4_PARAMS_H
#define SCENARIO4_PARAMS_H

#include <cstdint>

namespace ns3 {
namespace wsn {
namespace scenario4 {
namespace params {

// ===== SCENARIO PARAMETERS =====

// Grid parameters
constexpr uint32_t DEFAULT_GRID_SIZE = 10;
constexpr double DEFAULT_SPACING = 40.0;

// Base station location (far from network to avoid interference)
constexpr double BS_POSITION_X = -1000.0;
constexpr double BS_POSITION_Y = -1000.0;
constexpr double BS_POSITION_Z = 0.0;

// UAV parameters
constexpr double DEFAULT_UAV_ALTITUDE = 50.0;
constexpr double DEFAULT_UAV_SPEED = 10.0;  // m/s

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

// Fragment parameters
constexpr uint32_t DEFAULT_NUM_FRAGMENTS = 10;
constexpr double MIN_FRAGMENT_CONFIDENCE = 0.1;
constexpr double MAX_FRAGMENT_CONFIDENCE = 0.9;

} // namespace params
} // namespace scenario4
} // namespace wsn
} // namespace ns3

#endif // SCENARIO4_PARAMS_H
