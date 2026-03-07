#include "fragment-broadcast.h"
#include "../packet-header.h"
#include "../helper/calc-utils.h"
#include "../ground-node-routing/ground-node-routing.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/node-list.h"
#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("FragmentBroadcast");

namespace wsn {
namespace scenario4 {
namespace routing {

namespace
{
static constexpr double kBroadcastInterval = 1.0;
static constexpr uint32_t kMaxBroadcastRounds = 8;

void
BroadcastOneRound(uint32_t uavNodeId, uint32_t round)
{
    Ptr<Node> uav = NodeList::GetNode(uavNodeId);
    if (!uav)
    {
        return;
    }

    Ptr<MobilityModel> uavMobility = uav->GetObject<MobilityModel>();
    if (!uavMobility)
    {
        return;
    }

    Vector up = uavMobility->GetPosition();

    for (auto& [nodeId, state] : g_groundNetworkPerNode)
    {
        Ptr<Node> gn = NodeList::GetNode(nodeId);
        if (!gn)
        {
            continue;
        }
        Ptr<MobilityModel> gm = gn->GetObject<MobilityModel>();
        if (!gm)
        {
            continue;
        }

        Vector gp = gm->GetPosition();
        double d = helper::CalculateDistance(up.x, up.y, gp.x, gp.y);
        double syntheticRssi = -55.0 - 0.12 * d;

        Ptr<Packet> p = Create<Packet>();
        FragmentPacket f;
        f.SetFragmentId(round % 16);
        f.SetSourceId(uavNodeId);
        f.SetConfidence(std::max(0.05, 0.9 - 0.01 * d));

        PacketHeader h;
        h.SetType(PACKET_TYPE_FRAGMENT);

        p->AddHeader(f);
        p->AddHeader(h);
        OnGroundNodeReceivePacket(nodeId, p, syntheticRssi);
    }

    if (round + 1 < kMaxBroadcastRounds)
    {
        Simulator::Schedule(Seconds(kBroadcastInterval), &BroadcastOneRound, uavNodeId, round + 1);
    }
}
} // namespace

void
StartFragmentBroadcast(uint32_t uavNodeId)
{
    NS_LOG_INFO("UAV " << uavNodeId << " starts fragment broadcasting");
    Simulator::ScheduleNow(&BroadcastOneRound, uavNodeId, 0);
}

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3
