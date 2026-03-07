#ifndef SCENARIO4_UAV_NODE_ROUTING_H
#define SCENARIO4_UAV_NODE_ROUTING_H

#include "../base-station-node/base-station-node.h"
#include "ns3/node.h"

namespace ns3 {
namespace wsn {
namespace scenario4 {
namespace routing {

void InitializeUavRouting(Ptr<Node> uavNode);
void OnUavCommandReceived(uint32_t uavNodeId, const UavFlightPath& path);

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3

#endif