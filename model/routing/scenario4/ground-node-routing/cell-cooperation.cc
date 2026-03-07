#include "cell-cooperation.h"
#include "ground-node-routing.h"
#include "../helper/calc-utils.h"

namespace ns3 {
namespace wsn {
namespace scenario4 {
namespace routing {

static constexpr double kCooperationThreshold = 0.35;

void InitializeCellCooperation() {}

void
ShareFragments(uint32_t fromNode, uint32_t toNode)
{
    if (!g_groundNetworkPerNode.count(fromNode) || !g_groundNetworkPerNode.count(toNode))
    {
        return;
    }
    auto& src = g_groundNetworkPerNode[fromNode].fragments.fragments;
    auto& dst = g_groundNetworkPerNode[toNode].fragments;
    for (const auto& [id, frag] : src)
    {
        if (!dst.HasFragment(id) || dst.GetFragment(id)->confidence < frag.confidence)
        {
            dst.AddFragment(frag);
        }
    }
    g_groundNetworkPerNode[toNode].confidence = dst.totalConfidence;
}

void
RequestFragmentSharing(uint32_t nodeId, int32_t cellId)
{
    if (!g_groundNetworkPerNode.count(nodeId))
    {
        return;
    }
    auto& state = g_groundNetworkPerNode[nodeId];
    if (!helper::IsCooperationTriggered(state.confidence, kCooperationThreshold))
    {
        return;
    }
    for (const auto& [peerId, peerState] : g_groundNetworkPerNode)
    {
        if (peerId != nodeId && peerState.cellId == cellId)
        {
            ShareFragments(peerId, nodeId);
        }
    }
}

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3
