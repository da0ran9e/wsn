#ifndef SCENARIO4_FRAGMENT_GENERATOR_H
#define SCENARIO4_FRAGMENT_GENERATOR_H

#include "../fragment.h"
#include "base-station-node.h"
#include <map>

namespace ns3 {
namespace wsn {
namespace scenario4 {
namespace routing {

FragmentCollection GenerateBsFragments(uint32_t numFragments);
extern FragmentCollection g_bsGeneratedFragments;
const FragmentCollection& GetBsGeneratedFragments();
void SetBsGeneratedFragments(const FragmentCollection& fragments);

// Global storage for UAV flight paths (key = uavNodeId, value = flight path)
extern std::map<uint32_t, UavFlightPath> g_uavFlightPaths;
const std::map<uint32_t, UavFlightPath>& GetUavFlightPaths();
void SetUavFlightPath(uint32_t uavNodeId, const UavFlightPath& path);
void ClearUavFlightPaths();

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3

#endif