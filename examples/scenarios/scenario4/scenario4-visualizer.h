#ifndef SCENARIO4_VISUALIZER_H
#define SCENARIO4_VISUALIZER_H

#include "ns3/network-module.h"
#include "scenario4-config.h"

namespace ns3 {
namespace wsn {
namespace scenario4 {

void EnableScenario4Visualization();

void DumpScenario4InitialNodesToFile(const NodeContainer& groundNodes,
									 const Ptr<Node>& bsNode,
									 const NodeContainer& uavNodes,
									 const Scenario4RunConfig& config);

void DumpScenario4CellFormationSnapshot();

} // namespace scenario4
} // namespace wsn
} // namespace ns3

#endif