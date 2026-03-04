/*
 * Copyright (c) 2026 WSN Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: WSN Project <support@wsn-project.org>
 */

#ifndef GLOBAL_STARTUP_PHASE_H
#define GLOBAL_STARTUP_PHASE_H

#include "ns3/node-container.h"
#include "ns3/packet.h"

namespace ns3
{
namespace wsn
{
namespace scenario3
{

/**
 * @defgroup GlobalStartupPhase Global Startup Phase Per-Node Handlers
 * @ingroup Wsn
 *
 * This module provides per-node packet processing handlers for the Global Setup Phase.
 * Network-level scheduling functions have been moved to scenario3.cc for proper
 * architectural separation.
 *
 * **Architecture:**
 * - scenario3.cc: Network-level scheduling (ScheduleScenario3GlobalStartupPhase)
 * - global-startup-phase.cc: Per-node activation and packet handlers (this module)
 *
 * **Workflow:**
 * 1. Network scheduler calls StartGlobalSetupPhase() for each node
 * 2. Node schedules discovery broadcast with random delay
 * 3. GlobalSetupPhaseDiscovery() sends discovery packet
 * 4. Other nodes receive via HandleGlobalSetupPhaseDiscovery()
 *
 * @see ScheduleScenario3GlobalStartupPhase in scenario3.cc
 */

// Global topology parameters computed during EnsureGlobalHexTopology
// These represent the network's global knowledge about hex-cell partitioning
extern double g_hexCellRadius;          ///< Radius of hex cells in meters
extern double g_logicalNeighborRadius;  ///< Radius for logical neighbor discovery
extern int32_t g_hexGridOffset;         ///< Offset for computing hex cell IDs

/**
 * @brief Global network node topology information
 *
 * This structure stores per-node topology information computed during the global setup phase.
 * It provides global network knowledge that can be accessed by scenario-level code.
 */
struct GlobalNodeInfo
{
    uint32_t nodeId = 0;               ///< Node ID
    int32_t cellId = -1;               ///< Hex cell ID this node belongs to
    int32_t q = 0;                     ///< Hex cell q coordinate (axial)
    int32_t r = 0;                     ///< Hex cell r coordinate (axial)
    uint32_t cellColor = 0;            ///< 3-coloring of the hex cell (0, 1, or 2)
    uint32_t cellLeaderId = 0;         ///< Cell leader node ID
    int32_t intraCellParentId = -1;    ///< Parent in intra-cell tree
    uint32_t intraCellDepth = 0;       ///< Depth in intra-cell tree
    std::set<uint32_t> logicalNeighbors; ///< Logical neighbor nodes (within range)
    
