/*
 * Scenario 4 - Initialization Helper
 * 
 * Helper functions to initialize routing components from scenario layer.
 */

#include "base-station-node/base-station-node.h"
#include "ground-node-routing/ground-node-routing.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Scenario4NodeRouting");

namespace wsn {
namespace scenario4 {
namespace routing {

// Global BS instance
static BaseStationNode* g_baseStation = nullptr;
static double g_lastProcessedTopologyTs = -1.0;

void InitializeBaseStation(uint32_t nodeId)
{
    if (g_baseStation == nullptr) {
        g_baseStation = new BaseStationNode(nodeId);
        g_baseStation->Initialize();
    }
}

void TickBaseStationControl()
{
    if (g_baseStation == nullptr)
    {
        return;
    }

    const GlobalTopology* topo = GetLatestTopologySnapshotPtr();
    if (topo == nullptr)
    {
        return;
    }

    // Process only new snapshots
    if (topo->timestamp <= g_lastProcessedTopologyTs)
    {
        return;
    }

    g_lastProcessedTopologyTs = topo->timestamp;
    g_baseStation->ReceiveTopology(*topo);
    NS_LOG_INFO("BS pulled shared topology at t=" << topo->timestamp);
}

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3
