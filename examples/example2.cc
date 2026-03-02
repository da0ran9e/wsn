/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * CC2420 grid scenario with UAV - Full network simulation with fragment-based confidence tracking.
 *
 * Packets go through actual CC2420 NetDevice/MAC/PHY layers with FragmentHeader.
 * Ground nodes perform deduplication, confidence accumulation, and alert triggering.
 */

#include "scenarios/scenario1.h"
#include "../model/routing/scenario1/parameters.h"

#include "ns3/core-module.h"
#include "ns3/log.h"

using namespace ns3;
using namespace ns3::wsn;
using namespace ns3::wsn::scenario1;

NS_LOG_COMPONENT_DEFINE("Cc2420GridScenarioExample2");

int
main(int argc, char* argv[])
{
    Scenario1Config groundConfig;
    Scenario1UavConfig uavConfig;
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
    cmd.AddValue("numFragments", "Number of fragments for confidence tracking", numFragments);
    cmd.AddValue("totalConfidence", "Total confidence to distribute", totalConfidence);
    cmd.Parse(argc, argv);

    LogComponentEnable("Cc2420GridScenarioExample2", LOG_LEVEL_INFO);
    LogComponentEnable("Scenario1", LOG_LEVEL_INFO);
    LogComponentEnable("Scenario1GroundNodeRouting", LOG_LEVEL_INFO);
    LogComponentEnable("Scenario1UavNodeRouting", LOG_LEVEL_INFO);

    // Setup full network with actual CC2420 devices
    NodeContainer nodes;
    NetDeviceContainer devices;
    Ptr<SpectrumChannel> channel = SetupScenario1Network(groundConfig, nodes, devices);

    NS_LOG_INFO("Network setup complete: " << nodes.GetN() << " ground nodes");

    // Setup UAV with fragment-based broadcasts
    uint32_t uavNodeId = ScheduleScenario1UavFragmentTraffic(groundConfig, uavConfig, nodes, 
                                                             channel, numFragments, totalConfidence);
    
    NS_LOG_INFO("UAV node created: ID=" << uavNodeId << ", now total " << nodes.GetN() << " nodes");
    NS_LOG_INFO("Fragment tracking: " << numFragments << " fragments, confidence threshold=0.75");

    // Run actual network simulation
    Simulator::Stop(Seconds(groundConfig.simTimeSeconds));
    Simulator::Run();

    const uint32_t numNodes = groundConfig.gridSize * groundConfig.gridSize;
    const double gridWidth = scenario1::ParameterCalculators::CalculateGridDimension(
        groundConfig.gridSize, groundConfig.spacing);
    const double gridHeight = scenario1::ParameterCalculators::CalculateGridDimension(
        groundConfig.gridSize, groundConfig.spacing);

    NS_LOG_INFO("\n=== Network Simulation Summary ===");
    NS_LOG_INFO("Nodes: " << numNodes << " ground + 1 UAV = " << nodes.GetN() << " total");
    NS_LOG_INFO("Grid dimensions: " << gridWidth << "m x " << gridHeight << "m");
    NS_LOG_INFO("Spacing: " << groundConfig.spacing << " m, TX power: "
                << groundConfig.txPowerDbm << " dBm");
    NS_LOG_INFO("Ground node TX: " << GetScenario1TotalTx());
    NS_LOG_INFO("Ground node RX: " << GetScenario1TotalRx());
    NS_LOG_INFO("UAV TX: " << GetScenario1UavTotalTx());
    NS_LOG_INFO("UAV RX: " << GetScenario1UavTotalRx());
    NS_LOG_INFO("Total TX: " << (GetScenario1TotalTx() + GetScenario1UavTotalTx()));
    NS_LOG_INFO("Total RX: " << (GetScenario1TotalRx() + GetScenario1UavTotalRx()));

    // Show per-node confidence and alert status
    NS_LOG_INFO("\n=== Ground Node Fragment Processing ===");
    uint32_t alertedCount = 0;
    for (uint32_t i = 0; i < numNodes; ++i)
    {
        double conf = GetScenario1NetworkNodeConfidence(i);
        uint32_t frags = GetScenario1NetworkNodeFragments(i);
        bool alerted = GetScenario1NetworkNodeAlerted(i);
        
        if (conf > 0.0)
        {
            NS_LOG_INFO("Node " << i << ": confidence=" << conf 
                       << " fragments=" << frags
                       << (alerted ? " [ALERTED]" : ""));
            if (alerted)
            {
                alertedCount++;
            }
        }
    }
    NS_LOG_INFO("Total nodes alerted: " << alertedCount << "/" << numNodes);

    Simulator::Destroy();

    return 0;
}
