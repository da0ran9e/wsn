#include "uav-node-routing.h"
#include "fragment-broadcast.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/mobility-model.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("UavNodeRouting");

namespace wsn {
namespace scenario4 {
namespace routing {

static Ptr<Node> g_uavNode;
static UavFlightPath g_latestPath;

namespace
{
void
MoveUavToWaypoint(Vector position)
{
    if (!g_uavNode)
    {
        return;
    }
    Ptr<MobilityModel> mobility = g_uavNode->GetObject<MobilityModel>();
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
    NS_LOG_INFO("UAV " << uavNodeId << " received path with " << path.waypoints.size() << " waypoints");
    g_latestPath = path;

    // Schedule waypoint movement
    for (const auto& wp : g_latestPath.waypoints)
    {
        Simulator::Schedule(Seconds(wp.arrivalTime), &MoveUavToWaypoint, wp.position);
    }

    StartFragmentBroadcast(uavNodeId);
}

void
InitializeUavRouting(Ptr<Node> uavNode)
{
    g_uavNode = uavNode;
    g_bsUavCommandCallback = &OnUavCommandReceived;
}

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3
