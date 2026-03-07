#include "network-setup.h"
#include "../ground-node-routing/ground-node-routing.h"
#include "../helper/calc-utils.h"
#include "ns3/mobility-model.h"

namespace ns3 {
namespace wsn {
namespace scenario4 {
namespace routing {

void
SetupGroundNeighbors(NodeContainer nodes, double rangeMeters)
{
    for (uint32_t i = 0; i < nodes.GetN(); ++i)
    {
        auto ni = nodes.Get(i);
        auto mi = ni->GetObject<MobilityModel>();
        if (!mi) continue;
        auto& si = g_groundNetworkPerNode[ni->GetId()];
        for (uint32_t j = i + 1; j < nodes.GetN(); ++j)
        {
            auto nj = nodes.Get(j);
            auto mj = nj->GetObject<MobilityModel>();
            if (!mj) continue;
            double d = helper::CalculateDistance(mi->GetPosition().x, mi->GetPosition().y,
                                                 mj->GetPosition().x, mj->GetPosition().y);
            if (d <= rangeMeters)
            {
                si.neighbors.insert(nj->GetId());
                g_groundNetworkPerNode[nj->GetId()].neighbors.insert(ni->GetId());
            }
        }
    }
}

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3
