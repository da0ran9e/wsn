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
 * Calls HandleGlobalSetupPhaseCompletion() for each node with the final state snapshot.
 *
 * **This is a network-level function** that schedules per-node completion handlers.
 * Per-node handlers are in ground-node-routing/global-startup-phase.cc.
 *
 * @param nodes NodeContainer with all ground nodes
 * @param gridSize Size of the ground node grid
 * @param completionTime Time (in seconds) when completion signal is sent
 * @param totalActivatedNodes Total number of nodes that were activated during startup
 *
 * @example
 * ```cpp
 * NodeContainer groundNodes;
 * ScheduleScenario3GlobalSetupPhaseCompletion(groundNodes, 10, 6.0, 100);
 * ```
 */
void ScheduleScenario3GlobalSetupPhaseCompletion(NodeContainer nodes,
                                                 uint32_t gridSize,
                                                 double completionTime,
                                                 uint32_t totalActivatedNodes);

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

} // namespace wsn
} // namespace ns3

#endif // SCENARIO3_H
