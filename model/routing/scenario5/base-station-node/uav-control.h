#ifndef SCENARIO5_UAV_CONTROL_H
#define SCENARIO5_UAV_CONTROL_H

#include "base-station-node.h"

namespace ns3 {
namespace wsn {
namespace scenario5 {
namespace routing {

UavFlightPath BuildGreedyFlightPath(const GlobalTopology& topology, const std::set<uint32_t>& targets, double altitude, double speed);

} // namespace routing
} // namespace scenario5
} // namespace wsn
} // namespace ns3

#endif