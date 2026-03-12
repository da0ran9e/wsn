#include "fragment-broadcast.h"
#include "../packet-header.h"
#include "../helper/calc-utils.h"
#include "../ground-node-routing/ground-node-routing.h"
#include "../base-station-node/fragment-generator.h"
#include "../../../radio/cc2420/cc2420-net-device.h"
#include "ns3/mac16-address.h"
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

const Fragment*
GetFragmentByRound(uint32_t round)
{
    const FragmentCollection& pool = GetBsGeneratedFragments();
    if (pool.fragments.empty())
    {
        return nullptr;
    }

    const uint32_t index = round % static_cast<uint32_t>(pool.fragments.size());
    auto it = pool.fragments.begin();
    std::advance(it, index);
    return &(it->second);
}

Ptr<wsn::Cc2420NetDevice>
GetCc2420Device(Ptr<Node> node)
{
    if (!node || node->GetNDevices() == 0)
    {
        return nullptr;
    }
    return DynamicCast<wsn::Cc2420NetDevice>(node->GetDevice(0));
}

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

    Ptr<wsn::Cc2420NetDevice> uavDev = GetCc2420Device(uav);
    if (!uavDev)
    {
        return;
    }

    Vector up = uavMobility->GetPosition();

    const Fragment* selectedFragment = GetFragmentByRound(round);

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

        Ptr<wsn::Cc2420NetDevice> dstDev = GetCc2420Device(gn);
        if (!dstDev)
        {
            continue;
        }

        Mac16Address dstAddr = Mac16Address::ConvertFrom(dstDev->GetAddress());

        Ptr<Packet> p = Create<Packet>();
        FragmentPacket f;
        const uint32_t fragmentId = selectedFragment ? selectedFragment->fragmentId : (round % 16);
        const double confidence = selectedFragment ? selectedFragment->confidence
                               : std::max(0.05, 0.9 - 0.01 * d);
        f.SetFragmentId(fragmentId);
        f.SetSourceId(uavNodeId);
        f.SetConfidence(confidence);

        PacketHeader h;
        h.SetType(PACKET_TYPE_FRAGMENT);

        p->AddHeader(f);
        p->AddHeader(h);
        uavDev->Send(p, dstAddr, 0);
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
    const uint32_t poolSize = static_cast<uint32_t>(GetBsGeneratedFragments().fragments.size());
    NS_LOG_INFO("UAV " << uavNodeId << " starts fragment broadcasting"
                << " | bsFragmentPool=" << poolSize);
    Simulator::ScheduleNow(&BroadcastOneRound, uavNodeId, 0);
}

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3
