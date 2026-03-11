#include "uav-control.h"
#include "../helper/calc-utils.h"

namespace ns3 {
namespace wsn {
namespace scenario5 {
namespace routing {

UavFlightPath
BuildGreedyFlightPath(const GlobalTopology& topology, const std::set<uint32_t>& targets, double altitude, double speed)
{
    UavFlightPath path;
    if (targets.empty())
    {
        return path;
    }

    double cx = 0.0, cy = 0.0;
    for (auto id : targets)
    {
        const auto& p = topology.nodes.at(id).position;
        cx += p.x;
        cy += p.y;
    }
    cx /= targets.size();
    cy /= targets.size();

    Waypoint wp0{Vector(cx, cy, altitude), 0.0};
    path.waypoints.push_back(wp0);

    Vector cur = wp0.position;
    double t = 0.0;
    for (auto id : targets)
    {
        auto p = topology.nodes.at(id).position;
        Vector next(p.x, p.y, altitude);
        t += helper::CalculateDistance(cur.x, cur.y, next.x, next.y) / speed;
        path.waypoints.push_back({next, t});
        cur = next;
    }
    path.totalTime = t;
    return path;
}

} // namespace routing
} // namespace scenario5
} // namespace wsn
} // namespace ns3
