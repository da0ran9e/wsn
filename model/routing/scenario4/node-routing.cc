/*
 * Scenario 4 - Initialization Helper
 * 
 * Helper functions to initialize routing components from scenario layer.
 */

#include "base-station-node/base-station-node.h"
#include "base-station-node/fragment-generator.h"
#include "ground-node-routing/ground-node-routing.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/node-list.h"
#include "ns3/simulator.h"
#include "../../../examples/scenarios/scenario4/scenario4-params.h"

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

void InitializeUavFlight()
{
    NS_LOG_FUNCTION_NOARGS();
    
    const auto& uavPaths = GetUavFlightPaths();
    
    if (uavPaths.empty())
    {
        NS_LOG_WARN("[UAV-FLIGHT] No UAV flight paths available");
        return;
    }
    
    NS_LOG_INFO("[UAV-FLIGHT] Initializing flight for " << uavPaths.size() << " UAVs");
    
    // Schedule waypoint movements for each UAV
    for (const auto& [uavNodeId, path] : uavPaths)
    {
        Ptr<Node> uavNode = NodeList::GetNode(uavNodeId);
        if (!uavNode)
        {
            NS_LOG_WARN("[UAV-FLIGHT] UAV node " << uavNodeId << " not found");
            continue;
        }
        
        Ptr<MobilityModel> mobility = uavNode->GetObject<MobilityModel>();
        if (!mobility)
        {
            NS_LOG_WARN("[UAV-FLIGHT] UAV node " << uavNodeId << " has no MobilityModel");
            continue;
        }
        
        NS_LOG_INFO("[UAV-FLIGHT] Scheduling " << path.waypoints.size() 
                    << " waypoints for UAV " << uavNodeId
                    << " | totalTime=" << path.totalTime << "s");
        
        // Schedule movement to each waypoint
        for (size_t i = 0; i < path.waypoints.size(); ++i)
        {
            const Waypoint& wp = path.waypoints[i];
            
            // Schedule position update at waypoint arrival time
            Simulator::Schedule(Seconds(wp.arrivalTime), [uavNodeId, wp, i]() {
                Ptr<Node> node = NodeList::GetNode(uavNodeId);
                if (!node) return;
                
                Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
                if (!mob) return;
                
                // Set UAV position to waypoint
                mob->SetPosition(wp.position);
                
                NS_LOG_INFO("[UAV-FLIGHT] UAV " << uavNodeId 
                            << " arrived at waypoint " << (i + 1)
                            << " | pos=(" << wp.position.x << "," << wp.position.y << "," << wp.position.z << ")"
                            << " | t=" << Simulator::Now().GetSeconds() << "s");

                // TODO:  in log vào `g_resultFileStream` tại đây
                // Format: [EVENT] time | event=UAVWaypointArrival | nodeId=... | pos=(x,y,z)
                if (ns3::wsn::scenario4::params::g_resultFileStream)
                {
                    *ns3::wsn::scenario4::params::g_resultFileStream
                        << "[EVENT] " << Simulator::Now().GetSeconds()
                        << " | event=UAVWaypointArrival"
                        << " | nodeId=" << uavNodeId
                        << " | pos=(" << wp.position.x << "," << wp.position.y << "," << wp.position.z << ")"
                        << std::endl;
                }
            });
        }
    }
}

void InitializeUavBroadcast()
{
    NS_LOG_FUNCTION_NOARGS();
    NS_LOG_INFO("[UAV-BROADCAST] Fragment broadcast initialization (placeholder)");
    // TODO: phần này chỉ khởi tạo broadcast cho UAV thứ 2
}

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3
