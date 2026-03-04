/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Scenario3: Multi-UAV grid network setup with fragment-based confidence tracking
 */

#include "scenario3.h"
#include "parameters.h"
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
    // NS_LOG_INFO("t=" << Simulator::Now().GetSeconds() << "s Global Startup Phase: "
    //             << "Node " << nodeId << " activated (grid position: row=" 
    //             << (nodeId / gridSize) << ", col=" << (nodeId % gridSize) << ")");
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

/**
 * @brief Demonstrate accessing global network topology after setup phase
 *
 * This callback demonstrates that the entire network topology is observable
 * after the global setup phase. It shows cell structure, node distribution,
 * routing trees, and gateway information.
 *
 * @param totalNodes Total number of nodes in the network
 */
static void
OnDemonstrateGlobalTopology(uint32_t totalNodes)
{
    NS_LOG_INFO("=== GLOBAL TOPOLOGY DEMONSTRATION at t=" << Simulator::Now().GetSeconds() << "s ===");
    
    // Show overall network statistics
    std::set<int32_t> allCells = scenario3::GetAllCellIds();
    NS_LOG_INFO("[NETWORK STATS] Total Nodes: " << scenario3::g_globalNodeTopology.size() 
                << " | Total Cells: " << allCells.size()
                << " | Hex Cell Radius: " << scenario3::g_hexCellRadius << " meters");
    
    // Show cell distribution statistics
    std::map<int32_t, uint32_t> cellSizes;
    for (const auto& entry : scenario3::g_globalNodeTopology)
    {
        cellSizes[entry.second.cellId]++;
    }
    
    NS_LOG_INFO("[CELL DISTRIBUTION]");
    for (const auto& entry : cellSizes)
    {
        NS_LOG_INFO("  Cell " << entry.first << ": " << entry.second << " nodes");
    }
    
    // // Demonstrate accessing detailed node information
    // NS_LOG_INFO("[SAMPLE NODE DETAILS]");
    // uint32_t sampleCount = 0;
    // for (const auto& entry : scenario3::g_globalNodeTopology)
    // {
    //     if (sampleCount >= 3) break; // Show first 3 nodes as samples
        
    //     const scenario3::GlobalNodeInfo* info = scenario3::GetNodeInfo(entry.first);
    //     if (info)
    //     {
    //         NS_LOG_INFO("  Node " << info->nodeId 
    //                     << " | Cell: " << info->cellId << " (q=" << info->q << ", r=" << info->r << ")"
    //                     << " | Pos: (" << info->posX << ", " << info->posY << ")"
    //                     << " | Leader: " << info->cellLeaderId
    //                     << " | Neighbors: " << info->logicalNeighbors.size()
    //                     << " | Color: " << info->cellColor
    //                     << " | Tree Depth: " << info->intraCellDepth
    //                     << " | Gateway Trees: " << info->parentByGatewayCell.size());
    //     }
    //     sampleCount++;
    // }
    
    // NS_LOG_INFO("=== END GLOBAL TOPOLOGY DEMONSTRATION ===");
}

/**
 * @brief Internal callback invoked after global setup phase to select signal source location
 *
 * After the network topology is fully discovered and configured, this callback:
 * 1. Observes the entire grid network
 * 2. Selects a random coordinate point within the grid area
 * 3. Determines which hex cell contains this point
 * 4. Stores this location and cell information for future signal source node placement
 *
 * This callback reads g_hexCellRadius and g_hexGridOffset DYNAMICALLY (at execution time)
 * to ensure the globals have been computed by EnsureGlobalHexTopology()
 *
 * @param gridSize Size of the ground node grid (gridSize x gridSize)
 * @param spacing Grid node spacing in meters
 */
