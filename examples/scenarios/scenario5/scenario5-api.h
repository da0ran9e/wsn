/*
 * Scenario 5 - API Runner Interface
 */

#ifndef SCENARIO5_API_H
#define SCENARIO5_API_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "scenario5-config.h"

namespace ns3 {
namespace wsn {
namespace scenario5 {

/**
 * Main runner for Scenario 5 simulation.
 */
class Scenario5Runner
{
  public:
    explicit Scenario5Runner(const Scenario5RunConfig& config);
    ~Scenario5Runner();

    void Build();
    void Schedule();
    void Run();

  private:
    Scenario5RunConfig m_config;

    // Network components
    NodeContainer m_groundNodes;
    Ptr<Node> m_bsNode;
    NodeContainer m_uavNodes;

    // Helper methods
    void BuildGroundNetwork();
    void BuildBaseStation();
    void BuildUavNodes();
    void InstallProtocolStack();
};

} // namespace scenario5
} // namespace wsn
} // namespace ns3

#endif // SCENARIO5_API_H
