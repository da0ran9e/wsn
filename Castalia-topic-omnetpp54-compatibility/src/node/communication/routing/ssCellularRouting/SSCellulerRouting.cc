#include "node/communication/routing/ssCellularRouting/SSCellularRouting.h"

Define_Module(SSCellularRouting);
static int g_CELLPkt=43470;
static int g_DATAPkt=0;
void SSCellularRouting::startup()
{
    myCLId = -1;
    myCHId = -1;
    isCH = false;
    isCL = false;
    myCellId = -1;
    myColor = -1;

    cModule *parentNode = getParentModule();
    numberOfNodes = parentNode->getParentModule()->getParentModule()->par("numNodes");
    myX = parentNode->getParentModule()->par("xCoor");
    myY = parentNode->getParentModule()->par("yCoor");
    isCH = par("isCH");
    maxHopCount = numberOfNodes; 
    resModule = check_and_cast <ResourceManager*>(getParentModule()->getParentModule()->getSubmodule("ResourceManager"));

    SSCRNodeData nodeData;
    nodeData.id = self;
    nodeData.x = myX;
    nodeData.y = myY;
    nodeData.isCH = isCH;
    nodeData.isCL = isCL;
    if (traceMode == 0) trace() << "#NODE " << nodeData.id << " (" << nodeData.x << ", " << nodeData.y << ")";
    calculateCellInfo();
    nodeData.cellId = myCellId;
    nodeData.color = myColor;
    nodeData.clId = myCLId;
    nodeData.chId = myCHId;
    nodeData.numSent = 0;
    nodeData.numRecv = 0;
    nodeData.energyConsumtion = 0.0;
    nodeData.el = 100;
    if (traceMode == 0) trace() << "#CELL_COLOR " << self << ": " << myColor;
    g_ssNodesDataList.push_back(nodeData);
        if (!g_isPrecalculated){
            g_isPrecalculated = true;
            setTimer(PRECALCULATE_TIMERS, 100);
        }

    setTimer(RECONFIGURATION_TIMER, 200);
    if (traceMode == 1) trace() << "******************** Round " << g_rotationCount << " ********************";
}

void SSCellularRouting::fromApplicationLayer(cPacket * pkt, const char *destination)
{
    SSCellularRoutingPacket *netPacket = new SSCellularRoutingPacket("SSCellularRouting packet", NETWORK_LAYER_PACKET);
    //netPacket->setByteLength(netPacket->getByteLength()/20);
    netPacket->setSource(SELF_NETWORK_ADDRESS);
    netPacket->setDestination(destination);
    encapsulatePacket(netPacket, pkt);
    toMacLayer(netPacket, resolveNetworkAddress(destination));
}

void SSCellularRouting::fromMacLayer(cPacket * pkt, int srcMacAddress, double rssi, double lqi)
{
    SSCellularRoutingPacket *netPacket = dynamic_cast<SSCellularRoutingPacket*>(pkt);
    if (!netPacket) {
        return;
    }
    resModule->consumeEnergy(0.0000001);
    SSCRNodeData* rNode = getNodeData(self);
    rNode->numRecv++;
    rNode->energyConsumtion+=calculateConsumption(srcMacAddress, 0);

    switch (netPacket->getPacketType()) {
        case CH_ANNOUNCEMENT_PACKET:
            handleCHAnnouncementPacket(netPacket);
            if(g_rotationCount>1) g_CELLPkt++;
            break;

        case ANNOUNCE_CELL_HOP:
            handleCellHopAnnouncementPacket(netPacket);
            if(g_rotationCount>1) g_CELLPkt++;
            break;

        case SENSOR_DATA:
            handleSensorDataPacket(netPacket);
            g_DATAPkt++;
            break;

        case FINALIZE_PKT:
            break;

        default:
            trace() << "SSCellularRouting::fromMacLayer() - Error: unknown packet type!";
            break;
    }
}

