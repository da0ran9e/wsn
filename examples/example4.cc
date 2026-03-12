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
#include "scenarios/scenario4/scenario4-params.h"
#include "../model/routing/scenario4/ground-node-routing/ground-node-routing.h"
#include "../model/routing/scenario4/base-station-node/fragment-generator.h"
#include "../model/routing/scenario4/node-routing.h"

#include <algorithm>
#include <fstream>
#include <iomanip>

namespace
{
void
WriteScenario4Summary(const std::string& outputPath, const ns3::wsn::scenario4::Scenario4RunConfig& config)
{
    std::ofstream out(outputPath, std::ios::out | std::ios::app);
    if (!out.is_open())
    {
        return;
    }

    const auto& states = ns3::wsn::scenario4::routing::g_groundNetworkPerNode;
    const auto& suspiciousNodes = ns3::wsn::scenario4::routing::GetSuspiciousNodes();

    out << "\n\n=== SCENARIO4 SUMMARY ===\n";
    out << std::fixed << std::setprecision(3);
    out << "SCENARIO scenario4\n";
    out << "RUN seed=" << config.seed << " runId=" << config.runId << "\n";
    out << "\n[CONFIG]\n";
    out << "gridSize=" << config.gridSize << "\n";
    out << "gridSpacing=" << config.gridSpacing << "\n";
    out << "simTime=" << config.simTime << "\n";
    out << "startupPhaseDuration=" << config.startupPhaseDuration << "\n";
    out << "fragmentBroadcastInterval=" << config.fragmentBroadcastInterval << "\n";
    out << "numFragments=" << config.numFragments << "\n";
    out << "numUavs=" << config.numUavs << "\n";
    out << "bsPosition=(" << config.bsPositionX << "," << config.bsPositionY << "," << config.bsPositionZ << ")\n";
    out << "uavAltitude=" << config.uavAltitude << "\n";
    out << "uavSpeed=" << config.uavSpeed << "\n";
    out << "cooperationThreshold=" << config.cooperationThreshold << "\n";
    out << "alertThreshold=" << config.alertThreshold << "\n";
    out << "suspiciousPercent=" << config.suspiciousPercent << "\n";

    out << "\n[PARAMS]\n";
    out << "cellRadius=" << ns3::wsn::scenario4::params::HEX_CELL_RADIUS << "\n";
    out << "neighborDiscoveryRadius=" << ns3::wsn::scenario4::params::NEIGHBOR_DISCOVERY_RADIUS << "\n";
    out << "broadcastRadius=" << ns3::wsn::scenario4::params::UAV_BROADCAST_RADIUS << "\n";
    out << "uav1Speed=" << ns3::wsn::scenario4::params::UAV1_SPEED << "\n";
    out << "uav1HoverTime=" << ns3::wsn::scenario4::params::UAV1_HOVER_TIME << "\n";
    out << "uav2Speed=" << ns3::wsn::scenario4::params::UAV2_SPEED << "\n";
    out << "uav2HoverTime=" << ns3::wsn::scenario4::params::UAV2_HOVER_TIME << "\n";
    out << "masterFileConfidence=" << ns3::wsn::scenario4::params::DEFAULT_MASTER_FILE_CONFIDENCE << "\n";

    out << "\n[NETWORK]\n";
    out << "groundNodes=" << states.size() << "\n";
    out << "suspiciousNodes=" << suspiciousNodes.size() << "\n";
    out << "uavPaths=" << ns3::wsn::scenario4::routing::GetUavFlightPaths().size() << "\n";
    out << "generatedFragments=" << ns3::wsn::scenario4::routing::GetBsGeneratedFragments().fragments.size() << "\n";

    out << "\n[MISSION]\n";
    if (ns3::wsn::scenario4::routing::IsUav1MissionCompleted())
    {
        out << "uav1CompletedTime=" << ns3::wsn::scenario4::routing::GetUav1MissionCompletedTime() << "\n";
    }
    else
    {
        out << "uav1CompletedTime=not-completed\n";
    }
    if (ns3::wsn::scenario4::routing::IsUav2MissionCompleted())
    {
        out << "uav2CompletedTime=" << ns3::wsn::scenario4::routing::GetUav2MissionCompletedTime() << "\n";
    }
    else
    {
        out << "uav2CompletedTime=not-completed\n";
    }
}
} // namespace

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
    
    // ===== Validate Configuration =====
    std::string errorMsg;
    if (!config.Validate(errorMsg)) {
        NS_LOG_ERROR("Configuration validation failed: " << errorMsg);
        return 1;
    }
    
    std::ostringstream resultFilename;
    resultFilename << "/Users/mophan/Github/ns-3-dev-git-ns-3.46/src/wsn/examples/visualize/results/scenario4_result_"
                   << config.seed << "_" << config.runId << ".txt";

    std::ofstream resultStream(resultFilename.str(), std::ios::out | std::ios::trunc);
    if (!resultStream.is_open())
    {
        NS_LOG_ERROR("Failed to open result log file: " << resultFilename.str());
        return 1;
    }
    ns3::wsn::scenario4::params::g_resultFileStream = &resultStream;
    
    // ===== Run Scenario =====
    NS_LOG_INFO("=== Scenario 4 Starting ===");
    NS_LOG_INFO("Grid: " << config.gridSize << "x" << config.gridSize 
                << ", Spacing: " << config.gridSpacing << "m");
    NS_LOG_INFO("Simulation Time: " << config.simTime << "s");
    NS_LOG_INFO("Fragments: " << config.numFragments);
    NS_LOG_INFO("UAVs: " << config.numUavs);
    NS_LOG_INFO("Seed: " << config.seed << ", Run ID: " << config.runId);
    NS_LOG_INFO("BS Position: (" << config.bsPositionX << ", " << config.bsPositionY << ", " << config.bsPositionZ << ")");
    NS_LOG_INFO("Params: cellRadius=" << ns3::wsn::scenario4::params::HEX_CELL_RADIUS
                << ", neighborRadius=" << ns3::wsn::scenario4::params::NEIGHBOR_DISCOVERY_RADIUS
                << ", broadcastRadius=" << ns3::wsn::scenario4::params::UAV_BROADCAST_RADIUS);
    
    Scenario4Runner runner(config);
    runner.Build();
    runner.Schedule();
    runner.Run();

    // Close event log stream before writing summary section.
    resultStream.flush();
    ns3::wsn::scenario4::params::g_resultFileStream = nullptr;
    resultStream.close();
    
    NS_LOG_INFO("=== Scenario 4 Complete ===");
    
    WriteScenario4Summary(resultFilename.str(), config);
    NS_LOG_INFO("UAV1 completion time: "
                << (ns3::wsn::scenario4::routing::IsUav1MissionCompleted()
                        ? std::to_string(ns3::wsn::scenario4::routing::GetUav1MissionCompletedTime()) + "s"
                        : std::string("not-completed")));
    NS_LOG_INFO("UAV2 completion time: "
                << (ns3::wsn::scenario4::routing::IsUav2MissionCompleted()
                        ? std::to_string(ns3::wsn::scenario4::routing::GetUav2MissionCompletedTime()) + "s"
                        : std::string("not-completed")));
    NS_LOG_INFO("Results saved to: " << resultFilename.str());
    
    Simulator::Destroy();
    return 0;
}
