#include "pecee-routing-protocol.h"
#include "ns3/simulator.h"
#include "ns3/random-variable-stream.h"
#include "ns3/log.h"
#include "../wsn-routing-header.h"
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;

namespace ns3 {
namespace wsn {

NS_LOG_COMPONENT_DEFINE("PeceeRoutingProtocol");
NS_OBJECT_ENSURE_REGISTERED(PeceeRoutingProtocol);

TypeId PeceeRoutingProtocol::GetTypeId()
{
    static TypeId tid = TypeId("ns3::wsn::PeceeRoutingProtocol")
        .SetParent<WsnRoutingProtocol>()
        .SetGroupName("Wsn")
        .AddConstructor<PeceeRoutingProtocol>()
        .AddAttribute("isCH",
                      "Whether this node is a Cluster Head",
                      BooleanValue(false),
                      MakeBooleanAccessor(&PeceeRoutingProtocol::isCH),
                      MakeBooleanChecker())
        .AddAttribute("cellRadius",
                      "Cell radius for hexagonal grid",
                      DoubleValue(20.0),
                      MakeDoubleAccessor(&PeceeRoutingProtocol::cellRadius),
                      MakeDoubleChecker<double>());
    return tid;
}

PeceeRoutingProtocol::PeceeRoutingProtocol()
    : WsnRoutingProtocol(),
      traceMode(0),
      grid_offset(100),
      cellRadius(20.0),
      sensingDuration(100),
      reconfigurationTime(10000),
      colorTimeSlot(100.0),
      sensorDataDub(1),
      numberOfNodes(0),
    maxHopCount(60),
      initEnergy(2.0),
      dataFusion(false),
      isCH(false),
      isCL(false),
      myCellId(-1),
      myColor(-1),
      myX(0.0),
      myY(0.0),
      myCLId(-1),
      myCHId(-1),
      self(-1),
      levelInCell(-1),
      ssSentHop(-1)
{
    NS_LOG_FUNCTION(this);
    for (int i = 0; i < 1000; i++) {
        myCellPathToCH[i] = -1;
    }
    for (int i = 0; i < 7; i++) {
        neighborCells[i] = -1;
    }
    for (int i = 0; i < 6; i++) {
        cellGateways[i] = -1;
        neighborCellGateways[i] = -1;
    }
}

PeceeRoutingProtocol::~PeceeRoutingProtocol()
{
    NS_LOG_FUNCTION(this);
}

void PeceeRoutingProtocol::Start()
{
    NS_LOG_FUNCTION(this);
    
    // Initialize node properties from base class
    self = m_selfNodeProps.nodeId;
    myX = m_selfNodeProps.xCoord;
    myY = m_selfNodeProps.yCoord;
    
    NS_LOG_INFO("Node " << self << " starting at (" << myX << ", " << myY << ")");
    
    // Calculate cell information based on coordinates
    calculateCellInfo();
    
    // Register this node in global node list
    bool nodeExists = false;
    for (auto& node : g_ssNodesDataList) {
        if (node.id == self) {
            nodeExists = true;
            node.x = myX;
            node.y = myY;
            node.cellId = myCellId;
            node.color = myColor;
            break;
        }
    }
    
    if (!nodeExists) {
        PeceeNodeData newNode;
        newNode.id = self;
        newNode.x = myX;
        newNode.y = myY;
        newNode.isCH = false;
        newNode.isCL = false;
        newNode.cellId = myCellId;
        newNode.color = myColor;
        newNode.clId = -1;
        newNode.chId = -1;
        newNode.numSent = 0;
        newNode.numRecv = 0;
        newNode.energyConsumtion = 0.0;
        newNode.castaliaConsumtion = 0.0;
        newNode.el = 100.0;
        newNode.controlPacketCount = 0;
        newNode.controlPacketsConsumtion = 0.0;
        g_ssNodesDataList.push_back(newNode);
        numberOfNodes = g_ssNodesDataList.size();
    }
    
    // Pre-calculate simulation results if not done yet
    if (!g_isPrecalculated) {
        g_isPrecalculated = true;
        // Delay to allow ALL nodes to register (startupRandomization=10s, so wait 12-13s)
        Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
        double delay = rand->GetValue(12.0, 13.0);
        Simulator::Schedule(Seconds(delay), &PeceeRoutingProtocol::PrecalculateSimulationResults, this);
        //std::cout << "#PRECALC_SCHEDULED at " << delay << "s" << std::endl;
    }
    
    // Note: CL status will be determined in PrecalculateSimulationResults() and updated via selectClusterHead()
    
    // Schedule initial CH announcement at a synchronized time for all CH nodes
    if (isCH) {
        double startDelay = 17.0;  // Node 0 announces first
        if (self == 99) {
            startDelay = 20.0;  // Node 99 delays 3s to fully separate from Node 0's propagation
        }
        Simulator::Schedule(Seconds(startDelay), &PeceeRoutingProtocol::sendCHAnnouncement, this);
        
        if (self == 99) {
            std::cout << "#CH99_SCHEDULED delay:" << startDelay << "s" << std::endl;
        }
    }
    
    // Schedule data sending phase after CHA propagation completes
    // Random offset to avoid collisions
    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
    double dataStartDelay = rand->GetValue(25.0, 30.0);  // Start after CHA completes
    Simulator::Schedule(Seconds(dataStartDelay), &PeceeRoutingProtocol::sendSensorDataPacket, this);
    std::cout << "#DATA_SCHEDULED Node:" << self << " delay:" << dataStartDelay << "s" << std::endl;
    
    // Schedule CH rotation (synchronized across all nodes)
    Simulator::Schedule(Seconds(20.0), &PeceeRoutingProtocol::rotationCH, this);
    
    NS_LOG_INFO("Node " << self << " in cell " << myCellId << " color " << myColor 
                << (isCL ? " [CL]" : "") << (isCH ? " [CH]" : ""));
}

void PeceeRoutingProtocol::FromMacLayer(Ptr<Packet> pkt,
                                        const uint16_t src)
{
    // drop all packets if not from neighbor nodes
    for (auto neighborId : getNodeData(self)->neighbors) {
        if (neighborId == src) {
            break;
        }
        //std::cout << "#PECEE_DROP_NON_NEIGHBOR Node:" << self << " From:" << src << std::endl;
        return;
    }

    NS_LOG_FUNCTION(this << src);
    
    //std::cout << "#PECEE_FROMMAC Node:" << self << " From:" << src << std::endl;
    
    // Remove WsnRoutingHeader first (added by SendPacket for forwarder)
    WsnRoutingHeader wsnHeader;
    pkt->RemoveHeader(wsnHeader);
    
    // Extract PECEE header to check packet type
    PeceeHeader peceeHeader;
    pkt->RemoveHeader(peceeHeader);
    PeceePacketType packetType = peceeHeader.GetPacketType();
    
    // Filter: only process if destined for this node, broadcast, or data packet (needs forwarding)
    uint16_t dest = wsnHeader.GetDestination();
    if (dest != self && dest != 0xFFFF && packetType != SENSOR_DATA) {
        //std::cout << "#PECEE_FILTER_DROP Node:" << self << " Dest:" << dest << " Type:" << packetType << std::endl;
        return;
    }
    
    //std::cout << "#PECEE_ACCEPT Node:" << self << " From:" << src << " Dest:" << dest << std::endl;
    
    //std::cout << "#PECEE_HEADER_TYPE Node:" << self << " Type:" << peceeHeader.GetPacketType() << std::endl;
    
    // Update statistics
    PeceeNodeData* node = getNodeData(self);
    if (node) {
        node->numRecv++;
    }
    
    // Route to appropriate handler based on packet type (already extracted above)
    
    if (self == 11 && packetType == CH_ANNOUNCEMENT_PACKET) {
        std::cout << "#DEBUG_NODE11_FROMMAC From:" << src << " Dest:" << dest 
                  << " Type:" << packetType << std::endl;
    }
    
    //NS_LOG_DEBUG("Node " << self << " received packet type " << packetType << " from " << src);
    if (packetType == CH_ANNOUNCEMENT_PACKET) {
        //std::cout << "#PECEE_FROMMAC_CHA Node:" << self << " From:" << src
        //          << " Dest:" << dest << " Type:" << packetType << std::endl;
    }
    
    switch (packetType) {
        case CH_ANNOUNCEMENT_PACKET:
            // Re-add header before passing to handler
            pkt->AddHeader(peceeHeader);
            handleCHAnnouncementPacket(pkt);
            break;
            
        case ANNOUNCE_CELL_HOP:
            pkt->AddHeader(peceeHeader);
            handleCellHopAnnouncementPacket(pkt);
            break;
            
        case SENSOR_DATA:
            pkt->AddHeader(peceeHeader);
            handleSensorDataPacket(pkt);
            break;
            
        case FINALIZE_PKT:
            NS_LOG_INFO("Node " << self << " received finalize packet");
            break;
            
        default:
            NS_LOG_WARN("Node " << self << " received unknown packet type: " << packetType);
            break;
    }
}


double PeceeRoutingProtocol::calculateDistance(double x1, double y1, double x2, double y2) {
    return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
}

PeceeNodeData* PeceeRoutingProtocol::getNodeData(int nodeId) {
    for (auto& nodeData : g_ssNodesDataList) {
            if (nodeData.id == nodeId) {
                return &nodeData;
            }
        }
        PeceeNodeData* emptyData = new PeceeNodeData();
        return emptyData;
}

PeceeCellData* PeceeRoutingProtocol::getCellData(int cellId) {
    for (auto& cellData : g_ssCellDataList) {
        if (cellData.cellId == cellId) {
            return &cellData;
        }
    }
    PeceeCellData* emptyData = new PeceeCellData();
    return emptyData;
}

bool PeceeRoutingProtocol::isNodeInList(int nodeId, const vector<int>& nodeList) {
    for (int id : nodeList) {
            if (id == nodeId) {
                return true;
            }
        }
        return false;
}

void PeceeRoutingProtocol::PrecalculateSimulationResults()
{
    g_ssSensorDataSent.clear();
    g_ssSensorDataReceived.clear();
    g_ssSensorDataSentCount = 0;
    g_ssSensorDataReceivedCount = 0;
     for (int i=0; i<numberOfNodes; i++) {
         g_ssSensorData[i] = -1;
         g_ssSensorDataArr[i] = -1;
//         g_networkConsumption[i][0] = 0;
//         g_networkConsumption[i][1] = 0;
     }

    for (auto &node : g_ssNodesDataList) {
    node.neighbors.clear();
    node.clId = -1;
    }
    g_ssCellDataList.clear();

    // Calculate node neighbors
    for (auto& nodeData : g_ssNodesDataList) {
        for (auto& otherNodeData : g_ssNodesDataList) {
            if (nodeData.id == otherNodeData.id) continue;
            double distance = calculateDistance(nodeData.x, nodeData.y, otherNodeData.x, otherNodeData.y);
            if (distance <= cellRadius) {
                nodeData.neighbors.push_back(otherNodeData.id);
                //if (traceMode == 0) std::cout << "#NEIGHBOR " << nodeData.id << ": " << otherNodeData.id ;
            }
        }
    }

    // Calculate cell members
    for (auto& nodeData : g_ssNodesDataList) {
        // check if the cell already exists, add the node to the cell members
        bool cellExists = false;
        for (auto& cellData : g_ssCellDataList) {
            if (cellData.cellId == nodeData.cellId) {
                cellExists = true;
                cellData.members.push_back(nodeData.id);
                break;
            }
        }
        // else create a new cell and add the node as a member
        if (!cellExists) {
            PeceeCellData cellData;
            cellData.cellId = nodeData.cellId;
            cellData.color = nodeData.color;
            cellData.clId = nodeData.clId;
            cellData.members.push_back(nodeData.id);
            g_ssCellDataList.push_back(cellData);
        }
    }
    //calculate cell neighbors
    for (auto& cellData : g_ssCellDataList) { // for each cell
        int thisCellId = cellData.cellId;
        for (auto& nodeData : g_ssNodesDataList) { // for each node 
            if (nodeData.cellId == thisCellId) { // in the cell
                for (const int neighborId : nodeData.neighbors) { // for each neighbor
                    PeceeNodeData* neighbor = getNodeData(neighborId);
                    int neighborCellId = neighbor->cellId;
                    if (neighborCellId != thisCellId) { // if different cell
                        if (!isNodeInList(neighborCellId, cellData.neighbors)) {
                            cellData.neighbors.push_back(neighborCellId);
                            cellData.gateways[neighborCellId] = -1; // Initialize NGW ID
                        }
                    }
                }
            }
        }
    }

    // Calculate cell leaders
    for (auto& cellData : g_ssCellDataList) {
        int bestFitness = -1;
        int bestCLId = -1;
        for (int memberId : cellData.members) {
            PeceeNodeData* member = getNodeData(memberId);
            int r = round((double)cellData.cellId / grid_offset);
            int q = cellData.cellId - r * grid_offset;

            double centerX = cellRadius * (sqrt(3.0) * q + sqrt(3.0) / 2.0 * r);
            double centerY = cellRadius * (3.0 / 2.0 * r);

            double distance = sqrt(pow(member->x - centerX, 2) + pow(member->y - centerY, 2));

            int fitnessScore = static_cast<int>(1000 / (1 + distance));
            if (fitnessScore > bestFitness) {
                bestFitness = fitnessScore;
                bestCLId = member->id;  
            }
        }
        //std::cout << "#CELL_LEADER Cell:" << cellData.cellId << " CL:" << bestCLId << std::endl;
        getCellData(cellData.cellId)->clId = bestCLId;
        for (auto& nodeData : g_ssNodesDataList) {
            if (isNodeInList(nodeData.id, cellData.members)) {
                nodeData.clId = bestCLId;

            }
        }
    }

    // calculate gateway candidates for each pair of cell
    for (auto& cellData : g_ssCellDataList) {
        for (int neighborId : cellData.neighbors) {
            //choose the best pair of CGW and NGW
            double bestDistance = 9999.0;
            int bestCGWId = -1;
            int bestNGWId = -1;
            if (cellData.gateways[neighborId] == -1) { // If NGW ID is not set
                for (auto& nodeData : g_ssNodesDataList) {
                    int nodeCellId = nodeData.cellId;
                    if (nodeCellId == cellData.cellId) {
                        for (int neighborNodeId : nodeData.neighbors) {
                            PeceeNodeData* neighborNode = getNodeData(neighborNodeId);
                            //if (cellData.cellId >= neighbor.cellId) continue;
                            if (neighborNode->cellId == neighborId) {
                                double linkDistance = sqrt(pow(nodeData.x - neighborNode->x, 2) + pow(nodeData.y - neighborNode->y, 2));
                                const double EPS = 1e-6;
                                if (linkDistance < bestDistance) {
                                    bestDistance = linkDistance;
                                    bestCGWId = nodeData.id;
                                    bestNGWId = neighborNode->id;
                                }
                                else if (fabs(linkDistance - bestDistance) < EPS) {
                                    // If the distance is equal, prefer the one with lower ID
                                    if (nodeData.id < bestCGWId) {
                                        bestCGWId = nodeData.id;
                                        bestNGWId = neighborNode->id;
                                    }
                                }
                            }
                        }
                    }
                }

                if (bestCGWId != -1 && bestNGWId != -1) {
                    cellData.gateways[neighborId] = bestCGWId;
                    //std::cout << "#GATEWAY_PAIR Cell:" << cellData.cellId << " CGW:" << bestCGWId << " <-> NGW:" << bestNGWId << std::endl;
                }
            }
            // update next hop ID for the CGW
            for (auto& nodeData : g_ssNodesDataList) {
                int nodeDataId = nodeData.id;
                if (nodeDataId == cellData.gateways[neighborId]) {
                    g_ssRoutingTable[nodeDataId][neighborId] = bestNGWId; // Set next hop to NGW
                    //if (traceMode == 0) std::cout << "#ROUTING_TABLE " << nodeDataId << " (" << cellData.cellId << ") -> " << g_ssRoutingTable[nodeDataId][neighborId] << " (" << neighborId << ")";
                }
            }
        }
    }

    // calculate intra-cell routing table for each cell gateways and CL
    for (auto& cellData : g_ssCellDataList) {
        for (int neighborId : cellData.neighbors) {
            int gatewayId = cellData.gateways[neighborId];
            if (gatewayId != -1) {
                PeceeNodeData* cellGateway = getNodeData(gatewayId);
                for (int memberId : cellData.members) {
                    PeceeNodeData* member = getNodeData(memberId);
                    bool isGatewayInRange = false;
                    if (memberId != gatewayId) {
                        g_ssRoutingTable[memberId][neighborId] = -1;
                        for (const int neighborNodeId : member->neighbors) {
                            if (neighborNodeId == gatewayId) {
                                isGatewayInRange = true;
                                g_ssRoutingTable[memberId][neighborId] = gatewayId;
                                // if (traceMode == 0) std::cout << "#ROUTING_TABLE " << memberId << " (" << member->cellId << ") -> " << g_ssRoutingTable[memberId][neighborId] << " (" << neighborId << ")";
                            }

                        }
                        if (!isGatewayInRange) {
                            // If the gateway is not in range, find the best next hop
                            double minDistance = 9999.0;
                            int bestNextHopId = -1;

                            for (const int neighborNodeId : member->neighbors) {
                                bool isNeighborInRange = false;
                                // continue if the neighbor is not in range of the gateway
                                for (const auto& neighborNodeData : g_ssNodesDataList) {
                                    if (neighborNodeData.id == neighborNodeId) {
                                        for (const int neighborOfNeighborId : neighborNodeData.neighbors) {
                                            if (neighborOfNeighborId == gatewayId) {
                                                isNeighborInRange = true;
                                                break;
                                            }
                                        }
                                    }
                                }
                                if (isNeighborInRange) {
                                    // save the distance and choose the best next hop based on total distance from this node to neighbor to gateway
                                    PeceeNodeData* neighborNode = getNodeData(neighborNodeId);
                                    double distanceNodeToNeighbor = sqrt(pow(member->x - neighborNode->x, 2) + pow(member->y - neighborNode->y, 2));
                                    double distanceNeighborToGateway = sqrt(pow(neighborNode->x - cellGateway->x, 2) + pow(neighborNode->y - cellGateway->y, 2));
                                    double totalDistance = distanceNodeToNeighbor + distanceNeighborToGateway;
                                    if (totalDistance < minDistance) {
                                        minDistance = totalDistance;
                                        bestNextHopId = neighborNodeId;
                                    } else if (totalDistance == minDistance) {
                                        // If the distance is equal, prefer the one with lower ID
                                        if (neighborNodeId < bestNextHopId) {
                                            bestNextHopId = neighborNodeId;
                                        }
                                    }
                                }
                            }
                            if (bestNextHopId != -1) {
                                g_ssRoutingTable[member->id][neighborId] = bestNextHopId;
                                //if (traceMode == 0) std::cout << "#ROUTING_TABLE " << member->id << " (" << member->cellId << ") -> " << g_ssRoutingTable[member->id][neighborId] << " (" << neighborId << ")";
                            }
                        }
                    }
                }
            }
        }

        // routing table for CL
        for (const int memberId : cellData.members) {
            if (memberId == cellData.clId) {
                continue;
            }
            g_ssRoutingTable[memberId][cellData.cellId] = -1;
            PeceeNodeData* member = getNodeData(memberId);
            if (isNodeInList(cellData.clId, member->neighbors)) { // if CL is in range
                g_ssRoutingTable[memberId][cellData.cellId] = cellData.clId;
                //if (traceMode == 0) std::cout << "#ROUTING_TABLE " << memberId << " (" << cellData.cellId << ") -> " << g_ssRoutingTable[memberId][cellData.cellId] << " (" << cellData.cellId << ")";
            } else {
                double minDistance = 9999.0;
                int bestNextHopId = -1;

                for (const int neighborId : member->neighbors) {
                    // Check if the neighbor is in range of the CL
                    bool isNeighborInRange = false;
                    for (const auto& neighborNodeData : g_ssNodesDataList) {
                        if (neighborNodeData.id == neighborId) {
                            for (const int neighborOfNeighborId : neighborNodeData.neighbors) {
                                if (neighborOfNeighborId == cellData.clId) {
                                    isNeighborInRange = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (isNeighborInRange) {
                        PeceeNodeData* clNode = getNodeData(cellData.clId);
                        PeceeNodeData* neighbor = getNodeData(neighborId);
                        double distanceNodeToNeighbor = sqrt(pow(member->x - neighbor->x, 2) + pow(member->y - neighbor->y, 2));
                        double distanceNeighborToCL = sqrt(pow(neighbor->x - clNode->x, 2) + pow(neighbor->y - clNode->y, 2));
                        double totalDistance = distanceNodeToNeighbor + distanceNeighborToCL;
                        if (totalDistance < minDistance) {
                            minDistance = totalDistance;
                            bestNextHopId = neighborId;
                        } else if (totalDistance == minDistance) {
                            // If the distance is equal, prefer the one with lower ID
                            if (neighborId < bestNextHopId) {
                                bestNextHopId = neighborId;
                            }
                        }
                    }
                }
                if (bestNextHopId != -1) {
                    g_ssRoutingTable[member->id][cellData.cellId] = bestNextHopId;
                    // if (traceMode == 0) std::cout << "#ROUTING_TABLE " << member->id << " (" << cellData.cellId << ") -> " << g_ssRoutingTable[member->id][cellData.cellId] << " (" << cellData.cellId << ")";
                }
            }
        }
    }
}

Point PeceeRoutingProtocol::calculateCellCenter(int cell_id) {
    int r = round((double)cell_id / grid_offset);
    int q = cell_id - r * grid_offset;

    Point center;
    center.x = cellRadius * (sqrt(3.0) * q + sqrt(3.0) / 2.0 * r);
    center.y = cellRadius * (3.0 / 2.0 * r);

    return center;
}

void PeceeRoutingProtocol::calculateCellInfo()
{
    double frac_q = (sqrt(3.0)/3.0 * myX - 1.0/3.0 * myY) / cellRadius;
    double frac_r = (2.0/3.0 * myY) / cellRadius;
    double frac_s = -frac_q - frac_r;

    int q = round(frac_q);
    int r = round(frac_r);
    int s = round(frac_s);

    double q_diff = abs(q - frac_q);
    double r_diff = abs(r - frac_r);
    double s_diff = abs(s - frac_s);

    if (q_diff > r_diff && q_diff > s_diff) {
        q = -r - s;
    } else if (r_diff > s_diff) {
        r = -q - s;
    } else {
        s = -q - r;
    }

    myCellId = q + r * grid_offset;
    myColor = ((q - r) % 3 + 3) % 3;
}

void PeceeRoutingProtocol::sendCHAnnouncement()
{
    NS_LOG_FUNCTION(this);
    
    // Only CH nodes can initiate CHA packets
    if (!isCH) {
        return;
    }
    
    if (self == 99) {
        PeceeNodeData* node99 = getNodeData(99);
        std::cout << "#CH99_ANNOUNCE neighbors:" << (node99 ? node99->neighbors.size() : 0) << std::endl;
        if (node99) {
            std::cout << "#CH99_NEIGHBORS ";
            for (int n : node99->neighbors) {
                std::cout << n << " ";
            }
            std::cout << std::endl;
        }
    }
    
    SSCHAnnouncementInfo chInfo;
    chInfo.chId = self;

    PeceeCellData* cellData = getCellData(myCellId);
    if (!cellData || cellData->members.empty()) {
        NS_LOG_WARN("Node " << self << " - Cannot find cell data for cell " << myCellId);
        return;
    }

    // Cluster Head broadcasts CH announcement to neighbor cells
    myCHId = self;
    myCellPathToCH[0] = myCellId;
    
    // std::cout << "#DEBUG_CHA_SENT CH:" << self << " Cell:" << myCellId << " at " << Simulator::Now().GetSeconds() << "s" << std::endl;
    
    std::ostringstream chPathInit;
    chPathInit << myCellId;
    //std::cout << "#CHA_PATH_CH_INIT CH:" << self << " Cell:" << myCellId 
    //          << " InitPath:" << chPathInit.str() << std::endl;
    
        // std::cout << "#CH_ANNOUNCE_START CH:" << self << " Cell:" << myCellId 
        //           << " NeighborCells:";
    // for (const int neighborCellId : cellData->neighbors) {
    //     // std::cout << neighborCellId << " ";
    // }
    // std::cout << std::endl;
    
    // Create packets for all neighbor cells
    //std::cout << "#CHA_CREATE_INTER Node:" << self << " Cell:" << myCellId 
    //          << " NeighborCells:" << cellData->neighbors.size() << std::endl;
    for (const int neighborCellId : cellData->neighbors) {
        Ptr<Packet> pkt = Create<Packet>();
        PeceeHeader header;
        
        header.SetPacketType(CH_ANNOUNCEMENT_PACKET);
        header.SetCellSource(myCellId);
        header.SetCellHopCount(1);
        header.SetCellDestination(-1);
        header.SetCellPath(0, myCellId);
        header.SetCellPath(1, -1);
        header.SetTtl(maxHopCount);
        header.SetCellSent(myCellId);
        header.SetCHAnnouncementData(chInfo);
        header.SetSource(self);
        header.SetCellNext(neighborCellId);
        
        // DEBUG: Verify source is set correctly
        // if (self == 0 || self == 99) {
        //     uint16_t verifySource = header.GetSource();
        //     std::cout << "#DEBUG_CH_SET_SOURCE CH:" << self << " VerifySource:" << verifySource << std::endl;
        // }
        
        // Add header before queueing so TTL and fields persist
        pkt->AddHeader(header);
        announcementQueue.push({pkt, neighborCellId});
        //std::cout << "#CHA_QUEUED_INTER Node:" << self << " Cell:" << myCellId 
        //          << " -> NextCell:" << neighborCellId << std::endl;
    }
    
    // Broadcast within own cell
    //std::cout << "#CHA_CREATE_INTRA Node:" << self << " Cell:" << myCellId 
    //          << " Members:" << cellData->members.size() << std::endl;
    for (int memberId : cellData->members) {
        if (memberId != self) {
            // if (memberId == 11) {
            //     std::cout << "#DEBUG_CH_SEND_TO_NODE11 CH:" << self << " Cell:" << myCellId << std::endl;
            // }
            Ptr<Packet> pkt = Create<Packet>();
            PeceeHeader header;
            
            header.SetPacketType(CH_ANNOUNCEMENT_PACKET);
            header.SetCellSource(myCellId);
            header.SetCellHopCount(1);
            header.SetCellDestination(-1);
            header.SetCellPath(0, myCellId);
            header.SetCellPath(1, -1);
            header.SetTtl(maxHopCount);
            header.SetCellSent(myCellId);
            header.SetCHAnnouncementData(chInfo);
            header.SetSource(self);
            header.SetCellNext(myCellId);
            
            // Add header and queue for intra-cell broadcast
            pkt->AddHeader(header);
            boardcastAnnouncementQueue.push(memberId);
            announcementQueue.push({pkt, myCellId});
            //std::cout << "#CHA_QUEUED_INTRA Node:" << self << " Cell:" << myCellId 
            //          << " -> Member:" << memberId << std::endl;
        }
    }
    
    NS_LOG_INFO("#CH_SELECTION " << self << ": " << myCHId);
    selectClusterHead();
    
    // Schedule queue processing
        // CH should transmit immediately (very small delay) so CLs can process fast
        double delay = GetRandomDelay(0.01, 0.02);
    ScheduleTimer(SEND_ANNOUNCEMENT_QUEUE, delay);
    
        // CH should also notify itself immediately (not wait for MAC loopback)
        // so it can start forwarding other CHs' announcements
        // std::cout << "#DEBUG_CH_SELF_ASSIGN Node:" << self << " sets own CH:" << self << std::endl;
}

// TODO: Convert to ns-3 style
/*
void PeceeRoutingProtocol::handleCHAnnouncementPacket(Ptr<Packet> pkt)
{
    // TEMPORARILY COMMENTED - TODO TO ns-3 Ptr<Packet> style
    // Original OMNeT++ code needs refactoring
}
*/

void PeceeRoutingProtocol::handleCHAnnouncementPacket(Ptr<Packet> pkt)
{
    NS_LOG_FUNCTION(this);
    
    // DEBUG: Track all CHA packets
    // std::cout << "#CHA_HANDLE_ENTRY Node:" << self << " Cell:" << myCellId << std::endl;
    
    // Trace reception for debugging
    // std::cout << "#CHA_RECV_START Node:" << self << " Cell:" << myCellId << std::endl;
    
    // Local trace variable to track CHA packet's journey through nodes
    std::ostringstream packetTrace;
    packetTrace << "[Node:" << self << " Cell:" << myCellId << " t:" << Simulator::Now().GetSeconds() << "s]";
    
    // Update CL status first if not already set
    if (!isCL) {
        PeceeCellData* cellData = getCellData(myCellId);
        if (cellData && cellData->clId == self) {
            isCL = true;
            myCLId = self;
        }
    }
    
    // Extract header
    PeceeHeader header;
    pkt->RemoveHeader(header);
    
    uint16_t sourceId = header.GetSource();
    int cellSource = header.GetCellSource();
    int hopCount = header.GetCellHopCount();
    int ttl = header.GetTtl();
    SSCHAnnouncementInfo chInfo = header.GetCHAnnouncementData();
    int chId = chInfo.chId;
    int cellSent = header.GetCellSent();

    // std::cout << "#CHA_RECV Node:" << self << " \tCell:" << myCellId
    //           << "  \tCH:" << chId << " \tHop:" << hopCount
    //           << " \tTTL:" << ttl << " \tCellSent:" << cellSent << std::endl;

    // if (self == 11) {
    //     std::cout << "#DEBUG_NODE11_RECV From:" << sourceId << " CH:" << chId 
    //               << " isCL:" << isCL << " isCH:" << isCH << " cellSent:" << cellSent 
    //               << " myCellId:" << myCellId << " hopCount:" << hopCount << std::endl;
    // }
        // std::cout << "#DEBUG_CH99_RECV Node:" << self << " isCH:" << isCH << " chId:" << chId << " self:" << self << std::endl;
    
    // Check if we've already forwarded CHA from this CH from this specific cellSent
    // This prevents the same announcement from propagating multiple times through this node
    // BUT: CH nodes should NOT filter their own CHA - only CHs can originate CHA
    if (!isCH && receivedCHAFromCells[chId].count(cellSent) > 0) {
        // Duplicate check: Node already processed announcement from this CH via this cell
        // Skip for now - use CL-level check instead (CL only processes first announcement)
        // std::cout << "#CHA_DROP_DUP Node:" << self << " Cell:" << myCellId 
        //           << " CH:" << chId << " CellSent:" << cellSent << std::endl;
        // Don't return - continue forwarding (allow inter-cell broadcast from same cell)
    }
    // For CHs, mark that we've processed this to avoid re-processing loops
    if (isCH && chId == self && cellSent == myCellId) {
        // This is our own CHA bouncing back - process it once but don't re-forward
        if (receivedCHAFromCells[chId].count(cellSent) > 0) {
            return;  // Already processed our own CHA
        }
    }
    receivedCHAFromCells[chId].insert(cellSent);
    g_CHANodeTraces[chId].push_back(self); // For global trace of CH announcement propagation
    if (isCL){
        g_CHACellTraces[chId].push_back(myCellId); // For global trace of CH announcement propagation
    }
    
    // Loop detection: check if myCellId already exists in the path (cell-level loop prevention)
    // Only check for inter-cell packets (cellSent != myCellId)
    if (cellSent != myCellId) {
        for (int i = 0; i < hopCount && i < 3; ++i) {
            if (header.GetCellPath(i) == myCellId) {
                // Cell loop detected - this cell already appeared in path
                //std::cout << "#CHA_DROP_LOOP Node:" << self << " Cell:" << myCellId
                //          << " CH:" << chId << " FromCell:" << cellSent << std::endl;
                return;
            }
        }
    }
    
    // std::cout << "#CHA_RECV Node:" << self << " Cell:" << myCellId 
    //           << " From:" << sourceId << "(Cell " << cellSource << ") CH:" << chId 
    //           << " Hops:" << hopCount << " TTL:" << ttl << std::endl;
    
    // ONLY CL can accept and update CH information
    if (isCL) {
        // CL: Accept only the FIRST CH announcement
        if (myCHId == -1) {
            myCHId = chId;
            
            // Save path to CH
            int dbgPath[3] = {-1, -1, -1};
            for (int i = 0; i < hopCount && i < 3; ++i) {
                dbgPath[i] = header.GetCellPath(i);
                myCellPathToCH[i] = dbgPath[i];
            }
            myCellPathToCH[hopCount] = myCellId;
            
            std::ostringstream pathOs;
            for (int i = 0; i <= hopCount && i < 3; ++i) {
                int v = myCellPathToCH[i];
                if (v != -1 && v != 0xFFFF) {
                    pathOs << v << ' ';
                }
            }
            std::cout << "#CHA_CH_ACCEPT_CL Node:" << self << " (CL) CH:" << myCHId 
                      << " PathLen:" << (hopCount + 1)
                      << " SrcCell:" << cellSource
                      << " SentCell:" << cellSent
                      << " Hop:" << hopCount
                      << " TTL:" << ttl
                      << " Path:" << pathOs.str()
                      << " Trace:" << packetTrace.str()
                      << std::endl;
            
            selectClusterHead();
            
            // CL broadcasts CH info to all members in its cell
            PeceeCellData* cellData = getCellData(myCellId);
            if (cellData && cellData->members.size() > 0) {  // Has members (members != include CL)
                // Count members excluding CL itself
                int memberCount = 0;
                for (int mid : cellData->members) {
                    if (mid != self) memberCount++;
                }
                
                if (memberCount > 0) {
                    std::cout << "#CL_BROADCAST_TO_MEMBERS Node:" << self << " (CL) Cell:" << myCellId 
                              << " CH:" << myCHId << " MemberCount:" << memberCount << std::endl;
                    
                    for (int memberId : cellData->members) {
                        if (memberId != self) {  // Don't send to self
                        // Create a new CHA packet for intra-cell broadcast
                        Ptr<Packet> memberPkt = Create<Packet>();
                        PeceeHeader memberHeader;
                        
                        // Set packet type and CH info
                        SSCHAnnouncementInfo memberChInfo;
                        memberChInfo.chId = myCHId;
                        memberHeader.SetCHAnnouncementData(memberChInfo);
                        memberHeader.SetPacketType(CH_ANNOUNCEMENT_PACKET);
                        
                        // Set routing info: CL is the next hop toward CH
                        memberHeader.SetSource(self);  // CL is source
                        memberHeader.SetDestination(memberId);
                        memberHeader.SetCellSource(myCellId);
                        memberHeader.SetCellSent(myCellId);
                        memberHeader.SetCellNext(myCellId);
                        memberHeader.SetTtl(60);
                        
                        // Mark this as intra-cell broadcast from CL
                        memberHeader.SetCellHopCount(0);  // Direct from CL
                        
                        memberPkt->AddHeader(memberHeader);
                        boardcastAnnouncementQueue.push(memberId);
                        announcementQueue.push({memberPkt, myCellId});
                        
                        std::cout << "#CL_NOTIFY_MEMBER Node:" << self << " (CL) -> Member:" << memberId 
                                  << " CH:" << myCHId << " NextHop:" << self 
                                  << " HopCount:" << 0 << " SentCell:" << myCellId << std::endl;
                    }
                }
                
                // Schedule queue processing
                double delay = GetRandomDelay(0.05, 0.1);
                ScheduleTimer(SEND_ANNOUNCEMENT_QUEUE, delay);
                }  // End if memberCount > 0
            }  // End if cellData && members.size() > 0
        } else {
            //std::cout << "#CHA_IGNORE_CL Node:" << self << " (CL) Already has CH:" << myCHId 
            //          << " (new CH:" << chId << " ignored)" << std::endl;
            // CL continues below to forward CHA even if it already knows this CH
            // This ensures CHA propagates throughout the network
        }
    }
    
    // Members: Accept CH info from intra-cell CHA broadcasts (hop=0, same cell)
    // This runs for non-CLs after CL has processed
    // Detection: hopCount==0 = intra-cell broadcast (CH just initiated)
    //            hopCount>0 = inter-cell forward (already passed through cells)
    if (!isCL && hopCount == 0) {
        if (myCHId == -1) {
            myCHId = chId;
            std::cout << "#MEMBER_ACCEPT_CH Node:" << self << " (Member) Cell:" << myCellId
                      << " CH:" << myCHId << " From:" << sourceId << std::endl;
        } else {
            //std::cout << "#MEMBER_IGNORE_CH Node:" << self << " (Member) Already has CH:" << myCHId << std::endl;
        }
        return;  // Members don't forward intra-cell broadcasts (hopCount=0)
    }
    
    // At this point, either:
    // 1. We are CL (isCL=true) → forward inter-cell
    // 2. We are non-CL receiving inter-cell CHA (hopCount>0, forwarded) → forward
    // All other cases already returned above
    
    // ALL nodes (CL and non-CL) forward CHA to neighbor cells until TTL expires
    PeceeCellData* cellData = getCellData(myCellId);
    
    // // Debug Node 99
    // if (self == 99) {
    //     PeceeNodeData* node99 = getNodeData(99);
    //     std::cout << "#DEBUG_NODE99 neighbors:" << (node99 ? node99->neighbors.size() : 0) 
    //               << " cellData:" << (cellData ? "Y" : "N") << std::endl;
    //     if (node99) {
    //         std::cout << "#DEBUG_NODE99_NEIGHBORS ";
    //         for (int n : node99->neighbors) {
    //             std::cout << n << " ";
    //         }
    //         std::cout << std::endl;
    //     }
    // }
    
    if (!cellData) {
        // std::cout << "#CHA_NO_CELLDATA Node:" << self << " Cell:" << myCellId 
        //           << " CH:" << chId << std::endl;
        return;
    }
    
    if (cellData->members.size() == 0) {
        // std::cout << "#CHA_NO_MEMBERS Node:" << self << " Cell:" << myCellId 
        //           << " CH:" << chId << std::endl;
        return;
    }
    
    if (cellData) {
        bool hasForwarded = false;
        
        // if (self == 11) {
        //     std::cout << "#DEBUG_NODE11_FORWARD isCL:" << isCL << " neighborCount:" 
        //               << cellData->neighbors.size() << " gatewayCount:"
        //               << cellData->gateways.size() << " gateways:";
        //     for (const auto& gw : cellData->gateways) {
        //         std::cout << gw.first << ":" << gw.second << " ";
        //     }
        //     std::cout << std::endl;
        // }
        
        // 1. Forward to neighbor cells (inter-cell) - ONLY via GATEWAYS
        // Gateway nodes are the bridge between cells, NOT all neighbor cells
        for (const int neighborCellId : cellData->neighbors) {
            if (neighborCellId == cellSent) {
                continue;  // Don't send back to source cell
            }
            
            // Find the gateway node to this neighbor cell
            auto gwIt = cellData->gateways.find(neighborCellId);
            if (gwIt == cellData->gateways.end()) {
                // No gateway to this neighbor cell, skip
                // if (self == 11) {
                //     std::cout << "#DEBUG_NODE11_NO_GATEWAY to cell:" << neighborCellId << std::endl;
                // }
                continue;
            }
            
            int gatewayNodeId = gwIt->second;
            
            // if (self == 11) {
            //     std::cout << "#DEBUG_NODE11_SEND_VIA_GATEWAY Cell:" << neighborCellId 
            //               << " Gateway:" << gatewayNodeId << " TTL:" << (ttl - 1) << std::endl;
            // }
            
            // std::cout << "#CHA_FORWARD_GATEWAY Node:" << self << " Cell:" << myCellId 
            //           << " -> Cell:" << neighborCellId << " via GW:" << gatewayNodeId 
            //           << " CH:" << chId << " TTL:" << (ttl - 1) << std::endl;
            Ptr<Packet> dupPkt = pkt->Copy();
            PeceeHeader dupHeader = header;
            
            dupHeader.SetCellSent(myCellId);
            dupHeader.SetCellHopCount(hopCount + 1);
            if (hopCount + 1 < 3) {
                dupHeader.SetCellPath(hopCount + 1, myCellId);
            }
            dupHeader.SetTtl(ttl - 1);
            // NOTE: DO NOT change source when forwarding!
            // Source should remain the ORIGINATOR (CH node)
            // dupHeader.SetSource(self);  // REMOVED: Keep original source
            dupHeader.SetCellNext(neighborCellId);
            dupHeader.SetDestination(gatewayNodeId);  // Send to gateway node
            
            std::ostringstream fwdPath;
            for (int i = 0; i < hopCount + 1 && i < 3; ++i) {
                int v = dupHeader.GetCellPath(i);
                if (v != -1 && v != 0xFFFF) {
                    fwdPath << v << ' ';
                }
            }
            
            // Update packet trace with current forward step
            packetTrace << " -> [Node:" << self << " FwdTo:" << neighborCellId 
                       << " Hop:" << (hopCount + 1) << " TTL:" << (ttl - 1) << "]";
            
            //std::cout << "#CHA_PATH_FWD Node:" << self << " Cell:" << myCellId 
            //          << " -> NextCell:" << neighborCellId
            //          << " CH:" << chId << " Hop:" << (hopCount + 1) << " TTL:" << (ttl - 1)
            //          << " Path:" << fwdPath.str()
            //          << " Trace:" << packetTrace.str() << std::endl;
            
            // Add updated header before queueing
            dupPkt->AddHeader(dupHeader);
            announcementQueue.push({dupPkt, neighborCellId});
            
            // Add small delay at each forward step to simulate node processing time
            double fwdDelay = GetRandomDelay(0.05, 0.1);
            ScheduleTimer(SEND_ANNOUNCEMENT_QUEUE, fwdDelay);
            
            // std::cout << "#CHA_FORWARD Node:" << self << (isCL ? " (CL)" : " (non-CL)")
            //           << " Cell:" << myCellId << " -> Cell:" << neighborCellId 
            //           << " TTL:" << (ttl-1) << std::endl;
            hasForwarded = true;
        }
        
        // 2. Broadcast within own cell (intra-cell) - ONLY if packet came from different cell
        // This prevents infinite loop: Node A → Node B (same cell) → Node A → ...
        if (cellSent != myCellId) {
            for (int memberId : cellData->members) {
                if (memberId != self && memberId != sourceId) {  // Don't send back to source or self
                    Ptr<Packet> dupPkt = pkt->Copy();
                    PeceeHeader dupHeader = header;
                    
                    dupHeader.SetSource(self);
                    dupHeader.SetCellNext(myCellId);
                    dupHeader.SetCellSent(myCellId);  // Mark as sent from this cell
                    
                    // Add updated header before queueing
                    dupPkt->AddHeader(dupHeader);
                    boardcastAnnouncementQueue.push(memberId);
                    announcementQueue.push({dupPkt, myCellId});
                    
                    // std::cout << "#CHA_INTRA_BROADCAST Node:" << self << (isCL ? " (CL)" : " (non-CL)")
                    //           << " Cell:" << myCellId << " -> Member:" << memberId << std::endl;
                    hasForwarded = true;
                }
            }
        }
        
        if (hasForwarded) {
            double delay = GetRandomDelay(0.1, 0.5);
            ScheduleTimer(SEND_ANNOUNCEMENT_QUEUE, delay);
        }
    }
}


void PeceeRoutingProtocol::sendAnnouncementQueue()
{
    NS_LOG_FUNCTION(this);
    
    // if (self == 11 && !announcementQueue.empty()) {
    //     std::cout << "#DEBUG_NODE11_SEND_QUEUE QueueSize:" << announcementQueue.size() << std::endl;
    // }
    
    if (announcementQueue.empty()) {
        //std::cout << "#CHA_QUEUE_EMPTY Node:" << self << " Cell:" << myCellId << std::endl;
        return;
    }
    
    auto [pkt, nextCellId] = announcementQueue.front();
    announcementQueue.pop();
    
    // std::cout << "#CHA_QUEUE_PROCESS Node:" << self << " Cell:" << myCellId 
    //           << " NextCell:" << nextCellId << " QueueSize:" << announcementQueue.size() << std::endl;

    // Extract and modify header
    PeceeHeader header;
    pkt->RemoveHeader(header);
    
    int ttl = header.GetTtl() - 1;
    header.SetTtl(ttl);
    
    if (ttl <= 0) {
        // std::cout << "#CHA_TTL_EXPIRED Node:" << self << " Cell:" << myCellId 
        //           << " NextCell:" << nextCellId << std::endl;
        if (!announcementQueue.empty()) {
            double delay = GetRandomDelay(0.1, 0.5);
            ScheduleTimer(SEND_ANNOUNCEMENT_QUEUE, delay);
        }
        return;
    }
    
    int nextHopId = -1;
    
    if (nextCellId != myCellId) {
        // Inter-cell routing - look up gateway
        nextHopId = g_ssRoutingTable[self][nextCellId];
        
        if (nextHopId == -1) {
            // std::cout << "#CHA_SEND_NO_ROUTE Node:" << self << " Cell:" << myCellId 
            //           << " -> Cell:" << nextCellId << " QueueRemaining:" << announcementQueue.size() << std::endl;
            if (!announcementQueue.empty()) {
                double delay = GetRandomDelay(0.1, 0.5);
                ScheduleTimer(SEND_ANNOUNCEMENT_QUEUE, delay);
            }
            return;
        }
        
        // std::cout << "#CHA_INTER_CELL Node:" << self << " Cell:" << myCellId 
        //           << " -> Cell:" << nextCellId << " NextHop:" << nextHopId << std::endl;
    } else {
        // Intra-cell forward - pop from broadcast queue
        if (!boardcastAnnouncementQueue.empty()) {
            nextHopId = boardcastAnnouncementQueue.front();
            boardcastAnnouncementQueue.pop();
            
            // std::cout << "#CHA_INTRA_CELL Node:" << self << " Cell:" << myCellId 
            //           << " -> Node:" << nextHopId << std::endl;
        } else {
            //std::cout << "#CHA_NO_BROADCAST_TARGET Node:" << self << " Cell:" << myCellId << std::endl;
            if (!announcementQueue.empty()) {
                double delay = GetRandomDelay(0.1, 0.5);
                ScheduleTimer(SEND_ANNOUNCEMENT_QUEUE, delay);
            }
            return;
        }
    }

    if (nextHopId == -1) {
        std::cout << "#CHA_NO_NEXT_HOP Node:" << self << " Cell:" << myCellId 
                  << " NextCell:" << nextCellId << std::endl;
        if (!announcementQueue.empty()) {
            double delay = GetRandomDelay(0.1, 0.5);
            ScheduleTimer(SEND_ANNOUNCEMENT_QUEUE, delay);
        }
        return;
    }
    
    // Avoid sending to self
    if (nextHopId == self) {
        std::cout << "#CHA_SKIP_SELF Node:" << self << " Cell:" << myCellId 
                  << " NextCell:" << nextCellId << std::endl;
        if (!announcementQueue.empty()) {
            double delay = GetRandomDelay(0.1, 0.5);
            ScheduleTimer(SEND_ANNOUNCEMENT_QUEUE, delay);
        }
        return;
    }
    
    // Check if next hop is in range
    bool isInRange = false;
    PeceeNodeData* selfData = getNodeData(self);
    if (selfData) {
        for (int neighborNodeId : selfData->neighbors) {
            if (neighborNodeId == nextHopId) {
                isInRange = true;
                break;
            }
        }
    }

    if (!isInRange) {
        // std::cout << "#CHA_NOT_IN_RANGE Node:" << self << " Cell:" << myCellId 
        //           << " NextHop:" << nextHopId << " NextCell:" << nextCellId << std::endl;
        if (!announcementQueue.empty()) {
            double delay = GetRandomDelay(0.1, 0.5);
            ScheduleTimer(SEND_ANNOUNCEMENT_QUEUE, delay);
        }
        return;
    }
    
    // Send packet with trace
    // std::cout << "#CHA_SEND Node:" << self << " Cell:" << myCellId 
    //           << " -> NextHop:" << nextHopId << " NextCell:" << nextCellId 
    //           << " TTL:" << ttl << std::endl;
    
    SendPacket(pkt, header, nextHopId);
    
    // Track CHA timing for rotation measurement
    if (g_rotationStartTime > 0) {
        g_CHsProcessedCHA.insert(self);
        g_lastCHACompleteTime = Simulator::Now().GetSeconds();
    }
    
    // Schedule next queue processing only if queue is not empty
    if (!announcementQueue.empty()) {
        double delay = GetRandomDelay(0.1, 0.5);
        ScheduleTimer(SEND_ANNOUNCEMENT_QUEUE, delay);
    }
}

void PeceeRoutingProtocol::selectClusterHead()
{
    NS_LOG_FUNCTION(this);
    
    // Update CL status if this node is now the CL
    PeceeCellData* cellData = getCellData(myCellId);
    if (cellData && cellData->clId == self) {
        isCL = true;
        myCLId = self;
        if (self == 11) {
            std::cout << "#DEBUG_NODE11_IS_CL Cell:" << myCellId << " CH:" << myCHId << std::endl;
        }
    }
    
    NS_LOG_INFO("#CH_SELECTION " << self << ": " << myCHId << (isCL ? " [IS_CL]" : ""));
    
    PeceeNodeData* node = getNodeData(self);
    if (node) {
        node->chId = myCHId;
    }
    
    double delay = GetRandomDelay(10, 20);
    ScheduleTimer(ANNOUNCE_CELL_HOP_TIMER, delay);
}
/*
void PeceeRoutingProtocol::sendCellHopAnnouncementPacket()
{
    // TODO - Uses PeceeRoutingProtocolPacket*
}

void PeceeRoutingProtocol::handleCellHopAnnouncementPacket(Ptr<Packet> pkt)
{
    // TODO - Uses packet methods that need header extraction
}

void PeceeRoutingProtocol::sendSensorDataPacket()
{
    // TODO - Uses PeceeRoutingProtocolPacket* and resModule
}

void PeceeRoutingProtocol::handleSensorDataPacket(Ptr<Packet> pkt)
{
    // TODO - Complex packet handling logic
}

void PeceeRoutingProtocol::sendCellPacket()
{
    // TODO - Complex queue management and packet operations
}

void PeceeRoutingProtocol::savePacketCopy(Ptr<Packet> pkt, int des)
{
    // TODO - Packet data extraction
}

void PeceeRoutingProtocol::overhearingPacket()
{
    // TODO - Packet tracking logic
}
*/

// Stub implementations to allow compilation
void PeceeRoutingProtocol::sendCellHopAnnouncementPacket()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_WARN("sendCellHopAnnouncementPacket not yet implemented");
    // TODO: Convert from OMNeT++ style
}

void PeceeRoutingProtocol::handleCellHopAnnouncementPacket(Ptr<Packet> pkt)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_WARN("handleCellHopAnnouncementPacket not yet implemented");
    // TODO: Convert from OMNeT++ style  
}

void PeceeRoutingProtocol::sendSensorDataPacket()
{
    NS_LOG_FUNCTION(this);
    
    // Skip if routing table is empty or no CH assigned
    if (g_ssRoutingTable.find(self) == g_ssRoutingTable.end() || 
        g_ssRoutingTable[self].empty() || myCHId == -1) {
        std::cout << "#DATA_SKIP Node:" << self << " NoRoutingTable or NoCH" << std::endl;
        return;
    }
    
    // Pick first non-self destination from routing table (or random)
    int destination = -1;
    for (const auto& entry : g_ssRoutingTable[self]) {
        int cellId = entry.first;
        int nextHop = entry.second;
        if (cellId != myCellId && nextHop != -1 && nextHop != self) {
            destination = cellId;  // Send to first valid cell
            break;
        }
    }
    
    if (destination == -1) {
        std::cout << "#DATA_SKIP Node:" << self << " NoValidDestination" << std::endl;
        return;
    }
    
    // Create data packet
    Ptr<Packet> pkt = Create<Packet>(100);  // 100 bytes payload
    PeceeHeader header;
    
    header.SetPacketType(SENSOR_DATA);
    header.SetSource(self);
    header.SetDestination(destination);
    header.SetCellSource(myCellId);
    header.SetCellDestination(destination);
    header.SetTtl(30);
    header.SetSequenceNumber(++g_ssSensorDataArr[self]);  // Increment sequence number
    
    // std::cout << "#DATA_SEND Node:" << self << " Cell:" << myCellId 
    //           << " -> Dst:" << destination << " Seq:" << g_ssSensorDataArr[self]
    //           << " Time:" << Simulator::Now().GetSeconds() << "s" << std::endl;
    
    // Get next hop from routing table
    int nextHop = g_ssRoutingTable[self][destination];
    if (nextHop == -1 || nextHop == self) {
        std::cout << "#DATA_NO_ROUTE Node:" << self << " -> Cell:" << destination << std::endl;
        return;
    }
    
    SendPacket(pkt, header, nextHop);
    
    // Schedule next data packet (periodic sending)
    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
    double nextDelay = rand->GetValue(5.0, 10.0);  // Send every 5-10 seconds
    Simulator::Schedule(Seconds(nextDelay), &PeceeRoutingProtocol::sendSensorDataPacket, this);
}

void PeceeRoutingProtocol::handleSensorDataPacket(Ptr<Packet> pkt)
{
    NS_LOG_FUNCTION(this);
    
    PeceeHeader header;
    pkt->PeekHeader(header);
    
    int source = header.GetSource();
    int destination = header.GetCellDestination();
    int ttl = header.GetTtl();
    int seqNum = header.GetSequenceNumber();
    
    std::cout << "#DATA_RECV Node:" << self << " Cell:" << myCellId 
              << " From:" << source << " Dst:" << destination 
              << " Seq:" << seqNum << " TTL:" << ttl << std::endl;
    
    // Check if this node is the destination
    if (destination == myCellId) {
        std::cout << "#DATA_DELIVERED Node:" << self << " Cell:" << myCellId 
                  << " From:" << source << " Seq:" << seqNum 
                  << " Time:" << Simulator::Now().GetSeconds() << "s" << std::endl;
        return;  // Packet delivered
    }
    
    // TTL check
    if (ttl <= 0) {
        std::cout << "#DATA_TTL_EXPIRED Node:" << self << " From:" << source 
                  << " Dst:" << destination << std::endl;
        return;
    }
    
    // Forward packet
    if (g_ssRoutingTable.find(self) == g_ssRoutingTable.end() ||
        g_ssRoutingTable[self].find(destination) == g_ssRoutingTable[self].end()) {
        std::cout << "#DATA_NO_ROUTE Node:" << self << " -> Cell:" << destination << std::endl;
        return;
    }
    
    int nextHop = g_ssRoutingTable[self][destination];
    if (nextHop == -1 || nextHop == self) {
        std::cout << "#DATA_NO_NEXTHOP Node:" << self << " -> Cell:" << destination << std::endl;
        return;
    }
    
    // Create forwarded packet
    Ptr<Packet> fwdPkt = pkt->Copy();
    PeceeHeader fwdHeader = header;
    fwdHeader.SetTtl(ttl - 1);
    
    std::cout << "#DATA_FORWARD Node:" << self << " Cell:" << myCellId 
              << " -> NextHop:" << nextHop << " Dst:" << destination 
              << " Seq:" << seqNum << " TTL:" << (ttl - 1) << std::endl;
    
    SendPacket(fwdPkt, fwdHeader, nextHop);
}

void PeceeRoutingProtocol::sendCellPacket()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_WARN("sendCellPacket not yet implemented");
    // TODO: Convert from OMNeT++ style
}

void PeceeRoutingProtocol::savePacketCopy(Ptr<Packet> pkt, int des)
{
    NS_LOG_FUNCTION(this << des);
    // TODO: Convert from OMNeT++ style
}

void PeceeRoutingProtocol::overhearingPacket()
{
    NS_LOG_FUNCTION(this);
    // TODO: Convert from OMNeT++ style
}

void PeceeRoutingProtocol::rotationCH()
{
    NS_LOG_FUNCTION(this);
    
    // Only node 0 manages rotation state
    if (self == 0) {
        g_rotationCount++;
        g_rotationStartTime = Simulator::Now().GetSeconds();
        g_CHsProcessedCHA.clear();
        std::cout << "\n======== CH ROTATION #" << g_rotationCount 
                  << " START at " << g_rotationStartTime << "s ========" << std::endl;
    }
    
    // Random CH selection per cell
    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
    
    for (auto& cellData : g_ssCellDataList) {
        if (cellData.members.empty()) {
            continue;
        }
        
        int randomIndex = rand->GetInteger(0, cellData.members.size() - 1);
        int newCHId = cellData.members[randomIndex];
        int oldCHId = cellData.chId;
        
        // Update cell CH
        cellData.chId = newCHId;
        
        // Update all nodes in this cell
        for (auto& nodeData : g_ssNodesDataList) {
            if (nodeData.cellId == cellData.cellId) {
                if (nodeData.id == oldCHId) {
                    nodeData.isCH = false;
                    nodeData.chId = newCHId;
                }
                if (nodeData.id == newCHId) {
                    nodeData.isCH = true;
                    nodeData.chId = newCHId;
                }
                // All members point to new CH
                nodeData.chId = newCHId;
            }
        }
        
        if (self == 0) {
            std::cout << "#CH_ROTATE Cell:" << cellData.cellId 
                      << " OldCH:" << oldCHId << " NewCH:" << newCHId << std::endl;
        }
    }
    
    // Update local node state
    PeceeNodeData* myNodeData = getNodeData(self);
    if (myNodeData) {
        isCH = myNodeData->isCH;
        myCHId = myNodeData->chId;
        
        // Reset rotation-related state
        receivedCHAFromCells.clear();
        announcementQueue = std::queue<std::pair<Ptr<Packet>, int>>();
        
        if (self == 0) {
            std::cout << "#NODE_UPDATE Node:" << self << " isCH:" << isCH 
                      << " myCHId:" << myCHId << std::endl;
        }
    }
    
    // New CHs broadcast announcement
    if (isCH) {
        Simulator::Schedule(Seconds(2.0), &PeceeRoutingProtocol::sendCHAnnouncement, this);
        //std::cout << "#CHA_SCHEDULED_ROTATION Node:" << self << " as new CH" << std::endl;
    }
    
    // Schedule next rotation
    Simulator::Schedule(Seconds(20.0), &PeceeRoutingProtocol::rotationCH, this);
    
    // Schedule completion check (only node 0)
    if (self == 0) {
        Simulator::Schedule(Seconds(5.0), &PeceeRoutingProtocol::checkCHACompletion, this);
        std::cout << "======== CH ROTATION #" << g_rotationCount 
                  << " STATE_UPDATED at " << Simulator::Now().GetSeconds() 
                  << "s ========\n" << std::endl;
    }
}

void PeceeRoutingProtocol::checkCHACompletion()
{
    NS_LOG_FUNCTION(this);
    
    if (g_rotationStartTime > 0 && g_lastCHACompleteTime > g_rotationStartTime) {
        double duration = g_lastCHACompleteTime - g_rotationStartTime;
        std::cout << "#CHA_COMPLETED_ROTATION #" << g_rotationCount 
                  << " Duration:" << duration << "s (LastSendAt:" 
                  << g_lastCHACompleteTime << "s, CHsActive:" 
                  << g_CHsProcessedCHA.size() << ")" << std::endl;
        g_rotationStartTime = 0.0;
    }
}

void PeceeRoutingProtocol::ScheduleTimer(PeceeTimerType timerType, double delaySeconds)
{
    NS_LOG_FUNCTION(this << timerType << delaySeconds);
    
    switch(timerType) {
        case SEND_ANNOUNCEMENT_QUEUE:
            Simulator::Schedule(Seconds(delaySeconds), 
                              &PeceeRoutingProtocol::sendAnnouncementQueue, this);
            break;
        case ANNOUNCE_CELL_HOP_TIMER:
            Simulator::Schedule(Seconds(delaySeconds), 
                              &PeceeRoutingProtocol::sendCellHopAnnouncementPacket, this);
            break;
        case COLOR_SCHEDULING_TIMER:
            Simulator::Schedule(Seconds(delaySeconds), 
                              &PeceeRoutingProtocol::sendSensorDataPacket, this);
            break;
        case SEND_CELL_PACKET:
            Simulator::Schedule(Seconds(delaySeconds), 
                              &PeceeRoutingProtocol::sendCellPacket, this);
            break;
        case CH_ROTATION_TIMER:
            Simulator::Schedule(Seconds(delaySeconds), 
                              &PeceeRoutingProtocol::rotationCH, this);
            break;
        default:
            NS_LOG_WARN("Unknown timer type: " << timerType);
            break;
    }
}

Ptr<Packet> PeceeRoutingProtocol::CreatePeceePacket(PeceePacketType type)
{
    Ptr<Packet> packet = Create<Packet>();
    PeceeHeader header;
    header.SetPacketType(type);
    header.SetSource(self);
    header.SetDestination(65535); // Broadcast by default
    return packet;
}

PeceeHeader PeceeRoutingProtocol::ExtractPeceeHeader(Ptr<Packet> packet)
{
    PeceeHeader header;
    packet->PeekHeader(header);
    return header;
}

void PeceeRoutingProtocol::SendPacket(Ptr<Packet> packet, PeceeHeader& header, uint16_t destination)
{
    NS_LOG_FUNCTION(this << destination);
    
    //std::cout << "#PECEE_SENDPACKET Node:" << self << " -> Dst:" << destination << std::endl;
    
    // Add PECEE header first
    packet->AddHeader(header);
    
    // Add WsnRoutingHeader for forwarder compatibility
    WsnRoutingHeader wsnHeader;
    wsnHeader.SetSource(self);
    wsnHeader.SetDestination(destination);
    packet->AddHeader(wsnHeader);
    
    // Update statistics
    PeceeNodeData* node = getNodeData(self);
    if (node) {
        node->numSent++;
        node->energyConsumtion += calculateConsumption(destination, 1);
    }
    
    // Use BROADCAST address at MAC layer, filter at routing layer
    //std::cout << "#PECEE_TOMAC Node:" << self << " -> Dst:" << destination 
    //          << " MAC:" << 0xFFFF << " PktSize:" << packet->GetSize() << std::endl;
    ToMacLayer(packet, 0xFFFF);
    //std::cout << "#PECEE_TOMAC_DONE Node:" << self << " -> Dst:" << destination << std::endl;
}

int PeceeRoutingProtocol::GetPathLength()
{
    int length = 0;
    for (int i = 0; i < maxHopCount; ++i) {
        if (myCellPathToCH[i] == -1) {
            break;
        }
        length++;
    }
    return length;
}

double PeceeRoutingProtocol::calculateConsumption(int desNode, int type)
{
    // TODO: Implement energy consumption calculation
    // For now, return a placeholder value
    return 0.001;  // 1mJ per packet
}

double PeceeRoutingProtocol::GetRandomDelay(double minMs, double maxMs)
{
    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
    return rand->GetValue(minMs, maxMs) / 1000.0; // Convert to seconds
}

}
}