    // Extended topology information
    double posX = 0.0;                 ///< Node X coordinate
    double posY = 0.0;                 ///< Node Y coordinate
    std::map<int32_t, int32_t> parentByGatewayCell; ///< Routing: targetCellId -> parent node for that gateway tree
    std::map<int32_t, uint32_t> depthByGatewayCell; ///< Routing: targetCellId -> depth to gateway in that tree
    std::set<int32_t> neighborCellIds; ///< IDs of neighboring cells (for gateway routing)
};

// Global network topology: maps nodeId -> topology info
// Populated after global setup phase completes
extern std::unordered_map<uint32_t, GlobalNodeInfo> g_globalNodeTopology;

/**
 * @brief Get nodes in a specific hex cell
 *
 * @param cellId The hex cell ID
 * @return Vector of node IDs in the specified cell
 */
std::vector<uint32_t> GetNodesInCell(int32_t cellId);

/**
 * @brief Get all unique cell IDs in the network
 *
 * @return Set of all cell IDs
 */
std::set<int32_t> GetAllCellIds();

/**
 * @brief Get node information by node ID
 *
 * @param nodeId The node ID to query
 * @return Pointer to GlobalNodeInfo, or nullptr if not found
 */
const GlobalNodeInfo* GetNodeInfo(uint32_t nodeId);

/**
 * @brief Start the Global Setup Phase for a single node
 *
 * Called by the network-level scheduler to activate a specific node.
 * The node initializes its state and schedules a discovery broadcast
 * with a small random delay (0-50ms) to avoid collisions.
 *
 * @param nodeId The node ID to activate
 * @param gridSize Grid size for coordinate calculation
 * @param packetSize Size of discovery packets to send
 */
void
StartGlobalSetupPhase(uint32_t nodeId,
                      uint32_t gridSize,
                      uint32_t packetSize,
                      NodeContainer nodes);

/**
 * @brief Broadcast a discovery message during Global Setup Phase
 *
 * Sends a discovery broadcast to announce this node's presence to neighbors.
 * Other nodes will receive this via HandleGlobalSetupPhaseDiscovery().
 *
 * @param nodeId The node ID broadcasting discovery
 * @param gridSize Grid size for coordinate calculation
 * @param packetSize Size of discovery packet
 */
void
GlobalSetupPhaseDiscovery(uint32_t nodeId,
                          uint32_t gridSize,
                          uint32_t packetSize,
                          NodeContainer nodes);

/**
 * @brief Handle a received Global Setup Phase packet
 *
 * This function processes incoming Global Setup Phase packets at ground nodes.
 * It extracts the packet header, validates the message type, and delegates to
 * specific handlers based on the packet content.
 *
 * **Packet Processing Flow:**
 * 1. Verify packet size (minimum header size)
 * 2. Extract main Scenario3PacketHeader
 * 3. Verify MSG_TYPE_GLOBAL_SETUP_PHASE
 * 4. Delegate to type-specific handler
 * 5. Update node state and statistics
 *
 * @param nodeId The node ID receiving the packet
 * @param packet The received packet (contains headers and payload)
 * @param sourceNodeId ID of the node that sent this packet
 * @param rssiDbm Signal strength in dBm
 *
 * @see HandleGlobalSetupPhaseDiscovery
 * @see HandleGlobalSetupPhaseSync
 * @see HandleGlobalSetupPhaseCompletion
 */
void
HandleGlobalStartupPhasePacket(uint32_t nodeId,
                               Ptr<Packet> packet,
                               uint32_t sourceNodeId,
                               double rssiDbm);

/**
 * @brief Handle node discovery during Global Setup Phase
 *
 * Processes discovery messages from other nodes (ground or UAV).
 * Updates the node's neighbor list and network topology information.
 *
 * **Processing:**
 * - Extract source node information
 * - Record neighbor in topology table
 * - Update connection quality metrics (RSSI)
 * - Log discovery event with timestamp
 *
 * @param nodeId The node ID receiving the discovery message
 * @param sourceNodeId The source node ID of the discovery message
 * @param sourceNodeType Type of source node (GROUND, UAV, SINK)
 * @param rssiDbm Signal strength from discovery message
 * @param timestamp Timestamp from discovery packet
 */
void
HandleGlobalSetupPhaseDiscovery(uint32_t nodeId,
                                uint32_t sourceNodeId,
                                uint8_t sourceNodeType,
                                double rssiDbm,
                                uint32_t timestamp);

/**
 * @brief Handle synchronization message during Global Setup Phase
 *
 * Processes time synchronization messages to align clocks across the network.
 *
 * **Processing:**
 * - Extract timestamp from synchronization packet
 * - Calculate time offset from source
 * - Update local clock adjustment
 * - Log sync event with offset information
 *
 * @param nodeId The node ID receiving the sync message
 * @param sourceNodeId The source node sending sync
 * @param sourceTimestamp Timestamp from the source node
 * @param roundTripDelay Estimated propagation delay
 */
void
HandleGlobalSetupPhaseSync(uint32_t nodeId,
                           uint32_t sourceNodeId,
                           uint32_t sourceTimestamp,
                           double roundTripDelay);

/**
 * @brief Handle Global Setup Phase completion signal
 *
 * Processes completion messages indicating that the startup phase has finished.
 * Transitions the node to normal operation mode.
 *
 * **Processing:**
 * - Verify completion message source (typically sink or coordinator)
 * - Lock node state to prevent further startup phase changes
 * - Begin normal data forwarding operation
 * - Log completion event with final statistics
 *
 * @param nodeId The node ID receiving the completion signal
 * @param completionNodeId ID of the node issuing completion (sink/coordinator)
 * @param totalActivatedNodes Total number of successfully activated nodes
 * @param completionTimestamp Timestamp of completion
 */
void
HandleGlobalSetupPhaseCompletion(uint32_t nodeId,
                                 uint32_t completionNodeId,
                                 uint32_t totalActivatedNodes,
                                 uint32_t completionTimestamp);

} // namespace scenario3
} // namespace wsn
} // namespace ns3

#endif /* GLOBAL_STARTUP_PHASE_H */
