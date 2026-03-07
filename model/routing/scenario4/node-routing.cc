/*
 * Scenario 4 - Initialization Helper
 * 
 * Helper functions to initialize routing components from scenario layer.
 */

#include "base-station-node/base-station-node.h"

namespace ns3 {
namespace wsn {
namespace scenario4 {
namespace routing {

// Global BS instance
static BaseStationNode* g_baseStation = nullptr;

void InitializeBaseStation(uint32_t nodeId)
{
    if (g_baseStation == nullptr) {
        g_baseStation = new BaseStationNode(nodeId);
        g_baseStation->Initialize();
    }
}

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3
