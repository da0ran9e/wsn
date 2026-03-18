#include "cell-cooperation.h"
#include "ground-node-routing.h"
#include "../helper/calc-utils.h"
#include "ns3/log.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "../../../../examples/scenarios/scenario4/scenario4-params.h"
#include <set>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Scenario4CellCooperation");

namespace wsn {
namespace scenario4 {
namespace routing {

namespace
{
uint32_t
ComputeNodeTreeLevelInCell(uint32_t nodeId, int32_t cellId)
{
    uint32_t level = 0;
    uint32_t current = nodeId;
    std::set<uint32_t> visited;

    while (true)
    {
        if (visited.count(current) > 0)
        {
            break;
        }
        visited.insert(current);

        auto routeIt = ::ns3::wsn::scenario4::params::g_intraCellRoutingTree.find(current);
        if (routeIt == ::ns3::wsn::scenario4::params::g_intraCellRoutingTree.end())
        {
            break;
        }

        auto parentIt = routeIt->second.find(cellId);
        if (parentIt == routeIt->second.end())
        {
            break;
        }

        const uint32_t parent = parentIt->second;
        if (parent == current)
        {
            break;
        }

        current = parent;
        ++level;
    }

    return level;
}
} // namespace

void InitializeCellCooperation() {}

double
ComputeCellCooperationStaggerDelay(uint32_t nodeId, int32_t cellId)
{
    const uint32_t level = ComputeNodeTreeLevelInCell(nodeId, cellId);
    const double levelDelay =
        static_cast<double>(level) * ::ns3::wsn::scenario4::params::COOPERATION_LEVEL_DELAY_STEP;

    double randomJitter = 0.0;
    if (::ns3::wsn::scenario4::params::COOPERATION_RANDOM_JITTER_MAX > 0.0)
    {
        Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
        randomJitter = uv->GetValue(0.0,
                                    ::ns3::wsn::scenario4::params::COOPERATION_RANDOM_JITTER_MAX);
    }

    return levelDelay + randomJitter;
}

void
ScheduleFragmentSharingRequest(uint32_t nodeId, int32_t cellId)
{
    if (!g_groundNetworkPerNode.count(nodeId))
    {
        return;
    }

    const double delay = ComputeCellCooperationStaggerDelay(nodeId, cellId);

    Simulator::Schedule(Seconds(delay), [nodeId, cellId]() {
        auto it = g_groundNetworkPerNode.find(nodeId);
        if (it == g_groundNetworkPerNode.end())
        {
            return;
        }

        auto& state = it->second;
        const bool hasAllFragments =
            (state.expectedFragmentCount > 0) &&
            (state.fragments.fragments.size() >= state.expectedFragmentCount);

        if (!state.cooperationEnabled || state.cellId != cellId || hasAllFragments)
        {
            return;
        }

        RequestFragmentSharing(nodeId, cellId);
    });
}

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
            if (ns3::wsn::scenario4::params::g_resultFileStream)
            {        *ns3::wsn::scenario4::params::g_resultFileStream << fromNode
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
    if (ns3::wsn::scenario4::params::g_resultFileStream)
    {        *ns3::wsn::scenario4::params::g_resultFileStream << "\n[EVENT] " << Simulator::Now().GetSeconds()
            << " | event=RequestFragmentSharing"
            << " | nodeId=" << nodeId
            << " | cellId=" << cellId
            << " | confidence=" << state.confidence
            << " | fragments=";
        for (const auto& [fragId, frag] : state.fragments.fragments)
        {             *ns3::wsn::scenario4::params::g_resultFileStream << fragId << " ";
        }
        *ns3::wsn::scenario4::params::g_resultFileStream << "\n";
    }
}

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3