void SSCellularRouting::timerFiredCallback(int index)
{
    switch (index) {
        case PRECALCULATE_TIMERS:
            PrecalculateSimulationResults();
            break;

        case CH_ADVERTISMENT_TIMER:
            g_isCHRotation = false;
            sendCHAnnouncement();
            break;
        case SEND_ANNOUNCEMENT_QUEUE:
            sendAnnouncementQueue();
            setTimer(SEND_ANNOUNCEMENT_QUEUE, uniform(0, 10));
            break;
        case SEND_CELL_PACKET:
            sendCellPacket();
            setTimer(SEND_CELL_PACKET, uniform(0, 1));
            break;
        case ANNOUNCE_CELL_HOP_TIMER:
            sendCellHopAnnouncementPacket();
            break;

        case SENSING_STATE:
            sendSensorDataPacket();
            //setTimer(SENSING_STATE, 5000); //uniform(9000,10000));
            break;

        case RECONFIGURATION_TIMER:
            cancelTimer(SENSING_STATE);
            for (int i = 0; i < maxHopCount; i++) {
                myCellPathToCH[i] = -1;
            }
            myCLId = getCellData(myCellId)->clId;
            myCHId = -1;
            if (myCLId == self) {
                isCL = true;
                myCLId = self;
                if (traceMode == 0) trace() << "#CELL_LEADER " << myCellId << ": " << self;
                for (int i=0; i<6; i++){
                    neighborCells[i] = getCellData(myCellId)->neighbors[i];
                }
            }
            ssSentPacket.clear();

            //g_ssCastaliaEnergy;
            isCH = false;
            setTimer(CH_ROTATION_SSCR, 200);
            setTimer(SENSING_STATE, uniform(1000, 1000 + 100));
            setTimer(OVERHEARING_TIMER, reconfigurationTime-2000);
            setTimer(RECONFIGURATION_TIMER, reconfigurationTime);
            break;
        case OVERHEARING_TIMER:
            overhearingPacket();
            break;
        case CH_ROTATION_SSCR:
            if (!g_isCHRotation) {
                rotationCH();
                g_ssSensorDataOverheared.clear();
            }
            if (isCH || getNodeData(self)->isCH) {
                isCH = true;
                myCHId = self;
                if (traceMode == 0 || traceMode == 1) trace() << "#CH " << self;
                setTimer(CH_ADVERTISMENT_TIMER, 200);
            }

            double energy = (resModule->getSpentEnergy());
            //trace() << "********* " << energy;
            getNodeData(self)->castaliaConsumtion = energy;
            break;

    }

}

double SSCellularRouting::calculateDistance(double x1, double y1, double x2, double y2) {
    return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
}

SSCRNodeData* SSCellularRouting::getNodeData(int nodeId) {
    for (auto& nodeData : g_ssNodesDataList) {
            if (nodeData.id == nodeId) {
                return &nodeData;
            }
        }
        SSCRNodeData* emptyData = new SSCRNodeData();
        return emptyData;
}

SSCRCellData* SSCellularRouting::getCellData(int cellId) {
    for (auto& cellData : g_ssCellDataList) {
        if (cellData.cellId == cellId) {
            return &cellData;
        }
    }
    SSCRCellData* emptyData = new SSCRCellData();
    return emptyData;
}

bool SSCellularRouting::isNodeInList(int nodeId, const vector<int>& nodeList) {
    for (int id : nodeList) {
            if (id == nodeId) {
                return true;
            }
        }
        return false;
}

