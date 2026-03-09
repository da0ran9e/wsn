#include "uav-node-routing.h"
#include "fragment-broadcast.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/mobility-model.h"
#include <map>
#include <limits>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("UavNodeRouting");

namespace wsn {
namespace scenario4 {
namespace routing {

static UavFlightPath g_latestPath;
static std::map<uint32_t, Ptr<Node>> g_uavNodes;

namespace
{
void
MoveSpecificUavToWaypoint(uint32_t nodeId, Vector position)
{
    auto it = g_uavNodes.find(nodeId);
    if (it == g_uavNodes.end() || !it->second)
    {
        return;
    }

    Ptr<MobilityModel> mobility = it->second->GetObject<MobilityModel>();
    if (!mobility)
    {
        return;
    }
    mobility->SetPosition(position);
}
} // namespace

void
OnUavCommandReceived(uint32_t uavNodeId, const UavFlightPath& path)
{
    g_latestPath = path;

    // Special ID means broadcast command to all registered UAVs
    if (uavNodeId == std::numeric_limits<uint32_t>::max())
    {
        NS_LOG_INFO("All UAVs received path with " << path.waypoints.size() << " waypoints");
        for (const auto& [id, _node] : g_uavNodes)
        {
            for (const auto& wp : g_latestPath.waypoints)
            {
                Simulator::Schedule(Seconds(wp.arrivalTime), &MoveSpecificUavToWaypoint, id, wp.position);
            }
            StartFragmentBroadcast(id);
        }
        return;
    }

    NS_LOG_INFO("UAV " << uavNodeId << " received path with " << path.waypoints.size() << " waypoints");

    // Schedule waypoint movement for a specific UAV
    for (const auto& wp : g_latestPath.waypoints)
    {
        Simulator::Schedule(Seconds(wp.arrivalTime), &MoveSpecificUavToWaypoint, uavNodeId, wp.position);
    }

    StartFragmentBroadcast(uavNodeId);
}

void
InitializeUavRouting(Ptr<Node> uavNode)
{
    if (!uavNode)
    {
        return;
    }

    uint32_t nodeId = uavNode->GetId();
    g_uavNodes[nodeId] = uavNode;

    //g_bsUavCommandCallback = &OnUavCommandReceived;
}

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3
