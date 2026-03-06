/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Scenario3: Multi-UAV grid network setup with fragment-based tracking
 */

#ifndef SCENARIO3_H
#define SCENARIO3_H

#include "../../model/routing/scenario3/parameters.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/spectrum-channel.h"
#include "ns3/core-module.h"

namespace ns3
{
namespace wsn
{

/**
 * @brief Configuration parameters for Scenario3 ground network
 */
struct Scenario3Config
{
    uint32_t gridSize;              ///< Grid size (N x N)
    double spacing;                 ///< Grid spacing in meters
    double txPowerDbm;              ///< TX power in dBm
    double rxSensitivityDbm;        ///< RX sensitivity in dBm
    uint32_t packetSize;            ///< Packet size in bytes
    double simTimeSeconds;          ///< Simulation time
    double firstTxTimeSeconds;      ///< Time of first transmission

    /**
     * @brief Default constructor with reasonable defaults
     */
    Scenario3Config()
                : gridSize(scenario3::GroundNetworkParams::DEFAULT_GRID_SIZE),
                    spacing(scenario3::GroundNetworkParams::DEFAULT_GRID_SPACING),
                    txPowerDbm(scenario3::GroundNetworkParams::DEFAULT_TX_POWER_DBM),
                    rxSensitivityDbm(scenario3::GroundNetworkParams::DEFAULT_RX_SENSITIVITY_DBM),
                    packetSize(scenario3::GroundNetworkParams::DEFAULT_PACKET_SIZE),
                    simTimeSeconds(scenario3::GroundNetworkParams::DEFAULT_SIM_TIME),
                    firstTxTimeSeconds(scenario3::GroundNetworkParams::DEFAULT_FIRST_TX_TIME)
    {
    }
};

/**
 * @brief Configuration parameters for UAV scheduler in Scenario3
 */
struct Scenario3UavConfig
{
    double uavAltitude;             ///< UAV altitude in meters
    double uavSpeed;                ///< UAV speed in m/s
    double uavBroadcastInterval;    ///< UAV broadcast interval in seconds

