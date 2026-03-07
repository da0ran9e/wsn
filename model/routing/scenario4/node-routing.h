/*
 * Scenario 4 - Routing Initialization Interface
 */

#ifndef SCENARIO4_NODE_ROUTING_H
#define SCENARIO4_NODE_ROUTING_H

#include <cstdint>

namespace ns3 {
namespace wsn {
namespace scenario4 {
namespace routing {

/**
 * Initialize base station component.
 */
void InitializeBaseStation(uint32_t nodeId);

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3

#endif // SCENARIO4_NODE_ROUTING_H
