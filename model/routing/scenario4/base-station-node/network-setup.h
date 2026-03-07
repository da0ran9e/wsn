#ifndef SCENARIO4_NETWORK_SETUP_H
#define SCENARIO4_NETWORK_SETUP_H

#include "ns3/node-container.h"

namespace ns3 {
namespace wsn {
namespace scenario4 {
namespace routing {

void SetupGroundNeighbors(NodeContainer nodes, double rangeMeters);

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3

#endif