void SSCellularRouting::PrecalculateSimulationResults()
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
                if (traceMode == 0) trace() << "#NEIGHBOR " << nodeData.id << ": " << otherNodeData.id;
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
            SSCRCellData cellData;
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
                    SSCRNodeData* neighbor = getNodeData(neighborId);
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
            SSCRNodeData* member = getNodeData(memberId);
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
                            SSCRNodeData* neighborNode = getNodeData(neighborNodeId);
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
                }
            }
            // update next hop ID for the CGW
            for (auto& nodeData : g_ssNodesDataList) {
                int nodeDataId = nodeData.id;
                if (nodeDataId == cellData.gateways[neighborId]) {
                    g_ssRoutingTable[nodeDataId][neighborId] = bestNGWId; // Set next hop to NGW
                    if (traceMode == 0) trace() << "#ROUTING_TABLE " << nodeDataId << " (" << cellData.cellId << ") -> " << g_ssRoutingTable[nodeDataId][neighborId] << " (" << neighborId << ")";
                }
            }
        }
    }

    // calculate intra-cell routing table for each cell gateways and CL
    for (auto& cellData : g_ssCellDataList) {
        for (int neighborId : cellData.neighbors) {
            int gatewayId = cellData.gateways[neighborId];
            if (gatewayId != -1) {
                SSCRNodeData* cellGateway = getNodeData(gatewayId);
                for (int memberId : cellData.members) {
                    SSCRNodeData* member = getNodeData(memberId);
                    bool isGatewayInRange = false;
                    if (memberId != gatewayId) {
                        g_ssRoutingTable[memberId][neighborId] = -1;
                        for (const int neighborNodeId : member->neighbors) {
                            if (neighborNodeId == gatewayId) {
                                isGatewayInRange = true;
                                g_ssRoutingTable[memberId][neighborId] = gatewayId;
                                if (traceMode == 0) trace() << "#ROUTING_TABLE " << memberId << " (" << member->cellId << ") -> " << g_ssRoutingTable[memberId][neighborId] << " (" << neighborId << ")";
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
                                    SSCRNodeData* neighborNode = getNodeData(neighborNodeId);
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
                                if (traceMode == 0) trace() << "#ROUTING_TABLE " << member->id << " (" << member->cellId << ") -> " << g_ssRoutingTable[member->id][neighborId] << " (" << neighborId << ")";
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
            SSCRNodeData* member = getNodeData(memberId);
            if (isNodeInList(cellData.clId, member->neighbors)) { // if CL is in range
                g_ssRoutingTable[memberId][cellData.cellId] = cellData.clId;
                if (traceMode == 0) trace() << "#ROUTING_TABLE " << memberId << " (" << cellData.cellId << ") -> " << g_ssRoutingTable[memberId][cellData.cellId] << " (" << cellData.cellId << ")";
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
                        SSCRNodeData* clNode = getNodeData(cellData.clId);
                        SSCRNodeData* neighbor = getNodeData(neighborId);
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
                    if (traceMode == 0) trace() << "#ROUTING_TABLE " << member->id << " (" << cellData.cellId << ") -> " << g_ssRoutingTable[member->id][cellData.cellId] << " (" << cellData.cellId << ")";
                }
            }
        }
    }
}

Point SSCellularRouting::calculateCellCenter(int cell_id) {
    int r = round((double)cell_id / grid_offset);
    int q = cell_id - r * grid_offset;

    Point center;
    center.x = cellRadius * (sqrt(3.0) * q + sqrt(3.0) / 2.0 * r);
    center.y = cellRadius * (3.0 / 2.0 * r);

    return center;
}

void SSCellularRouting::calculateCellInfo()
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

void SSCellularRouting::sendCHAnnouncement()
{
    SSCHAnnouncementInfo chInfo;
    chInfo.chId = self;

    for (const int neighborCellId : getCellData(myCellId)->neighbors) {
        SSCellularRoutingPacket* dupPkt = new SSCellularRoutingPacket("CH Announcement", NETWORK_LAYER_PACKET);
        //dupPkt->setByteLength(dupPkt->getByteLength()/20);
        dupPkt->setPacketType(CH_ANNOUNCEMENT_PACKET);
        dupPkt->setCellSource(myCellId);
        dupPkt->setCellHopCount(1);
        dupPkt->setCellDestination(-1);
        dupPkt->setCellPath(0, myCellId);
        dupPkt->setCellPath(1, -1);
        dupPkt->setTtl(maxHopCount);
        dupPkt->setCellSent(myCellId);
        dupPkt->setCHAnnouncementData(chInfo);
        dupPkt->setSource(SELF_NETWORK_ADDRESS);
        dupPkt->setCellNext(neighborCellId);
        int nextHopId = g_ssRoutingTable[self][neighborCellId];
        // check if node in range
        if (isNodeInList(nextHopId, getNodeData(self)->neighbors)) {
            announcementQueue.push({dupPkt, neighborCellId});
        } else {
            delete dupPkt;
        }
    }
    setTimer(SEND_ANNOUNCEMENT_QUEUE, uniform(1, 10));
    if (isCL) {
        myCHId = self;
        if (traceMode == 0) trace() << "#CH_SELECTION " << self << ": " << myCHId;

        myCellPathToCH[0] = myCellId;
        selectClusterHead();
    } else {
        SSCellularRoutingPacket* dupPkt = new SSCellularRoutingPacket("CH Announcement", NETWORK_LAYER_PACKET);
        //dupPkt->setByteLength(dupPkt->getByteLength()/20);
        dupPkt->setPacketType(CH_ANNOUNCEMENT_PACKET);
        dupPkt->setCellSource(myCellId);
        dupPkt->setCellHopCount(1);
        dupPkt->setCellDestination(-1);
        dupPkt->setCellPath(0, myCellId);
        dupPkt->setCellPath(1, -1);
        dupPkt->setTtl(maxHopCount);
        dupPkt->setCellSent(myCellId);
        dupPkt->setCHAnnouncementData(chInfo);
        dupPkt->setSource(SELF_NETWORK_ADDRESS);
        dupPkt->setCellNext(myCellId);
        announcementQueue.push({dupPkt, myCellId});
    }
}

void SSCellularRouting::handleCHAnnouncementPacket(SSCellularRoutingPacket* pkt)
{
    // If I am the CL, save my cell information to packet metadata
        // and forward the CH announcement to my cell members with destination as all neighbor cells
    int sourceId = atoi(pkt->getSource());
    if (traceMode == 0) trace() << "#CELL_MESSAGE " << sourceId << " -> " << self;
    if (isCL) {
        if (myCHId != -1) return;
        SSCHAnnouncementInfo chInfo = pkt->getCHAnnouncementData();
        int chId = chInfo.chId;
        myCHId = chId;

        selectClusterHead();
        int hopCount = pkt->getCellHopCount();
        for (int i = 0; i < maxHopCount; ++i) {
            myCellPathToCH[i] = pkt->getCellPath(i);
            if (i >= hopCount) {
                myCellPathToCH[i] = -1;
            }
        }
        
        if (myCellPathToCH[hopCount-1] != myCellId) {
            myCellPathToCH[hopCount] = myCellId;
            pkt->setCellHopCount(hopCount + 1);
            pkt->setCellPath(hopCount, myCellId);
        }

        int cellDestination = pkt->getCellDestination();
        int cellSent = pkt->getCellSent();
        
        pkt->setCellSent(myCellId);
        pkt->setSource(SELF_NETWORK_ADDRESS);

        if (cellDestination == -1) {
            for (int i=0; i<6; i++) {
                if (neighborCells[i] == -1 || neighborCells[i] == cellSent) {
                    continue;
                }
                SSCellularRoutingPacket* dupPkt = pkt->dup();
                dupPkt->setCellNext(neighborCells[i]);
                announcementQueue.push({dupPkt, neighborCells[i]});
            }
            setTimer(SEND_ANNOUNCEMENT_QUEUE, uniform(1, 10));
        }
        selectClusterHead();
    }

    if (!isCL) {
        int cellDestination = pkt->getCellDestination();
        int cellNext = pkt->getCellNext();

            pkt->setSource(SELF_NETWORK_ADDRESS);
            SSCellularRoutingPacket* dupPkt = pkt->dup();
            int des = g_ssRoutingTable[self][cellNext];
            announcementQueue.push({dupPkt, cellNext});
            setTimer(SEND_ANNOUNCEMENT_QUEUE, uniform(1, 10));
    }
}


void SSCellularRouting::sendAnnouncementQueue()
{
    if (!announcementQueue.empty()) {
        auto [pkt, nextCellId] = announcementQueue.front();
        announcementQueue.pop();

        pkt->setTtl(pkt->getTtl() - 1);
        if (pkt->getTtl() <= 0) {
            delete pkt;
            return;
        }
        int nextHopId = -1;
        if (nextCellId != myCellId){
            nextHopId = g_ssRoutingTable[self][nextCellId];
        } else {
            if (!boardcastAnnouncementQueue.empty()){
                nextHopId = boardcastAnnouncementQueue.front();
                boardcastAnnouncementQueue.pop();
            } else {
                nextHopId = myCLId;
            }
        }

        bool isInRange = false;
        for (int neighborNodeId : getNodeData(self)->neighbors) {
            if (neighborNodeId == nextHopId) {
                isInRange = true;
                break;
            }
        }

        if (!isInRange) {
            delete pkt;
            return;
        }
        SSCRNodeData* sNode = getNodeData(self);
        sNode->numSent++;
        sNode->energyConsumtion+=calculateConsumption(nextHopId, 1);
        toMacLayer(pkt, nextHopId);
    }
    setTimer(SEND_ANNOUNCEMENT_QUEUE, uniform(1, 10));
}

void SSCellularRouting::selectClusterHead()
{
    if (traceMode == 0) trace() << "#CH_SELECTION " << self << ": " << myCHId;
    getNodeData(self)->chId = myCHId;
    setTimer(ANNOUNCE_CELL_HOP_TIMER, uniform(10, 20));
}
void SSCellularRouting::sendCellHopAnnouncementPacket()
    {
        if (isCL) {
            for (int i=0; i<maxHopCount; i++){
                if (myCellPathToCH[i] == myCellId){
                    SSCellHopAnnouncementInfo pktInfo;
                    pktInfo.nextCell = myCellPathToCH[i-1];

                    for (int memberId : getCellData(myCellId)->members) {
                        if (memberId == self) {
                            continue;
                        }
                        SSCellularRoutingPacket* dupPkt = new SSCellularRoutingPacket("CH Announcement", NETWORK_LAYER_PACKET);
                        //dupPkt->setByteLength(dupPkt->getByteLength()/20);
                        dupPkt->setPacketType(ANNOUNCE_CELL_HOP);
                        dupPkt->setCellSource(myCellId);
                        dupPkt->setCellDestination(myCellId);
                        dupPkt->setClusterHead(myCHId);
                        dupPkt->setTtl(2);
                        for (int i = 0; i < maxHopCount; ++i) {
                            dupPkt->setCellPath(i, myCellPathToCH[i]);
                        }
                        dupPkt->setSSCellHopAnnouncementData(pktInfo);
                        dupPkt->setSource(SELF_NETWORK_ADDRESS);
                        dupPkt->setDestination(std::to_string(memberId).c_str());
//                        announcementQueue.push({dupPkt, myCellId});
//                        boardcastAnnouncementQueue.push(memberId);
                        SSCRNodeData* sNode = getNodeData(self);
                        sNode->numSent++;
                        sNode->energyConsumtion+=calculateConsumption(memberId, 1);
                        toMacLayer(dupPkt, memberId);
                    }
                }
            }
            setTimer(SEND_ANNOUNCEMENT_QUEUE, uniform(1, 10));
        }
    }

void SSCellularRouting::handleCellHopAnnouncementPacket(SSCellularRoutingPacket* pkt)
    {
        SSCellHopAnnouncementInfo pktInfo = pkt->getSSCellHopAnnouncementData();
        int nextCell = pktInfo.nextCell;
        if (nextCell != -1) {


            for (int i = 0; i < maxHopCount; ++i) {
                myCellPathToCH[i] = pkt->getCellPath(i);
            }

            myCHId = pkt->getClusterHead();

            if (traceMode == 0) trace() << "#CH_SELECTION " << self << ": " << myCHId;

            setTimer(COLOR_SCHEDULING_TIMER, colorTimeSlot*myColor);
        }
    }

void SSCellularRouting::sendSensorDataPacket(){
    getNodeData(self)->controlPacketCount = getNodeData(self)->numSent;
    getNodeData(self)->controlPacketsConsumtion = resModule->getSpentEnergy() - getNodeData(self)->castaliaConsumtion;
    SSSensorInfo sensorData;
    sensorData.dataId = simTime().dbl();

    g_ssSensorData[self] = sensorData.dataId;
    g_ssSensorDataSent.push_back((sensorData.dataId*100 + self));
    g_ssSensorDataSentCount ++;

    sensorData.sensorId = self;
    sensorData.hopCount = 0;
    sensorData.destinationCH = myCHId;

    for (int i=0; i<sensorDataDub; i++) {
        SSCellularRoutingPacket* pkt = new SSCellularRoutingPacket("Sensing State 1", NETWORK_LAYER_PACKET);
        //pkt->setByteLength(13);
        for (int i = 0; i < maxHopCount; ++i) {
           pkt->setCellPath(i, myCellPathToCH[i]);
           if (myCellPathToCH[i] == myCellId){
               pkt->setCellNext(myCellPathToCH[i-1]);
           }
        }
            pkt->setPacketType(SENSOR_DATA);
            pkt->setCellSource(myCellId);
            pkt->setCellSent(myCellId);
            pkt->setCellDestination(myCellPathToCH[0]);
            pkt->setTtl(maxHopCount);
            pkt->setSensorData(sensorData);
            pkt->setSource(SELF_NETWORK_ADDRESS);
        cellPacketQueue.push({pkt, pkt->getCellNext()});
    }
    setTimer(SEND_CELL_PACKET, uniform(0, 1));
}

void SSCellularRouting::handleSensorDataPacket(SSCellularRoutingPacket* pkt){
    if (dataFusion) return;
    SSSensorInfo sensorData = pkt->getSensorData();
    if (myCHId == self && myCellId == pkt->getCellDestination()) {
        if (traceMode == 0) trace() << "Processing sensor data from  " << sensorData.sensorId << " hop count " << sensorData.hopCount;
        if (g_ssSensorData[sensorData.sensorId] == sensorData.dataId) {
            g_ssSensorDataArr[sensorData.sensorId] = sensorData.dataId;
        }
        g_ssSensorDataReceivedCount++;
        g_ssSensorDataReceived.push_back(sensorData.dataId*100 + sensorData.sensorId);
        savePacketCopy(pkt, -1);
        SSCellularRoutingPacket* dupPkt = new SSCellularRoutingPacket("Finalize", NETWORK_LAYER_PACKET);
        dupPkt->setPacketType(FINALIZE_PKT);
        toMacLayer(dupPkt, -1);
        if (traceMode == 0) trace() << "Received " << g_ssSensorDataReceivedCount << "/" << g_ssSensorDataSentCount << " sensor data pkts";
        return;
    }

    sensorData.hopCount++;

   SSCellularRoutingPacket* dupPkt = new SSCellularRoutingPacket("Sensing State 2", NETWORK_LAYER_PACKET);
   //dupPkt->setByteLength(dupPkt->getByteLength()/20);
    int prevCell = pkt->getCellSent();

    dupPkt->setCellSent(myCellId);
    dupPkt->setPacketType(SENSOR_DATA);
    dupPkt->setCellSource(pkt->getCellSource());
    dupPkt->setCellNext(pkt->getCellNext());
    dupPkt->setCellDestination(pkt->getCellDestination());
    dupPkt->setTtl(pkt->getTtl());
    for (int i = 0; i < maxHopCount; ++i) {
         dupPkt->setCellPath(i, pkt->getCellPath(i));
    }
    dupPkt->setCellNextNext(pkt->getCellNext());

    if (sensorData.destinationCH == -1) {
        for (int i = 0; i < maxHopCount; ++i) {
            dupPkt->setCellPath(i, myCellPathToCH[i]);
        }
        sensorData.destinationCH = myCHId;
    }

    dupPkt->setSensorData(sensorData);

    int nextCell = -1;
    for (int i = 0; i < maxHopCount; ++i) {
        int T = dupPkt->getCellPath(i);
        if (T == myCellId && i > 0) {
            nextCell = dupPkt->getCellPath(i-1);
            break;
        }
    }

    if (myCellId == nextCell || nextCell == -1) {
        if (prevCell == myCellId) {
            levelInCell = 1;
        } else {
            levelInCell = 2;
        }
    } else {
        levelInCell = 0;
    }
    cellPacketQueue.push({dupPkt, nextCell});
    setTimer(SEND_CELL_PACKET, uniform(0, 1));
}

void SSCellularRouting::sendCellPacket()
{
    if (!cellPacketQueue.empty()) {
        auto [pkt, nextCellId] = cellPacketQueue.top();
        if (pkt->getPacketType() == SENSOR_DATA) {

            double timeNow = simTime().dbl();
            double timeSlot = 600.0;
            double subTimeSlot = 60.0;
            double cycle = timeSlot * 3;
            double subCycle = subTimeSlot * 2;
            double phase = fmod(timeNow, cycle);
            double timeWithinSlot = phase - (myColor * timeSlot);
            double subPhase = fmod(timeWithinSlot, subCycle);

            bool isInTimeSlot = (phase >= myColor * timeSlot) && (phase < (myColor+1) * timeSlot);

            bool isInSubTimeSlot;
            switch (levelInCell) {
            case 0:
            case 2:
                if (subPhase < subTimeSlot) {
                    isInSubTimeSlot = true;
                }
                break;

            case 1:
                if (subPhase >= subTimeSlot) {
                    isInSubTimeSlot = true;
                }
                break;
        }

            if (!isInTimeSlot) {
                double nextStart = myColor * timeSlot;
                if (phase >= nextStart) {
                    nextStart += cycle;
                }
                double delay = nextStart - phase;

                setTimer(SEND_CELL_PACKET, delay);
                return;
            }

            if (!isInSubTimeSlot) {
                double delay = fmod(timeWithinSlot, subCycle);

                setTimer(SEND_CELL_PACKET, delay);
                return;
            }

            SSCellularRoutingPacket* dupPkt = new SSCellularRoutingPacket("Sensing State 3", NETWORK_LAYER_PACKET);
            //dupPkt->setByteLength(250);
            dupPkt->setPacketType(pkt->getPacketType());
                        dupPkt->setCellSource(pkt->getCellSource());
                        dupPkt->setCellSent(pkt->getCellSent());
                        dupPkt->setCellNext(pkt->getCellNext());
                        dupPkt->setCellDestination(pkt->getCellDestination());
                        for (int i = 0; i < maxHopCount; ++i) {
                            dupPkt->setCellPath(i, pkt->getCellPath(i));
                        }
                        dupPkt->setTtl(pkt->getTtl());
                        dupPkt->setSensorData(pkt->getSensorData());
            for (const auto& sspkt : ssSentPacket){
                if (sspkt.dataId == dupPkt->getSensorData().dataId){
                    delete dupPkt;
                    cellPacketQueue.pop();
                    return;
                }
            }

            if (nextCellId == -1 || nextCellId == myCellId) {
                cellPacketQueue.pop();

                if (myCHId != -1) {
                    bool chInRange = false;
                    for (int neighborNodeId : getNodeData(self)->neighbors) {
                        if (neighborNodeId == myCHId) {
                            chInRange = true;
                            break;
                        }
                    }

                    if (chInRange) {
                        SSCRNodeData* sNode = getNodeData(self);
                        sNode->numSent++;
                        sNode->energyConsumtion+=calculateConsumption(myCHId, 1);
                        toMacLayer(dupPkt, myCHId);
                        savePacketCopy(dupPkt, myCHId);
                        if (traceMode == 0) trace() << "#SENSOR_DATA: " << self << " -> " << myCHId;
                    } else {
                        SSCRNodeData* sNode = getNodeData(self);
                        sNode->numSent++;
                        sNode->energyConsumtion+=calculateConsumption(myCLId, 1);
                        toMacLayer(dupPkt, myCLId);
                        savePacketCopy(dupPkt, myCLId);
                        if (traceMode == 0) trace() << "#SENSOR_DATA: " << self << " -> " << myCLId;
                    }
                }
                delete pkt;
                return;
            }

            cellPacketQueue.pop();

            dupPkt->setTtl(pkt->getTtl() - 1);
            dupPkt->setSource(SELF_NETWORK_ADDRESS);
            if (dupPkt->getTtl() <= 0) {
                delete dupPkt;
                delete pkt;
                return;
            }

            int nextHopId = g_ssRoutingTable[self][nextCellId];
            // check if node in range
            bool isInRange = false;
            bool isInMyCell = true;
            for (int neighborNodeId : getNodeData(self)->neighbors) {
                if (neighborNodeId == nextHopId) {
                    isInRange = true;
                    //isInMyCell = (neighborNode.cellId == myCellId);
                    break;
                }
            }

            if (isInRange) {
                if (traceMode == 0) trace() << "#SENSOR_DATA: " << self << " -> " << nextHopId;
                SSCRNodeData* sNode = getNodeData(self);
                sNode->numSent++;
                sNode->energyConsumtion+=calculateConsumption(nextHopId, 1);
                toMacLayer(dupPkt, nextHopId);
                savePacketCopy(dupPkt, nextHopId);
                delete pkt;
            } else {
                for (int neighborNodeId : getNodeData(self)->neighbors) {
                    if (getNodeData(neighborNodeId)->cellId == myCellId) {
                        SSCRNodeData* sNode = getNodeData(self);
                        sNode->numSent++;
                        sNode->energyConsumtion+=calculateConsumption(neighborNodeId, 1);
                        toMacLayer(dupPkt, neighborNodeId);
                        savePacketCopy(dupPkt, neighborNodeId);
                        delete pkt;
                        break;
                    }
                }
            }
        }
        if (dataFusion){
            while (!cellPacketQueue.empty()){
                auto [opkt, nextCellId] = cellPacketQueue.top();
                delete opkt;
                cellPacketQueue.pop();
            }
        }
        setTimer(SEND_CELL_PACKET, uniform(0, 1));
    }
}

void SSCellularRouting::rotationCH(){
    g_rotationCount++;
    g_isCHRotation = true;
    double top1 = -1, top2 = -1, top3 = -1;
    int id1 = -1, id2 = -1, id3 = -1;
    double low1 = 999999, low2 = 999999, low3 = 999999;
    int lid1 = -1, lid2 = -1, lid3 = -1;
    double totalEnergy = 0.0;
    double totalCastaliaEnergy = 0.0;
    int n = numberOfNodes/33;
    int totalControl = 0;
    double totalControlCons = 0.0;
    int totalPkt = 0;
    double alpha = 0.9;
    int minSent = 99999;
    int minIdSent = -1;
    int Pkt1count = 0;
    int Pkt4count = 0;
    int totalControlPkts = 0;
    int totalPkts = 0;
    int totalControlBits = 0;
    int totalPktBits = 0;
    for (auto& ssNode : g_ssNodesDataList){
        ssNode.isCH = false;
        double eLeft = (initEnergy - ssNode.castaliaConsumtion);
        ssNode.el = (eLeft/initEnergy)*100;
        totalEnergy += ssNode.el;
        totalCastaliaEnergy += (eLeft/initEnergy)*100;
        totalControl += ssNode.controlPacketCount;
        totalControlCons += ssNode.controlPacketsConsumtion;
        totalPkt += ssNode.numSent;
        if (ssNode.numSent < minSent){
            minSent = ssNode.numSent;
            minIdSent = ssNode.id;
        }
        if (ssNode.numSent < 4) Pkt1count++;
        if (ssNode.numSent >= 4) Pkt4count++;
        ssNode.totalPacket += ssNode.numSent;
        ssNode.totalControlPacket += ssNode.controlPacketCount;
        ssNode.totalSensorPacket += ssNode.controlPacketCount;
        totalControlPkts = ssNode.totalControlPacket;
        totalPkts += ssNode.controlPacketCount;

        ssNode.numSent = 0;

        if (ssNode.el > top1) {
                top3 = top2; id3 = id2;
                top2 = top1; id2 = id1;
                top1 = ssNode.el;   id1 = ssNode.id;
            } else if (ssNode.el > top2) {
                top3 = top2; id3 = id2;
                top2 = ssNode.el;   id2 = ssNode.id;
            } else if (ssNode.el > top3) {
                top3 = ssNode.el;   id3 = ssNode.id;
            }
        if (ssNode.el < low1) {
                low3 = low2; lid3 = lid2;
                low2 = low1; lid2 = lid1;
                low1 = ssNode.el; lid1 = ssNode.id;
            } else if (ssNode.el < low2) {
                low3 = low2; lid3 = lid2;
                low2 = ssNode.el; lid2 = ssNode.id;
            } else if (ssNode.el < low3) {
                low3 = ssNode.el; lid3 = ssNode.id;
            }
        }

        double avgEnergy = totalEnergy / numberOfNodes;
        double avgCasEn = totalCastaliaEnergy / numberOfNodes;
        double rctControl = ((double)totalControl / (double)totalPkt)*100;
        double energyThreshold = alpha * avgEnergy;

        std::vector<int> selectedIds;
        std::vector<SSCRNodeData> ss_CHcandidates;
        for (const auto& node : g_ssNodesDataList) {
            if (node.el > avgEnergy){
                ss_CHcandidates.push_back(node);
            }

            if (node.isCH && node.el > energyThreshold ) {
                selectedIds.push_back(node.id);
            }
        }

        if (ss_CHcandidates.empty()) {
            getNodeData(id1)->isCH = true;
            getNodeData(id2)->isCH = true;
            getNodeData(id3)->isCH = true;
            return;
        }

        // Farthest-Point Sampling ---


        auto first = std::max_element(ss_CHcandidates.begin(), ss_CHcandidates.end(),
                                      [](const SSCRNodeData& a, const SSCRNodeData& b) {
                                          return a.el < b.el;
                                      });
        selectedIds.push_back(first->id);

        while (selectedIds.size() < static_cast<size_t>(n) && selectedIds.size() < ss_CHcandidates.size()) {
            double bestMinDist = -1.0;
            int bestNodeId = -1;

            for (const auto& cand : ss_CHcandidates) {
                if (std::find(selectedIds.begin(), selectedIds.end(), cand.id) != selectedIds.end())
                    continue;

                double minDistToSelected = DBL_MAX;
                for (int selId : selectedIds) {
                    auto it = std::find_if(ss_CHcandidates.begin(), ss_CHcandidates.end(),
                                           [selId](const SSCRNodeData& nd){ return nd.id == selId; });
                    if (it != ss_CHcandidates.end()) {
                        double d = sqrt(pow(cand.x - it->x, 2) + pow(cand.y - it->y, 2));
                        if (d < minDistToSelected) minDistToSelected = d;
                    }
                }

                // max of (min distance)
                if (minDistToSelected > bestMinDist) {
                    bestMinDist = minDistToSelected;
                    bestNodeId = cand.id;
                }
            }

            if (bestNodeId != -1)
                selectedIds.push_back(bestNodeId);
            else
                break;
        }
        if (traceMode == 1) trace() << "******************** Round " << g_rotationCount << " ********************";


        for (int id : selectedIds) {
            const auto it = std::find_if(g_ssNodesDataList.begin(), g_ssNodesDataList.end(),
                                         [id](const SSCRNodeData& n){ return n.id == id; });
            if (it != g_ssNodesDataList.end()) {
                getNodeData(id)->isCH = true;
                if (traceMode == 1) trace() << "New CHs: " << id
                          << " | EL: " << std::fixed << std::setprecision(2) << it->el << "%"
                          << " | Pos(" << it->x << ", " << it->y << ")";
            }
        }
        //trace () << " ************ " << getNodeData(lid1)->castaliaConsumtion;
        if ( traceMode == 1 ) trace()  << "MIN: " << lid1 << std::setprecision(5) << "(" << (initEnergy - getNodeData(lid1)->castaliaConsumtion)/initEnergy * 100
                                << "), AVG: "  << avgCasEn << " Controls/TotalBits: " << (totalControlPkts*100)/(totalControlPkts*100 + (totalPkts - totalControlPkts)*2000)*100 << " :( " << totalControlPkts <<  " / " << totalPkts << " )";
        if ( traceMode == 1 ) trace()  << "Min Pkts " << minSent << ": " <<  minSent << " Avg pkt " << totalPkt/numberOfNodes;
        if ( traceMode == 1 ) trace()  << " 1 Pkt " << Pkt1count << " *** " << ((double)g_CELLPkt*100 / ((double)g_CELLPkt*100 + (double)g_DATAPkt*2000))*100;
//    if (traceMode == 1) trace() << "Top Nodes: " << id1 << "(" << top1Pkt1count
//            << ") consumed " << getNodeData(id1)->energyConsumtion << " actual " << std::setprecision(5) << getNodeData(id1)->castaliaConsumtion << " J";
//    if (traceMode == 1) trace() << "Low Nodes: " << lid1 << "(" << low1
//                << "), consumed " << getNodeData(lid1)->energyConsumtion << " actual " << std::setprecision(5) << getNodeData(lid1)->castaliaConsumtion << " J";
//    trace() << "Max sent: " << getNodeData(lid1)->numSent << " Max recv: " << getNodeData(lid1)->numRecv;

}

double SSCellularRouting::calculateConsumption(int desNode, int type){
    double distance = sqrt(pow(getNodeData(desNode)->x - myX, 2) + pow(getNodeData(desNode)->y - myY, 2));
                    int kBits = 500;
                    if (type) kBits = 2000;
                    double E_elec = 50e-9;       // 50 nJ/bit
                    double eps_fs  = 10e-12;     // 10 pJ/bit/m^2
                    double eps_mp  = 0.0013e-12;
                    double d0 = std::sqrt(eps_fs/eps_mp);

                    if (distance < d0) {
                        return kBits * (E_elec + eps_fs * distance * distance);
                    } else {
                        return kBits * (E_elec + eps_mp * pow(distance, 4));
                    }
}

void SSCellularRouting::savePacketCopy(SSCellularRoutingPacket* pkt, int des){
    SSCRPacket newSSCRPacket;
    newSSCRPacket.nextHop = des;
    SSSensorInfo pktSensorInfo = pkt->getSensorData();
    newSSCRPacket.sensorId = pktSensorInfo.sensorId;
    newSSCRPacket.dataId = pktSensorInfo.dataId;
    newSSCRPacket.desCH = pktSensorInfo.destinationCH;
    newSSCRPacket.hopCount = pktSensorInfo.hopCount;
    newSSCRPacket.cellSource = pkt->getCellSource();
    newSSCRPacket.cellSent = pkt->getCellSent();
    newSSCRPacket.cellDes = pkt->getCellDestination();
    newSSCRPacket.ttl = pkt->getTtl();
    newSSCRPacket.source = atoi(pkt->getSource());
    for (int i=0; i<maxHopCount; i++){
        newSSCRPacket.cellPath.push_back(pkt->getCellPath(i));
    }
    g_ssSensorDataOverheared[self].push_back(newSSCRPacket);
    ssSentPacket.push_back(newSSCRPacket);
}

void SSCellularRouting::overhearingPacket(){
    for (const auto& sscrPkt:ssSentPacket){
        int des = sscrPkt.nextHop;
        if (des == -1) continue;
        bool foundPkt = false;
        for (const auto& overhearedPkt:g_ssSensorDataOverheared[des]){
            //trace() << sscrPkt.dataId << " : " << overhearedPkt.dataId;
            if (sscrPkt.dataId == overhearedPkt.dataId){
                foundPkt = true;
                break;
            }
        }
        //if (!foundPkt) trace() << "Pkt lost: " << sscrPkt.dataId << " from " << sscrPkt.sensorId;
    }
}
