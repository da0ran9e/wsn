/*
 * Scenario 4 - Configuration Validation
 */

#include "scenario4-config.h"
#include <sstream>

namespace ns3 {
namespace wsn {
namespace scenario4 {

bool
Scenario4RunConfig::Validate(std::string& errorMsg) const
{
    std::ostringstream oss;
    
    // Grid validation
    if (gridSize == 0) {
        oss << "Grid size must be > 0";
        errorMsg = oss.str();
        return false;
    }
    
    if (gridSpacing <= 0.0) {
        oss << "Grid spacing must be > 0.0";
        errorMsg = oss.str();
        return false;
    }
    
    // Timing validation
    if (simTime <= 0.0) {
        oss << "Simulation time must be > 0.0";
        errorMsg = oss.str();
        return false;
    }

    if (startupPhaseDuration <= 0.0 || startupPhaseDuration >= simTime) {
        oss << "Startup phase duration must be > 0 and < simulation time";
        errorMsg = oss.str();
        return false;
    }

    if (uavPlanningDelay < 0.0) {
        oss << "UAV planning delay must be >= 0";
        errorMsg = oss.str();
        return false;
    }

    if (fragmentBroadcastInterval <= 0.0) {
        oss << "Fragment broadcast interval must be > 0";
        errorMsg = oss.str();
        return false;
    }
    
    // Fragment validation
    if (numFragments == 0) {
        oss << "Number of fragments must be > 0";
        errorMsg = oss.str();
        return false;
    }

    if (numUavs == 0) {
        oss << "Number of UAVs must be > 0";
        errorMsg = oss.str();
        return false;
    }

    // UAV/BS validation
    if (uavAltitude <= 0.0) {
        oss << "UAV altitude must be > 0";
        errorMsg = oss.str();
        return false;
    }

    if (uavSpeed <= 0.0) {
        oss << "UAV speed must be > 0";
        errorMsg = oss.str();
        return false;
    }
    
    // Threshold validation
    if (cooperationThreshold <= 0.0 || cooperationThreshold >= 1.0) {
        oss << "Cooperation threshold must be in (0, 1)";
        errorMsg = oss.str();
        return false;
    }
    
    if (alertThreshold <= 0.0 || alertThreshold >= 1.0) {
        oss << "Alert threshold must be in (0, 1)";
        errorMsg = oss.str();
        return false;
    }
    
    if (cooperationThreshold >= alertThreshold) {
        oss << "Cooperation threshold must be < alert threshold";
        errorMsg = oss.str();
        return false;
    }
    
    if (suspiciousPercent <= 0.0 || suspiciousPercent >= 1.0) {
        oss << "Suspicious percent must be in (0, 1)";
        errorMsg = oss.str();
        return false;
    }
    
    return true;
}

} // namespace scenario4
} // namespace wsn
} // namespace ns3
