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
#include <fstream>
#include <sstream>
#include <iomanip>

namespace ns3
{
namespace wsn
{

NS_LOG_COMPONENT_DEFINE("Scenario3");

// Global storage for suspicious nodes detected during setup phase
static std::set<uint32_t> g_suspiciousNodes;

//
// VISUALIZATION LOGGING (for TXT format debug visualizer)
//

class NetworkVisualLogger
{
public:
    NetworkVisualLogger(const std::string& filename = "network_log.txt")
        : m_filename(filename), m_file(nullptr)
    {
    }

    void Open()
    {
        m_file = std::make_shared<std::ofstream>(m_filename);
        if (!m_file->is_open())
        {
            NS_LOG_ERROR("Cannot open log file: " << m_filename);
            return;
        }
        NS_LOG_INFO("[VISUALIZER] Opened log file: " << m_filename);
    }

    void Close()
    {
        if (m_file && m_file->is_open())
        {
            m_file->close();
        }
    }

    void LogMeta(const std::string& key, const std::string& value)
    {
        if (!m_file || !m_file->is_open())
            return;
        *m_file << "META " << key << "=" << value << "\n";
        m_file->flush();
    }

    void LogNode(uint32_t id, double x, double y, int32_t cell, uint32_t leader, uint32_t color = 0)
    {
        if (!m_file || !m_file->is_open())
            return;
        std::ostringstream oss;
        oss << "NODE id=" << id << " x=" << x << " y=" << y << " cell=" << cell
            << " leader=" << leader << " color=" << color << "\n";
        *m_file << oss.str();
    }

    void LogLink(uint32_t a, uint32_t b, const std::string& kind = "logical")
    {
        if (!m_file || !m_file->is_open())
            return;
        *m_file << "LINK a=" << a << " b=" << b << " kind=" << kind << "\n";
    }

    void StartStep(double t)
    {
        if (!m_file || !m_file->is_open())
            return;
        m_currentStep << "STEP t=" << t;
    }

    void AddSuspiciousCells(const std::set<int32_t>& cells)
    {
        if (cells.empty())
            return;
        m_currentStep << " suspiciousCells=";
        bool first = true;
        for (int32_t cell : cells)
        {
            if (!first) m_currentStep << ",";
            m_currentStep << cell;
            first = false;
        }
    }

    void AddSuspiciousNodes(const std::set<uint32_t>& nodes)
    {
        if (nodes.empty())
            return;
        m_currentStep << " suspiciousNodes=";
        bool first = true;
        for (uint32_t node : nodes)
        {
            if (!first) m_currentStep << ",";
            m_currentStep << node;
            first = false;
        }
    }

