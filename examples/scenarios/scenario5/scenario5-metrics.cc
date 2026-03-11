#include "scenario5-metrics.h"
#include "ns3/log.h"
#include "../../../model/routing/scenario5/ground-node-routing/ground-node-routing.h"

#include <algorithm>
#include <limits>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Scenario5Metrics");

namespace wsn {
namespace scenario5 {

void
ReportScenario5Metrics()
{
    const auto& states = routing::g_groundNetworkPerNode;
    if (states.empty())
    {
        NS_LOG_INFO("Scenario5 metrics: no ground node state available");
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

    NS_LOG_INFO("=== Scenario5 Metrics ===");
    NS_LOG_INFO("Ground nodes: " << states.size());
    NS_LOG_INFO("Confidence: avg=" << avgConfidence << ", min=" << minConfidence << ", max=" << maxConfidence);
    NS_LOG_INFO("Packets per node: avg=" << avgPackets << ", total=" << totalPackets);
    NS_LOG_INFO("Neighbors per node: avg=" << avgNeighbors);
}

} // namespace scenario5
} // namespace wsn
} // namespace ns3
