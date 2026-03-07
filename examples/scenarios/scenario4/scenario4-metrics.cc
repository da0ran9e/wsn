#include "scenario4-metrics.h"
#include "ns3/log.h"
#include "../../../model/routing/scenario4/ground-node-routing/ground-node-routing.h"

#include <algorithm>
#include <limits>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Scenario4Metrics");

namespace wsn {
namespace scenario4 {

void
ReportScenario4Metrics()
{
    using namespace routing;

    const auto& states = g_groundNetworkPerNode;
    if (states.empty())
    {
        NS_LOG_INFO("Scenario4 metrics: no ground node state available");
        return;
    }

    double sumConfidence = 0.0;
    double minConfidence = std::numeric_limits<double>::max();
    double maxConfidence = std::numeric_limits<double>::lowest();
    uint64_t totalPackets = 0;
    uint64_t totalNeighbors = 0;

    for (const auto& [nodeId, st] : states)
    {
        (void)nodeId;
        sumConfidence += st.confidence;
        minConfidence = std::min(minConfidence, st.confidence);
        maxConfidence = std::max(maxConfidence, st.confidence);
        totalPackets += st.packetCount;
        totalNeighbors += st.neighbors.size();
    }

    double avgConfidence = sumConfidence / states.size();
    double avgNeighbors = static_cast<double>(totalNeighbors) / states.size();
    double avgPackets = static_cast<double>(totalPackets) / states.size();

    NS_LOG_INFO("=== Scenario4 Metrics ===");
    NS_LOG_INFO("Ground nodes: " << states.size());
    NS_LOG_INFO("Confidence: avg=" << avgConfidence
                << ", min=" << minConfidence
                << ", max=" << maxConfidence);
    NS_LOG_INFO("Packets per node: avg=" << avgPackets
                << ", total=" << totalPackets);
    NS_LOG_INFO("Neighbors per node: avg=" << avgNeighbors);
}

} // namespace scenario4
} // namespace wsn
} // namespace ns3
