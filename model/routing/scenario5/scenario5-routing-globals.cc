#include "../../../examples/scenarios/scenario5/scenario5-params.h"

namespace ns3 {
namespace wsn {
namespace scenario5 {
namespace params {

std::map<uint32_t, std::map<int32_t, uint32_t>> g_intraCellRoutingTree;
std::map<int32_t, std::map<int32_t, std::vector<uint32_t>>> g_cellGatewayPairs;

} // namespace params
} // namespace scenario5
} // namespace wsn
} // namespace ns3
