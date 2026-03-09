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
    // Defaults are copied from scenario4-params.h via Scenario4RunConfig.
    // CLI values below override those defaults before runtime use.
    Scenario4RunConfig config;
    
    // ===== CLI Parsing =====
    CommandLine cmd(__FILE__);
    cmd.AddValue("gridSize", "Size of the ground node grid (N x N)", config.gridSize);
    cmd.AddValue("gridSpacing", "Spacing between ground nodes (meters)", config.gridSpacing);
    cmd.AddValue("simTime", "Total simulation time (seconds)", config.simTime);
    cmd.AddValue("startupPhaseDuration", "Startup phase duration (seconds)", config.startupPhaseDuration);
    cmd.AddValue("uavPlanningDelay", "Delay after startup before UAV planning (seconds)", config.uavPlanningDelay);
    cmd.AddValue("fragmentBroadcastInterval", "UAV fragment broadcast interval (seconds)", config.fragmentBroadcastInterval);
    cmd.AddValue("numFragments", "Number of file fragments to generate", config.numFragments);
    cmd.AddValue("numUavs", "Number of UAV nodes", config.numUavs);
    cmd.AddValue("bsX", "Base station X position", config.bsPositionX);
    cmd.AddValue("bsY", "Base station Y position", config.bsPositionY);
    cmd.AddValue("bsZ", "Base station Z position", config.bsPositionZ);
    cmd.AddValue("uavAltitude", "UAV altitude", config.uavAltitude);
    cmd.AddValue("uavSpeed", "UAV speed", config.uavSpeed);
    cmd.AddValue("cooperationThreshold", "Cooperation threshold (0,1)", config.cooperationThreshold);
    cmd.AddValue("alertThreshold", "Alert threshold (0,1)", config.alertThreshold);
    cmd.AddValue("suspiciousPercent", "Suspicious coverage percent (0,1)", config.suspiciousPercent);
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
    
    // ===== Open Global Result File =====
    std::ostringstream resultFilename;
    resultFilename << "/Users/mophan/Github/ns-3-dev-git-ns-3.46/src/wsn/examples/visualize/results/scenario4_result_"
                   << config.seed << "_" << config.runId << ".txt";
    ns3::wsn::scenario4::params::g_resultFileStream = new std::ofstream(resultFilename.str());
    
    if (ns3::wsn::scenario4::params::g_resultFileStream && 
        ns3::wsn::scenario4::params::g_resultFileStream->is_open())
    {
        *ns3::wsn::scenario4::params::g_resultFileStream 
            << "======================================" << std::endl
            << "    Scenario 4 Simulation Results    " << std::endl
            << "======================================" << std::endl
            << "Configuration:" << std::endl
            << "  Grid: " << config.gridSize << "x" << config.gridSize << std::endl
            << "  Spacing: " << config.gridSpacing << "m" << std::endl
            << "  Simulation Time: " << config.simTime << "s" << std::endl
            << "  Fragments: " << config.numFragments << std::endl
            << "  UAVs: " << config.numUavs << std::endl
            << "  Seed: " << config.seed << std::endl
            << "  Run ID: " << config.runId << std::endl
            << "======================================" << std::endl
            << std::endl;
        ns3::wsn::scenario4::params::g_resultFileStream->flush();
    }
    
    // ===== Run Scenario =====
    NS_LOG_INFO("=== Scenario 4 Starting ===");
    NS_LOG_INFO("Grid: " << config.gridSize << "x" << config.gridSize 
                << ", Spacing: " << config.gridSpacing << "m");
    NS_LOG_INFO("Simulation Time: " << config.simTime << "s");
    NS_LOG_INFO("Fragments: " << config.numFragments);
    NS_LOG_INFO("UAVs: " << config.numUavs);
    NS_LOG_INFO("Seed: " << config.seed << ", Run ID: " << config.runId);
    
    Scenario4Runner runner(config);
    runner.Build();
    runner.Schedule();
    runner.Run();
    
    NS_LOG_INFO("=== Scenario 4 Complete ===");
    
    // ===== Close Global Result File =====
    if (ns3::wsn::scenario4::params::g_resultFileStream)
    {
        if (ns3::wsn::scenario4::params::g_resultFileStream->is_open())
        {
            *ns3::wsn::scenario4::params::g_resultFileStream
                << std::endl
                << "======================================" << std::endl
                << "      Simulation Complete" << std::endl
                << "======================================" << std::endl;
            ns3::wsn::scenario4::params::g_resultFileStream->close();
        }
        delete ns3::wsn::scenario4::params::g_resultFileStream;
        ns3::wsn::scenario4::params::g_resultFileStream = nullptr;
        NS_LOG_INFO("Results saved to: " << resultFilename.str());
    }
    
    Simulator::Destroy();
    return 0;
}
