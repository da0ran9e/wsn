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
#include "../model/routing/scenario4/ground-node-routing/ground-node-routing.h"
#include "../model/routing/scenario4/base-station-node/fragment-generator.h"

#include <algorithm>
#include <iomanip>

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
            << "  Cell Radius: " << ns3::wsn::scenario4::params::HEX_CELL_RADIUS << "m" << std::endl
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
            // ===== Final Summary Tables =====
            auto& states = ns3::wsn::scenario4::routing::g_groundNetworkPerNode;
            auto& out = *ns3::wsn::scenario4::params::g_resultFileStream;

            const auto& suspiciousNodes = ns3::wsn::scenario4::routing::GetSuspiciousNodes();

            out << "\n=== Final Ground Node Summary (Suspicious Nodes Only) ===\n";
            out << "[NodeID] | [Conf] | [UAVFrags] | [UAVConf] | [CoopFrags] | [CoopConf] | [CoopCount] | [Alert]\n";
            for (const auto& [nodeId, st] : states)
            {
                if (!suspiciousNodes.empty() && suspiciousNodes.find(nodeId) == suspiciousNodes.end())
                {
                    continue;
                }

                const uint32_t uniqueFragments = static_cast<uint32_t>(st.fragments.fragments.size());
                const double uavFraction = (uniqueFragments > 0)
                                               ? std::min(1.0,
                                                          static_cast<double>(st.fragmentsReceivedFromUav) /
                                                              static_cast<double>(uniqueFragments))
                                               : 0.0;
                const double uavConf = st.confidence * uavFraction;
                const double coopConf = std::max(0.0, st.confidence - uavConf);
                const uint32_t alert = (st.confidence >= config.alertThreshold) ? 1u : 0u;

                out << std::fixed << std::setprecision(3)
                    << nodeId << " \t| "
                    << st.confidence << " \t| "
                    << st.fragmentsReceivedFromUav << " \t| "
                    << uavConf << " \t| "
                    << st.fragmentsReceivedFromPeers << " \t| "
                    << coopConf << " \t| "
                    << st.cooperationRequestsSent << " \t| "
                    << alert << "\n";
            }

            out << "\n=== Final UAV Summary ===\n";
            out << "[UAVID] \t| [FragsBroadcast] \t| [Coverage] \t| [AvgRSSI] \t| [TimeToComplete]\n";

            const auto& uavPaths = ns3::wsn::scenario4::routing::GetUavFlightPaths();
            const auto& fragments = ns3::wsn::scenario4::routing::GetBsGeneratedFragments();

            // Derive coarse network-wide receive coverage/RSSI from ground node stats.
            uint32_t coveredNodes = 0;
            double rssiSum = 0.0;
            uint32_t rssiSamples = 0;
            for (const auto& [nodeId, st] : states)
            {
                (void)nodeId;
                if (st.fragmentsReceivedFromUav > 0)
                {
                    coveredNodes++;
                }
                if (st.rssiSampleCount > 0)
                {
                    rssiSum += st.avgPacketRssiDbm;
                    rssiSamples++;
                }
            }
            const double coveragePct = states.empty()
                                           ? 0.0
                                           : (100.0 * static_cast<double>(coveredNodes) /
                                              static_cast<double>(states.size()));
            const double avgRssi = (rssiSamples > 0) ? (rssiSum / rssiSamples) : 0.0;

            uint32_t uavIndex = 0;
            for (const auto& [uavId, path] : uavPaths)
            {
                uint32_t fragsBroadcast = 0;

                // UAV2 is used for fragment broadcasting in current design.
                if (uavIndex == 1 && !path.waypoints.empty() && !fragments.fragments.empty())
                {
                    const double broadcastStartTime = path.waypoints.front().arrivalTime;
                    const double broadcastEndTime = path.waypoints.back().arrivalTime;
                    const double interval = config.fragmentBroadcastInterval;
                    const double cycleDuration = static_cast<double>(fragments.fragments.size()) * interval;
                    if (cycleDuration > 0.0)
                    {
                        const uint32_t numCycles = static_cast<uint32_t>(
                            std::ceil((broadcastEndTime - broadcastStartTime) / cycleDuration));
                        fragsBroadcast = numCycles * static_cast<uint32_t>(fragments.fragments.size());
                    }
                }

                out << std::fixed << std::setprecision(3)
                    << uavId << " \t| "
                    << fragsBroadcast << " \t| "
                    << coveragePct << "% \t| "
                    << avgRssi << " \t| "
                    << path.totalTime << "\n";

                uavIndex++;
            }

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
