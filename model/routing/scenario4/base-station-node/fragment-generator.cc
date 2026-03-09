#include "fragment-generator.h"

namespace ns3 {
namespace wsn {
namespace scenario4 {
namespace routing {

FragmentCollection g_bsGeneratedFragments;

FragmentCollection
GenerateBsFragments(uint32_t numFragments)
{
    return GenerateFragments(numFragments);
}

const FragmentCollection&
GetBsGeneratedFragments()
{
    return g_bsGeneratedFragments;
}

void
SetBsGeneratedFragments(const FragmentCollection& fragments)
{
    g_bsGeneratedFragments = fragments;
}

// Global UAV flight paths storage
std::map<uint32_t, UavFlightPath> g_uavFlightPaths;

const std::map<uint32_t, UavFlightPath>&
GetUavFlightPaths()
{
    return g_uavFlightPaths;
}

void
SetUavFlightPath(uint32_t uavNodeId, const UavFlightPath& path)
{
    g_uavFlightPaths[uavNodeId] = path;
}

void
ClearUavFlightPaths()
{
    g_uavFlightPaths.clear();
}

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3