    void EndStep()
    {
        if (!m_file || !m_file->is_open())
            return;
        m_currentStep << "\n";
        *m_file << m_currentStep.str();
        *m_file << "ENDSTEP\n";
        m_currentStep.str("");
        m_file->flush();
    }

private:
    std::string m_filename;
    std::shared_ptr<std::ofstream> m_file;
    std::ostringstream m_currentStep;
};

// Global logger instance
static NetworkVisualLogger g_visualLogger;



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
 * @brief Log network data to visualizer TXT format
 *
 * Outputs nodes, links, and metadata to the TXT log file for visualization.
 * This should be called after global topology is fully computed.
 *
 * @param gridSize Size of the grid
 * @param spacing Spacing between nodes
 */
static void
OnLogNetworkForVisualization(uint32_t gridSize, double spacing)
{
    NS_LOG_INFO("[VISUALIZER] Logging network data...");
    
    // Log metadata
    std::ostringstream metaValue;
    metaValue << "scenario3_grid" << gridSize << "_spacing" << (int)spacing;
    g_visualLogger.LogMeta("scenario", "scenario3");
    g_visualLogger.LogMeta("gridSize", std::to_string(gridSize));
    g_visualLogger.LogMeta("spacing", std::to_string((int)spacing));
    g_visualLogger.LogMeta("totalNodes", std::to_string(scenario3::g_globalNodeTopology.size()));
    g_visualLogger.LogMeta("hexCellRadius", std::to_string((int)scenario3::g_hexCellRadius));
    
    // Log all nodes
    for (const auto& entry : scenario3::g_globalNodeTopology)
    {
        const scenario3::GlobalNodeInfo& info = entry.second;
        g_visualLogger.LogNode(
            info.nodeId,
            info.posX,
            info.posY,
            info.cellId,
            info.cellLeaderId,
            info.cellColor
        );
    }
    
    // Log logical neighbor links
    for (const auto& entry : scenario3::g_globalNodeTopology)
    {
        const scenario3::GlobalNodeInfo& info = entry.second;
        // Only log each link once (from lower ID to higher ID)
        for (uint32_t neighborId : info.logicalNeighbors)
        {
            if (info.nodeId < neighborId)
            {
                g_visualLogger.LogLink(info.nodeId, neighborId, "logical");
            }
        }
    }
    
    NS_LOG_INFO("[VISUALIZER] Network data logged successfully");
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
        for (uint32_t nodeId : cellNodes)
        {
            if (suspiciousNodes.find(nodeId) != suspiciousNodes.end())
            {
                const scenario3::GlobalNodeInfo* info = scenario3::GetNodeInfo(nodeId);
                if (info)
                {
                    NS_LOG_INFO("    Node " << nodeId << " at (" << info->posX << ", " 
                                << info->posY << ")");
                }
            }
        }
    }

    // Log suspicious region to visualizer TXT format
    g_visualLogger.StartStep(Simulator::Now().GetSeconds());
    g_visualLogger.AddSuspiciousCells(suspiciousRegion);
    g_visualLogger.AddSuspiciousNodes(suspiciousNodes);
    g_visualLogger.EndStep();

    // Store suspicious nodes globally for later UAV flight planning
    g_suspiciousNodes = suspiciousNodes;
    
    NS_LOG_INFO("[SUSPICIOUS REGION] Ready for UAV flight planning. Suspicious nodes: " 
                << suspiciousNodes.size() << " nodes detected.");
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

