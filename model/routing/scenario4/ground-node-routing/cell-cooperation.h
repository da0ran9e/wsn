#ifndef SCENARIO4_CELL_COOPERATION_H
#define SCENARIO4_CELL_COOPERATION_H

#include <cstdint>

namespace ns3 {
namespace wsn {
namespace scenario4 {
namespace routing {

void InitializeCellCooperation();
void RequestFragmentSharing(uint32_t nodeId, int32_t cellId);
void ShareFragments(uint32_t fromNode, uint32_t toNode);

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3

#endif