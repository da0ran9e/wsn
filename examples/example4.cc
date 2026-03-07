/*
 * Scenario 4 - Clean Architecture with Layered Design
 * 
 * This example demonstrates:
 * - Separated orchestration and routing layers
 * - Base station node with callback communication
 * - Ground node cooperation protocol
 * - UAV waypoint planning and fragment broadcast
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"

#include "scenarios/scenario4/scenario4-api.h"
#include "scenarios/scenario4/scenario4-config.h"

using namespace ns3;
using namespace ns3::wsn::scenario4;

NS_LOG_COMPONENT_DEFINE("Example4");

int main(int argc, char* argv[])
{
    // ===== Default Configuration =====
    Scenario4RunConfig config;
    config.gridSize = 10;
    config.gridSpacing = 40.0;
    config.simTime = 60.0;
    config.numFragments = 10;
    config.seed = 42;
    config.runId = 1;
    
    // ===== CLI Parsing =====
    CommandLine cmd(__FILE__);
    cmd.AddValue("gridSize", "Size of the ground node grid (N x N)", config.gridSize);
    cmd.AddValue("gridSpacing", "Spacing between ground nodes (meters)", config.gridSpacing);
    cmd.AddValue("simTime", "Total simulation time (seconds)", config.simTime);
    cmd.AddValue("numFragments", "Number of file fragments to generate", config.numFragments);
    cmd.AddValue("seed", "Random seed for reproducibility", config.seed);
    cmd.AddValue("runId", "Run ID for multiple simulation runs", config.runId);
    cmd.Parse(argc, argv);
    
    // ===== Logging =====
    LogComponentEnable("Example4", LOG_LEVEL_INFO);
    LogComponentEnable("Scenario4Api", LOG_LEVEL_INFO);
    LogComponentEnable("Scenario4Scheduler", LOG_LEVEL_INFO);
    LogComponentEnable("Scenario4StartupPhase", LOG_LEVEL_INFO);
    LogComponentEnable("Scenario4Metrics", LOG_LEVEL_INFO);
    LogComponentEnable("BaseStationNode", LOG_LEVEL_INFO);
    LogComponentEnable("GroundNodeRouting", LOG_LEVEL_INFO);
    LogComponentEnable("UavNodeRouting", LOG_LEVEL_INFO);
    LogComponentEnable("FragmentBroadcast", LOG_LEVEL_INFO);
    
    // ===== Validate Configuration =====
    std::string errorMsg;
    if (!config.Validate(errorMsg)) {
        NS_LOG_ERROR("Configuration validation failed: " << errorMsg);
        return 1;
    }
    
    // ===== Run Scenario =====
    NS_LOG_INFO("=== Scenario 4 Starting ===");
    NS_LOG_INFO("Grid: " << config.gridSize << "x" << config.gridSize 
                << ", Spacing: " << config.gridSpacing << "m");
    NS_LOG_INFO("Simulation Time: " << config.simTime << "s");
    NS_LOG_INFO("Fragments: " << config.numFragments);
    NS_LOG_INFO("Seed: " << config.seed << ", Run ID: " << config.runId);
    
    Scenario4Runner runner(config);
    runner.Build();
    runner.Schedule();
    runner.Run();
    
    NS_LOG_INFO("=== Scenario 4 Complete ===");
    
    Simulator::Destroy();
    return 0;
}
