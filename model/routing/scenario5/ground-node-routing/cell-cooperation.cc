#include "cell-cooperation.h"
#include "ground-node-routing.h"
#include "../helper/calc-utils.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "../../../../examples/scenarios/scenario5/scenario5-params.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Scenario5CellCooperation");

namespace wsn {
namespace scenario5 {
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
        // Only accept fragments that destination node does not have yet
        if (!dst.HasFragment(id))
        {
            dst.AddFragment(frag);
            g_groundNetworkPerNode[toNode].fragmentLastUpdateTime[id] = Simulator::Now().GetSeconds();
            mergedCount++;
            // Format: srcNodeId1-S-dstNodeId1(fragId1) srcNodeId2-S-dstNodeId2(fragId2) ...
            if (ns3::wsn::scenario5::params::g_resultFileStream)
            {        *ns3::wsn::scenario5::params::g_resultFileStream << fromNode
                    << "-S-" << toNode
                    << "(" << id << ") ";
            }
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

    // Node already has full fragment set -> no need to request sharing
    if (state.expectedFragmentCount > 0 &&
        state.fragments.fragments.size() >= state.expectedFragmentCount)
    {
        return;
    }
    
    state.cooperationRequestsSent++;
    state.lastCooperationTime = Simulator::Now().GetSeconds();
    
    for (const auto& [peerId, peerState] : g_groundNetworkPerNode)
    {
        if (peerId != nodeId && peerState.cellId == cellId)
        {
            bool peerHasUsefulFragment = false;
            for (const auto& [fragId, frag] : peerState.fragments.fragments)
            {
                (void)frag;
                if (!state.fragments.HasFragment(fragId))
                {
                    peerHasUsefulFragment = true;
                    break;
                }
            }

            if (!peerHasUsefulFragment)
            {
                continue;
            }

            state.cellPeers.insert(peerId);
            state.peerConfidence[peerId] = peerState.confidence;
            ShareFragments(peerId, nodeId);
        }
    }
    // Format: [EVENT] time | event=RequestFragmentSharing | nodeId=... | cellId=... | confidence=... | fragments= fragId1 fragId2 ...
    if (ns3::wsn::scenario5::params::g_resultFileStream)
    {        *ns3::wsn::scenario5::params::g_resultFileStream << "\n[EVENT] " << Simulator::Now().GetSeconds()
            << " | event=RequestFragmentSharing"
            << " | nodeId=" << nodeId
            << " | cellId=" << cellId
            << " | confidence=" << state.confidence
            << " | fragments=";
        for (const auto& [fragId, frag] : state.fragments.fragments)
        {             *ns3::wsn::scenario5::params::g_resultFileStream << fragId << " ";
        }
        *ns3::wsn::scenario5::params::g_resultFileStream << "\n";
    }
}

} // namespace routing
} // namespace scenario5
} // namespace wsn
} // namespace ns3
