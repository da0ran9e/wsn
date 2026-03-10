/*
 * Scenario 4 - Configuration Structures
 */

#ifndef SCENARIO4_CONFIG_H
#define SCENARIO4_CONFIG_H

#include <cstdint>
#include <string>

#include "scenario4-params.h"

namespace ns3 {
namespace wsn {
namespace scenario4 {

/**
 * Configuration for Scenario 4 simulation run.
 */
struct Scenario4RunConfig
{
    // Grid topology
    uint32_t gridSize = params::DEFAULT_GRID_SIZE;
    double gridSpacing = params::DEFAULT_SPACING;
    
    // Simulation timing
    double simTime = 160.0;
    double startupPhaseDuration = params::STARTUP_PHASE_DURATION;
    double uavPlanningDelay = params::UAV_PLANNING_DELAY;
    double fragmentBroadcastInterval = params::FRAGMENT_BROADCAST_INTERVAL;
    
    // Fragment generation
    uint32_t numFragments = params::DEFAULT_NUM_FRAGMENTS;

    // UAV fleet
    uint32_t numUavs = params::DEFAULT_NUM_UAVS;
    
    // Reproducibility
    uint32_t seed = 34;
    uint32_t runId = 1;

    // Base station position
    double bsPositionX = params::BS_POSITION_X;
    double bsPositionY = params::BS_POSITION_Y;
    double bsPositionZ = params::BS_POSITION_Z;

    // UAV parameters
    double uavAltitude = params::DEFAULT_UAV_ALTITUDE;
    double uavSpeed = params::DEFAULT_UAV_SPEED;
    
    // Thresholds (optional overrides)
    double cooperationThreshold = params::COOPERATION_THRESHOLD;
    double alertThreshold = params::ALERT_THRESHOLD;
    double suspiciousPercent = params::SUSPICIOUS_COVERAGE_PERCENT;
    
    /**
     * Validate configuration parameters.
     * 
     * \param errorMsg Output parameter for error message if validation fails
     * \return true if valid, false otherwise
     */
    bool Validate(std::string& errorMsg) const;
};

} // namespace scenario4
} // namespace wsn
} // namespace ns3

#endif // SCENARIO4_CONFIG_H
