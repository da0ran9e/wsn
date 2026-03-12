#include "startup-phase.h"
#include "ground-node-routing.h"
#include "../packet-header.h"
#include "../helper/calc-utils.h"
#include "../../../radio/cc2420/cc2420-net-device.h"
#include "ns3/mac16-address.h"
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

namespace
{
Ptr<wsn::Cc2420NetDevice>
GetCc2420Device(Ptr<Node> node)
{
    if (!node || node->GetNDevices() == 0)
    {
        return nullptr;
    }
    return DynamicCast<wsn::Cc2420NetDevice>(node->GetDevice(0));
}
}

void
RunStartupPhase()
{
    NS_LOG_INFO("[STARTUP] Running startup discovery phase at t=" << Simulator::Now().GetSeconds());

    // 1) Build neighbor map by geometric distance
    static constexpr double kNeighborRangeMeters = 120.0;

    for (auto& [nodeId, state] : g_groundNetworkPerNode)
    {
        state.neighbors.clear();
        state.twoHopNeighbors.clear();
        state.neighborRssi.clear();
        state.neighborDistance.clear();
        state.cellPeers.clear();
        state.isCellLeader = false;
        state.lifecyclePhase = GroundNodeLifecyclePhase::DISCOVERY;
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
                itA->second.neighborDistance[itB->first] = dist;
                itB->second.neighborDistance[itA->first] = dist;
            }
        }
    }

    // 2) Exchange startup packets among neighbors through CC2420 stack
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
        Ptr<wsn::Cc2420NetDevice> srcDev = GetCc2420Device(srcNode);
        if (!srcDev)
        {
            continue;
        }

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
            (void)dist;

            Ptr<wsn::Cc2420NetDevice> dstDev = GetCc2420Device(dstNode);
            if (!dstDev)
            {
                continue;
            }

            Mac16Address dstAddr = Mac16Address::ConvertFrom(dstDev->GetAddress());

            Ptr<Packet> p = Create<Packet>();
            StartupPhasePacket startup;
            startup.SetNodeId(srcId);
            startup.SetPosition(srcPos.x, srcPos.y);
            startup.SetTimestamp(Simulator::Now());

            PacketHeader base;
            base.SetType(PACKET_TYPE_STARTUP);

            p->AddHeader(startup);
            p->AddHeader(base);
            srcDev->Send(p, dstAddr, 0);
        }
    }

    // 3) Compute 2-hop neighbors and cell peers
    for (auto& [nodeId, state] : g_groundNetworkPerNode)
    {
        for (uint32_t neighborId : state.neighbors)
        {
            auto itNeighbor = g_groundNetworkPerNode.find(neighborId);
            if (itNeighbor == g_groundNetworkPerNode.end())
            {
                continue;
            }
            for (uint32_t n2 : itNeighbor->second.neighbors)
            {
                if (n2 != nodeId && !state.neighbors.count(n2))
                {
                    state.twoHopNeighbors.insert(n2);
                }
            }
        }

        for (const auto& [peerId, peerState] : g_groundNetworkPerNode)
        {
            if (peerId != nodeId && peerState.cellId == state.cellId)
            {
                state.cellPeers.insert(peerId);
            }
        }
    }

    // 4) Select deterministic cell leader = smallest nodeId in each cell
    std::map<int32_t, uint32_t> leaderByCell;
    for (const auto& [nodeId, state] : g_groundNetworkPerNode)
    {
        auto [it, inserted] = leaderByCell.emplace(state.cellId, nodeId);
        if (!inserted && nodeId < it->second)
        {
            it->second = nodeId;
        }
    }

    for (auto& [nodeId, state] : g_groundNetworkPerNode)
    {
        auto it = leaderByCell.find(state.cellId);
        state.isCellLeader = (it != leaderByCell.end() && it->second == nodeId);
        state.startupComplete = true;
        state.isTimeSynchronized = true;
        state.lastSyncTime = Simulator::Now().GetSeconds();
        state.isIsolated = state.neighbors.empty();
        state.lifecyclePhase = (state.remainingEnergy > 0.0)
                               ? GroundNodeLifecyclePhase::ACTIVE
                               : GroundNodeLifecyclePhase::DEAD;
    }

    // 5) Send aggregated topology to BS callback
    SendTopologyToBS();

    NS_LOG_INFO("[STARTUP] Discovery completed");
}

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3
