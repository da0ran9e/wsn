#include "region-selection.h"
#include "../helper/calc-utils.h"
#include <algorithm>

namespace ns3 {
namespace wsn {
namespace scenario5 {
namespace routing {

std::set<uint32_t>
SelectSuspiciousRegionFromTopology(const GlobalTopology& topology, double percent)
{
    std::vector<std::pair<uint32_t, double>> scores;
    for (const auto& [id, info] : topology.nodes)
    {
        double s = helper::ComputeSuspiciousScore(info.avgConfidence, info.packetCount, -80.0);
        scores.push_back({id, s});
    }
    std::sort(scores.begin(), scores.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

    size_t count = std::max<size_t>(1, static_cast<size_t>(scores.size() * percent));
    std::set<uint32_t> out;
    for (size_t i = 0; i < count && i < scores.size(); ++i)
    {
        out.insert(scores[i].first);
    }
    return out;
}

} // namespace routing
} // namespace scenario5
} // namespace wsn
} // namespace ns3
