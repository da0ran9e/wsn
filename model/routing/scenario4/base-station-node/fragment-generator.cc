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

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3
