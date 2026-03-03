/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Scenario3: Multi-UAV grid network setup with fragment-based confidence tracking
 */

#include "scenario3.h"
#include "../../model/routing/scenario3/ground-node-routing.h"
#include "../../model/routing/scenario3/uav-node-routing.h"
#include "../../model/routing/scenario3/ground-node-routing/global-startup-phase.h"

#include "ns3/mobility-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/cc2420-helper.h"
#include "ns3/cc2420-net-device.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

#include <vector>
#include <algorithm>
#include <cmath>

namespace ns3
{
namespace wsn
{

NS_LOG_COMPONENT_DEFINE("Scenario3");

//
// NETWORK-LEVEL SCHEDULING FUNCTIONS
// (Moved from routing layer for proper architectural separation)
//

/**
 * @brief Internal callback invoked when a node is activated during startup phase
 *
 * Logs the node activation with its grid position (row, column).
 * This callback is triggered at the scheduled activation time for each node.
 *
 * @param nodeId The node ID being activated (0-indexed)
 * @param gridSize The size of the ground node grid for coordinate calculation
 */
static void
OnGlobalStartupPhaseActivation(uint32_t nodeId, uint32_t gridSize)
{
    NS_LOG_INFO("t=" << Simulator::Now().GetSeconds() << "s Global Startup Phase: "
                << "Node " << nodeId << " activated (grid position: row=" 
                << (nodeId / gridSize) << ", col=" << (nodeId % gridSize) << ")");
}

/**
 * @brief Internal callback invoked when global startup phase completes
 *
 * Logs the completion of the global startup phase and provides
 * a summary of how many nodes were activated.
 *
 * @param totalNodes Total number of nodes that were activated
 */
static void
OnGlobalStartupPhaseCompletion(uint32_t totalNodes)
{
    NS_LOG_WARN("*** GLOBAL STARTUP PHASE COMPLETE *** "
                << "All " << totalNodes << " ground nodes initialized at t=" 
                << Simulator::Now().GetSeconds() << "s");
}

void
ScheduleScenario3GlobalStartupPhase(NodeContainer nodes,
                                    uint32_t gridSize,
                                    uint32_t packetSize,
                                    double startupTime,
                                    double startupDuration)
{
    NS_LOG_FUNCTION(gridSize << packetSize << startupTime << startupDuration);

    const uint32_t totalNodes = nodes.GetN();
    
    // Validate input
    if (totalNodes == 0)
    {
        NS_LOG_WARN("ScheduleScenario3GlobalStartupPhase: No ground nodes available");
        return;
    }

    if (gridSize == 0)
    {
        NS_LOG_WARN("ScheduleScenario3GlobalStartupPhase: Invalid gridSize = 0");
        return;
    }

    if (startupDuration <= 0)
    {
        NS_LOG_WARN("ScheduleScenario3GlobalStartupPhase: Invalid startupDuration = " 
                   << startupDuration);
        return;
    }

    // Log startup phase configuration
    NS_LOG_INFO("Global Startup Phase scheduled: " << totalNodes << " nodes, "
                << "gridSize=" << gridSize << ", packetSize=" << packetSize 
                << ", duration=" << startupDuration << "s, start=" << startupTime << "s");

    // Calculate per-node activation interval to spread activations evenly
    const double nodeActivationInterval = startupDuration / totalNodes;
    
    // Schedule individual node activation events during startup phase
    for (uint32_t i = 0; i < totalNodes; ++i)
    {
        double activationTime = startupTime + (i * nodeActivationInterval);
        
        // Schedule node activation callback (logging only)
        Simulator::Schedule(Seconds(activationTime),
                          &OnGlobalStartupPhaseActivation,
                          i,
                          gridSize);
        
        // Schedule actual per-node startup phase logic
        Simulator::Schedule(Seconds(activationTime),
                          &scenario3::StartGlobalSetupPhase,
                          i,
                          gridSize,
                          packetSize,
                          nodes);
    }

    // Schedule startup phase completion event
    double completionTime = startupTime + startupDuration;
    Simulator::Schedule(Seconds(completionTime),
                       &OnGlobalStartupPhaseCompletion,
                       totalNodes);

    NS_LOG_DEBUG("Scheduled " << totalNodes << " node activation events "
                << "with interval=" << nodeActivationInterval << "s, "
                << "completion at t=" << completionTime << "s");
}

void
ScheduleScenario3GlobalSetupPhaseCompletion(NodeContainer nodes,
                                            uint32_t gridSize,
                                            double completionTime,
                                            uint32_t totalActivatedNodes)
{
    NS_LOG_FUNCTION(gridSize << completionTime << totalActivatedNodes);

    const uint32_t totalNodes = nodes.GetN();
    uint32_t completionNodeId = 0;  // Sink/coordinator node initiating completion

    // Validate input
    if (totalNodes == 0)
    {
        NS_LOG_WARN("ScheduleScenario3GlobalSetupPhaseCompletion: No ground nodes available");
        return;
    }

    // Schedule completion handler for each node
    for (uint32_t i = 0; i < totalNodes; ++i)
    {
        Simulator::Schedule(Seconds(completionTime),
                          &scenario3::HandleGlobalSetupPhaseCompletion,
                          i,
                          completionNodeId,
                          totalActivatedNodes,
                          (uint32_t)completionTime);
    }

    NS_LOG_INFO("Global Setup Phase Completion scheduled for " << totalNodes 
                << " nodes at t=" << completionTime << "s, total activated=" 
                << totalActivatedNodes);
}

void
ScheduleScenario3GroundTrafficPattern(NodeContainer nodes,
                                      uint32_t gridSize,
                                      uint32_t packetSize,
                                      double firstTxTime)
{
    NS_LOG_FUNCTION(gridSize << packetSize << firstTxTime);

    // Calculate node positions in grid
    const uint32_t center = (gridSize / 2) * gridSize + (gridSize / 2);
    const uint32_t topLeft = 0;
    const uint32_t topRight = gridSize - 1;
    const uint32_t bottomLeft = gridSize * (gridSize - 1);
    const uint32_t bottomRight = gridSize * gridSize - 1;

    Mac16Address broadcast = Mac16Address("FF:FF");

    // Schedule broadcast from center node
    scenario3::ScheduleGroundTransmission(nodes, center, broadcast, packetSize, Seconds(firstTxTime));

    // Schedule broadcasts from four corner nodes
    scenario3::ScheduleGroundTransmission(nodes, topLeft, broadcast, packetSize, Seconds(firstTxTime + 1.0));
    scenario3::ScheduleGroundTransmission(nodes, topRight, broadcast, packetSize, Seconds(firstTxTime + 1.2));
    scenario3::ScheduleGroundTransmission(nodes, bottomLeft, broadcast, packetSize, Seconds(firstTxTime + 1.4));
    scenario3::ScheduleGroundTransmission(nodes, bottomRight, broadcast, packetSize, Seconds(firstTxTime + 1.6));

    NS_LOG_INFO("Ground node traffic pattern scheduled: center + 4 corners");
}

Ptr<SpectrumChannel>
SetupScenario3Network(const Scenario3Config& config,
                      NodeContainer& nodes,
                      NetDeviceContainer& devices)
{
    NS_LOG_FUNCTION_NOARGS();

    // Create nodes
    const uint32_t numNodes = config.gridSize * config.gridSize;
    nodes.Create(numNodes);

    // Setup mobility (grid layout)
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(0.0),
                                  "MinY", DoubleValue(0.0),
                                  "DeltaX", DoubleValue(config.spacing),
                                  "DeltaY", DoubleValue(config.spacing),
                                  "GridWidth", UintegerValue(config.gridSize),
                                  "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // Setup CC2420 devices
    Cc2420Helper cc2420;
    Ptr<SingleModelSpectrumChannel> channel = cc2420.CreateChannel();
    cc2420.SetChannel(channel);
    cc2420.SetPhyAttribute("TxPower", DoubleValue(config.txPowerDbm));
    cc2420.SetPhyAttribute("RxSensitivity", DoubleValue(config.rxSensitivityDbm));

    devices = cc2420.Install(nodes);

    // Initialize ground node network layer routing
    scenario3::InitializeGroundNodeRouting(devices, config.packetSize);

    NS_LOG_INFO("Scenario3 network setup complete: " << numNodes << " nodes in "
                << config.gridSize << "x" << config.gridSize << " grid");

    return channel;
}

void
ScheduleScenario3Traffic(const Scenario3Config& config, NodeContainer nodes)
{
    NS_LOG_FUNCTION_NOARGS();

    // Use the network-level function (moved from routing layer)
    ScheduleScenario3GroundTrafficPattern(nodes,
                                          config.gridSize,
                                          config.packetSize,
                                          config.firstTxTimeSeconds);
}

uint32_t
GetScenario3TotalTx()
{
    return scenario3::GetGroundTotalTransmissions();
}

uint32_t
GetScenario3TotalRx()
{
    return scenario3::GetGroundTotalReceptions();
}

uint32_t
GetScenario3UavTotalTx()
{
    return scenario3::GetUavTotalTransmissions();
}

uint32_t
GetScenario3UavTotalRx()
{
    return scenario3::GetUavTotalReceptions();
}

uint32_t
ScheduleScenario3UavTraffic(const Scenario3Config& groundConfig,
                            const Scenario3UavConfig& uavConfig,
                            NodeContainer& nodes,
                            Ptr<SpectrumChannel> channel,
                            uint32_t uavIndex)
{
    NS_LOG_FUNCTION_NOARGS();

    // Validate UAV parameters
    if (!scenario3::ParameterCalculators::ValidateAltitude(uavConfig.uavAltitude))
    {
        NS_FATAL_ERROR("Invalid UAV altitude: " << uavConfig.uavAltitude 
                       << " (min: " << scenario3::UAVParams::MIN_ALTITUDE 
                       << ", max: " << scenario3::UAVParams::MAX_ALTITUDE << ")");
    }
    if (!scenario3::ParameterCalculators::ValidateSpeed(uavConfig.uavSpeed))
    {
        NS_FATAL_ERROR("Invalid UAV speed: " << uavConfig.uavSpeed 
                       << " (min: " << scenario3::UAVParams::MIN_SPEED 
                       << ", max: " << scenario3::UAVParams::MAX_SPEED << ")");
    }
    if (!scenario3::ParameterCalculators::ValidateBroadcastInterval(uavConfig.uavBroadcastInterval))
    {
        NS_FATAL_ERROR("Invalid broadcast interval: " << uavConfig.uavBroadcastInterval 
                       << " (min: " << scenario3::UAVParams::MIN_BROADCAST_INTERVAL << ")");
    }

    // Create UAV node
    Ptr<Node> uavNode = CreateObject<Node>();
    
    // Setup UAV mobility - waypoint trajectory
    Ptr<WaypointMobilityModel> uavMobility = CreateObject<WaypointMobilityModel>();
    uavNode->AggregateObject(uavMobility);

    // Calculate grid boundaries
    double gridWidth = scenario3::ParameterCalculators::CalculateGridDimension(
        groundConfig.gridSize, groundConfig.spacing);
    double gridHeight = scenario3::ParameterCalculators::CalculateGridDimension(
        groundConfig.gridSize, groundConfig.spacing);

    // Multi-UAV trajectories: each UAV follows different path
    // UAV 0: West to East (along center line)
    // UAV 1: North to South (along center line)
    // UAV 2 and beyond: Diagonal patterns
    
    double centerX = gridWidth / 2.0;
    double centerY = gridHeight / 2.0;
    double startX, startY, endX, endY;
    
    if (uavIndex == 0)
    {
        // Horizontal: West to East
        startX = -scenario3::UAVParams::APPROACH_DISTANCE;
        startY = centerY;
        endX = gridWidth + scenario3::UAVParams::APPROACH_DISTANCE;
        endY = centerY;
    }
    else if (uavIndex == 1)
    {
        // Vertical: North to South
        startX = centerX;
        startY = gridHeight + scenario3::UAVParams::APPROACH_DISTANCE;
        endX = centerX;
        endY = -scenario3::UAVParams::APPROACH_DISTANCE;
    }
    else
    {
        // Diagonal pattern for additional UAVs
        double angleOffset = (uavIndex - 2) * M_PI / 4.0;  // Every 45 degrees
        double radius = std::sqrt(gridWidth * gridWidth + gridHeight * gridHeight) / 2.0 + 
                       scenario3::UAVParams::APPROACH_DISTANCE;
        startX = centerX + radius * std::cos(angleOffset);
        startY = centerY + radius * std::sin(angleOffset);
        endX = centerX - radius * std::cos(angleOffset);
        endY = centerY - radius * std::sin(angleOffset);
    }

    // Initial position
    uavMobility->AddWaypoint(Waypoint(Seconds(0.0), 
                                     Vector(startX, startY, uavConfig.uavAltitude)));

    // Calculate flight path distance and time
    double pathLength = std::sqrt((endX - startX) * (endX - startX) + 
                                 (endY - startY) * (endY - startY));
    double flyTime = scenario3::ParameterCalculators::CalculateFlightTime(
        pathLength, uavConfig.uavSpeed);
    
    uavMobility->AddWaypoint(Waypoint(
        Seconds(groundConfig.firstTxTimeSeconds + flyTime), 
        Vector(endX, endY, uavConfig.uavAltitude)));

    // Return journey
    double returnTime = groundConfig.firstTxTimeSeconds + flyTime + 
                       scenario3::ParameterCalculators::CalculateFlightTime(
                           pathLength, uavConfig.uavSpeed);
    uavMobility->AddWaypoint(Waypoint(Seconds(returnTime), 
                                     Vector(startX, startY, uavConfig.uavAltitude)));

    // Install CC2420 device on UAV
    Cc2420Helper cc2420Uav;
    cc2420Uav.SetChannel(channel);
    cc2420Uav.SetPhyAttribute("TxPower", DoubleValue(groundConfig.txPowerDbm));
    cc2420Uav.SetPhyAttribute("RxSensitivity", DoubleValue(groundConfig.rxSensitivityDbm));
    
    NetDeviceContainer uavDevices = cc2420Uav.Install(uavNode);
    
    // Initialize UAV routing
    scenario3::InitializeUavNodeRouting(uavDevices, groundConfig.packetSize);

    // Add UAV to node container
    uint32_t uavNodeId = nodes.GetN();
    nodes.Add(uavNode);
    
    // Schedule periodic UAV broadcasts
    scenario3::ScheduleUavPeriodicBroadcasts(nodes, uavNodeId, groundConfig.packetSize,
                                            groundConfig.firstTxTimeSeconds,
                                            returnTime,
                                            uavConfig.uavBroadcastInterval,
                                            uavIndex);

    NS_LOG_INFO("UAV #" << (uavIndex + 1) << " scheduler: altitude=" << uavConfig.uavAltitude 
                << "m, speed=" << uavConfig.uavSpeed << "m/s, interval=" 
                << uavConfig.uavBroadcastInterval << "s");
    NS_LOG_INFO("UAV #" << (uavIndex + 1) << " flight time: " << flyTime << "s, total time: " 
                << returnTime << "s, path length: " << pathLength << "m");

    return uavNodeId;
}

uint32_t
ScheduleScenario3UavFragmentTraffic(const Scenario3Config& groundConfig,
                                    const Scenario3UavConfig& uavConfig,
                                    NodeContainer& nodes,
                                    Ptr<SpectrumChannel> channel,
                                    uint32_t numFragments,
                                    double totalConfidence,
                                    uint32_t uavIndex)
{
    NS_LOG_FUNCTION(numFragments << totalConfidence << uavIndex);

    // Generate network fragments for this UAV
    scenario3::GenerateUavNetworkFragments(numFragments, totalConfidence, uavIndex);

    // Call standard UAV traffic setup (which will now use generated fragments)
    uint32_t uavNodeId = ScheduleScenario3UavTraffic(groundConfig, uavConfig, nodes, 
                                                     channel, uavIndex);

    NS_LOG_INFO("UAV #" << (uavIndex + 1) << " fragment scheduler: " << numFragments 
                << " fragments generated, total confidence=" << totalConfidence);

    return uavNodeId;
}

double
GetScenario3NetworkNodeConfidence(uint32_t nodeId)
{
    return scenario3::GetGroundNodeNetworkConfidence(nodeId);
}

uint32_t
GetScenario3NetworkNodeFragments(uint32_t nodeId)
{
    return scenario3::GetGroundNodeNetworkFragments(nodeId);
}

bool
GetScenario3NetworkNodeAlerted(uint32_t nodeId)
{
    return scenario3::HasGroundNodeNetworkAlerted(nodeId);
}

} // namespace wsn
} // namespace ns3
