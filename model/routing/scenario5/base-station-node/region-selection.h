#ifndef SCENARIO5_REGION_SELECTION_H
#define SCENARIO5_REGION_SELECTION_H

#include "base-station-node.h"
#include <set>

namespace ns3 {
namespace wsn {
namespace scenario5 {
namespace routing {

std::set<uint32_t> SelectSuspiciousRegionFromTopology(const GlobalTopology& topology, double percent);

} // namespace routing
} // namespace scenario5
} // namespace wsn
} // namespace ns3

#endif