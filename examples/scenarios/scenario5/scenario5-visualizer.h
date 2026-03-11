#ifndef SCENARIO5_VISUALIZER_H
#define SCENARIO5_VISUALIZER_H

#include "ns3/network-module.h"
#include "scenario5-config.h"

namespace ns3 {
namespace wsn {
namespace scenario5 {

void EnableScenario5Visualization();

void DumpScenario5InitialNodesToFile(const NodeContainer& groundNodes,
                                     const Ptr<Node>& bsNode,
                                     const NodeContainer& uavNodes,
                                     const Scenario5RunConfig& config);

void DumpScenario5CellFormationSnapshot();

} // namespace scenario5
} // namespace wsn
} // namespace ns3

#endif
