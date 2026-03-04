/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Example3: Multi-UAV grid scenario with fragment-based confidence tracking.
 *
 * Extends Scenario1 with multiple UAVs operating simultaneously over a grid network.
 * Packets go through actual CC2420 NetDevice/MAC/PHY layers with FragmentHeader.
 * Ground nodes perform deduplication, confidence accumulation, and alert triggering.
 */

#include "scenarios/scenario3.h"
#include "../model/routing/scenario3/parameters.h"

#include "ns3/core-module.h"
#include "ns3/log.h"

using namespace ns3;
using namespace ns3::wsn;
using namespace ns3::wsn::scenario3;

NS_LOG_COMPONENT_DEFINE("MultiUavGridScenarioExample3");

int
main(int argc, char* argv[])
{
    Scenario3Config groundConfig;
    Scenario3UavConfig uavConfig;
    uint32_t numUavs = 1;
    uint32_t numFragments = 10;
    double totalConfidence = 1.0;

    CommandLine cmd;
    cmd.AddValue("gridSize", "Grid size N (N x N)", groundConfig.gridSize);
    cmd.AddValue("spacing", "Grid spacing in meters", groundConfig.spacing);
    cmd.AddValue("txPower", "TX power in dBm", groundConfig.txPowerDbm);
    cmd.AddValue("rxSensitivity", "RX sensitivity in dBm", groundConfig.rxSensitivityDbm);
    cmd.AddValue("simTime", "Simulation time in seconds", groundConfig.simTimeSeconds);
    cmd.AddValue("uavAltitude", "UAV altitude in meters", uavConfig.uavAltitude);
    cmd.AddValue("uavBroadcastInterval", "UAV broadcast interval in seconds",
                 uavConfig.uavBroadcastInterval);
    cmd.AddValue("numUavs", "Number of UAVs", numUavs);
    cmd.AddValue("numFragments", "Number of fragments for confidence tracking", numFragments);
    cmd.AddValue("totalConfidence", "Total confidence to distribute", totalConfidence);
    cmd.Parse(argc, argv);

    LogComponentEnable("MultiUavGridScenarioExample3", LOG_LEVEL_INFO);
    LogComponentEnable("Scenario3", LOG_LEVEL_INFO);
    LogComponentEnable("Scenario3GroundNodeRouting", LOG_LEVEL_INFO);
    LogComponentEnable("Scenario3UavNodeRouting", LOG_LEVEL_INFO);

    LogComponentEnable("GlobalStartupPhase", LOG_LEVEL_INFO);

    // Setup full network with actual CC2420 devices
    NodeContainer nodes;
    NetDeviceContainer devices;
    Ptr<SpectrumChannel> channel = SetupScenario3Network(groundConfig, nodes, devices);

    NS_LOG_INFO("Network setup complete: " << nodes.GetN() << " ground nodes");

    // Schedule Global Setup Phase for ground nodes
    double startupTime = 1.0;        // Start at t=1.0s
    double startupDuration = 5.0;    // Duration: 5 seconds
    uint32_t discoveryPacketSize = 64;  // Discovery packet size
    
    ScheduleScenario3GlobalStartupPhase(nodes,
                                        groundConfig.gridSize,
                                        discoveryPacketSize,
                                        startupTime,
                                        startupDuration);
    
    NS_LOG_INFO("Global Setup Phase scheduled: start=" << startupTime 
                << "s, duration=" << startupDuration << "s");

    // Initialize visualizer for network debugging (outputs to network_log.txt)
    InitializeScenario3Visualizer("network_log.txt");

    // Schedule completion of Global Setup Phase
    double completionTime = startupTime + startupDuration;
    uint32_t totalActivatedNodes = nodes.GetN();
    
    ScheduleScenario3GlobalSetupPhaseCompletion(nodes,
                                                groundConfig.gridSize,
                                                completionTime,
                                                totalActivatedNodes,
                                                groundConfig.spacing);
    
    NS_LOG_INFO("Global Setup Phase Completion scheduled at t=" << completionTime << "s");

    // Schedule UAV communication range calculation after global setup phase
    ScheduleUavRangeCalculation(uavConfig.uavAltitude,
                               groundConfig.txPowerDbm,
                               groundConfig.rxSensitivityDbm,
                               groundConfig.gridSize,
                               groundConfig.spacing,
                               0.15);  // Calculate 0.15s after setup completion
    
    NS_LOG_INFO("UAV range calculation scheduled");

    // std::vector<uint32_t> uavNodeIds;
    // for (uint32_t i = 0; i < numUavs; ++i)
    // {
    //     uint32_t uavNodeId = ScheduleScenario3UavFragmentTraffic(groundConfig, uavConfig, nodes, 
    //                                                              channel, numFragments, 
    //                                                              totalConfidence, i);
    //     uavNodeIds.push_back(uavNodeId);
    //     NS_LOG_INFO("UAV #" << (i + 1) << " created: ID=" << uavNodeId);
    // }

    // Run actual network simulation
    Simulator::Stop(Seconds(groundConfig.simTimeSeconds));
    Simulator::Run();

    // Close visualizer logger
    CloseScenario3Visualizer();

    Simulator::Destroy();
    return 0;
}