    /**
     * @brief Default constructor with UAV parameter defaults
     */
    Scenario3UavConfig()
                : uavAltitude(scenario3::UAVParams::DEFAULT_ALTITUDE),
                    uavSpeed(scenario3::UAVParams::DEFAULT_SPEED),
                    uavBroadcastInterval(scenario3::UAVParams::DEFAULT_BROADCAST_INTERVAL)
    {
    }
};

/**
 * @brief Setup grid network with CC2420 devices for Scenario3
 * @param config Network configuration parameters
 * @param nodes Output: created nodes
 * @param devices Output: installed network devices
 * @return Spectrum channel used for the network
 */
Ptr<SpectrumChannel> SetupScenario3Network(const Scenario3Config& config,
                                           NodeContainer& nodes,
                                           NetDeviceContainer& devices);

/**
 * @brief Schedule Global Startup Phase for all ground nodes (Network-level)
 *
 * Coordinates the synchronized activation of all ground nodes during network startup.
 * Node activations are distributed evenly across the specified startup duration.
 *
 * **This is a network-level function** that schedules events for all nodes in the system.
 * Per-node packet handlers are in ground-node-routing/global-startup-phase.cc.
 *
 * @param nodes NodeContainer with all ground nodes to activate
 * @param gridSize Size of the ground node grid (nodes arranged in gridSize x gridSize)
 * @param packetSize Standard packet size in bytes (for logging/reference)
 * @param startupTime Time (in seconds) when startup phase begins (e.g., 0.1)
 * @param startupDuration Duration (in seconds) over which to spread node activations (e.g., 1.0)
 *
 * @example
 * ```cpp
 * NodeContainer groundNodes;
 * SetupScenario3Network(config, groundNodes, devices);
 * ScheduleScenario3GlobalStartupPhase(groundNodes, 10, 64, 0.1, 1.0);
 * ```
 */
void ScheduleScenario3GlobalStartupPhase(NodeContainer nodes,
                                         uint32_t gridSize,
                                         uint32_t packetSize,
                                         double startupTime,
                                         double startupDuration);

/**
 * @brief Schedule Global Setup Phase completion for all ground nodes (Network-level)
 *
 * Coordinates the completion signal for all ground nodes after the startup phase ends.
 * Also selects a random signal source location within the grid for future use.
 * Calls HandleGlobalSetupPhaseCompletion() for each node with the final state snapshot.
 *
 * **This is a network-level function** that schedules per-node completion handlers
 * and signal source location selection.
 * Per-node handlers are in ground-node-routing/global-startup-phase.cc.
 *
 * @param nodes NodeContainer with all ground nodes
 * @param gridSize Size of the ground node grid
 * @param completionTime Time (in seconds) when completion signal is sent
 * @param totalActivatedNodes Total number of nodes that were activated during startup
 * @param spacing Grid node spacing in meters (used to compute grid boundaries)
 *
 * @example
 * ```cpp
 * NodeContainer groundNodes;
 * ScheduleScenario3GlobalSetupPhaseCompletion(groundNodes, 10, 6.0, 100, 20.0);
 * // After this, signal source location can be retrieved via:
 * // scenario3::SignalSourceLocation::GetLocation()
 * ```
 */
void ScheduleScenario3GlobalSetupPhaseCompletion(NodeContainer nodes,
                                                 uint32_t gridSize,
                                                 double completionTime,
                                                 uint32_t totalActivatedNodes,
                                                 double spacing);

/**
 * @brief Schedule ground traffic pattern (Network-level)
 *
 * Schedules broadcasts from center and four corner nodes at predefined intervals.
 * **This is a network-level function** that coordinates multiple nodes.
 *
 * @param nodes Node container
 * @param gridSize Grid size (N x N)
 * @param packetSize Packet size in bytes
 * @param firstTxTime Time of first transmission
 */
void ScheduleScenario3GroundTrafficPattern(NodeContainer nodes,
                                           uint32_t gridSize,
                                           uint32_t packetSize,
                                           double firstTxTime);

/**
 * @brief Schedule traffic pattern for ground nodes in Scenario3
 * 
 * Schedules broadcasts from:
 * - Center node
 * - Four corner nodes (top-left, top-right, bottom-left, bottom-right)
 * 
 * @param config Scenario configuration
 * @param nodes Nodes to schedule traffic on
 */
void ScheduleScenario3Traffic(const Scenario3Config& config, NodeContainer nodes);

/**
 * @brief Setup and schedule UAV node traffic pattern with fragments
 * 
 * Creates a UAV node, adds it to the network, and schedules periodic broadcasts
 * along a predefined waypoint trajectory across the grid.
 * 
 * @param groundConfig Ground network configuration
 * @param uavConfig UAV configuration
 * @param nodes Node container (will be modified to include UAV)
 * @param channel Spectrum channel for UAV device
 * @param numFragments Number of fragments for this UAV
 * @param totalConfidence Total confidence to distribute
 * @param uavIndex Index of this UAV (0, 1, 2, ...)
 * @return UAV node ID (nodes.GetN() - 1 after adding UAV)
 */
uint32_t ScheduleScenario3UavFragmentTraffic(const Scenario3Config& groundConfig,
                                             const Scenario3UavConfig& uavConfig,
                                             NodeContainer& nodes,
                                             Ptr<SpectrumChannel> channel,
                                             uint32_t numFragments,
                                             double totalConfidence,
                                             uint32_t uavIndex);

/**
 * @brief Get total transmissions count from ground nodes
 * @return Total number of ground node transmission requests
 */
uint32_t GetScenario3TotalTx();

/**
 * @brief Get total receptions count for ground nodes
 * @return Total number of ground node received packets
 */
uint32_t GetScenario3TotalRx();

/**
 * @brief Get total transmissions count from UAV nodes
 * @return Total number of UAV transmission requests
 */
uint32_t GetScenario3UavTotalTx();

/**
 * @brief Get total receptions count for UAV nodes
 * @return Total number of UAV received packets
 */
uint32_t GetScenario3UavTotalRx();

/**
 * @brief Get confidence of one ground node
 * @param nodeId Node ID
 * @return Confidence value [0, 1]
 */
double GetScenario3NetworkNodeConfidence(uint32_t nodeId);

/**
 * @brief Get number of unique fragments received by one node
 * @param nodeId Node ID
 * @return Number of fragments
 */
uint32_t GetScenario3NetworkNodeFragments(uint32_t nodeId);

/**
 * @brief Check if one node has alerted (confidence >= 0.75)
 * @param nodeId Node ID
 * @return True if node alerted
 */
bool GetScenario3NetworkNodeAlerted(uint32_t nodeId);

/**
 * @brief Initialize visualizer logger for network debugging
 * @param filename Output file for TXT log (default: "network_log.txt")
 */
void InitializeScenario3Visualizer(const std::string& filename = "network_log.txt");

/**
 * @brief Close visualizer logger
 */
void CloseScenario3Visualizer();

/**
 * @brief UAV-to-Ground communication range calculation result
 */
struct UavGroundCommRange
{
    double distance3D;           ///< 3D Euclidean distance (meters)
    double pathLossDb;          ///< Path loss in dB
    double receivedPowerDbm;    ///< Received power in dBm
    bool isInRange;             ///< Whether communication is possible
    double linkMarginDb;        ///< Link margin (rxPower - rxSensitivity)
};

/**
 * @brief Calculate effective communication range between UAV and ground node
 *
 * Computes 3D distance, path loss (Free Space Path Loss at 2.4 GHz),
 * received power, and determines if communication is possible.
 *
 * @param uavX UAV X coordinate (meters)
 * @param uavY UAV Y coordinate (meters)
 * @param uavZ UAV altitude (meters)
 * @param groundX Ground node X coordinate (meters)
 * @param groundY Ground node Y coordinate (meters)
 * @param txPowerDbm Transmit power in dBm
 * @param rxSensitivityDbm Receiver sensitivity in dBm
 * @return UavGroundCommRange with distance, path loss, and link quality metrics
 */
UavGroundCommRange CalculateUavGroundCommRange(double uavX, double uavY, double uavZ,
                                               double groundX, double groundY,
                                               double txPowerDbm, double rxSensitivityDbm);

/**
 * @brief Find all ground nodes within effective communication range of UAV
 *
 * @param uavX UAV X coordinate (meters)
 * @param uavY UAV Y coordinate (meters)
 * @param uavZ UAV altitude (meters)
 * @param txPowerDbm Transmit power in dBm
 * @param rxSensitivityDbm Receiver sensitivity in dBm
 * @return Vector of node IDs that are within communication range
 */
std::vector<uint32_t> GetGroundNodesInUavRange(double uavX, double uavY, double uavZ,
                                               double txPowerDbm, double rxSensitivityDbm);

/**
 * @brief Calculate maximum horizontal communication range for UAV at given altitude
 *
 * @param altitude UAV altitude (meters)
 * @param txPowerDbm Transmit power in dBm
 * @param rxSensitivityDbm Receiver sensitivity in dBm
 * @return Maximum horizontal communication range in meters
 */
double CalculateMaxUavCommRange(double altitude, double txPowerDbm, double rxSensitivityDbm);

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
void ScheduleUavRangeCalculation(double uavAltitude, double txPowerDbm, 
                                 double rxSensitivityDbm, uint32_t gridSize, 
                                 double spacing, double delaySeconds = 0.15);

/**
 * @brief UAV flight waypoint information
 */
struct UavFlightWaypoint
{
    double x;                   ///< X coordinate (meters)
    double y;                   ///< Y coordinate (meters)
    double z;                   ///< Z altitude (meters)
    uint32_t targetNodeId;      ///< Target suspicious node ID
    double distanceFromPrevious; ///< Distance from previous waypoint (meters)
    double arrivalTime;         ///< Estimated arrival time (seconds)
};

/**
 * @brief UAV flight path planning result
 */
struct UavFlightPath
{
    std::vector<UavFlightWaypoint> waypoints;      ///< List of waypoints
    double totalDistance;                           ///< Total flight distance (meters)
    double estimatedFlightTime;                     ///< Estimated flight duration (seconds)
    uint32_t nodesToVisit;                          ///< Total nodes to visit
    uint32_t nodesReachable;                        ///< Nodes reachable at UAV altitude
    double coveragePercentage;                      ///< Coverage percentage
    bool isValid;                                   ///< Whether path is valid
};

/**
 * @brief Calculate Greedy Nearest Neighbor flight path to cover all suspicious nodes
 *
 * Implements a greedy algorithm that starts at a given position and iteratively
 * visits the nearest unvisited suspicious node until all are covered.
 *
 * @param suspiciousNodeIds Set of node IDs in suspicious region to visit
 * @param startX Starting X coordinate (meters)
 * @param startY Starting Y coordinate (meters)
 * @param startZ Starting altitude (meters)
 * @param uavSpeed UAV flying speed (m/s)
 * @param txPowerDbm UAV transmit power in dBm
 * @param rxSensitivityDbm Ground node receiver sensitivity in dBm
 * @return UavFlightPath with waypoints and coverage statistics
 */
UavFlightPath CalculateGreedyFlightPath(const std::set<uint32_t>& suspiciousNodeIds,
                                        double startX, double startY, double startZ,
                                        double uavSpeed,
                                        double txPowerDbm, double rxSensitivityDbm);

/**
 * @brief Immediately calculate and log greedy flight path for current suspicious nodes
 *
 * This is a direct calculation function that processes the globally-stored suspicious nodes.
 * Call this after suspicious nodes have been detected and stored.
 *
 * @param uavAltitude UAV altitude in meters
 * @param uavSpeed UAV speed in m/s
 * @param txPowerDbm UAV transmit power in dBm
 * @param rxSensitivityDbm Ground node receiver sensitivity in dBm
 * @param gridSize Grid size for starting position calculation
 * @param spacing Grid spacing
 */
void CalculateAndLogGreedyFlightPath(double uavAltitude, double uavSpeed,
                                     double txPowerDbm, double rxSensitivityDbm,
                                     uint32_t gridSize, double spacing);

/**
 * @brief Schedule UAV node movement through suspicious-region waypoints
 *
 * Builds a waypoint trajectory from the current suspicious node set using
 * the greedy flight path planner, then applies it to an existing UAV node
 * via WaypointMobilityModel.
 *
 * @param nodes Node container containing the UAV node
 * @param uavNodeId Node ID of UAV to control
 * @param startTimeSeconds Absolute simulation time to start flight plan
 * @param uavAltitude Planned UAV altitude in meters
 * @param uavSpeed UAV speed in m/s
 * @param txPowerDbm UAV transmit power in dBm
 * @param rxSensitivityDbm Ground node receiver sensitivity in dBm
 * @param gridSize Grid size for center/start calculation
 * @param spacing Grid spacing
 */
void ScheduleUavWaypointFlightOverSuspiciousRegion(NodeContainer nodes,
                                                   uint32_t uavNodeId,
                                                   double startTimeSeconds,
                                                   double uavAltitude,
                                                   double uavSpeed,
                                                   double txPowerDbm,
                                                   double rxSensitivityDbm,
                                                   uint32_t gridSize,
                                                   double spacing);

/**
 * @brief Data fragment structure for file dissemination
 */
struct DataFragment
{
    uint32_t fragmentId;        ///< Unique fragment identifier
    uint32_t sequenceNumber;    ///< Position in sequence
    uint32_t totalFragments;    ///< Total number of fragments in set
    uint32_t fragmentSize;      ///< Size of this fragment in bytes
    uint32_t fileHash;          ///< Hash of original file
    double priority;            ///< Priority for transmission [0, 1]
    uint32_t checksum;          ///< CRC32 checksum for integrity
};

/**
 * @brief Generate fragmented data from a large file
 * 
 * Divides a master file (MF) of ~2MB into N fragments (NF) of ~100KB-200KB each.
 * Creates realistic fragment metadata including:
 * - Fragment IDs and sequence numbers
 * - Fragment sizes with some variation  
 * - File hashes and checksums for integrity verification
 * - Priority levels based on importance
 * 
 * @param masterFileSize Size of original file in bytes (default 2MB = 2097152 bytes)
 * @param targetFragmentSize Target size per fragment in bytes (default 150KB = 153600 bytes)
 * @param masterFileConfidence Master file confidence level (0.0-1.0, default 0.95 = 95%)
 * @return Vector of data fragments
 */
std::vector<DataFragment> GenerateFileFragments(uint32_t masterFileSize = 2097152,
                                                uint32_t targetFragmentSize = 153600,
                                                double masterFileConfidence = 0.95);

/**
 * @brief Get the set of suspicious nodes detected during setup phase
 * @return Const reference to set of suspicious node IDs
 */
const std::set<uint32_t>& GetSuspiciousNodes();

/**
 * @brief Schedule UAV fragment broadcasts
 * 
 * Simplified wrapper that generates fragments and delegates to 
 * ScheduleUavPeriodicBroadcasts for actual transmission scheduling.
 * 
 * @param uavNodeId UAV node identifier
 * @param fragments Vector of data fragments to broadcast (unused, kept for compatibility)
 * @param txBitrate Transmission bitrate (unused, kept for compatibility)
 * @param processingDelay Receiver processing delay (unused, kept for compatibility)
 * 
 * @note This function is simplified and does not perform actual scheduling.
 *       The real broadcast scheduling is handled by ScheduleUavPeriodicBroadcasts.
 */
void ScheduleUavFragmentBroadcast(uint32_t uavNodeId,
                                 const std::vector<DataFragment>& fragments,
                                 uint32_t txBitrate = 250000,
                                 double processingDelay = 0.1);

/**
 * @brief Schedule periodic UAV fragment broadcasts
 * 
 * Schedules a UAV to broadcast data fragments at regular intervals.
 * Implements round-robin fragment selection and sequence numbering.
 * 
 * @param nodes NodeContainer with UAV nodes (can be empty, UAV accessed by ID)
 * @param uavNodeId ID of the UAV node to broadcast
 * @param fragments Vector of data fragments to broadcast
 * @param packetSize Packet size in bytes
 * @param startTime Start time for broadcasts
 * @param endTime End time for broadcasts
 * @param interval Interval between broadcasts
 * @param uavIndex Index of this UAV (0, 1, 2, ...)
 */
void ScheduleUavPeriodicBroadcasts(NodeContainer nodes,
                                   uint32_t uavNodeId,
                                   const std::vector<DataFragment>& fragments,
                                   uint32_t packetSize,
                                   double startTime,
                                   double endTime,
                                   double interval,
                                   uint32_t uavIndex);

} // namespace wsn
} // namespace ns3

#endif // SCENARIO3_H