static void
OnSelectSignalSourceLocation(uint32_t gridSize, double spacing)
{
    // Read global topology parameters WHEN CALLBACK EXECUTES (not when scheduled)
    // This ensures EnsureGlobalHexTopology() has already computed these values
    double hexCellRadius = scenario3::g_hexCellRadius;
    int32_t hexGridOffset = scenario3::g_hexGridOffset;
    
    NS_LOG_INFO("[DEBUG] OnSelectSignalSourceLocation at t=" << Simulator::Now().GetSeconds() 
                << "s: hexCellRadius=" << hexCellRadius 
                << ", hexGridOffset=" << hexGridOffset);
    
    // Calculate grid boundaries
    double gridWidth = scenario3::ParameterCalculators::CalculateGridDimension(gridSize, spacing);
    double gridHeight = scenario3::ParameterCalculators::CalculateGridDimension(gridSize, spacing);

    // Select random location within grid and determine containing cell
    scenario3::SignalSourceLocationData selectedData = 
        scenario3::SignalSourceLocation::SelectRandomLocation(
            gridWidth, gridHeight, 
            hexCellRadius, 
            hexGridOffset);

    NS_LOG_WARN("[SIGNAL SOURCE LOCATION SELECTED] "
                << "at t=" << Simulator::Now().GetSeconds() << "s | "
                << "Location: (" << selectedData.location.x << ", " 
                << selectedData.location.y << ") "
                << "| Cell ID: " << selectedData.cellId 
                << " (q=" << selectedData.cellQ << ", r=" << selectedData.cellR << ") "
                << "| Grid bounds: [0," << gridWidth << "] x [0," << gridHeight << "]");

    // Expand suspicious region around the selected signal source cell
    // The selected cell becomes the initial suspicious region
    std::set<int32_t> suspiciousRegion; // Set of cell IDs in suspicious region
    std::set<uint32_t> suspiciousNodes; // Set of node IDs in suspicious region
    
    // Initialize with the selected cell
    suspiciousRegion.insert(selectedData.cellId);
    
    // Target: 30% of total nodes in the network
    const uint32_t totalNodes = scenario3::g_globalNodeTopology.size();
    const uint32_t targetNodeCount = static_cast<uint32_t>(totalNodes * 0.3); // 30% target
    
    // NS_LOG_INFO("[SUSPICIOUS REGION INIT] Starting cell: " << selectedData.cellId 
    //             << " | Target nodes: " << targetNodeCount << " (" << (100*0.3) << "% of " << totalNodes << ")");
    
    // Iteratively expand the suspicious region
    uint32_t iteration = 0;
    while (suspiciousNodes.size() < targetNodeCount && iteration < 100) // Safety limit
    {
        iteration++;
        
        // (1) Find all nodes in current suspicious region
        suspiciousNodes.clear();
        for (const auto& entry : scenario3::g_globalNodeTopology)
        {
            const scenario3::GlobalNodeInfo& nodeInfo = entry.second;
            if (suspiciousRegion.find(nodeInfo.cellId) != suspiciousRegion.end())
            {
                suspiciousNodes.insert(nodeInfo.nodeId);
            }
        }
        
        // Check if we've reached target
        if (suspiciousNodes.size() >= targetNodeCount)
        {
            break;
        }
        
        // (2) Find candidate neighbors outside suspicious region
        std::vector<uint32_t> candidateNeighbors;
        for (uint32_t nodeId : suspiciousNodes)
        {
            const scenario3::GlobalNodeInfo* nodeInfo = scenario3::GetNodeInfo(nodeId);
            if (nodeInfo)
            {
                for (uint32_t neighborId : nodeInfo->logicalNeighbors)
                {
                    // Check if neighbor is outside suspicious region
                    const scenario3::GlobalNodeInfo* neighborInfo = scenario3::GetNodeInfo(neighborId);
                    if (neighborInfo && suspiciousRegion.find(neighborInfo->cellId) == suspiciousRegion.end())
                    {
                        candidateNeighbors.push_back(neighborId);
                    }
                }
            }
        }
        
        // If no candidates found, stop expansion
        if (candidateNeighbors.empty())
        {
            NS_LOG_INFO("[SUSPICIOUS REGION] No more neighbors to expand at iteration " << iteration);
            break;
        }
        
        // Randomly select one neighbor and add its cell to suspicious region
        uint32_t randomIndex = rand() % candidateNeighbors.size();
        uint32_t selectedNeighbor = candidateNeighbors[randomIndex];
        const scenario3::GlobalNodeInfo* selectedNeighborInfo = scenario3::GetNodeInfo(selectedNeighbor);
        
        if (selectedNeighborInfo)
        {
            suspiciousRegion.insert(selectedNeighborInfo->cellId);
            // NS_LOG_INFO("[SUSPICIOUS REGION EXPAND] Iteration " << iteration 
            //             << " | Added cell " << selectedNeighborInfo->cellId 
            //             << " via node " << selectedNeighbor
            //             << " | Region cells: " << suspiciousRegion.size()
            //             << " | Region nodes: " << suspiciousNodes.size() + 1);
        }
    }
    
    // Final update of suspicious nodes
    suspiciousNodes.clear();
    for (const auto& entry : scenario3::g_globalNodeTopology)
    {
        const scenario3::GlobalNodeInfo& nodeInfo = entry.second;
        if (suspiciousRegion.find(nodeInfo.cellId) != suspiciousRegion.end())
        {
            suspiciousNodes.insert(nodeInfo.nodeId);
        }
    }
    
    // NS_LOG_WARN("[SUSPICIOUS REGION FINAL] "
    //             << "Cells: " << suspiciousRegion.size() 
    //             << " | Nodes: " << suspiciousNodes.size() 
    //             << " (" << (100.0 * suspiciousNodes.size() / totalNodes) << "% of network)"
    //             << " | Iterations: " << iteration
    //             << " | Target reached: " << (suspiciousNodes.size() >= targetNodeCount ? "YES" : "NO"));
    
    // Log cell IDs in suspicious region
    NS_LOG_INFO("[SUSPICIOUS REGION CELLS]");
    for (int32_t cellId : suspiciousRegion)
    {
        std::vector<uint32_t> cellNodes = scenario3::GetNodesInCell(cellId);
        NS_LOG_INFO("  Cell " << cellId << ": " << cellNodes.size() << " nodes");
    }
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
                                            uint32_t totalActivatedNodes,
                                            double spacing)
{
    NS_LOG_FUNCTION(gridSize << completionTime << totalActivatedNodes << spacing);

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

    // Demonstrate accessing global node topology information
    // This shows that the network is fully observable at this point
    Simulator::Schedule(Seconds(completionTime + 0.05),
                       &OnDemonstrateGlobalTopology,
                       totalNodes);

    // Schedule signal source location selection after all nodes have completed setup
    // This allows the network to be fully observable before selecting the location.
    // Use the global network knowledge computed by EnsureGlobalHexTopology:
    // - g_hexCellRadius: actual hex cell radius used in topology
    // - g_hexGridOffset: actual grid offset used for cell ID computation
    // Add delay to allow time for topology to be computed by first node
    Simulator::Schedule(Seconds(completionTime + 0.1),
                       &OnSelectSignalSourceLocation,
                       gridSize,
                       spacing);

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

    // Compute waypoint trajectory from centralized scenario parameters.
    const scenario3::WaypointEndpoints trajectory = scenario3::WaypointTrajectoryCalculator::Calculate(
        uavIndex,
        gridWidth,
        gridHeight,
        scenario3::UAVParams::APPROACH_DISTANCE);

    const double startX = trajectory.startX;
    const double startY = trajectory.startY;
    const double endX = trajectory.endX;
    const double endY = trajectory.endY;

    // Initial position
    uavMobility->AddWaypoint(Waypoint(Seconds(scenario3::WaypointTrajectoryParams::START_TIME_SECONDS), 
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