    // Log network data for visualizer
    Simulator::Schedule(Seconds(completionTime + 0.075),
                       &OnLogNetworkForVisualization,
                       gridSize,
                       spacing);

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

void
InitializeScenario3Visualizer(const std::string& filename)
{
    g_visualLogger.Open();
    NS_LOG_INFO("Scenario3 visualizer initialized with filename: " << filename);
}

void
CloseScenario3Visualizer()
{
    g_visualLogger.Close();
    NS_LOG_INFO("Scenario3 visualizer closed");
}

/**
 * @brief Calculate effective communication range between UAV and ground node
 *
 * This function computes the 3D distance, path loss, received power, and link quality
 * between a UAV at altitude and a ground node. It uses the Free Space Path Loss model
 * suitable for line-of-sight air-to-ground communications.
 *
 * FSPL (dB) = 20*log10(distance) + 20*log10(frequency) + 20*log10(4π/c)
 * For 2.4 GHz (CC2420): FSPL ≈ 40.2 + 20*log10(distance_in_meters)
 *
 * @param uavX UAV X coordinate (meters)
 * @param uavY UAV Y coordinate (meters)
 * @param uavZ UAV altitude/Z coordinate (meters)
 * @param groundX Ground node X coordinate (meters)
 * @param groundY Ground node Y coordinate (meters)
 * @param txPowerDbm Transmit power in dBm
 * @param rxSensitivityDbm Receiver sensitivity in dBm
 * @return UavGroundCommRange structure with distance, path loss, and link quality metrics
 */
UavGroundCommRange
CalculateUavGroundCommRange(double uavX, double uavY, double uavZ,
                           double groundX, double groundY,
                           double txPowerDbm, double rxSensitivityDbm)
{
    UavGroundCommRange result;
    
    // Calculate 3D Euclidean distance
    double dx = uavX - groundX;
    double dy = uavY - groundY;
    double dz = uavZ - 0.0; // Ground nodes at z=0
    result.distance3D = std::sqrt(dx*dx + dy*dy + dz*dz);
    
    if (result.distance3D < 1.0)
    {
        // Avoid log10(0) or very small distances
        result.distance3D = 1.0;
    }
    
    // Air-to-Ground Path Loss Model for 2.4 GHz
    // More realistic than free space due to:
    // - Path loss exponent > 2.0 (typically 2.5-3.0 for A2G)
    // - Additional losses (fading, shadowing, ground reflection)
    //
    // Model: PL(dB) = PL0 + 10*n*log10(d/d0) + X_sigma
    // Where:
    //   PL0 = 40.2 dB (path loss at reference distance d0=1m for 2.4 GHz)
    //   n = 2.5 (path loss exponent for air-to-ground suburban/rural)
    //   d = distance in meters
    //   X_sigma = 4 dB (shadow fading margin, moderate for open areas)
    
    const double FSPL_CONSTANT_2_4GHZ = 40.2;  // dB at 1m for 2.4 GHz
    const double PATH_LOSS_EXPONENT = 2.5;     // Air-to-ground exponent (moderate)
    const double SHADOW_FADING_MARGIN = 4.0;   // Moderate margin for outdoor conditions
    
    // Calculate path loss with air-to-ground model
    result.pathLossDb = FSPL_CONSTANT_2_4GHZ 
                       + 10.0 * PATH_LOSS_EXPONENT * std::log10(result.distance3D)
                       + SHADOW_FADING_MARGIN;
    
    // Calculate received power: Rx = Tx - PathLoss
    result.receivedPowerDbm = txPowerDbm - result.pathLossDb;
    
    // Determine if communication is possible
    result.isInRange = (result.receivedPowerDbm >= rxSensitivityDbm);
    
    // Calculate link margin (positive = good, negative = no link)
    result.linkMarginDb = result.receivedPowerDbm - rxSensitivityDbm;
    
    return result;
}

/**
 * @brief Find all ground nodes within effective communication range of UAV
 *
 * Iterates through all ground nodes in the global topology and determines
 * which nodes are within communication range of the UAV at its current position.
 *
 * @param uavX UAV X coordinate (meters)
 * @param uavY UAV Y coordinate (meters)
 * @param uavZ UAV altitude (meters)
 * @param txPowerDbm Transmit power in dBm
 * @param rxSensitivityDbm Receiver sensitivity in dBm
 * @return Vector of node IDs that are within communication range
 */
std::vector<uint32_t>
GetGroundNodesInUavRange(double uavX, double uavY, double uavZ,
                        double txPowerDbm, double rxSensitivityDbm)
{
    std::vector<uint32_t> nodesInRange;
    
    // Iterate through all nodes in global topology
    for (const auto& entry : scenario3::g_globalNodeTopology)
    {
        const scenario3::GlobalNodeInfo& nodeInfo = entry.second;
        
        // Calculate communication range
        UavGroundCommRange range = CalculateUavGroundCommRange(
            uavX, uavY, uavZ,
            nodeInfo.posX, nodeInfo.posY,
            txPowerDbm, rxSensitivityDbm);
        
        if (range.isInRange)
        {
            nodesInRange.push_back(nodeInfo.nodeId);
        }
    }
    
    return nodesInRange;
}

/**
 * @brief Calculate maximum communication range for UAV at given altitude
 *
 * Computes the theoretical maximum horizontal distance at which a ground node
 * can communicate with a UAV at the specified altitude, given the radio parameters.
 *
 * @param altitude UAV altitude (meters)
 * @param txPowerDbm Transmit power in dBm
 * @param rxSensitivityDbm Receiver sensitivity in dBm
 * @return Maximum horizontal communication range in meters
 */
double
CalculateMaxUavCommRange(double altitude, double txPowerDbm, double rxSensitivityDbm)
{
    // Maximum path loss budget: TxPower - RxSensitivity
    double maxPathLossDb = txPowerDbm - rxSensitivityDbm;
    
    // Air-to-Ground Path Loss Model (same as CalculateUavGroundCommRange)
    // PL(dB) = PL0 + 10*n*log10(d) + X_sigma
    // Solve for max distance:
    // maxPL = 40.2 + 10*2.5*log10(d) + 4.0
    // maxPL - 40.2 - 4.0 = 25*log10(d)
    // d = 10^((maxPL - 44.2) / 25)
    
    const double FSPL_CONSTANT_2_4GHZ = 40.2;
    const double PATH_LOSS_EXPONENT = 2.5;
    const double SHADOW_FADING_MARGIN = 4.0;
    const double PL_COEFFICIENT = 10.0 * PATH_LOSS_EXPONENT;  // 25.0
    
    // Subtract constant and margin from path loss budget
    double adjustedPathLoss = maxPathLossDb - FSPL_CONSTANT_2_4GHZ - SHADOW_FADING_MARGIN;
    
    // Solve for max 3D distance
    double max3DDistance = std::pow(10.0, adjustedPathLoss / PL_COEFFICIENT);
    
    // Calculate maximum horizontal range using Pythagorean theorem
    // max3DDistance^2 = horizontalRange^2 + altitude^2
    // horizontalRange = sqrt(max3DDistance^2 - altitude^2)
    
    if (max3DDistance <= altitude)
    {
        // UAV is too high, no ground coverage
        return 0.0;
    }
    
    double maxHorizontalRange = std::sqrt(max3DDistance * max3DDistance - altitude * altitude);
    
    return maxHorizontalRange;
}

/**
 * @brief Callback to calculate and log UAV communication range after global setup phase
 *
 * This function demonstrates UAV coverage calculation using the global node topology
 * after the setup phase is complete. It calculates theoretical max range and checks
 * coverage for sample UAV positions.
 *
 * @param uavAltitude UAV altitude in meters
 * @param txPowerDbm UAV transmit power in dBm
 * @param rxSensitivityDbm Ground node receiver sensitivity in dBm
 * @param gridSize Grid size for analysis
 * @param spacing Grid spacing
 */
static void
OnCalculateUavRange(double uavAltitude, double txPowerDbm, double rxSensitivityDbm,
                    uint32_t gridSize, double spacing)
{
    NS_LOG_INFO("=== UAV COMMUNICATION RANGE CALCULATION at t=" 
                << Simulator::Now().GetSeconds() << "s ===");
    
    // Calculate maximum theoretical range
    double maxHorizontalRange = CalculateMaxUavCommRange(uavAltitude, txPowerDbm, rxSensitivityDbm);
    double coverageArea = 3.14159 * maxHorizontalRange * maxHorizontalRange;
    
    NS_LOG_INFO("[UAV RANGE THEORY] Altitude: " << uavAltitude << "m | "
                << "Max horizontal range: " << maxHorizontalRange << "m | "
                << "Coverage area: " << (coverageArea / 1000000.0) << " km²");
    
    // // Get grid dimensions
    // double gridWidth = scenario3::ParameterCalculators::CalculateGridDimension(gridSize, spacing);
    // double gridHeight = scenario3::ParameterCalculators::CalculateGridDimension(gridSize, spacing);
    
    // // Test UAV at different positions and count reachable nodes
    // NS_LOG_INFO("[UAV COVERAGE TEST] Testing " << gridSize << "x" << gridSize 
    //             << " grid (" << scenario3::g_globalNodeTopology.size() << " nodes)");
    
    // struct UavTestPosition {
    //     double x, y;
    //     std::string name;
    // };
    
    // std::vector<UavTestPosition> testPositions = {
    //     {gridWidth / 2, gridHeight / 2, "grid center"},
    //     {0, 0, "top-left corner"},
    //     {gridWidth, 0, "top-right corner"},
    //     {0, gridHeight, "bottom-left corner"},
    //     {gridWidth, gridHeight, "bottom-right corner"},
    //     {gridWidth / 4, gridHeight / 4, "first quadrant center"},
    //     {3 * gridWidth / 4, 3 * gridHeight / 4, "fourth quadrant center"}
    // };
    
    // uint32_t totalPositions = testPositions.size();
    // uint32_t totalNodesCovered = 0;
    // double avgCoveragePercent = 0.0;
    
    // for (const auto& testPos : testPositions)
    // {
    //     // Get nodes in range at this position
    //     std::vector<uint32_t> nodesInRange = GetGroundNodesInUavRange(
    //         testPos.x, testPos.y, uavAltitude,
    //         txPowerDbm, rxSensitivityDbm);
        
    //     uint32_t nodesCount = nodesInRange.size();
    //     uint32_t totalNodes = scenario3::g_globalNodeTopology.size();
    //     double coveragePercent = (totalNodes > 0) ? (100.0 * nodesCount / totalNodes) : 0.0;
        
    //     totalNodesCovered += nodesCount;
    //     avgCoveragePercent += coveragePercent;
        
    //     NS_LOG_INFO("[UAV@(" << testPos.x << "," << testPos.y << "," << uavAltitude << ")] "
    //                 << testPos.name << ": " << nodesCount << "/" << totalNodes 
    //                 << " nodes (" << std::fixed << std::setprecision(1) << coveragePercent << "%)");
    // }
    
    // avgCoveragePercent /= totalPositions;
    
    // NS_LOG_WARN("[UAV RANGE SUMMARY] "
    //             << "Altitude: " << uavAltitude << "m | "
    //             << "Max range: " << maxHorizontalRange << "m | "
    //             << "Avg coverage: " << std::fixed << std::setprecision(1) << avgCoveragePercent << "% | "
    //             << "Nodes per position (avg): " << (totalNodesCovered / totalPositions));
    
    // NS_LOG_INFO("=== END UAV RANGE CALCULATION ===");
}

/**
 * @brief Schedule UAV communication range calculation after global setup phase
 *
 * @param uavAltitude UAV altitude in meters
 * @param txPowerDbm UAV transmit power in dBm
 * @param rxSensitivityDbm Ground node receiver sensitivity in dBm
 * @param gridSize Grid size for analysis
 * @param spacing Grid spacing
 * @param delaySeconds Delay in seconds after global setup phase completes
 */
void
ScheduleUavRangeCalculation(double uavAltitude, double txPowerDbm, 
                           double rxSensitivityDbm, uint32_t gridSize, 
                           double spacing, double delaySeconds)
{
    // Get completion time (6.0s startup + completion delay)
    // This is roughly when the setup phase ends + some margin
    double setupCompletionTime = 6.0 + delaySeconds;
    
    Simulator::Schedule(Seconds(setupCompletionTime),
                       &OnCalculateUavRange,
                       uavAltitude,
                       txPowerDbm,
                       rxSensitivityDbm,
                       gridSize,
                       spacing);
    
    NS_LOG_INFO("UAV range calculation scheduled for t=" << setupCompletionTime << "s");
}

//
// GREEDY FLIGHT PATH PLANNING
//

/**
 * @brief Calculate Greedy Nearest Neighbor flight path
 *
 * Implements a greedy algorithm: at each step, fly to the nearest unvisited 
 * suspicious node, update the path and continue until all nodes are visited.
 */
UavFlightPath
CalculateGreedyFlightPath(const std::set<uint32_t>& suspiciousNodeIds,
                         double startX, double startY, double startZ,
                         double uavSpeed,
                         double txPowerDbm, double rxSensitivityDbm)
{
    UavFlightPath result;
    result.waypoints.clear();
    result.totalDistance = 0.0;
    result.estimatedFlightTime = 0.0;
    result.nodesToVisit = suspiciousNodeIds.size();
    result.nodesReachable = 0;
    result.coveragePercentage = 0.0;
    result.isValid = false;

    // Check if suspicious region is empty
    if (suspiciousNodeIds.empty())
    {
        NS_LOG_WARN("[GREEDY FLIGHT PATH] Empty suspicious region");
        return result;
    }

    // Initialize: track visited nodes
    std::set<uint32_t> unvisited = suspiciousNodeIds;
    std::set<uint32_t> visited;

    // Starting position
    double currentX = startX;
    double currentY = startY;
    double currentZ = startZ;
    double currentTime = 0.0;

    // Log start of greedy traversal
    NS_LOG_INFO("[GREEDY FLIGHT PATH] Starting from (" << currentX << ", " << currentY 
                << ", " << currentZ << ") to visit " << unvisited.size() << " nodes");

    // Greedy loop: always go to nearest unvisited node
    uint32_t waypointCount = 0;
    while (!unvisited.empty())
    {
        double minDistance = std::numeric_limits<double>::max();
        uint32_t nearestNodeId = UINT32_MAX;
        double nearestX = 0, nearestY = 0;

        // Find nearest unvisited node
        for (uint32_t nodeId : unvisited)
        {
            const scenario3::GlobalNodeInfo* nodeInfo = scenario3::GetNodeInfo(nodeId);
            if (!nodeInfo)
                continue;

            // Calculate 2D horizontal distance (flying is mostly horizontal)
            double dx = nodeInfo->posX - currentX;
            double dy = nodeInfo->posY - currentY;
            double distance = std::sqrt(dx * dx + dy * dy);

            if (distance < minDistance)
            {
                minDistance = distance;
                nearestNodeId = nodeId;
                nearestX = nodeInfo->posX;
                nearestY = nodeInfo->posY;
            }
        }

        // No valid node found
        if (nearestNodeId == UINT32_MAX)
        {
            NS_LOG_WARN("[GREEDY FLIGHT PATH] Could not find nearest unvisited node");
            break;
        }

        // Check if node is reachable at this altitude
        UavGroundCommRange commRange = CalculateUavGroundCommRange(
            nearestX, nearestY, currentZ, nearestX, nearestY, txPowerDbm, rxSensitivityDbm);

        if (commRange.isInRange)
        {
            result.nodesReachable++;
        }

        // Update current position
        currentX = nearestX;
        currentY = nearestY;
        currentTime += minDistance / uavSpeed;

        // Create waypoint
        UavFlightWaypoint wp;
        wp.x = nearestX;
        wp.y = nearestY;
        wp.z = currentZ;
        wp.targetNodeId = nearestNodeId;
        wp.distanceFromPrevious = minDistance;
        wp.arrivalTime = currentTime;

        result.waypoints.push_back(wp);
        result.totalDistance += minDistance;

        // Mark as visited
        visited.insert(nearestNodeId);
        unvisited.erase(nearestNodeId);
        
        waypointCount++;

        NS_LOG_INFO("[GREEDY FLIGHT PATH] Waypoint " << visited.size() 
                     << ": Node " << nearestNodeId << " at (" << nearestX << ", " 
                     << nearestY << "), distance=" << minDistance << "m, time=" << currentTime << "s");
    }

    // Calculate final statistics
    result.estimatedFlightTime = result.totalDistance / uavSpeed;
    result.coveragePercentage = (result.nodesToVisit > 0)
        ? (100.0 * result.nodesReachable / result.nodesToVisit)
        : 0.0;
    result.isValid = (visited.size() == result.nodesToVisit);

    NS_LOG_INFO("[GREEDY FLIGHT PATH] "
                << "Total distance: " << result.totalDistance << "m | "
                << "Flight time: " << result.estimatedFlightTime << "s | "
                << "Visited: " << visited.size() << "/" << result.nodesToVisit << " nodes | "
                << "Reachable: " << result.nodesReachable << "/" << result.nodesToVisit << " | "
                << "Coverage: " << std::fixed << std::setprecision(1) 
                << result.coveragePercentage << "% | "
                << "Valid: " << (result.isValid ? "YES" : "NO"));

    return result;
}

/**
 * @brief Callback to calculate greedy flight path for suspicious region
 *
 * Called after suspicious region is detected. Calculates a greedy nearest-neighbor
 * path that visits all suspicious nodes.
 */
static void
OnCalculateGreedyFlightPath(double uavAltitude, double uavSpeed,
                           double txPowerDbm, double rxSensitivityDbm,
                           uint32_t gridSize, double spacing)
{
    NS_LOG_INFO("=== GREEDY FLIGHT PATH CALCULATION at t=" 
                << Simulator::Now().GetSeconds() << "s ===");

    // Use globally stored suspicious nodes
    if (g_suspiciousNodes.empty())
    {
        NS_LOG_WARN("[GREEDY FLIGHT PATH] No suspicious nodes available");
        return;
    }

    // Calculate starting position (grid center)
    double gridDim = scenario3::ParameterCalculators::CalculateGridDimension(gridSize, spacing);
    double startX = gridDim / 2.0;
    double startY = gridDim / 2.0;
    double startZ = uavAltitude;

    // Calculate greedy path
    UavFlightPath flightPath = CalculateGreedyFlightPath(
        g_suspiciousNodes, startX, startY, startZ, uavSpeed, txPowerDbm, rxSensitivityDbm);

    // Log detailed waypoints
    if (flightPath.isValid)
    {
        NS_LOG_INFO("[GREEDY FLIGHT PATH] Waypoints Summary:");
        for (size_t i = 0; i < flightPath.waypoints.size(); ++i)
        {
            const auto& wp = flightPath.waypoints[i];
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2);
            oss << "  WP" << (i + 1) << ": Node " << wp.targetNodeId 
                << " at (" << wp.x << ", " << wp.y << "), "
                << "dist=" << wp.distanceFromPrevious << "m, "
                << "arr_time=" << wp.arrivalTime << "s";
            NS_LOG_INFO(oss.str());
        }
    }

