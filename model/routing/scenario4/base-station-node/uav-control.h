#ifndef SCENARIO4_UAV_CONTROL_H
#define SCENARIO4_UAV_CONTROL_H

#include "base-station-node.h"

namespace ns3 {
namespace wsn {
namespace scenario4 {
namespace routing {

UavFlightPath BuildGreedyFlightPath(const GlobalTopology& topology, const std::set<uint32_t>& targets, double altitude, double speed);

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3

#endif