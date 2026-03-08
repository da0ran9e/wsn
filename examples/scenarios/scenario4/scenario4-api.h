/*
 * Scenario 4 - API Runner Interface
 */

#ifndef SCENARIO4_API_H
#define SCENARIO4_API_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "scenario4-config.h"

namespace ns3 {
namespace wsn {
namespace scenario4 {

// Forward declarations from routing layer
namespace routing {
    class BaseStationNode;
    void InitializeGroundNodeRouting(NodeContainer nodes, uint32_t numFragments);
    void SendTopologyToBS();
}

/**
 * Main runner for Scenario 4 simulation.
 * 
 * Orchestrates:
 * - Network topology creation
 * - Event scheduling
 * - Simulation execution
 */
class Scenario4Runner
{
public:
    /**
     * Constructor.
     * 
     * \param config Simulation configuration
     */
    explicit Scenario4Runner(const Scenario4RunConfig& config);
    
    /**
     * Destructor.
     */
    ~Scenario4Runner();
    
    /**
     * Build network topology.
     * 
     * Creates:
     * - Ground node grid
     * - Base station node
     * - Network devices and channels
     */
    void Build();
    
    /**
     * Schedule simulation events.
     * 
     * Schedules:
     * - Startup phase
     * - UAV planning and deployment
     * - Metrics collection
     */
    void Schedule();
    
    /**
     * Run simulation.
     * 
     * Executes simulator and collects results.
     */
    void Run();
    
private:
    Scenario4RunConfig m_config;
    
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

} // namespace scenario4
} // namespace wsn
} // namespace ns3

#endif // SCENARIO4_API_H
