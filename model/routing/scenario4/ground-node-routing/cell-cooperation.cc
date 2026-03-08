#include "cell-cooperation.h"
#include "ground-node-routing.h"
#include "../helper/calc-utils.h"
#include "ns3/simulator.h"

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
    uint32_t mergedCount = 0;
    for (const auto& [id, frag] : src)
    {
        if (!dst.HasFragment(id) || dst.GetFragment(id)->confidence < frag.confidence)
        {
            dst.AddFragment(frag);
            g_groundNetworkPerNode[toNode].fragmentLastUpdateTime[id] = Simulator::Now().GetSeconds();
            mergedCount++;
        }
    }
    g_groundNetworkPerNode[toNode].confidence = dst.totalConfidence;
    auto& toState = g_groundNetworkPerNode[toNode];
    toState.fragmentsReceivedFromPeers += mergedCount;
    toState.fragmentCoverageRatio = (toState.expectedFragmentCount > 0)
                                    ? static_cast<double>(dst.fragments.size()) /
                                          toState.expectedFragmentCount
                                    : 0.0;
    toState.lastCooperationTime = Simulator::Now().GetSeconds();
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
    state.cooperationRequestsSent++;
    state.lastCooperationTime = Simulator::Now().GetSeconds();
    for (const auto& [peerId, peerState] : g_groundNetworkPerNode)
    {
        if (peerId != nodeId && peerState.cellId == cellId)
        {
            state.cellPeers.insert(peerId);
            state.peerConfidence[peerId] = peerState.confidence;
            ShareFragments(peerId, nodeId);
        }
    }
}

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3
