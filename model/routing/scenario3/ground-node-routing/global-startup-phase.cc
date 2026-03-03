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

#include "global-startup-phase.h"
#include "../packet-header.h"

#include "ns3/cc2420-net-device.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/random-variable-stream.h"
#include "ns3/double.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <queue>
#include <set>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace ns3
{
namespace wsn
{
namespace scenario3
{

NS_LOG_COMPONENT_DEFINE("GlobalStartupPhase");

//
// PER-NODE STATE TRACKING AND PACKET HANDLERS
// (Network-level scheduling functions moved to scenario3.cc)
//

// Namespace for internal state tracking during Global Setup Phase
namespace
{
/**
 * @brief Track Global Setup Phase state per node
 */
struct GlobalSetupPhaseState
{
    uint32_t discoveredNeighbors = 0;      ///< Number of discovered neighbors
    uint32_t logicalNeighbors = 0;         ///< Number of logical neighbors (global knowledge)
    bool isSynchronized = false;           ///< Whether node has synchronized
    bool setupPhaseComplete = false;       ///< Whether setup phase completed
    double clockOffset = 0.0;              ///< Clock offset from source (ms)
    std::set<uint32_t> neighborSet;        ///< Set of discovered neighbors
    uint32_t lastDiscoveryTime = 0;        ///< Timestamp of last discovery
    uint32_t totalActivatedNodes = 0;      ///< Total nodes in the network
    uint32_t discoveryPacketsReceived = 0; ///< Count of discovery packets received
    double lastDiscoveryRssi = 0.0;        ///< RSSI of last discovery packet
    int32_t cellId = -1;                   ///< Hex-cell id of this node
    int32_t cellLeaderId = -1;             ///< Cell leader id for this node
    int32_t intraCellParentId = -1;        ///< Parent id in intra-cell routing tree
    uint32_t intraCellDepth = 0;           ///< Depth in intra-cell tree (leader depth=0)
    uint32_t cellColor = 0;                ///< 3-coloring index for hex-cell
    uint32_t parentVariants = 0;           ///< Number of parent choices (one per CGW tree)
};

struct NodeLogicalTopology
{
    uint32_t nodeId = 0;
    int32_t cellId = -1;
    int32_t q = 0;
    int32_t r = 0;
    uint32_t cellColor = 0;
    uint32_t cellLeaderId = 0;
    int32_t intraCellParentId = -1;
    uint32_t intraCellDepth = 0;
    std::set<uint32_t> logicalNeighbors;
    std::map<int32_t, int32_t> parentByGatewayCell; ///< targetCellId -> parent node id
    std::map<int32_t, uint32_t> depthByGatewayCell; ///< targetCellId -> depth to CGW
};

struct CellLogicalTopology
{
    int32_t cellId = -1;
    int32_t q = 0;
    int32_t r = 0;
    uint32_t color = 0;
    double centerX = 0.0;
    double centerY = 0.0;
    uint32_t leaderId = 0;
    std::vector<uint32_t> members;
    std::set<int32_t> neighbors;
    std::map<int32_t, uint32_t> gateways; ///< neighborCellId -> CGW node id in this cell
};

// Per-node state tracking
std::map<uint32_t, GlobalSetupPhaseState> g_setupPhaseStatePerNode;
std::unordered_map<uint32_t, NodeLogicalTopology> g_nodeLogicalTopology;
std::unordered_map<int32_t, CellLogicalTopology> g_cellLogicalTopology;
bool g_globalTopologyComputed = false;
uint32_t g_globalTopologyNodeCount = 0;
double g_hexCellRadius = 0.0;
double g_logicalNeighborRadius = 0.0;
int32_t g_hexGridOffset = 0;
bool g_printedDiscoveryTableHeader = false;
bool g_printedCompletionTableHeader = false;
bool g_printedLogicDiscoveryTableHeader = false;
bool g_printedLogicCompletionTableHeader = false;

static inline double
Distance2D(double x1, double y1, double x2, double y2)
{
    const double dx = x1 - x2;
    const double dy = y1 - y2;
    return std::sqrt(dx * dx + dy * dy);
}

static inline int32_t
MakeCellId(int32_t q, int32_t r, int32_t gridOffset)
{
    return q + r * gridOffset;
}

static inline uint32_t
HexColor(int32_t q, int32_t r)
{
    return static_cast<uint32_t>(((q - r) % 3 + 3) % 3);
}

static inline void
HexRound(double fracQ, double fracR, int32_t& q, int32_t& r)
{
    const double fracS = -fracQ - fracR;

    int32_t rq = static_cast<int32_t>(std::round(fracQ));
    int32_t rr = static_cast<int32_t>(std::round(fracR));
    int32_t rs = static_cast<int32_t>(std::round(fracS));

    const double qDiff = std::fabs(rq - fracQ);
    const double rDiff = std::fabs(rr - fracR);
    const double sDiff = std::fabs(rs - fracS);

    if (qDiff > rDiff && qDiff > sDiff)
    {
        rq = -rr - rs;
    }
    else if (rDiff > sDiff)
    {
        rr = -rq - rs;
    }

    q = rq;
    r = rr;
}

static void
EnsureGlobalHexTopology(NodeContainer nodes, uint32_t gridSize)
{
    if (g_globalTopologyComputed && g_globalTopologyNodeCount == nodes.GetN())
    {
        return;
    }

    g_nodeLogicalTopology.clear();
    g_cellLogicalTopology.clear();

    const uint32_t totalNodes = nodes.GetN();
    if (totalNodes == 0)
    {
        g_globalTopologyComputed = true;
        g_globalTopologyNodeCount = 0;
        return;
    }

    std::vector<double> posX(totalNodes, 0.0);
    std::vector<double> posY(totalNodes, 0.0);

    for (uint32_t i = 0; i < totalNodes; ++i)
    {
        Ptr<Node> node = nodes.Get(i);
        Ptr<MobilityModel> mobility = node ? node->GetObject<MobilityModel>() : nullptr;
        if (mobility)
        {
            Vector p = mobility->GetPosition();
            posX[i] = p.x;
            posY[i] = p.y;
        }
    }

    double minPositiveDistance = std::numeric_limits<double>::max();
    for (uint32_t i = 0; i < totalNodes; ++i)
    {
        for (uint32_t j = i + 1; j < totalNodes; ++j)
        {
            const double d = Distance2D(posX[i], posY[i], posX[j], posY[j]);
            if (d > 0.0 && d < minPositiveDistance)
            {
                minPositiveDistance = d;
            }
        }
    }

    if (minPositiveDistance == std::numeric_limits<double>::max())
    {
        minPositiveDistance = 50.0;
    }

    // Increase logical connection range to get denser neighbor graph
    // and avoid sparse intra-cell trees.
    g_hexCellRadius = std::max(1.0, minPositiveDistance * 1.60);
    g_logicalNeighborRadius = std::max(1.0, minPositiveDistance * 1.90);
    g_hexGridOffset = static_cast<int32_t>(std::max<uint32_t>(32, gridSize * 8 + 8));

    for (uint32_t i = 0; i < totalNodes; ++i)
    {
        NodeLogicalTopology topo;
        topo.nodeId = i;

        const double fracQ = (std::sqrt(3.0) / 3.0 * posX[i] - 1.0 / 3.0 * posY[i]) / g_hexCellRadius;
        const double fracR = (2.0 / 3.0 * posY[i]) / g_hexCellRadius;
        HexRound(fracQ, fracR, topo.q, topo.r);

        topo.cellId = MakeCellId(topo.q, topo.r, g_hexGridOffset);
        topo.cellColor = HexColor(topo.q, topo.r);

        auto& cell = g_cellLogicalTopology[topo.cellId];
        cell.cellId = topo.cellId;
        cell.q = topo.q;
        cell.r = topo.r;
        cell.color = topo.cellColor;
        cell.centerX = g_hexCellRadius * (std::sqrt(3.0) * cell.q + std::sqrt(3.0) / 2.0 * cell.r);
        cell.centerY = g_hexCellRadius * (3.0 / 2.0 * cell.r);
        cell.members.push_back(i);

        g_nodeLogicalTopology[i] = topo;
    }

    for (uint32_t i = 0; i < totalNodes; ++i)
    {
        for (uint32_t j = i + 1; j < totalNodes; ++j)
        {
            const double d = Distance2D(posX[i], posY[i], posX[j], posY[j]);
            if (d <= g_logicalNeighborRadius)
            {
                g_nodeLogicalTopology[i].logicalNeighbors.insert(j);
                g_nodeLogicalTopology[j].logicalNeighbors.insert(i);
            }
        }
    }

    static const int kHexDirs[6][2] = {{1, 0}, {1, -1}, {0, -1}, {-1, 0}, {-1, 1}, {0, 1}};
    for (auto& kv : g_cellLogicalTopology)
    {
        CellLogicalTopology& cell = kv.second;
        for (const auto& d : kHexDirs)
        {
            const int32_t nq = cell.q + d[0];
            const int32_t nr = cell.r + d[1];
            const int32_t nid = MakeCellId(nq, nr, g_hexGridOffset);
            if (g_cellLogicalTopology.find(nid) != g_cellLogicalTopology.end())
            {
                cell.neighbors.insert(nid);
            }
        }
    }

    // Select CGW per neighbor cell (closest cross-cell logical edge)
    for (auto& kv : g_cellLogicalTopology)
    {
        CellLogicalTopology& cell = kv.second;
        cell.gateways.clear();

        for (int32_t neighborCellId : cell.neighbors)
        {
            double bestDistance = std::numeric_limits<double>::max();
            uint32_t bestCgw = std::numeric_limits<uint32_t>::max();
            bool found = false;

            for (uint32_t memberId : cell.members)
            {
                const NodeLogicalTopology& memberTopo = g_nodeLogicalTopology[memberId];
                for (uint32_t nb : memberTopo.logicalNeighbors)
                {
                    const auto nbIt = g_nodeLogicalTopology.find(nb);
                    if (nbIt == g_nodeLogicalTopology.end() || nbIt->second.cellId != neighborCellId)
                    {
                        continue;
                    }

                    const double d = Distance2D(posX[memberId], posY[memberId], posX[nb], posY[nb]);
                    if (d < bestDistance ||
                        (std::fabs(d - bestDistance) < 1e-9 && memberId < bestCgw))
                    {
                        bestDistance = d;
                        bestCgw = memberId;
                        found = true;
                    }
                }
            }

            if (found)
            {
                cell.gateways[neighborCellId] = bestCgw;
            }
        }
    }

    for (auto& kv : g_cellLogicalTopology)
    {
        CellLogicalTopology& cell = kv.second;

        double bestDistance = std::numeric_limits<double>::max();
        uint32_t bestLeader = cell.members.front();
        for (uint32_t memberId : cell.members)
        {
            const double d = Distance2D(posX[memberId], posY[memberId], cell.centerX, cell.centerY);
            if (d < bestDistance || (std::fabs(d - bestDistance) < 1e-9 && memberId < bestLeader))
            {
                bestDistance = d;
                bestLeader = memberId;
            }
        }

        cell.leaderId = bestLeader;

        std::set<uint32_t> memberSet(cell.members.begin(), cell.members.end());
        std::queue<uint32_t> q;
        std::set<uint32_t> visited;

        q.push(bestLeader);
        visited.insert(bestLeader);
        g_nodeLogicalTopology[bestLeader].cellLeaderId = bestLeader;
        // Root node uses itself as next-hop (never -1)
        g_nodeLogicalTopology[bestLeader].intraCellParentId = static_cast<int32_t>(bestLeader);
        g_nodeLogicalTopology[bestLeader].intraCellDepth = 0;

        while (!q.empty())
        {
            const uint32_t current = q.front();
            q.pop();

            const uint32_t currentDepth = g_nodeLogicalTopology[current].intraCellDepth;
            for (uint32_t nb : g_nodeLogicalTopology[current].logicalNeighbors)
            {
                if (memberSet.find(nb) == memberSet.end() || visited.find(nb) != visited.end())
                {
                    continue;
                }

                visited.insert(nb);
                g_nodeLogicalTopology[nb].cellLeaderId = bestLeader;
                g_nodeLogicalTopology[nb].intraCellParentId = static_cast<int32_t>(current);
                g_nodeLogicalTopology[nb].intraCellDepth = currentDepth + 1;
                q.push(nb);
            }
        }

        for (uint32_t memberId : cell.members)
        {
            NodeLogicalTopology& memberTopo = g_nodeLogicalTopology[memberId];
            memberTopo.cellLeaderId = bestLeader;
            memberTopo.parentByGatewayCell.clear();
            memberTopo.depthByGatewayCell.clear();

            if (memberId == bestLeader)
            {
                continue;
            }

            if (visited.find(memberId) != visited.end())
            {
                continue;
            }

            memberTopo.intraCellParentId = static_cast<int32_t>(bestLeader);
            memberTopo.intraCellDepth = 1;
        }

        // Build one intra-cell tree per CGW (up to 6 in hex neighborhood)
        for (const auto& gwEntry : cell.gateways)
        {
            const int32_t targetCellId = gwEntry.first;
            const uint32_t cgwId = gwEntry.second;

            std::queue<uint32_t> q;
            std::set<uint32_t> visited;
            std::map<uint32_t, int32_t> parent;
            std::map<uint32_t, uint32_t> depth;

            q.push(cgwId);
            visited.insert(cgwId);
            // CGW root in this tree uses itself as next-hop
            parent[cgwId] = static_cast<int32_t>(cgwId);
            depth[cgwId] = 0;

            while (!q.empty())
            {
                const uint32_t current = q.front();
                q.pop();

                const uint32_t currentDepth = depth[current];
                for (uint32_t nb : g_nodeLogicalTopology[current].logicalNeighbors)
                {
                    if (memberSet.find(nb) == memberSet.end() || visited.find(nb) != visited.end())
                    {
                        continue;
                    }

                    visited.insert(nb);
                    parent[nb] = static_cast<int32_t>(current);
                    depth[nb] = currentDepth + 1;
                    q.push(nb);
                }
            }

            for (uint32_t memberId : cell.members)
            {
                NodeLogicalTopology& memberTopo = g_nodeLogicalTopology[memberId];

                if (memberId == cgwId)
                {
                    // Root CGW next-hop is itself (never -1)
                    memberTopo.parentByGatewayCell[targetCellId] = static_cast<int32_t>(cgwId);
                    memberTopo.depthByGatewayCell[targetCellId] = 0;
                    continue;
                }

                auto pIt = parent.find(memberId);
                auto dIt = depth.find(memberId);
                if (pIt != parent.end() && dIt != depth.end())
                {
                    memberTopo.parentByGatewayCell[targetCellId] = pIt->second;
                    memberTopo.depthByGatewayCell[targetCellId] = dIt->second;
                }
                else
                {
                    // Fallback: attach unreachable member directly to CGW in this logical tree
                    memberTopo.parentByGatewayCell[targetCellId] = static_cast<int32_t>(cgwId);
                    memberTopo.depthByGatewayCell[targetCellId] = 1;
                }
            }
        }
    }

    g_globalTopologyComputed = true;
    g_globalTopologyNodeCount = totalNodes;
}
} // anonymous namespace

//
// GLOBAL SETUP PHASE ACTIVATION AND DISCOVERY (PER-NODE LOGIC)
//

/**
 * @brief Start the Global Setup Phase for a single node
 *
 * This is called by the network-level scheduler (in scenario3.cc) to activate
 * a specific node. The node then broadcasts a discovery message after a small
 * random delay to avoid collisions.
 *
 * @param nodeId The node ID to activate
 * @param gridSize Grid size for coordinate calculation
 * @param packetSize Size of discovery packets to send
 */
void
StartGlobalSetupPhase(uint32_t nodeId,
                      uint32_t gridSize,
                      uint32_t packetSize,
                      NodeContainer nodes)
{
    NS_LOG_FUNCTION(nodeId << gridSize << packetSize << nodes.GetN());

    EnsureGlobalHexTopology(nodes, gridSize);

    // Initialize state for this node
    GlobalSetupPhaseState& state = g_setupPhaseStatePerNode[nodeId];
    state.discoveredNeighbors = 0;
    state.logicalNeighbors = 0;
    state.isSynchronized = false;
    state.setupPhaseComplete = false;
    state.clockOffset = 0.0;
    state.neighborSet.clear();
    state.lastDiscoveryTime = (uint32_t)(Simulator::Now().GetMilliSeconds());
    state.cellId = -1;
    state.cellLeaderId = -1;
    state.intraCellParentId = static_cast<int32_t>(nodeId);
    state.intraCellDepth = 0;
    state.cellColor = 0;
    state.parentVariants = 0;

    auto topoIt = g_nodeLogicalTopology.find(nodeId);
    if (topoIt != g_nodeLogicalTopology.end())
    {
        const NodeLogicalTopology& topo = topoIt->second;
        state.logicalNeighbors = static_cast<uint32_t>(topo.logicalNeighbors.size());
        state.cellId = topo.cellId;
        state.cellLeaderId = static_cast<int32_t>(topo.cellLeaderId);
        state.intraCellParentId = topo.intraCellParentId;
        state.intraCellDepth = topo.intraCellDepth;
        state.cellColor = topo.cellColor;
        state.parentVariants = static_cast<uint32_t>(topo.parentByGatewayCell.size());
    }

    NS_LOG_WARN("*** Node " << nodeId << " STARTING GLOBAL SETUP PHASE at t="
                << Simulator::Now().GetSeconds() << "s ***");

    if (!g_printedLogicDiscoveryTableHeader)
    {
        NS_LOG_INFO("[LOGIC_DISCOVERY] node | cell | color | cl | parent | depth | logical_neighbors | parent_variants");
        g_printedLogicDiscoveryTableHeader = true;
    }

    std::ostringstream logicDiscoveryLog;
    logicDiscoveryLog << "[LOGIC_DISCOVERY] "
                      << std::setw(4) << nodeId
                      << " | " << std::setw(4) << state.cellId
                      << " | " << std::setw(5) << state.cellColor
                      << " | " << std::setw(2) << state.cellLeaderId
                      << " | " << std::setw(6) << state.intraCellParentId
                      << " | " << std::setw(5) << state.intraCellDepth
                      << " | " << std::setw(17) << state.logicalNeighbors
                      << " | " << std::setw(15) << state.parentVariants;
    NS_LOG_INFO(logicDiscoveryLog.str());

    // Schedule discovery broadcast with small random delay (0-50ms) to avoid collisions
    Ptr<UniformRandomVariable> random = CreateObject<UniformRandomVariable>();
    random->SetAttribute("Min", DoubleValue(0.0));
    random->SetAttribute("Max", DoubleValue(0.05)); // 50ms max delay
    double randomDelay = random->GetValue();

    NS_LOG_INFO("Node " << nodeId << " will broadcast discovery in "
                << (randomDelay * 1000.0) << " ms");

    // Schedule the discovery broadcast
    Simulator::Schedule(Seconds(randomDelay),
                       &GlobalSetupPhaseDiscovery,
                       nodeId,
                       gridSize,
                       packetSize,
                       nodes);
}

/**
 * @brief Broadcast a discovery message during Global Setup Phase
 *
 * This function sends a discovery broadcast to announce this node's presence
 * to neighbors. Other nodes will receive this via HandleGlobalSetupPhaseDiscovery.
 *
 * @param nodeId The node ID broadcasting discovery
 * @param gridSize Grid size for coordinate calculation
 * @param packetSize Size of discovery packet
 */
void
GlobalSetupPhaseDiscovery(uint32_t nodeId,
                          uint32_t gridSize,
                          uint32_t packetSize,
                          NodeContainer nodes)
{
    NS_LOG_FUNCTION(nodeId << gridSize << packetSize << nodes.GetN());

    if (nodeId >= nodes.GetN())
    {
        NS_LOG_WARN("GlobalSetupPhaseDiscovery: invalid nodeId=" << nodeId
                    << " for nodes.GetN()=" << nodes.GetN());
        return;
    }

    // Calculate node coordinates
    uint32_t row = nodeId / gridSize;
    uint32_t col = nodeId % gridSize;

    Scenario3PacketHeader mainHeader;
    mainHeader.SetMessageType(MSG_TYPE_GLOBAL_SETUP_PHASE);
    mainHeader.SetNodeType(NODE_TYPE_GROUND);
    mainHeader.SetSourceNodeId(static_cast<uint16_t>(nodeId));
    mainHeader.SetDestinationNodeId(0xFFFF); // broadcast
    mainHeader.SetSequenceNumber(0);
    mainHeader.SetTimestamp(static_cast<uint32_t>(Simulator::Now().GetMilliSeconds()));

    const uint32_t headerSize = mainHeader.GetSerializedSize();
    uint32_t payloadSize = (packetSize > headerSize) ? (packetSize - headerSize) : 0;
    mainHeader.SetPayloadLength(static_cast<uint16_t>(payloadSize));

    Ptr<Packet> discoveryPacket = Create<Packet>(payloadSize);
    discoveryPacket->AddHeader(mainHeader);

    Ptr<Node> srcNode = nodes.Get(nodeId);
    Ptr<Cc2420NetDevice> dev = DynamicCast<Cc2420NetDevice>(srcNode->GetDevice(0));
    if (!dev)
    {
        NS_LOG_WARN("Node " << nodeId << " has no Cc2420NetDevice for discovery broadcast");
        return;
    }
    
    NS_LOG_INFO("t=" << Simulator::Now().GetSeconds() << "s Node " << nodeId
                << " broadcasting DISCOVERY packet"
                << " (position: [" << row << "," << col 
                << "], size: " << packetSize << " bytes)");

    if (!dev->Send(discoveryPacket, Mac16Address("FF:FF"), 0))
    {
        NS_LOG_WARN("Node " << nodeId << " failed to send discovery packet");
        return;
    }

    NS_LOG_DEBUG("Node " << nodeId << " discovery packet created"
                << " (packet size: " << packetSize << " bytes)");
}

//
// GLOBAL SETUP PHASE PACKET HANDLERS (PER-NODE LOGIC)
//

void
HandleGlobalStartupPhasePacket(uint32_t nodeId,
                               Ptr<Packet> packet,
                               uint32_t sourceNodeId,
                               double rssiDbm)
{
    NS_LOG_FUNCTION(nodeId << sourceNodeId << rssiDbm);

    if (!packet)
    {
        NS_LOG_WARN("Node " << nodeId << ": Received null packet from node " << sourceNodeId);
        return;
    }

    Ptr<Packet> copy = packet->Copy();
    uint32_t packetSize = copy->GetSize();

    // Minimum size check: at least 24 bytes for Scenario3PacketHeader
    if (packetSize < 24)
    {
        NS_LOG_WARN("Node " << nodeId << ": Received undersized Global Setup Phase packet "
                   << "from node " << sourceNodeId << " (size=" << packetSize << " bytes)");
        return;
    }

    Scenario3PacketHeader mainHeader;
    uint32_t removed = copy->RemoveHeader(mainHeader);
    if (removed == 0 || mainHeader.GetMessageType() != MSG_TYPE_GLOBAL_SETUP_PHASE)
    {
        return;
    }

    uint32_t headerSourceNodeId = mainHeader.GetSourceNodeId();

    // Update node state
    GlobalSetupPhaseState& state = g_setupPhaseStatePerNode[nodeId];

    bool isNewNeighbor = false;
    if (state.neighborSet.find(headerSourceNodeId) == state.neighborSet.end())
    {
        state.neighborSet.insert(headerSourceNodeId);
        state.discoveredNeighbors++;
        isNewNeighbor = true;
    }

    state.lastDiscoveryTime = static_cast<uint32_t>(Simulator::Now().GetMilliSeconds());
    state.discoveryPacketsReceived++;
    state.lastDiscoveryRssi = rssiDbm;

    if (!g_printedDiscoveryTableHeader)
    {
        NS_LOG_INFO("[DISCOVERY_RX]   t(s) | node | src | total_rx | neighbors |   RSSI(dBm) | new");
        g_printedDiscoveryTableHeader = true;
    }

    std::ostringstream tableLog;
    tableLog << std::fixed << std::setprecision(3);
    tableLog << "[DISCOVERY_RX] "
             << std::setw(6) << Simulator::Now().GetSeconds()
             << " | " << std::setw(4) << nodeId
             << " | " << std::setw(3) << headerSourceNodeId
             << " | " << std::setw(8) << state.discoveryPacketsReceived
             << " | " << std::setw(9) << state.discoveredNeighbors
             << " | " << std::setw(11) << rssiDbm
             << " | " << (isNewNeighbor ? "YES" : "NO");

    NS_LOG_INFO(tableLog.str());

    HandleGlobalSetupPhaseDiscovery(nodeId,
                                    headerSourceNodeId,
                                    mainHeader.GetNodeType(),
                                    rssiDbm,
                                    mainHeader.GetTimestamp());

    NS_LOG_DEBUG("Node " << nodeId << " processed Global Setup Phase packet from node "
                << sourceNodeId << " (total discovery packets: "
                << state.discoveryPacketsReceived << ")");
}

void
HandleGlobalSetupPhaseDiscovery(uint32_t nodeId,
                                uint32_t sourceNodeId,
                                uint8_t sourceNodeType,
                                double rssiDbm,
                                uint32_t timestamp)
{
    NS_LOG_FUNCTION(nodeId << sourceNodeId << sourceNodeType << rssiDbm << timestamp);

    GlobalSetupPhaseState& state = g_setupPhaseStatePerNode[nodeId];

    // Record discovery
    if (state.neighborSet.find(sourceNodeId) == state.neighborSet.end())
    {
        state.neighborSet.insert(sourceNodeId);
        state.discoveredNeighbors++;
    }

    // Log node type
    std::string nodeTypeStr;
    switch (sourceNodeType)
    {
    case 0:
        nodeTypeStr = "GROUND";
        break;
    case 1:
        nodeTypeStr = "UAV";
        break;
    case 2:
        nodeTypeStr = "SINK";
        break;
    default:
        nodeTypeStr = "UNKNOWN";
    }

    // NS_LOG_INFO("t=" << Simulator::Now().GetSeconds() << "s Node " << nodeId
    //             << " discovered node " << sourceNodeId << " (type=" << nodeTypeStr
    //             << ", RSSI=" << rssiDbm << " dBm)");

    // NS_LOG_DEBUG("Node " << nodeId << " total discovered neighbors: " << state.discoveredNeighbors);
}

void
HandleGlobalSetupPhaseSync(uint32_t nodeId,
                           uint32_t sourceNodeId,
                           uint32_t sourceTimestamp,
                           double roundTripDelay)
{
    NS_LOG_FUNCTION(nodeId << sourceNodeId << sourceTimestamp << roundTripDelay);

    GlobalSetupPhaseState& state = g_setupPhaseStatePerNode[nodeId];

    // Calculate local timestamp
    uint32_t localTimestamp = (uint32_t)(Simulator::Now().GetMilliSeconds());

    // Calculate clock offset (half of round-trip delay as one-way estimate)
    double oneWayDelay = roundTripDelay / 2.0;
    state.clockOffset = (double)sourceTimestamp + oneWayDelay - (double)localTimestamp;

    state.isSynchronized = true;

    // NS_LOG_INFO("t=" << Simulator::Now().GetSeconds() << "s Node " << nodeId
    //             << " synchronized with node " << sourceNodeId
    //             << " (clock offset: " << state.clockOffset << " ms, "
    //             << "roundTripDelay: " << roundTripDelay << " ms)");

    // NS_LOG_DEBUG("Node " << nodeId << " local timestamp: " << localTimestamp
    //             << ", source timestamp: " << sourceTimestamp
    //             << ", calculated offset: " << state.clockOffset);
}

void
HandleGlobalSetupPhaseCompletion(uint32_t nodeId,
                                 uint32_t completionNodeId,
                                 uint32_t totalActivatedNodes,
                                 uint32_t completionTimestamp)
{
    NS_LOG_FUNCTION(nodeId << completionNodeId << totalActivatedNodes << completionTimestamp);

    GlobalSetupPhaseState& state = g_setupPhaseStatePerNode[nodeId];

    // Mark setup phase as complete
    state.setupPhaseComplete = true;
    state.totalActivatedNodes = totalActivatedNodes;

    // if (!g_printedCompletionTableHeader)
    // {
    //     NS_LOG_INFO("[SETUP_COMPLETE] node | neighbors | discovery_rx | sync | clock_offset(ms) | last_rssi(dBm) | total_activated");
    //     g_printedCompletionTableHeader = true;
    // }

    // std::ostringstream completionTableLog;
    // completionTableLog << std::fixed << std::setprecision(3);
    // completionTableLog << "[SETUP_COMPLETE] "
    //                    << std::setw(4) << nodeId
    //                    << " | " << std::setw(9) << state.discoveredNeighbors
    //                    << " | " << std::setw(12) << state.discoveryPacketsReceived
    //                    << " | " << std::setw(4) << (state.isSynchronized ? "YES" : "NO")
    //                    << " | " << std::setw(16) << state.clockOffset
    //                    << " | " << std::setw(14) << state.lastDiscoveryRssi
    //                    << " | " << std::setw(15) << state.totalActivatedNodes;
    // NS_LOG_INFO(completionTableLog.str());

    if (!g_printedLogicCompletionTableHeader)
    {
        NS_LOG_INFO("[SETUP_LOGIC]    node | cell | color | cl | parent | depth | logical_neighbors | parent_variants");
        g_printedLogicCompletionTableHeader = true;
    }

    // std::ostringstream setupLogicLog;
    // setupLogicLog << "[SETUP_LOGIC]    "
    //               << std::setw(4) << nodeId
    //               << " | " << std::setw(4) << state.cellId
    //               << " | " << std::setw(5) << state.cellColor
    //               << " | " << std::setw(2) << state.cellLeaderId
    //               << " | " << std::setw(6) << state.intraCellParentId
    //               << " | " << std::setw(5) << state.intraCellDepth
    //               << " | " << std::setw(17) << state.logicalNeighbors
    //               << " | " << std::setw(15) << state.parentVariants;
    // NS_LOG_INFO(setupLogicLog.str());

    auto topoIt = g_nodeLogicalTopology.find(nodeId);
    if (topoIt != g_nodeLogicalTopology.end())
    {
        const NodeLogicalTopology& topo = topoIt->second;
        std::ostringstream cgwParentLog;
        cgwParentLog << "[SETUP_CGW_PARENT] node=" << nodeId << " ";

        if (topo.parentByGatewayCell.empty())
        {
            cgwParentLog << "none";
        }
        else
        {
            bool first = true;
            for (const auto& p : topo.parentByGatewayCell)
            {
                if (!first)
                {
                    cgwParentLog << ", ";
                }

                uint32_t depth = 0;
                auto dIt = topo.depthByGatewayCell.find(p.first);
                if (dIt != topo.depthByGatewayCell.end())
                {
                    depth = dIt->second;
                }

                cgwParentLog << "to:" << p.first << "->" << p.second
                             << "(d=" << depth << ")";
                first = false;
            }
        }

        NS_LOG_INFO(cgwParentLog.str());
    }

    // NS_LOG_WARN("*** GLOBAL SETUP PHASE COMPLETED for Node " << nodeId << " ***"
    //             << " at t=" << Simulator::Now().GetSeconds() << "s"
    //             << " (issued by node " << completionNodeId << ", "
    //             << "total activated: " << totalActivatedNodes << ")");

    // NS_LOG_INFO("Node " << nodeId << " transitioning to normal operation mode"
    //             << " (discovered: " << state.discoveredNeighbors << " neighbors, "
    //             << "synchronized: " << (state.isSynchronized ? "YES" : "NO") << ")");

    // NS_LOG_DEBUG("Node " << nodeId << " setup phase summary:"
    //             << "\n  - Neighbors discovered: " << state.discoveredNeighbors
    //             << "\n  - Synchronized: " << (state.isSynchronized ? "yes" : "no")
    //             << "\n  - Clock offset: " << state.clockOffset << " ms"
    //             << "\n  - Total nodes activated: " << state.totalActivatedNodes);
}

} // namespace scenario3
} // namespace wsn
} // namespace ns3
