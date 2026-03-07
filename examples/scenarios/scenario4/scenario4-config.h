/*
 * Scenario 4 - Configuration Structures
 */

#ifndef SCENARIO4_CONFIG_H
#define SCENARIO4_CONFIG_H

#include <cstdint>
#include <string>

namespace ns3 {
namespace wsn {
namespace scenario4 {

/**
 * Configuration for Scenario 4 simulation run.
 */
struct Scenario4RunConfig
{
    // Grid topology
    uint32_t gridSize = 10;
    double gridSpacing = 40.0;
    
    // Simulation timing
    double simTime = 60.0;
    
    // Fragment generation
    uint32_t numFragments = 10;
    
    // Reproducibility
    uint32_t seed = 42;
    uint32_t runId = 1;
    
    // Thresholds (optional overrides)
    double cooperationThreshold = 0.35;
    double alertThreshold = 0.75;
    double suspiciousPercent = 0.30;
    
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
