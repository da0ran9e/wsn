/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Scenario1: Grid network setup with corner and center broadcasts
 */

#ifndef SCENARIO1_H
#define SCENARIO1_H

#include "../../model/routing/scenario1/parameters.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/spectrum-channel.h"
#include "ns3/core-module.h"

namespace ns3
{
namespace wsn
{

/**
 * @brief Configuration parameters for Scenario1 ground network
 */
struct Scenario1Config
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
    Scenario1Config()
                : gridSize(scenario1::GroundNetworkParams::DEFAULT_GRID_SIZE),
                    spacing(scenario1::GroundNetworkParams::DEFAULT_GRID_SPACING),
                    txPowerDbm(scenario1::GroundNetworkParams::DEFAULT_TX_POWER_DBM),
                    rxSensitivityDbm(scenario1::GroundNetworkParams::DEFAULT_RX_SENSITIVITY_DBM),
                    packetSize(scenario1::GroundNetworkParams::DEFAULT_PACKET_SIZE),
                    simTimeSeconds(scenario1::GroundNetworkParams::DEFAULT_SIM_TIME),
                    firstTxTimeSeconds(scenario1::GroundNetworkParams::DEFAULT_FIRST_TX_TIME)
    {
    }
};

/**
 * @brief Configuration parameters for UAV scheduler
 */
struct Scenario1UavConfig
{
    double uavAltitude;             ///< UAV altitude in meters
    double uavSpeed;                ///< UAV speed in m/s
    double uavBroadcastInterval;    ///< UAV broadcast interval in seconds

    /**
     * @brief Default constructor with UAV parameter defaults
     */
    Scenario1UavConfig()
                : uavAltitude(scenario1::UAVParams::DEFAULT_ALTITUDE),
                    uavSpeed(scenario1::UAVParams::DEFAULT_SPEED),
                    uavBroadcastInterval(scenario1::UAVParams::DEFAULT_BROADCAST_INTERVAL)
    {
    }
};

/**
 * @brief Setup grid network with CC2420 devices
 * @param config Network configuration parameters
 * @param nodes Output: created nodes
 * @param devices Output: installed network devices
 * @return Spectrum channel used for the network
 */
Ptr<SpectrumChannel> SetupScenario1Network(const Scenario1Config& config,
                                           NodeContainer& nodes,
                                           NetDeviceContainer& devices);

/**
 * @brief Schedule traffic pattern for ground nodes in Scenario1
 * 
 * Schedules broadcasts from:
 * - Center node
 * - Four corner nodes (top-left, top-right, bottom-left, bottom-right)
 * 
 * @param config Scenario configuration
 * @param nodes Nodes to schedule traffic on
 */
void ScheduleScenario1Traffic(const Scenario1Config& config, NodeContainer nodes);

/**
 * @brief Setup and schedule UAV node traffic pattern
 * 
 * Creates a UAV node, adds it to the network, and schedules periodic broadcasts
 * along a predefined waypoint trajectory across the grid.
 * 
 * @param groundConfig Ground network configuration
 * @param uavConfig UAV configuration
 * @param nodes Node container (will be modified to include UAV)
 * @param channel Spectrum channel for UAV device
 * @return UAV node ID (nodes.GetN() - 1 after adding UAV)
 */
uint32_t ScheduleScenario1UavTraffic(const Scenario1Config& groundConfig,
                                     const Scenario1UavConfig& uavConfig,
                                     NodeContainer& nodes,
                                     Ptr<SpectrumChannel> channel);

/**
 * @brief Get total transmissions count from ground nodes
 * @return Total number of ground node transmission requests
 */
uint32_t GetScenario1TotalTx();

/**
 * @brief Get total receptions count for ground nodes
 * @return Total number of ground node received packets
 */
uint32_t GetScenario1TotalRx();

/**
 * @brief Get total transmissions count from UAV nodes
 * @return Total number of UAV transmission requests
 */
uint32_t GetScenario1UavTotalTx();

/**
 * @brief Get total receptions count for UAV nodes
 * @return Total number of UAV received packets
 */
uint32_t GetScenario1UavTotalRx();

/**
 * @brief Get total logic broadcasts
 * @return Number of logic broadcasts
 */
uint32_t GetScenario1LogicBroadcasts();

/**
 * @brief Get total logic receptions
 * @return Number of logic receptions
 */
uint32_t GetScenario1LogicReceptions();

/**
 * @brief Get confidence of one ground node (logic-only)
 * @param nodeId Node ID
 * @return Confidence value
 */
double GetScenario1LogicNodeConfidence(uint32_t nodeId);

/**
 * @brief Get fragment count for one ground node (logic-only)
 * @param nodeId Node ID
 * @return Number of fragments received
 */
uint32_t GetScenario1LogicNodeFragments(uint32_t nodeId);

/**
 * @brief Check if one ground node triggered alert (logic-only)
 * @param nodeId Node ID
 * @return True if alerted
 */
bool GetScenario1LogicNodeAlerted(uint32_t nodeId);

/**
 * @brief Generate logic fragment set for UAV broadcasts
 * @param numFragments Number of fragments
 * @param totalConfidence Total confidence to partition
 */
void GenerateScenario1LogicFragments(uint32_t numFragments, double totalConfidence = 1.0);

/**
 * @brief Simulate UAV broadcasts using logic-only routing (no network interaction)
 * @param groundConfig Ground network configuration
 * @param uavConfig UAV configuration
 * @param numFragments Number of fragments to use
 */
void SimulateScenario1LogicBroadcasts(const Scenario1Config& groundConfig,
                                      const Scenario1UavConfig& uavConfig,
                                      uint32_t numFragments);

/**
 * @brief Setup and schedule UAV node with fragment-based network broadcasts
 * 
 * This function extends ScheduleScenario1UavTraffic by:
 * 1. Creating and configuring a UAV node
 * 2. Generating network fragment set
 * 3. Scheduling periodic broadcasts with FragmentHeader
 * 
 * Packets will traverse actual CC2420 NetDevice/MAC/PHY layers with fragment data.
 * 
 * @param groundConfig Ground network configuration
 * @param uavConfig UAV configuration
 * @param nodes Node container (will be modified to include UAV)
 * @param channel Spectrum channel for UAV device
 * @param numFragments Number of fragments to generate
 * @param totalConfidence Total confidence to distribute among fragments
 * @return UAV node ID
 */
uint32_t ScheduleScenario1UavFragmentTraffic(const Scenario1Config& groundConfig,
                                              const Scenario1UavConfig& uavConfig,
                                              NodeContainer& nodes,
                                              Ptr<SpectrumChannel> channel,
                                              uint32_t numFragments,
                                              double totalConfidence = 1.0);

/**
 * @brief Get confidence of one ground node (network-based from actual packets)
 * @param nodeId Node ID
 * @return Confidence value
 */
double GetScenario1NetworkNodeConfidence(uint32_t nodeId);

/**
 * @brief Get fragment count for one ground node (network-based)
 * @param nodeId Node ID
 * @return Number of fragments received
 */
uint32_t GetScenario1NetworkNodeFragments(uint32_t nodeId);

/**
 * @brief Check if one ground node triggered alert (network-based)
 * @param nodeId Node ID
 * @return True if alerted
 */
bool GetScenario1NetworkNodeAlerted(uint32_t nodeId);

} // namespace wsn
} // namespace ns3

#endif // SCENARIO1_H