    NS_LOG_INFO("=== END GREEDY FLIGHT PATH CALCULATION ===");
}

/**
 * @brief Schedule Greedy flight path calculation
 *
 * @param uavAltitude UAV altitude in meters
 * @param uavSpeed UAV speed in m/s
 * @param txPowerDbm UAV transmit power in dBm
 * @param rxSensitivityDbm Ground node receiver sensitivity in dBm
 * @param gridSize Grid size for starting position calculation
 * @param spacing Grid spacing
 * @param delaySeconds Delay before scheduling calculation
 */
void
CalculateAndLogGreedyFlightPath(double uavAltitude, double uavSpeed,
                               double txPowerDbm, double rxSensitivityDbm,
                               uint32_t gridSize, double spacing)
{
    // Directly call the callback without additional scheduling
    OnCalculateGreedyFlightPath(uavAltitude, uavSpeed, txPowerDbm, rxSensitivityDbm,
                               gridSize, spacing);
}
// UAV nằm ngoài cách xa mạng về phía trái 
// Ý tưởng đơn giản: (hình xoắn ốc)
// UAV bay thẳng đến mép trên hoặc dưới của suspicious region (tuỳ theo khoảng cách gần nhất), 
// sau đó bay dọc theo mép đó vòng vào trong cho đến khi phủ kín toàn bộ suspicious region.
// Hàm này chỉ trả về đường bay logic được tính toán trước chưa có thực thi và tương tác mạng

void 
ScheduleUavSpiralTrajectory()
{
    // ...
}

} // namespace wsn
} // namespace ns3
