/*
 * Scenario 4 - Base Station Node Implementation
 */

#include "base-station-node.h"
#include "fragment-generator.h"
#include "../helper/calc-utils.h"
#include "../ground-node-routing/ground-node-routing.h"
#include "../../../../examples/scenarios/scenario4/scenario4-params.h"
#include "region-selection.h"
#include "uav-control.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/node-list.h"
#include "ns3/simulator.h"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <vector>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("BaseStationNode");

namespace wsn {
namespace scenario4 {
namespace routing {

// Global storage for suspicious nodes detected by base station
std::set<uint32_t> g_suspiciousNodes;

namespace {

void
AssignCellIdAndColorForGroundNodes(double cellRadius)
{
    uint32_t updatedCount = 0;
    const uint32_t nodeCount = static_cast<uint32_t>(g_groundNetworkPerNode.size());
    const int32_t gridOffset = ::ns3::wsn::scenario4::params::ComputeDefaultHexGridOffset(nodeCount);

    for (auto& [nodeId, state] : g_groundNetworkPerNode)
    {
        Ptr<Node> node = NodeList::GetNode(nodeId);
        if (!node)
        {
            NS_LOG_WARN("[BS-INIT] Node " << nodeId << " not found in NodeList");
            continue;
        }

        Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
        if (!mobility)
        {
            NS_LOG_WARN("[BS-INIT] Node " << nodeId << " has no MobilityModel");
            continue;
        }

        const Vector pos = mobility->GetPosition();
        state.position = pos;
        const helper::HexCellCoord coord = helper::ComputeHexCellCoord(pos.x, pos.y, cellRadius);
        state.cellId = helper::MakeCellId(coord.q, coord.r, gridOffset);
        state.cellColor = helper::ComputeHexColor(coord.q, coord.r);

        updatedCount++;
        NS_LOG_DEBUG("[BS-INIT] node=" << nodeId << " pos=(" << pos.x << "," << pos.y
                    << ") q=" << coord.q << " r=" << coord.r
                    << " cellId=" << state.cellId
                    << " color=" << state.cellColor);
        // TODO: in log vào `g_resultFileStream` tại đây
        // Format: [NODE] nodeId cellId cellColor posX posY
        if (ns3::wsn::scenario4::params::g_resultFileStream)
        {
            *ns3::wsn::scenario4::params::g_resultFileStream
                << "[NODE-INFO] " << nodeId << " "
                << state.cellId << " "
                << state.cellColor << " "
                << pos.x << " "
                << pos.y << std::endl;
        }
    }


    NS_LOG_INFO("[BS-INIT] Assigned cellId/cellColor for " << updatedCount
        << " ground nodes (gridOffset=" << gridOffset << ")");
}

void
DiscoverNeighborsAndTwoHopsForGroundNodes(double neighborRadius)
{
    uint32_t neighborLinks = 0;

    for (auto& [nodeId, state] : g_groundNetworkPerNode)
    {
        (void)nodeId;
        state.neighbors.clear();
        state.twoHopNeighbors.clear();
        state.neighborRssi.clear();
        state.neighborDistance.clear();
    }

    for (auto itA = g_groundNetworkPerNode.begin(); itA != g_groundNetworkPerNode.end(); ++itA)
    {
        for (auto itB = std::next(itA); itB != g_groundNetworkPerNode.end(); ++itB)
        {
            const double dist = helper::CalculateDistance(
                itA->second.position.x,
                itA->second.position.y,
                itB->second.position.x,
                itB->second.position.y);

            if (dist > neighborRadius)
            {
                continue;
            }

            const double syntheticRssi = -45.0 - 0.15 * dist;

            itA->second.neighbors.insert(itB->first);
            itB->second.neighbors.insert(itA->first);

            itA->second.neighborDistance[itB->first] = dist;
            itB->second.neighborDistance[itA->first] = dist;

            itA->second.neighborRssi[itB->first] = syntheticRssi;
            itB->second.neighborRssi[itA->first] = syntheticRssi;

            neighborLinks++;
        }
    }

    // in log `g_resultFileStream` tại đây
    // Format: [NEIGHBOR-DISCOVERY] nodeId neighbor1 neighbor2 ...
    if (ns3::wsn::scenario4::params::g_resultFileStream)
    {
        for (const auto& [nodeId, state] : g_groundNetworkPerNode)
        {
            *ns3::wsn::scenario4::params::g_resultFileStream
                << "[NEIGHBOR-DISCOVERY] [" << nodeId << "]";
            for (uint32_t neighborId : state.neighbors)
            {
                *ns3::wsn::scenario4::params::g_resultFileStream
                    << " " << neighborId;
            }
            *ns3::wsn::scenario4::params::g_resultFileStream << std::endl;
        }
    }

    for (auto& [nodeId, state] : g_groundNetworkPerNode)
    {
        for (uint32_t neighborId : state.neighbors)
        {
            auto itNeighbor = g_groundNetworkPerNode.find(neighborId);
            if (itNeighbor == g_groundNetworkPerNode.end())
            {
                continue;
            }

            for (uint32_t n2 : itNeighbor->second.neighbors)
            {
                if (n2 == nodeId || state.neighbors.count(n2) != 0)
                {
                    continue;
                }
                state.twoHopNeighbors.insert(n2);
            }
        }

        state.isIsolated = state.neighbors.empty();
        state.startupComplete = true;
    }

    NS_LOG_INFO("[BS-INIT] Neighbor discovery done with radius=" << neighborRadius
                << "m, links=" << neighborLinks);
}

void
SelectCellLeadersByNearestCellCenter(double cellRadius)
{
    std::map<int32_t, std::vector<uint32_t>> membersByCell;
    for (const auto& [nodeId, state] : g_groundNetworkPerNode)
    {
        membersByCell[state.cellId].push_back(nodeId);
    }

    for (auto& [nodeId, state] : g_groundNetworkPerNode)
    {
        (void)nodeId;
        state.isCellLeader = false;
    }

    uint32_t selectedLeaderCount = 0;

    for (const auto& [cellId, members] : membersByCell)
    {
        if (members.empty())
        {
            continue;
        }

        const auto& firstState = g_groundNetworkPerNode.at(members.front());
        const helper::HexCellCoord cellCoord =
            helper::ComputeHexCellCoord(firstState.position.x, firstState.position.y, cellRadius);

        double centerX = 0.0;
        double centerY = 0.0;
        helper::ComputeHexCellCenter(cellCoord.q, cellCoord.r, cellRadius, centerX, centerY);

        uint32_t bestLeaderId = members.front();
        double bestDistance = std::numeric_limits<double>::max();

        for (uint32_t nodeId : members)
        {
            const auto& state = g_groundNetworkPerNode.at(nodeId);
            const double dist = helper::CalculateDistance(
                state.position.x,
                state.position.y,
                centerX,
                centerY);

            if (dist < bestDistance ||
                (std::fabs(dist - bestDistance) < 1e-9 && nodeId < bestLeaderId))
            {
                bestDistance = dist;
                bestLeaderId = nodeId;
            }
        }

        // Update cell leader info in ground network state
        g_groundNetworkPerNode[bestLeaderId].isCellLeader = true;
        selectedLeaderCount++;

        NS_LOG_DEBUG("[BS-INIT] cellId=" << cellId << " CL=" << bestLeaderId
                                          << " center=(" << centerX << "," << centerY
                                          << ") dist=" << bestDistance);

        // TODO: in log vào `g_resultFileStream` tại đây
        // Format: [CELL-LEADER] cellId leaderNodeId (distance)
        if (ns3::wsn::scenario4::params::g_resultFileStream)
        {
            *ns3::wsn::scenario4::params::g_resultFileStream
                << "[CELL-LEADER] [" << cellId << "] " << bestLeaderId << " (" << bestDistance << ")" << std::endl;
        }
    }

    NS_LOG_INFO("[BS-INIT] Cell leader selection done: " << selectedLeaderCount << " cells");
}

void
ValidateIntraCellRoutingTrees()
{
    // Validate that all nodes have valid routing paths to neighboring cells
    std::map<int32_t, std::set<int32_t>> cellNeighbors;
    std::map<int32_t, std::vector<uint32_t>> nodesByCell;
    
    // Build data structures
    for (const auto& [nodeId, state] : g_groundNetworkPerNode)
    {
        nodesByCell[state.cellId].push_back(nodeId);
        
        for (uint32_t neighborId : state.neighbors)
        {
            auto neighborIt = g_groundNetworkPerNode.find(neighborId);
            if (neighborIt == g_groundNetworkPerNode.end())
                continue;
            
            const int32_t cellA = state.cellId;
            const int32_t cellB = neighborIt->second.cellId;
            if (cellA != cellB)
            {
                cellNeighbors[cellA].insert(cellB);
            }
        }
    }
    
    uint32_t validationErrors = 0;
    uint32_t validatedNodes = 0;
    
    // For each cell, verify all members have valid routes
    for (const auto& [cellId, members] : nodesByCell)
    {
        if (members.empty())
            continue;
        
        const auto& neighborCells = cellNeighbors[cellId];
        
        // For each node in the cell
        for (uint32_t nodeId : members)
        {
            // Check if node has valid route in main tree
            auto routeIt = ::ns3::wsn::scenario4::params::g_intraCellRoutingTree.find(nodeId);
            if (routeIt == ::ns3::wsn::scenario4::params::g_intraCellRoutingTree.end())
            {
                NS_LOG_WARN("[BS-VALIDATE] Node " << nodeId << " not in routing tree!");
                validationErrors++;
                continue;
            }
            
            const auto& nodeRoutes = routeIt->second;
            
            // Verify node can reach its own cell
            if (nodeRoutes.find(cellId) == nodeRoutes.end())
            {
                NS_LOG_WARN("[BS-VALIDATE] Node " << nodeId << " cannot route within own cell " << cellId);
                validationErrors++;
            }
            else
            {
                // Verify next-hop is valid
                uint32_t nextHop = nodeRoutes.at(cellId);
                if (g_groundNetworkPerNode.find(nextHop) == g_groundNetworkPerNode.end())
                {
                    NS_LOG_WARN("[BS-VALIDATE] Node " << nodeId << " has invalid next-hop " << nextHop);
                    validationErrors++;
                }
                else if (g_groundNetworkPerNode[nextHop].cellId != cellId)
                {
                    NS_LOG_WARN("[BS-VALIDATE] Node " << nodeId << " next-hop " << nextHop 
                               << " not in same cell");
                    validationErrors++;
                }
            }
            
            // For each neighboring cell, verify node can reach a gateway
            for (int32_t neighborCellId : neighborCells)
            {
                // Check if gateway exists
                if (::ns3::wsn::scenario4::params::g_cellGatewayPairs.find(cellId) ==
                    ::ns3::wsn::scenario4::params::g_cellGatewayPairs.end() ||
                    ::ns3::wsn::scenario4::params::g_cellGatewayPairs[cellId].find(neighborCellId) ==
                    ::ns3::wsn::scenario4::params::g_cellGatewayPairs[cellId].end() ||
                    ::ns3::wsn::scenario4::params::g_cellGatewayPairs[cellId][neighborCellId].empty())
                {
                    NS_LOG_WARN("[BS-VALIDATE] Cell " << cellId << " missing gateway to cell " 
                               << neighborCellId);
                    validationErrors++;
                    continue;
                }
                
                // Verify node has path to at least one gateway node
                bool hasGatewayPath = false;
                const auto& gateways = ::ns3::wsn::scenario4::params::g_cellGatewayPairs[cellId][neighborCellId];
                
                // Trace path from node to any gateway
                std::set<uint32_t> visited;
                uint32_t current = nodeId;
                uint32_t hops = 0;
                const uint32_t maxHops = members.size();  // Safety limit
                
                while (hops < maxHops)
                {
                    // Check if current is a gateway
                    if (std::find(gateways.begin(), gateways.end(), current) != gateways.end())
                    {
                        hasGatewayPath = true;
                        break;
                    }
                    
                    if (visited.count(current) > 0)
                        break;  // Cycle detected
                    
                    visited.insert(current);
                    
                    // Get next hop towards gateway for this destination cell.
                    auto routeIt = ::ns3::wsn::scenario4::params::g_intraCellRoutingTree.find(current);
                    if (routeIt == ::ns3::wsn::scenario4::params::g_intraCellRoutingTree.end())
                    {
                        break;
                    }

                    uint32_t nextHop = current;
                    if (routeIt->second.find(neighborCellId) != routeIt->second.end())
                    {
                        nextHop = routeIt->second.at(neighborCellId);
                    }
                    else if (routeIt->second.find(cellId) != routeIt->second.end())
                    {
                        nextHop = routeIt->second.at(cellId);
                    }
                    if (nextHop == current)
                        break;  // Stuck at leaf or root
                    
                    current = nextHop;
                    hops++;
                }
                
                if (!hasGatewayPath)
                {
                    NS_LOG_WARN("[BS-VALIDATE] Node " << nodeId << " cannot reach gateway to cell " 
                               << neighborCellId);
                    validationErrors++;
                }
                else
                {
                    NS_LOG_DEBUG("[BS-VALIDATE] Node " << nodeId << " can reach gateway to cell " 
                                << neighborCellId << " in " << hops << " hops");
                }
            }
            
            validatedNodes++;
        }
    }
    
    if (validationErrors == 0)
    {
        NS_LOG_INFO("[BS-VALIDATE] ✓ All " << validatedNodes << " nodes have valid routing paths");
    }
    else
    {
        NS_LOG_WARN("[BS-VALIDATE] ✗ Found " << validationErrors << " validation errors in " 
                   << validatedNodes << " checked nodes");
    }
}

void
BuildIntraCellRoutingTrees()
{
    // Group nodes by cell ID
    std::map<int32_t, std::vector<uint32_t>> nodesByCell;
    for (const auto& [nodeId, state] : g_groundNetworkPerNode)
    {
        nodesByCell[state.cellId].push_back(nodeId);
    }
    
    // Clear routing tree
    ::ns3::wsn::scenario4::params::g_intraCellRoutingTree.clear();
    
    uint32_t totalTreeNodes = 0;
    
    for (const auto& [cellId, members] : nodesByCell)
    {
        if (members.empty())
            continue;
        
        // Find cell leader (should already be marked during leader selection)
        uint32_t cellLeaderId = members.front();
        for (uint32_t memberId : members)
        {
            if (g_groundNetworkPerNode[memberId].isCellLeader)
            {
                cellLeaderId = memberId;
                break;
            }
        }
        
        std::set<uint32_t> memberSet(members.begin(), members.end());
        
        // ===== BUILD MAIN INTRA-CELL TREE (rooted at cell leader) =====
        std::queue<uint32_t> q;
        std::set<uint32_t> visited;
        std::map<uint32_t, uint32_t> parentInTree;  // parentInTree[node] = parent
        
        q.push(cellLeaderId);
        visited.insert(cellLeaderId);
        parentInTree[cellLeaderId] = cellLeaderId;  // Root points to itself
        
        // BFS from cell leader through neighbors (only within same cell)
        while (!q.empty())
        {
            const uint32_t current = q.front();
            q.pop();
            
            const auto& currentState = g_groundNetworkPerNode[current];
            
            // Traverse neighbors of current node
            for (uint32_t neighborId : currentState.neighbors)
            {
                // Only add neighbors in the same cell
                if (memberSet.find(neighborId) == memberSet.end() || visited.count(neighborId) > 0)
                    continue;
                
                visited.insert(neighborId);
                parentInTree[neighborId] = current;  // Parent is current
                q.push(neighborId);
            }
        }
        
        // Store main tree: routing[nodeId][cellId] = parentId
        for (const auto& [nodeId, parentId] : parentInTree)
        {
            ::ns3::wsn::scenario4::params::g_intraCellRoutingTree[nodeId][cellId] = parentId;
            totalTreeNodes++;
            
            NS_LOG_DEBUG("[BS-INIT] IntraCell tree: cell=" << cellId 
                        << " node=" << nodeId << " parent=" << parentId);
        }
        
        // ===== BUILD PER-GATEWAY ROUTING TREES =====
        // For cross-cell communication, build separate trees rooted at gateway nodes
        if (::ns3::wsn::scenario4::params::g_cellGatewayPairs.find(cellId) !=
            ::ns3::wsn::scenario4::params::g_cellGatewayPairs.end())
        {
            const auto& neighborGateways = ::ns3::wsn::scenario4::params::g_cellGatewayPairs[cellId];
            
            for (const auto& [neighborCellId, gatewayList] : neighborGateways)
            {
                if (gatewayList.empty())
                    continue;
                
                // Use first gateway as root for this neighbor cell's tree
                const uint32_t gatewayId = gatewayList.front();
                
                if (memberSet.find(gatewayId) == memberSet.end())
                    continue;
                
                // Build BFS tree rooted at gateway for this neighbor cell direction
                std::queue<uint32_t> gq;
                std::set<uint32_t> gvisited;
                std::map<uint32_t, uint32_t> gatewayTree;
                
                gq.push(gatewayId);
                gvisited.insert(gatewayId);
                gatewayTree[gatewayId] = gatewayId;  // Root points to itself
                
                while (!gq.empty())
                {
                    const uint32_t current = gq.front();
                    gq.pop();
                    
                    const auto& currentState = g_groundNetworkPerNode[current];
                    
                    for (uint32_t neighborId : currentState.neighbors)
                    {
                        if (memberSet.find(neighborId) == memberSet.end() || gvisited.count(neighborId) > 0)
                            continue;
                        
                        gvisited.insert(neighborId);
                        gatewayTree[neighborId] = current;
                        gq.push(neighborId);
                    }
                }
                
                // Store gateway-specific routes using targetCellId as key
                // This allows different routing paths based on destination cell
                for (const auto& [nodeId, parentId] : gatewayTree)
                {
                    ::ns3::wsn::scenario4::params::g_intraCellRoutingTree[nodeId][neighborCellId] = parentId;
                    NS_LOG_DEBUG("[BS-INIT] Gateway tree: cell=" << cellId 
                                << " gateway=" << gatewayId << " to_cell=" << neighborCellId
                                << " node=" << nodeId << " parent=" << parentId);
                }

                // Ensure every member has destination-specific next-hop entry.
                for (uint32_t memberId : members)
                {
                    auto& routeMap = ::ns3::wsn::scenario4::params::g_intraCellRoutingTree[memberId];
                    if (routeMap.find(neighborCellId) == routeMap.end())
                    {
                        if (routeMap.find(cellId) != routeMap.end())
                        {
                            routeMap[neighborCellId] = routeMap[cellId];
                        }
                        else
                        {
                            routeMap[neighborCellId] = memberId;
                        }
                    }
                }
            }
        }
    }
    
    NS_LOG_INFO("[BS-INIT] Intra-cell routing trees built: " << totalTreeNodes << " tree nodes total");
    // TODO: in log vào `g_resultFileStream` tại đây
    // Format: [INTRA-CELL-TREE] nodeId [cellId] parentId1 (parentCellId1) parentId2 (parentCellId2) ...
    if (ns3::wsn::scenario4::params::g_resultFileStream)
    {
        for (const auto& [nodeId, cellRoutes] : ::ns3::wsn::scenario4::params::g_intraCellRoutingTree)
        {            *ns3::wsn::scenario4::params::g_resultFileStream
                << "[INTRA-CELL-TREE] [" << nodeId << "]";
            for (const auto& [destCellId, parentId] : cellRoutes)
            {                *ns3::wsn::scenario4::params::g_resultFileStream
                    << " " << parentId << " (" << destCellId << ")";
            }
            *ns3::wsn::scenario4::params::g_resultFileStream << std::endl;
        }   
    }
}

void
EnhanceRoutingTreesForGatewayAccess()
{
    // Ensure all nodes can reach gateways to neighbor cells via direct routing entries
    std::map<int32_t, std::set<int32_t>> cellNeighbors;
    std::map<int32_t, std::vector<uint32_t>> nodesByCell;
    
    // Build data structures
    for (const auto& [nodeId, state] : g_groundNetworkPerNode)
    {
        nodesByCell[state.cellId].push_back(nodeId);
        
        for (uint32_t neighborId : state.neighbors)
        {
            auto neighborIt = g_groundNetworkPerNode.find(neighborId);
            if (neighborIt == g_groundNetworkPerNode.end())
                continue;
            
            const int32_t cellA = state.cellId;
            const int32_t cellB = neighborIt->second.cellId;
            if (cellA != cellB)
                cellNeighbors[cellA].insert(cellB);
        }
    }
    
    uint32_t routesAdded = 0;
    
    // For each cell, ensure all nodes can reach at least one gateway
    for (const auto& [cellId, members] : nodesByCell)
    {
        if (members.empty())
            continue;
        
        // For each neighbor cell
        for (int32_t neighborCellId : cellNeighbors[cellId])
        {
            // Get gateways for this neighbor
            if (::ns3::wsn::scenario4::params::g_cellGatewayPairs.find(cellId) ==
                ::ns3::wsn::scenario4::params::g_cellGatewayPairs.end() ||
                ::ns3::wsn::scenario4::params::g_cellGatewayPairs[cellId].find(neighborCellId) ==
                ::ns3::wsn::scenario4::params::g_cellGatewayPairs[cellId].end())
                continue;
            
            const auto& gateways = ::ns3::wsn::scenario4::params::g_cellGatewayPairs[cellId][neighborCellId];
            if (gateways.empty())
                continue;
            
            // For each node, add direct route to primary gateway for cross-cell traffic
            for (uint32_t nodeId : members)
            {
                // Find shortest path from node to gateway
                std::queue<uint32_t> q;
                std::map<uint32_t, uint32_t> parent;
                std::set<uint32_t> visited;
                
                q.push(nodeId);
                visited.insert(nodeId);
                parent[nodeId] = nodeId;
                
                uint32_t closestGateway = nodeId;
                
                while (!q.empty())
                {
                    uint32_t current = q.front();
                    q.pop();
                    
                    // Check if current is a gateway
                    if (std::find(gateways.begin(), gateways.end(), current) != gateways.end())
                    {
                        closestGateway = current;
                        break;
                    }
                    
                    const auto& currentState = g_groundNetworkPerNode[current];
                    for (uint32_t nb : currentState.neighbors)
                    {
                        if (g_groundNetworkPerNode[nb].cellId != cellId || visited.count(nb) > 0)
                            continue;
                        
                        visited.insert(nb);
                        parent[nb] = current;
                        q.push(nb);
                    }
                }
                
                // If found gateway, trace back to get first hop
                if (closestGateway != nodeId)
                {
                    uint32_t current = closestGateway;
                    while (parent[current] != nodeId && parent[current] != current)
                    {
                        current = parent[current];
                    }
                    uint32_t firstHop = current;
                    
                    // Store route from nodeId to neighborCellId via this gateway
                    ::ns3::wsn::scenario4::params::g_intraCellRoutingTree[nodeId][neighborCellId] = firstHop;
                    routesAdded++;
                    
                    NS_LOG_DEBUG("[BS-ENHANCE] Route: node=" << nodeId << " to_cell=" 
                                << neighborCellId << " via=" << firstHop);
                }
            }
        }
    }
    
    NS_LOG_INFO("[BS-INIT] Enhanced routing: " << routesAdded << " routes for gateway access");
}

void
FinalizeGroundNodeStateFields()
{
    const double nowSec = Simulator::Now().GetSeconds();
    std::map<int32_t, std::vector<uint32_t>> membersByCell;

    for (const auto& [nodeId, state] : g_groundNetworkPerNode)
    {
        (void)nodeId;
        membersByCell[state.cellId].push_back(state.nodeId);
    }

    for (auto& [nodeId, state] : g_groundNetworkPerNode)
    {
        // Cell peers
        state.cellPeers.clear();
        const auto itMembers = membersByCell.find(state.cellId);
        if (itMembers != membersByCell.end())
        {
            for (uint32_t peerId : itMembers->second)
            {
                if (peerId != nodeId)
                {
                    state.cellPeers.insert(peerId);
                }
            }
        }

        // Time synchronization and startup completion
        state.isTimeSynchronized = true;
        state.clockOffsetSec = 0.0;
        state.lastSyncTime = nowSec;
        state.startupComplete = true;

        // Fragment expectations (if not set yet)
        if (state.expectedFragmentCount == 0)
        {
            state.expectedFragmentCount = params::DEFAULT_NUM_FRAGMENTS;
        }

        // Lifecycle + activity timestamps
        state.lifecyclePhase = state.isIsolated ? GroundNodeLifecyclePhase::DEGRADED
                                                : GroundNodeLifecyclePhase::ACTIVE;
        if (state.initializationTime <= 0.0)
        {
            state.initializationTime = nowSec;
        }
        state.lastActivityTime = nowSec;
    }

    NS_LOG_INFO("[BS-INIT] Finalized remaining state fields for "
                << g_groundNetworkPerNode.size() << " nodes");
}

void
SelectSuspiciousRegionForBsInit()
{
    const double nowSec = Simulator::Now().GetSeconds();
    
    if (g_groundNetworkPerNode.empty())
    {
        NS_LOG_WARN("[BS-SUSPICIOUS] No ground nodes available for suspicious region selection");
        return;
    }

    // Step 1: Select random suspicious point (random node as seed)
    std::vector<uint32_t> allNodeIds;
    for (const auto& [nodeId, state] : g_groundNetworkPerNode)
    {
        (void)state;
        allNodeIds.push_back(nodeId);
    }
    
    uint32_t randomIndex = rand() % allNodeIds.size();
    uint32_t seedNodeId = allNodeIds[randomIndex];
    const auto& seedNode = g_groundNetworkPerNode.at(seedNodeId);
    int32_t seedCellId = seedNode.cellId;

    NS_LOG_INFO("[BS-SUSPICIOUS] Seed node " << seedNodeId 
                << " selected at cell " << seedCellId 
                << " | Position: (" << seedNode.position.x << ", " << seedNode.position.y << ")");

    // Step 2: Expand suspicious region around seed cell
    std::set<int32_t> suspiciousCells;   // Set of cell IDs in suspicious region
    std::set<uint32_t> suspiciousNodes;  // Set of node IDs in suspicious region
    
    // Initialize with seed cell
    suspiciousCells.insert(seedCellId);
    
    // Target coverage from centralized params
    const uint32_t totalNodes = g_groundNetworkPerNode.size();
    const double targetPercent = ::ns3::wsn::scenario4::params::BS_INIT_SUSPICIOUS_TARGET_PERCENT;
    const uint32_t targetNodeCount = std::max(
        ::ns3::wsn::scenario4::params::BS_INIT_SUSPICIOUS_MIN_TARGET_NODES,
        static_cast<uint32_t>(totalNodes * targetPercent));
    
    NS_LOG_INFO("[BS-SUSPICIOUS] Starting expansion from cell " << seedCellId
                << " | Target nodes: " << targetNodeCount 
                << " (" << std::fixed << std::setprecision(1) << (targetPercent * 100.0)
                << "% of " << totalNodes << ")");
    
    // Iteratively expand suspicious region
    uint32_t iteration = 0;
    const uint32_t MAX_ITERATIONS = ::ns3::wsn::scenario4::params::BS_INIT_SUSPICIOUS_MAX_ITERATIONS;
    
    while (suspiciousNodes.size() < targetNodeCount && iteration < MAX_ITERATIONS)
    {
        iteration++;
        
        // (1) Find all nodes in current suspicious region
        suspiciousNodes.clear();
        for (const auto& [nodeId, state] : g_groundNetworkPerNode)
        {
            if (suspiciousCells.find(state.cellId) != suspiciousCells.end())
            {
                suspiciousNodes.insert(nodeId);
            }
        }
        
        // Check if we've reached target
        if (suspiciousNodes.size() >= targetNodeCount)
        {
            break;
        }
        
        // (2) Find candidate neighbor cells outside suspicious region
        std::vector<int32_t> candidateNeighborCells;
        for (uint32_t nodeId : suspiciousNodes)
        {
            const auto& nodeState = g_groundNetworkPerNode.at(nodeId);
            
            // Check all neighbors of this node
            for (uint32_t neighborId : nodeState.neighbors)
            {
                auto neighborIt = g_groundNetworkPerNode.find(neighborId);
                if (neighborIt == g_groundNetworkPerNode.end())
                {
                    continue;
                }
                
                int32_t neighborCellId = neighborIt->second.cellId;
                
                // If neighbor is in a different cell not yet in suspicious region
                if (suspiciousCells.find(neighborCellId) == suspiciousCells.end())
                {
                    candidateNeighborCells.push_back(neighborCellId);
                }
            }
        }
        
        // If no candidates found, stop expansion
        if (candidateNeighborCells.empty())
        {
            NS_LOG_INFO("[BS-SUSPICIOUS] No more neighbor cells to expand at iteration " 
                        << iteration);
            break;
        }
        
        // Randomly select one neighbor cell and add to suspicious region
        uint32_t randomCellIndex = rand() % candidateNeighborCells.size();
        int32_t selectedCellId = candidateNeighborCells[randomCellIndex];
        suspiciousCells.insert(selectedCellId);
        
        NS_LOG_DEBUG("[BS-SUSPICIOUS] Iteration " << iteration 
                     << " | Added cell " << selectedCellId
                     << " | Region cells: " << suspiciousCells.size()
                     << " | Region nodes: " << suspiciousNodes.size());
    }
    
    // Final update of suspicious nodes
    suspiciousNodes.clear();
    for (const auto& [nodeId, state] : g_groundNetworkPerNode)
    {
        if (suspiciousCells.find(state.cellId) != suspiciousCells.end())
        {
            suspiciousNodes.insert(nodeId);
        }
    }
    
    // Calculate coverage percentage
    double coveragePercent = (totalNodes > 0) ? (100.0 * suspiciousNodes.size() / totalNodes) : 0.0;
    bool targetReached = (suspiciousNodes.size() >= targetNodeCount);
    
    NS_LOG_WARN("[BS-SUSPICIOUS] Region selection complete at t=" << nowSec << "s"
                << " | Cells: " << suspiciousCells.size()
                << " | Nodes: " << suspiciousNodes.size() << "/" << totalNodes
                << " (" << std::fixed << std::setprecision(1) << coveragePercent << "%)"
                << " | Iterations: " << iteration
                << " | Target reached: " << (targetReached ? "YES" : "NO"));
    
    // Log detailed cell distribution
    NS_LOG_INFO("[BS-SUSPICIOUS] Cell distribution:");
    for (int32_t cellId : suspiciousCells)
    {
        uint32_t cellNodeCount = 0;
        for (const auto& [nodeId, state] : g_groundNetworkPerNode)
        {
            if (state.cellId == cellId && suspiciousNodes.find(nodeId) != suspiciousNodes.end())
            {
                cellNodeCount++;
            }
        }
        NS_LOG_INFO("  Cell " << cellId << ": " << cellNodeCount << " nodes");
    }
    
    // Store suspicious nodes globally for UAV flight planning
    g_suspiciousNodes = suspiciousNodes;
    // TODO: in log vào `g_resultFileStream` tại đây
    // Format: [SUSPICIOUS-POINT] pointX pointY 
    // Format: [SUSPICIOUS-REGION] nodeId1 nodeId2 ... (Cell: cell1 cell2 ...)
    if (ns3::wsn::scenario4::params::g_resultFileStream)
    {
        const auto& seedPos = seedNode.position;
        *ns3::wsn::scenario4::params::g_resultFileStream
            << "[SUSPICIOUS-POINT] " << seedPos.x << " " << seedPos.y << std::endl;
        *ns3::wsn::scenario4::params::g_resultFileStream
            << "[SUSPICIOUS-REGION] ";
        for (uint32_t nodeId : suspiciousNodes)
        {            *ns3::wsn::scenario4::params::g_resultFileStream << " " << nodeId;
        }   
        *ns3::wsn::scenario4::params::g_resultFileStream
            << " (Cells: ";
        for (int32_t cellId : suspiciousCells)
        {            *ns3::wsn::scenario4::params::g_resultFileStream << " " << cellId;
        }
        *ns3::wsn::scenario4::params::g_resultFileStream << ")" << std::endl;
    }
    NS_LOG_INFO("[BS-SUSPICIOUS] Suspicious region stored for UAV flight planning");
}

void
GenerateFragmentsForBsInit()
{
    const uint32_t fragmentCount = ::ns3::wsn::scenario4::params::BS_INIT_FRAGMENT_GENERATION_COUNT;
    FragmentCollection generated = GenerateBsFragments(fragmentCount);
    SetBsGeneratedFragments(generated);

    NS_LOG_INFO("[BS-FRAGMENT] Generated " << generated.fragments.size()
                << " fragments at BS init"
                << " | avgConfidence=" << std::fixed << std::setprecision(3)
                << generated.totalConfidence);

    // TODO: in log vào `g_resultFileStream` tại đây
    // Format: [FRAGMENTS] fragmentSize1(confidence1) fragmentSize2(confidence2) ...
    if (ns3::wsn::scenario4::params::g_resultFileStream)
    {        *ns3::wsn::scenario4::params::g_resultFileStream
            << "[FRAGMENTS]";
        for (int i = 0; i < generated.fragments.size(); ++i)
        {
            const auto& frag = generated.fragments[i];
            *ns3::wsn::scenario4::params::g_resultFileStream
                << " " << frag.size << "(" << std::fixed << std::setprecision(3) << frag.confidence << ")";
        }
        *ns3::wsn::scenario4::params::g_resultFileStream << std::endl;
    }    
}

void
PlanUavFlightPathsForBsInit()
{
    if (g_suspiciousNodes.empty())
    {
        NS_LOG_WARN("[BS-UAV-PATH] No suspicious nodes to plan paths for");
        return;
    }

    // Clear previous paths
    ClearUavFlightPaths();

    // Get suspicious node positions with node IDs
    std::vector<std::pair<uint32_t, Vector>> suspiciousNodePositions;
    for (uint32_t nodeId : g_suspiciousNodes)
    {
        auto it = g_groundNetworkPerNode.find(nodeId);
        if (it != g_groundNetworkPerNode.end())
        {
            suspiciousNodePositions.push_back({nodeId, it->second.position});
        }
    }

    if (suspiciousNodePositions.empty())
    {
        NS_LOG_WARN("[BS-UAV-PATH] No valid positions for suspicious nodes");
        return;
    }

    // Get UAV nodes (assumes UAVs are nodes with specific IDs or mobility model)
    std::vector<uint32_t> uavNodeIds;
    for (uint32_t i = 0; i < NodeList::GetNNodes(); ++i)
    {
        Ptr<Node> node = NodeList::GetNode(i);
        if (!node) continue;
        
        Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
        if (!mobility) continue;
        
        const Vector pos = mobility->GetPosition();
        // UAVs are at altitude > 0
        if (pos.z > 1.0)
        {
            uavNodeIds.push_back(i);
        }
    }

    if (uavNodeIds.empty())
    {
        NS_LOG_WARN("[BS-UAV-PATH] No UAV nodes found");
        return;
    }

    const double altitude = ::ns3::wsn::scenario4::params::BS_INIT_UAV_PATROL_ALTITUDE;
    const double speed = ::ns3::wsn::scenario4::params::UAV_PATH_SPEED;
    const double hoverTime = ::ns3::wsn::scenario4::params::UAV_WAYPOINT_HOVER_TIME;
    const double broadcastRadius = ::ns3::wsn::scenario4::params::UAV_BROADCAST_RADIUS;

    // ===== UAV 1: Greedy Nearest Neighbor (visit every node) =====
    const uint32_t uav1NodeId = uavNodeIds[0];
    Ptr<Node> uav1Node = NodeList::GetNode(uav1NodeId);
    Ptr<MobilityModel> uav1Mobility = uav1Node->GetObject<MobilityModel>();
    const Vector uav1StartPos = uav1Mobility->GetPosition();

    UavFlightPath path1;
    std::set<uint32_t> visitedNodes;
    Vector currentPos = uav1StartPos;
    double currentTime = 0.0;
    
    NS_LOG_INFO("[BS-UAV-PATH] UAV1: Planning path using Greedy Nearest Neighbor"
                << " | suspiciousNodes=" << suspiciousNodePositions.size()
                << " | startPos=(" << std::fixed << std::setprecision(1) 
                << uav1StartPos.x << "," << uav1StartPos.y << "," << uav1StartPos.z << ")");
    
    // Visit all suspicious nodes using nearest neighbor heuristic
    while (visitedNodes.size() < suspiciousNodePositions.size())
    {
        // Find nearest unvisited node
        double minDistance = std::numeric_limits<double>::max();
        uint32_t nearestNodeId = std::numeric_limits<uint32_t>::max();
        Vector nearestPos;
        
        for (const auto& [nodeId, pos] : suspiciousNodePositions)
        {
            if (visitedNodes.count(nodeId) > 0)
                continue;
            
            double dist = helper::CalculateDistance(currentPos.x, currentPos.y, pos.x, pos.y);
            if (dist < minDistance)
            {
                minDistance = dist;
                nearestNodeId = nodeId;
                nearestPos = pos;
            }
        }
        
        if (nearestNodeId == std::numeric_limits<uint32_t>::max())
            break;
        
        currentTime += minDistance / speed;
        
        Waypoint wp;
        wp.position = Vector(nearestPos.x, nearestPos.y, altitude);
        wp.arrivalTime = currentTime;
        path1.waypoints.push_back(wp);
        
        currentTime += hoverTime;
        visitedNodes.insert(nearestNodeId);
        currentPos = nearestPos;
        
        NS_LOG_DEBUG("[BS-UAV-PATH] UAV1 Waypoint " << path1.waypoints.size() 
                     << " | node=" << nearestNodeId
                     << " | pos=(" << nearestPos.x << "," << nearestPos.y << ")"
                     << " | dist=" << minDistance << "m");
    }
    
    path1.totalTime = currentTime;
    SetUavFlightPath(uav1NodeId, path1);
    
    // Calculate total path distance for UAV1
    double totalDistance1 = 0.0;
    Vector prevPos1 = uav1StartPos;
    for (const auto& wp : path1.waypoints)
    {
        totalDistance1 += helper::CalculateDistance(prevPos1.x, prevPos1.y, wp.position.x, wp.position.y);
        prevPos1 = wp.position;
    }

    NS_LOG_INFO("[BS-UAV-PATH] UAV1 path planned"
                << " | waypoints=" << path1.waypoints.size()
                << " | totalTime=" << std::fixed << std::setprecision(1) << path1.totalTime << "s"
                << " | totalDistance=" << totalDistance1 << "m"
                << " | avgSpeed=" << (totalDistance1 / (path1.totalTime - path1.waypoints.size() * hoverTime)) << "m/s");

    // ===== UAV 2: Greedy Set Cover (minimize waypoints with coverage radius) =====
    if (uavNodeIds.size() >= 2)
    {
        const uint32_t uav2NodeId = uavNodeIds[1];
        Ptr<Node> uav2Node = NodeList::GetNode(uav2NodeId);
        Ptr<MobilityModel> uav2Mobility = uav2Node->GetObject<MobilityModel>();
        const Vector uav2StartPos = uav2Mobility->GetPosition();

        UavFlightPath path2;
        std::set<uint32_t> coveredNodes;  // Nodes already covered by waypoints
        Vector currentPos2 = uav2StartPos;
        double currentTime2 = 0.0;
        
        NS_LOG_INFO("[BS-UAV-PATH] UAV2: Planning coverage-based path"
                    << " | broadcastRadius=" << broadcastRadius << "m"
                    << " | startPos=(" << std::fixed << std::setprecision(1)
                    << uav2StartPos.x << "," << uav2StartPos.y << "," << uav2StartPos.z << ")");
        
        // Greedy Set Cover: repeatedly choose waypoint that covers most uncovered nodes
        while (coveredNodes.size() < suspiciousNodePositions.size())
        {
            Vector bestWaypointPos;
            uint32_t maxNewCoverage = 0;
            double bestDistance = std::numeric_limits<double>::max();
            
            // Try each suspicious node position as potential waypoint
            for (const auto& [candidateNodeId, candidatePos] : suspiciousNodePositions)
            {
                // Count how many uncovered nodes this waypoint would cover
                uint32_t newCoverageCount = 0;
                for (const auto& [nodeId, nodePos] : suspiciousNodePositions)
                {
                    if (coveredNodes.count(nodeId) > 0)
                        continue;  // Already covered
                    
                    double dist = helper::CalculateDistance(
                        candidatePos.x, candidatePos.y, nodePos.x, nodePos.y);
                    
                    if (dist <= broadcastRadius)
                    {
                        newCoverageCount++;
                    }
                }
                
                // Select waypoint with best coverage, breaking ties by distance from current position
                double distFromCurrent = helper::CalculateDistance(
                    currentPos2.x, currentPos2.y, candidatePos.x, candidatePos.y);
                
                if (newCoverageCount > maxNewCoverage ||
                    (newCoverageCount == maxNewCoverage && distFromCurrent < bestDistance))
                {
                    maxNewCoverage = newCoverageCount;
                    bestWaypointPos = candidatePos;
                    bestDistance = distFromCurrent;
                }
            }
            
            if (maxNewCoverage == 0)
            {
                NS_LOG_WARN("[BS-UAV-PATH] UAV2: No more nodes can be covered, but "
                           << (suspiciousNodePositions.size() - coveredNodes.size())
                           << " nodes remain uncovered");
                break;
            }
            
            // Add this waypoint
            currentTime2 += bestDistance / speed;
            
            Waypoint wp2;
            wp2.position = Vector(bestWaypointPos.x, bestWaypointPos.y, altitude);
            wp2.arrivalTime = currentTime2;
            path2.waypoints.push_back(wp2);
            
            currentTime2 += hoverTime;
            
            // Mark all nodes covered by this waypoint
            uint32_t actualCovered = 0;
            for (const auto& [nodeId, nodePos] : suspiciousNodePositions)
            {
                if (coveredNodes.count(nodeId) > 0)
                    continue;
                
                double dist = helper::CalculateDistance(
                    bestWaypointPos.x, bestWaypointPos.y, nodePos.x, nodePos.y);
                
                if (dist <= broadcastRadius)
                {
                    coveredNodes.insert(nodeId);
                    actualCovered++;
                }
            }
            
            currentPos2 = bestWaypointPos;
            
            NS_LOG_DEBUG("[BS-UAV-PATH] UAV2 Waypoint " << path2.waypoints.size()
                        << " | pos=(" << bestWaypointPos.x << "," << bestWaypointPos.y << ")"
                        << " | covered=" << actualCovered << " new nodes"
                        << " | totalCovered=" << coveredNodes.size() << "/" << suspiciousNodePositions.size()
                        << " | dist=" << bestDistance << "m");
        }
        
        path2.totalTime = currentTime2;
        SetUavFlightPath(uav2NodeId, path2);
        
        // Calculate total path distance for UAV2
        double totalDistance2 = 0.0;
        Vector prevPos2 = uav2StartPos;
        for (const auto& wp : path2.waypoints)
        {
            totalDistance2 += helper::CalculateDistance(prevPos2.x, prevPos2.y, wp.position.x, wp.position.y);
            prevPos2 = wp.position;
        }
        
        NS_LOG_INFO("[BS-UAV-PATH] UAV2 coverage-based path planned"
                    << " | waypoints=" << path2.waypoints.size()
                    << " | covered=" << coveredNodes.size() << "/" << suspiciousNodePositions.size()
                    << " | totalTime=" << std::fixed << std::setprecision(1) << path2.totalTime << "s"
                    << " | totalDistance=" << totalDistance2 << "m"
                    << " | avgSpeed=" << (totalDistance2 / (path2.totalTime - path2.waypoints.size() * hoverTime)) << "m/s");
        
        // Log UAV2 to file
        if (ns3::wsn::scenario4::params::g_resultFileStream)
        {
            *ns3::wsn::scenario4::params::g_resultFileStream
                << "[UAV-PATH] " << uav2NodeId
                << " strategy=GreedySetCover"
                << " totalDistance=" << std::fixed << std::setprecision(1) << totalDistance2 << "m"
                << " totalTime=" << path2.totalTime << "s"
                << " avgSpeed=" << (totalDistance2 / (path2.totalTime - path2.waypoints.size() * hoverTime)) << "m/s"
                << " broadcastRadius=" << broadcastRadius << "m"
                << " coverage=" << coveredNodes.size() << "/" << suspiciousNodePositions.size()
                << " waypoints:";
            for (const auto& wp : path2.waypoints)
            {
                *ns3::wsn::scenario4::params::g_resultFileStream
                    << " (" << std::fixed << std::setprecision(1)
                    << wp.position.x << "," << wp.position.y << ")";
            }
            *ns3::wsn::scenario4::params::g_resultFileStream << std::endl;
        }
    }

    // Log UAV1 to file
    if (ns3::wsn::scenario4::params::g_resultFileStream)
    {
        *ns3::wsn::scenario4::params::g_resultFileStream
            << "[UAV-PATH] " << uav1NodeId
            << " strategy=GreedyNearestNeighbor"
            << " totalDistance=" << std::fixed << std::setprecision(1) << totalDistance1 << "m"
            << " totalTime=" << path1.totalTime << "s"
            << " avgSpeed=" << (totalDistance1 / (path1.totalTime - path1.waypoints.size() * hoverTime)) << "m/s"
            << " waypoints:";
        for (const auto& wp : path1.waypoints)
        {
            *ns3::wsn::scenario4::params::g_resultFileStream
                << " (" << std::fixed << std::setprecision(1)
                << wp.position.x << "," << wp.position.y << ")";
        }
        *ns3::wsn::scenario4::params::g_resultFileStream << std::endl;
    }
}

void
SelectCrosscellGatewayPairs(double neighborRadius)
{
    // Build cell neighbor adjacency list
    std::map<int32_t, std::set<int32_t>> cellNeighbors;
    
    for (const auto& [nodeId, state] : g_groundNetworkPerNode)
    {
        (void)nodeId;
        for (uint32_t neighborId : state.neighbors)
        {
            auto neighborIt = g_groundNetworkPerNode.find(neighborId);
            if (neighborIt == g_groundNetworkPerNode.end())
            {
                continue;
            }

            const int32_t cellA = state.cellId;
            const int32_t cellB = neighborIt->second.cellId;

            if (cellA != cellB)
            {
                cellNeighbors[cellA].insert(cellB);
                cellNeighbors[cellB].insert(cellA);
            }
        }
    }
    
    // Group nodes by cell ID
    std::map<int32_t, std::vector<uint32_t>> nodesByCell;
    for (const auto& [nodeId, state] : g_groundNetworkPerNode)
    {
        nodesByCell[state.cellId].push_back(nodeId);
    }
    
    // For each cell, find gateway nodes to neighbor cells
    ::ns3::wsn::scenario4::params::g_cellGatewayPairs.clear();
    uint32_t gatewayPairCount = 0;
    
    for (const auto& [cellId, neighbors] : cellNeighbors)
    {
        auto cellMembersIt = nodesByCell.find(cellId);
        if (cellMembersIt == nodesByCell.end() || cellMembersIt->second.empty())
            continue;
        
        const auto& members = cellMembersIt->second;
        
        for (int32_t neighborCellId : neighbors)
        {
            auto neighborMembersIt = nodesByCell.find(neighborCellId);
            if (neighborMembersIt == nodesByCell.end() || neighborMembersIt->second.empty())
                continue;
            
            const auto& neighborMembers = neighborMembersIt->second;
            
            // Find closest pair: node in cellId to node in neighborCellId
            double bestDistance = std::numeric_limits<double>::max();
            uint32_t bestGw1 = members.front();
            uint32_t bestGw2 = neighborMembers.front();
            bool found = false;
            
            for (uint32_t memberId : members)
            {
                const auto& memberState = g_groundNetworkPerNode[memberId];
                
                // Check if this member has a neighbor in neighborCellId
                for (uint32_t neighborId : memberState.neighbors)
                {
                    const auto& neighborState = g_groundNetworkPerNode[neighborId];
                    if (neighborState.cellId != neighborCellId)
                        continue;
                    
                    const double dist = helper::CalculateDistance(
                        memberState.position.x,
                        memberState.position.y,
                        neighborState.position.x,
                        neighborState.position.y);
                    
                    if (dist < bestDistance ||
                        (std::fabs(dist - bestDistance) < 1e-9 && memberId < bestGw1))
                    {
                        bestDistance = dist;
                        bestGw1 = memberId;
                        bestGw2 = neighborId;
                        found = true;
                    }
                }
            }
            
            if (found)
            {
                ::ns3::wsn::scenario4::params::g_cellGatewayPairs[cellId][neighborCellId].push_back(bestGw1);
                ::ns3::wsn::scenario4::params::g_cellGatewayPairs[neighborCellId][cellId].push_back(bestGw2);
                gatewayPairCount++;
                
                NS_LOG_DEBUG("[BS-INIT] Gateway pair: cell=" << cellId << " gw=" << bestGw1
                            << " <-> cell=" << neighborCellId << " gw=" << bestGw2
                            << " dist=" << bestDistance);
            }
        }
    }
    
    NS_LOG_INFO("[BS-INIT] Gateway selection done: " << gatewayPairCount << " gateway pairs");
}

} // namespace

// Global callback definitions
std::function<void(const routing::GlobalTopology&)> g_bsTopologyCallback;
std::function<void(uint32_t, const routing::UavFlightPath&)> g_bsUavCommandCallback;

routing::BaseStationNode::BaseStationNode(uint32_t nodeId)
    : m_nodeId(nodeId),
      m_topologyReceived(false)
{
    NS_LOG_FUNCTION(this << nodeId);
}

routing::BaseStationNode::~BaseStationNode()
{
    NS_LOG_FUNCTION(this);
}

void
routing::BaseStationNode::Initialize()
{
    NS_LOG_FUNCTION(this);
    
    NS_LOG_INFO("BS Node " << m_nodeId << " initialized");

    // Write to global result file if open
    if (::ns3::wsn::scenario4::params::g_resultFileStream && 
        ::ns3::wsn::scenario4::params::g_resultFileStream->is_open())
    {
        *::ns3::wsn::scenario4::params::g_resultFileStream
            << "\n=== Base Station Initialization ===" << std::endl
            << "BS Node ID: " << m_nodeId << std::endl
            << "Start Time: " << Simulator::Now().GetSeconds() << "s" << std::endl
            << std::endl;
        ::ns3::wsn::scenario4::params::g_resultFileStream->flush();
    }

    // Step 1: tính cellId + cellColor cho từng ground node từ position hiện tại
    AssignCellIdAndColorForGroundNodes(::ns3::wsn::scenario4::params::HEX_CELL_RADIUS);
    if (::ns3::wsn::scenario4::params::g_resultFileStream && 
        ::ns3::wsn::scenario4::params::g_resultFileStream->is_open())
    {
        *::ns3::wsn::scenario4::params::g_resultFileStream
            << "Step 1: Assign Cell ID and Color" << std::endl
            << "  Total ground nodes: " << g_groundNetworkPerNode.size() << std::endl
            << "  Cell radius: " << ::ns3::wsn::scenario4::params::HEX_CELL_RADIUS << "m" << std::endl
            << std::endl;
        ::ns3::wsn::scenario4::params::g_resultFileStream->flush();
    }

    // Step 2: neighbor + 2-hop discovery theo bán kính truyền tin
    DiscoverNeighborsAndTwoHopsForGroundNodes(::ns3::wsn::scenario4::params::NEIGHBOR_DISCOVERY_RADIUS);
    if (::ns3::wsn::scenario4::params::g_resultFileStream && 
        ::ns3::wsn::scenario4::params::g_resultFileStream->is_open())
    {
        uint32_t totalNeighbors = 0;
        for (const auto& [nodeId, state] : g_groundNetworkPerNode)
        {
            totalNeighbors += state.neighbors.size();
        }
        *::ns3::wsn::scenario4::params::g_resultFileStream
            << "Step 2: Neighbor Discovery" << std::endl
            << "  Discovery radius: " << ::ns3::wsn::scenario4::params::NEIGHBOR_DISCOVERY_RADIUS << "m" << std::endl
            << "  Total neighbor links: " << totalNeighbors / 2 << std::endl
            << std::endl;
        ::ns3::wsn::scenario4::params::g_resultFileStream->flush();
    }

    // Step 3: chọn cell leader (CL) gần tâm cell nhất
    SelectCellLeadersByNearestCellCenter(::ns3::wsn::scenario4::params::HEX_CELL_RADIUS);
    if (::ns3::wsn::scenario4::params::g_resultFileStream && 
        ::ns3::wsn::scenario4::params::g_resultFileStream->is_open())
    {
        uint32_t leaderCount = 0;
        std::map<int32_t, uint32_t> cellLeaders;
        for (const auto& [nodeId, state] : g_groundNetworkPerNode)
        {
            if (state.isCellLeader)
            {
                leaderCount++;
                cellLeaders[state.cellId] = nodeId;
            }
        }
        *::ns3::wsn::scenario4::params::g_resultFileStream
            << "Step 3: Select Cell Leaders" << std::endl
            << "  Total cells: " << leaderCount << std::endl;
        for (const auto& [cellId, leaderId] : cellLeaders)
        {
            *::ns3::wsn::scenario4::params::g_resultFileStream
                << "    Cell " << cellId << ": Leader " << leaderId << std::endl;
        }
        *::ns3::wsn::scenario4::params::g_resultFileStream << std::endl;
        ::ns3::wsn::scenario4::params::g_resultFileStream->flush();
    }

    // Step 4: chọn gateway pairs cho cross-cell communication
    SelectCrosscellGatewayPairs(::ns3::wsn::scenario4::params::NEIGHBOR_DISCOVERY_RADIUS);
    if (::ns3::wsn::scenario4::params::g_resultFileStream && 
        ::ns3::wsn::scenario4::params::g_resultFileStream->is_open())
    {
        uint32_t gatewayPairCount = 0;
        for (const auto& [cellId, neighbors] : ::ns3::wsn::scenario4::params::g_cellGatewayPairs)
        {
            gatewayPairCount += neighbors.size();
        }
        *::ns3::wsn::scenario4::params::g_resultFileStream
            << "Step 4: Select Gateway Pairs" << std::endl
            << "  Total gateway pairs: " << gatewayPairCount / 2 << std::endl
            << std::endl;
        ::ns3::wsn::scenario4::params::g_resultFileStream->flush();
    }

    // Step 5: xây dựng intra-cell routing trees cho mỗi cell
    BuildIntraCellRoutingTrees();
    if (::ns3::wsn::scenario4::params::g_resultFileStream && 
        ::ns3::wsn::scenario4::params::g_resultFileStream->is_open())
    {
        *::ns3::wsn::scenario4::params::g_resultFileStream
            << "Step 5: Build Intra-Cell Routing Trees" << std::endl
            << "  Total routing entries: " << ::ns3::wsn::scenario4::params::g_intraCellRoutingTree.size() << std::endl
            << std::endl;
        ::ns3::wsn::scenario4::params::g_resultFileStream->flush();
    }

    // Step 6: bổ sung route để đảm bảo reachability tới neighboring cells
    EnhanceRoutingTreesForGatewayAccess();
    if (::ns3::wsn::scenario4::params::g_resultFileStream && 
        ::ns3::wsn::scenario4::params::g_resultFileStream->is_open())
    {
        *::ns3::wsn::scenario4::params::g_resultFileStream
            << "Step 6: Enhance Routing for Gateway Access" << std::endl
            << "  Routing tree enhanced for cross-cell communication" << std::endl
            << std::endl;
        ::ns3::wsn::scenario4::params::g_resultFileStream->flush();
    }

    // Step 7: kiểm tra tính hợp lệ của routing trees
    ValidateIntraCellRoutingTrees();
    if (::ns3::wsn::scenario4::params::g_resultFileStream && 
        ::ns3::wsn::scenario4::params::g_resultFileStream->is_open())
    {
        *::ns3::wsn::scenario4::params::g_resultFileStream
            << "Step 7: Validate Routing Trees" << std::endl
            << "  Routing tree validation completed" << std::endl
            << std::endl;
        ::ns3::wsn::scenario4::params::g_resultFileStream->flush();
    }

    // Step 8: cập nhật các biến trạng thái còn lại
    FinalizeGroundNodeStateFields();
    if (::ns3::wsn::scenario4::params::g_resultFileStream && 
        ::ns3::wsn::scenario4::params::g_resultFileStream->is_open())
    {
        *::ns3::wsn::scenario4::params::g_resultFileStream
            << "Step 8: Finalize Ground Node States" << std::endl
            << "  All node states finalized" << std::endl
            << std::endl;
        ::ns3::wsn::scenario4::params::g_resultFileStream->flush();
    }

    // Step 9: chọn vùng khả nghi cho UAV flight planning
    SelectSuspiciousRegionForBsInit();
    if (::ns3::wsn::scenario4::params::g_resultFileStream && 
        ::ns3::wsn::scenario4::params::g_resultFileStream->is_open())
    {
        *::ns3::wsn::scenario4::params::g_resultFileStream
            << "Step 9: Select Suspicious Region" << std::endl
            << "  Suspicious nodes: " << g_suspiciousNodes.size() << std::endl
            << "  Coverage: " << std::fixed << std::setprecision(1) 
            << (100.0 * g_suspiciousNodes.size() / g_groundNetworkPerNode.size()) << "%" << std::endl
            << "  Node IDs: ";
        for (uint32_t nodeId : g_suspiciousNodes)
        {
            *::ns3::wsn::scenario4::params::g_resultFileStream << nodeId << " ";
        }
        *::ns3::wsn::scenario4::params::g_resultFileStream << std::endl << std::endl;
        ::ns3::wsn::scenario4::params::g_resultFileStream->flush();
    }

    // Step 10: BS generate fragments để UAV broadcast
    GenerateFragmentsForBsInit();
    if (::ns3::wsn::scenario4::params::g_resultFileStream && 
        ::ns3::wsn::scenario4::params::g_resultFileStream->is_open())
    {
        const FragmentCollection& fragments = GetBsGeneratedFragments();
        *::ns3::wsn::scenario4::params::g_resultFileStream
            << "Step 10: Generate Fragments" << std::endl
            << "  Total fragments: " << fragments.fragments.size() << std::endl
            << "  Average confidence: " << std::fixed << std::setprecision(3) 
            << fragments.totalConfidence << std::endl
            << std::endl;
        ::ns3::wsn::scenario4::params::g_resultFileStream->flush();
    }

    // Step 11: lên lịch đường bay cho UAV
    PlanUavFlightPathsForBsInit();
    if (::ns3::wsn::scenario4::params::g_resultFileStream && 
        ::ns3::wsn::scenario4::params::g_resultFileStream->is_open())
    {
        const auto& uavPaths = GetUavFlightPaths();
        *::ns3::wsn::scenario4::params::g_resultFileStream
            << "Step 11: Plan UAV Flight Paths" << std::endl
            << "  Total UAVs: " << uavPaths.size() << std::endl;
        
        for (const auto& [uavId, path] : uavPaths)
        {
            *::ns3::wsn::scenario4::params::g_resultFileStream
                << "  UAV " << uavId << ":" << std::endl
                << "    Waypoints: " << path.waypoints.size() << std::endl
                << "    Total time: " << std::fixed << std::setprecision(1) 
                << path.totalTime << "s" << std::endl;
            
            // Calculate total distance
            double totalDist = 0.0;
            Ptr<Node> uavNode = NodeList::GetNode(uavId);
            if (uavNode)
            {
                Ptr<MobilityModel> mobility = uavNode->GetObject<MobilityModel>();
                if (mobility)
                {
                    Vector prevPos = mobility->GetPosition();
                    for (const auto& wp : path.waypoints)
                    {
                        totalDist += helper::CalculateDistance(prevPos.x, prevPos.y, wp.position.x, wp.position.y);
                        prevPos = wp.position;
                    }
                }
            }
            *::ns3::wsn::scenario4::params::g_resultFileStream
                << "    Total distance: " << std::fixed << std::setprecision(1) 
                << totalDist << "m" << std::endl;
        }
        *::ns3::wsn::scenario4::params::g_resultFileStream << std::endl;
        ::ns3::wsn::scenario4::params::g_resultFileStream->flush();
    }

    if (::ns3::wsn::scenario4::params::g_resultFileStream && 
        ::ns3::wsn::scenario4::params::g_resultFileStream->is_open())
    {
        *::ns3::wsn::scenario4::params::g_resultFileStream
            << "=== Base Station Initialization Complete ===" << std::endl
            << "End Time: " << Simulator::Now().GetSeconds() << "s" << std::endl
            << std::endl;
        ::ns3::wsn::scenario4::params::g_resultFileStream->flush();
    }
}

void
routing::BaseStationNode::ReceiveTopology(const routing::GlobalTopology& topology)
{
    NS_LOG_FUNCTION(this);
    
    m_topology = topology;
    m_topologyReceived = true;
    
    NS_LOG_INFO("BS received topology with " << topology.nodes.size() 
                << " nodes at t=" << topology.timestamp);
    
    // Trigger region selection after receiving topology
    if (m_topologyReceived) {
        m_suspiciousNodes = SelectSuspiciousRegion();
        
        if (!m_suspiciousNodes.empty()) {
            UavFlightPath path = CalculateFlightPath(m_suspiciousNodes);
            // Broadcast command to all registered UAVs
            SendWaypointCommand(std::numeric_limits<uint32_t>::max(), path);
            NS_LOG_INFO("UAV flight path calculated: " << path.waypoints.size() 
                        << " waypoints, total time: " << path.totalTime << "s");
        }
    }
}

std::set<uint32_t>
routing::BaseStationNode::SelectSuspiciousRegion()
{
    NS_LOG_FUNCTION(this);
    
    if (!m_topologyReceived || m_topology.nodes.empty()) {
        NS_LOG_WARN("Cannot select suspicious region: no topology available");
        return std::set<uint32_t>();
    }
    
    // Calculate suspicious score for each node
    std::set<uint32_t> suspiciousNodes =
        SelectSuspiciousRegionFromTopology(m_topology, ::ns3::wsn::scenario4::params::SUSPICIOUS_COVERAGE_PERCENT);
    
    NS_LOG_INFO("Selected " << suspiciousNodes.size() << " suspicious nodes out of " 
                << m_topology.nodes.size() << " total nodes");
    
    return suspiciousNodes;
}

routing::UavFlightPath
routing::BaseStationNode::CalculateFlightPath(const std::set<uint32_t>& suspiciousNodes)
{
    NS_LOG_FUNCTION(this << suspiciousNodes.size());
    
    UavFlightPath path;
    
    if (suspiciousNodes.empty()) {
        NS_LOG_WARN("No suspicious nodes to visit");
        return path;
    }
    
    path = BuildGreedyFlightPath(
        m_topology,
        suspiciousNodes,
        ::ns3::wsn::scenario4::params::UAV_PATH_ALTITUDE,
        ::ns3::wsn::scenario4::params::UAV_PATH_SPEED);
    
    NS_LOG_INFO("Flight path calculated: " << path.waypoints.size() 
                << " waypoints, total time: " << path.totalTime << "s");
    
    return path;
}

void
routing::BaseStationNode::SendWaypointCommand(uint32_t uavNodeId, const routing::UavFlightPath& path)
{
    NS_LOG_FUNCTION(this << uavNodeId << path.waypoints.size());
    
    // Use callback to send command to UAV
    if (g_bsUavCommandCallback) {
        g_bsUavCommandCallback(uavNodeId, path);
        NS_LOG_INFO("BS sent waypoint command to UAV " << uavNodeId);
    } else {
        NS_LOG_WARN("UAV command callback not registered");
    }
}

uint32_t
routing::BaseStationNode::GetNodeId() const
{
    return m_nodeId;
}

const std::set<uint32_t>&
GetSuspiciousNodes()
{
    return g_suspiciousNodes;
}

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3
