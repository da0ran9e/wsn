#include "startup-phase.h"
#include "ground-node-routing.h"
#include "../packet-header.h"
#include "../helper/calc-utils.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/node-list.h"
#include "ns3/simulator.h"
#include <iterator>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Scenario4StartupPhase");

namespace wsn {
namespace scenario4 {
namespace routing {

void
RunStartupPhase()
{
    NS_LOG_INFO("[STARTUP] Running startup discovery phase at t=" << Simulator::Now().GetSeconds());

    // 1) Build neighbor map by geometric distance
    static constexpr double kNeighborRangeMeters = 120.0;

    for (auto& [nodeId, state] : g_groundNetworkPerNode)
    {
        state.neighbors.clear();
    }

    for (auto itA = g_groundNetworkPerNode.begin(); itA != g_groundNetworkPerNode.end(); ++itA)
    {
        Ptr<Node> nodeA = NodeList::GetNode(itA->first);
        if (!nodeA)
        {
            continue;
        }
        Ptr<MobilityModel> mobA = nodeA->GetObject<MobilityModel>();
        if (!mobA)
        {
            continue;
        }

        for (auto itB = std::next(itA); itB != g_groundNetworkPerNode.end(); ++itB)
        {
            Ptr<Node> nodeB = NodeList::GetNode(itB->first);
            if (!nodeB)
            {
                continue;
            }
            Ptr<MobilityModel> mobB = nodeB->GetObject<MobilityModel>();
            if (!mobB)
            {
                continue;
            }

            Vector posA = mobA->GetPosition();
            Vector posB = mobB->GetPosition();
            double dist = helper::CalculateDistance(posA.x, posA.y, posB.x, posB.y);

            if (dist <= kNeighborRangeMeters)
            {
                itA->second.neighbors.insert(itB->first);
                itB->second.neighbors.insert(itA->first);
            }
        }
    }

    // 2) Exchange startup packets among neighbors (simulated local delivery)
    for (const auto& [srcId, srcState] : g_groundNetworkPerNode)
    {
        Ptr<Node> srcNode = NodeList::GetNode(srcId);
        if (!srcNode)
        {
            continue;
        }
        Ptr<MobilityModel> srcMob = srcNode->GetObject<MobilityModel>();
        if (!srcMob)
        {
            continue;
        }

        Vector srcPos = srcMob->GetPosition();
        for (uint32_t dstId : srcState.neighbors)
        {
            Ptr<Node> dstNode = NodeList::GetNode(dstId);
            if (!dstNode)
            {
                continue;
            }
            Ptr<MobilityModel> dstMob = dstNode->GetObject<MobilityModel>();
            if (!dstMob)
            {
                continue;
            }

            Vector dstPos = dstMob->GetPosition();
            double dist = helper::CalculateDistance(srcPos.x, srcPos.y, dstPos.x, dstPos.y);
            double syntheticRssi = -45.0 - 0.15 * dist;

            Ptr<Packet> p = Create<Packet>();
            StartupPhasePacket startup;
            startup.SetNodeId(srcId);
            startup.SetPosition(srcPos.x, srcPos.y);
            startup.SetTimestamp(Simulator::Now());

            PacketHeader base;
            base.SetType(PACKET_TYPE_STARTUP);

            p->AddHeader(startup);
            p->AddHeader(base);

            OnGroundNodeReceivePacket(dstId, p, syntheticRssi);
        }
    }

    // 3) Send aggregated topology to BS callback
    SendTopologyToBS();

    NS_LOG_INFO("[STARTUP] Discovery completed");
}

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3
