#ifndef SCENARIO5_NETWORK_SETUP_H
#define SCENARIO5_NETWORK_SETUP_H

#include "ns3/node-container.h"

namespace ns3 {
namespace wsn {
namespace scenario5 {
namespace routing {

void SetupGroundNeighbors(NodeContainer nodes, double rangeMeters);

} // namespace routing
} // namespace scenario5
} // namespace wsn
} // namespace ns3

#endif