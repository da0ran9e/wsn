#include "node/communication/routing/gstebRouting/GSTEBRouting.h"

Define_Module(GSTEBRouting);

static int g_GstebControlPkt=0;
static int g_GstebDataPkt=0;
void GSTEBRouting::startup()
{
    // TODO: startup
    // Initialize parameters: coordinates, communication interval, routing table, etc.
    nodeId = self;
    myCHId = -1;
    isCH = false;

    cModule *parentNode = getParentModule();
    xCoor = parentNode->getParentModule()->par("xCoor");
    yCoor = parentNode->getParentModule()->par("yCoor");
    if ( traceMode == 0 ) trace()  << "#NODE " << self << " (" << xCoor << ", " << yCoor << ")";
    energy = 100;
    isCH = par("isCH");
    alpha = par("alpha").doubleValue();
    numNodes = getParentModule()->getParentModule()->getParentModule()->par("numNodes");
    resModule = check_and_cast <ResourceManager*>(getParentModule()->getParentModule()->getSubmodule("ResourceManager"));

    GSTEBNodeInfo nodeInfo;
    nodeInfo.id = nodeId;
    nodeInfo.x = xCoor;
    nodeInfo.y = yCoor;

    nodeInfo.el = 100;
    nodeInfo.numSent = 0;
    nodeInfo.numRecv = 0;
    nodeInfo.consumtion = 0;
    nodeInfo.chId = chId;
    nodeInfo.isCH = isCH;
    nodeInfo.parentId = -1;
    nodeInfo.childrenIds.clear();
    nodeInfo.neighborIds.clear();
    g_GSTEBNodesInfo.push_back(nodeInfo);

    setTimer(CH_ROTATION, 0);

    if (!g_isVariableInitiated){
        g_isVariableInitiated = true;
        g_GSTEBPacketSent.clear();
        g_GSTEBPacketRecv.clear();
    }
}

void GSTEBRouting::fromApplicationLayer(cPacket * pkt, const char *destination)
{

}

void GSTEBRouting::fromMacLayer(cPacket * pkt, int srcMacAddress, double rssi, double lqi)
{
    // TODO: fromMac
    GSTEBRoutingPacket* netPacket = dynamic_cast<GSTEBRoutingPacket*>(pkt);
    if (!netPacket) {
        return;
    }

    resModule->consumeEnergy(0.0000001);

    GSTEBNodeInfo* sourcePktNode = getGSTEBNodeInfo(srcMacAddress);
    if (calculateDistance(sourcePktNode->x, sourcePktNode->y, xCoor, yCoor) > communicationRadius) return;
    GSTEBNodeInfo* recvNode = getGSTEBNodeInfo(self);
    recvNode->consumtion += calcTxEnergy(500, calculateDistance(atoi(netPacket->getSource())));
    recvNode->numRecv++;
    recvNode->tNumRecv++;

    netPacket->setTtl(netPacket->getTtl() - 1);

    switch (netPacket->getPacketType()) {

    case BS_BROADCAST_PACKET: {
        handleBSBroadcast(netPacket);
        g_GstebControlPkt++;
        break;
    }

    case SENSOR_BROADCAST_PACKET: {
        handleSensorBroadcast(netPacket);
        g_GstebControlPkt++;
        break;
    }

    case NEIGHBOR_BROADCAST_PACKET: {
        handleNeighborsTable(netPacket);
        g_GstebControlPkt++;
        break;
    }

    case NODE_CONTROL_PACKET: {
        handleRoutingTree(netPacket);
        g_GstebControlPkt++;
        break;
    }

    case NODE_INFO_PACKET: {
        handleInfoFromNode(netPacket);
        g_GstebControlPkt++;
        break;
    }

    case DATA_PACKET: {
        handleDataPacket(netPacket);
        g_GstebDataPkt++;
        break;
    }

    case FINALIZE:{
        break;
    }


    default:
        break;
    }
}

void GSTEBRouting::timerFiredCallback(int index)
{
    // TODO: timer
    switch (index) {
    case INITIAL_PHRASE: {
        //if ( traceMode == 0 ) trace()  << "#INITIAL*********";
        if (getGSTEBNodeInfo(self)->isCH) {
            isCH = true;
            chId = self;
            getGSTEBNodeInfo(self)->chId = self;
            if ( traceMode == 0 ) trace()  << "#CH " << self;
            setTimer(BS_BROADCAST, g_GSTEBTimeNow - simTime().dbl());
        }
        break;
    }

    case BS_BROADCAST: {
        setTimer(TREE_CONSTRUCTION_PHASE, numNodes*5*timeSlot); //initialPhraseTimeout); // Broadcast every 10 seconds
        sendBSBroadcast();
        break;
    }

    case FORWARD_BC_PKT: {
        toMacLayer(gstebBSPacket, -1);
        break;
    }

    case PHRASE_I_TIMESLOT: {
        //setTimer(PHRASE_I_TIMESLOT, phaseITimeslot);
        sendSensorBroadcast();
        break;
    }

    case SENSOR_BROADCAST_TIMEOUT: {
        // After all nodes send their information, each
        // node records a table in their memory which contains
        // the information of all its neighbors

        sendNeighborsTable();
        break;
    }

    case TREE_CONSTRUCTION_PHASE: {
        if (isCH) {
            // BS always assigns itself as root
            // BS can collect the initial EL and coordinates
            // information of all the sensor nodes in Initial Phase. For each
            // round, BS builds the routing tree and the schedule of the
            // network by using the EL and coordinates information.
            calculateRoutingTree();
            broadcastRoutingTree();
        } else {
            // Each node tries to select a parent in its neighbors
            // using EL and coordinates which are recorded in
            // Table I. The selection criteria are:
                // 1. distance between its parent node and the root
                // should be shorter than that between itself and the
                // root

                // 2. only the
                // nodes with the largest EL of all its neighbors
                // and itself can act as relay nodes
                // The relay node which causes
                // minimum consumption will be chosen as the
                // parent node.

            // If the sensor node cannot
            // find a suitable parent node, it will transmit its
            // data directly to BS.
            sendInfoToBS();
            //setTimer(DATA_COLLECTING_PHASE, 100);

        }
        // Because every node chooses the parent from its
        // neighbors and every node records its neighborsâ€™
        // information in Table II, each node can
        // know all its neighborsâ€™ parent nodes by computing,
        // and it can also know all its child nodes. If a node
        // has no child node, it defines itself as a leaf node,
        // from which the data transmitting begins.
        break;
    }

    case SELF_CONSTRUCTION_PHASE: {
        chooseRelayNode();
        chooseParentNode();
        break;
    }

    case DATA_COLLECTING_PHASE: {
        // Each node sends its data to its parent node
        // and finally to BS along the tree constructed
        // in Step 4.
        sendDataPacket();
        //setTimer(DATA_COLLECTING_PHASE, 5000);
        break;
    }

    case TDMA_DATA: {
                sendGSTEBTDMA();
                break;
            }

    case CH_ROTATION: {
//        /if ( traceMode == 0 ) trace()  << "#CH_ROTATION****************";
        if (isCH){
            for (const auto& nodeI : g_GSTEBNodesInfo){
                if (nodeI.chId == self){
                    GSTEBRoutingPacket *netPacket = new GSTEBRoutingPacket("GSTEBRouting packet", NETWORK_LAYER_PACKET);
                    netPacket->setPacketType(FINALIZE);
                    toMacLayer(netPacket, nodeI.id);
                    getGSTEBNodeInfo(self)->numSent++;
                    getGSTEBNodeInfo(self)->tNumSent++;
                }
            }
        }
        setTimer(CH_ROTATION, 10000);
        setTimer(INITIAL_PHRASE, 20);
        if (!g_isCHsRotated) rotationCH();
        myCHId = -1;
        chId = -1;
        isCH = false;
        tableI.clear();
        tableII.clear();
        parentId = -1;
        getGSTEBNodeInfo(self)->chId = -1;
        getGSTEBNodeInfo(self)->castaliaConsumtion = resModule->getSpentEnergy();
        break;
    }
    default:
        break;
    }
}

GSTEBNodeInfo* GSTEBRouting::getGSTEBNodeInfo(int nodeId){
    for (auto& nodeData : g_GSTEBNodesInfo) {
                if (nodeData.id == nodeId) {
                    return &nodeData;
                }
    }
}

void GSTEBRouting::sendBSBroadcast()
{
    // TODO: BS Broadcast
    // BS broadcasts a packet to all the nodes to inform them of
    // beginning time, the length of time slot and the number of nodes N
    g_isCHsRotated = false;
    GSTEBRoutingPacket *netPacket = new GSTEBRoutingPacket("GSTEBRouting packet", NETWORK_LAYER_PACKET);
    netPacket->setPacketType(BS_BROADCAST_PACKET);
    numNodes = getParentModule()->getParentModule()->getParentModule()->par("numNodes");
    GSTEBBSBroadcastInfo bSBroadcastData;
    bSBroadcastData.x = xCoor;
    bSBroadcastData.y = yCoor;
    bSBroadcastData.numNodes = numNodes;
    bSBroadcastData.timeSlot = timeSlot;
    bSBroadcastData.timeStart = timeStart;
    bSBroadcastData.senderX = xCoor;
    bSBroadcastData.senderY = yCoor;
    bSBroadcastData.sinkId = self;
    netPacket->setBSBroadcastData(bSBroadcastData);
    netPacket->setSource(SELF_NETWORK_ADDRESS);
    netPacket->setDestination(BROADCAST_NETWORK_ADDRESS);
    netPacket->setTtl(numNodes);
    g_receivedGSTEBBC.push_back(self);
    setTimer(PHRASE_I_TIMESLOT, (self+numNodes)*timeSlot+10);
    setTimer(SENSOR_BROADCAST_TIMEOUT, (self+numNodes*2)*timeSlot+10);

    GSTEBNodeInfo* sentNode = getGSTEBNodeInfo(self);
                            sentNode->consumtion += calcTxEnergy(2000, 80);
                            sentNode->numSent ++;
                            sentNode->tNumSent ++;
    toMacLayer(netPacket, BROADCAST_MAC_ADDRESS);
    //if ( traceMode == 0 ) trace()  << "BS broadcast " << numNodes;
}
void GSTEBRouting::handleBSBroadcast(GSTEBRoutingPacket* pkt)
{
    // they will compute their own energy-level (EL) using function
    // EL(i) = [residual energy(i) / alpha]

        // EL is a parameter for load balance,
        // and it is an estimated energy value
        // rather than a true one and only used in Case2

        // i is the ID of each node

        // is a constant which reflects the minimum energy unit
        // and can be changed depending on our demands
    computeEL();
    if (chId != -1 || isCH) return;
    int sourcePktId = atoi(pkt->getSource());
    GSTEBNodeInfo* sourcePktNode = getGSTEBNodeInfo(sourcePktId);
    if (calculateDistance(sourcePktNode->x, sourcePktNode->y, xCoor, yCoor) > communicationRadius) return;
    GSTEBBSBroadcastInfo bSBroadcastData = pkt->getBSBroadcastData();
    if (calculateDistance(bSBroadcastData.senderX, bSBroadcastData.senderY, xCoor, yCoor) > communicationRadius) return;
    chX = bSBroadcastData.x;
    chY = bSBroadcastData.y;
    chId = bSBroadcastData.sinkId;
    GSTEBNodeInfo* thisNode = getGSTEBNodeInfo(self);
    thisNode->chId = chId;
    thisNode->chx = chX;
    thisNode->chy = chY;
    numNodes = bSBroadcastData.numNodes;
    timeSlot = bSBroadcastData.timeSlot;
    bSBroadcastData.senderX = xCoor;
    bSBroadcastData.senderY = yCoor;
    prevHop = sourcePktId;
    //if ( traceMode == 0 ) trace()  << prevHop << " *** " << self;
    if ( traceMode == 0 ) trace()  << "#CH_SELECTION " << self << ": " << chId;
    setTimer(PHRASE_I_TIMESLOT, (self+numNodes)*timeSlot+10);
    setTimer(SENSOR_BROADCAST_TIMEOUT, (self+numNodes*2)*timeSlot+10);
    if (!dataFusion){
        setTimer(TREE_CONSTRUCTION_PHASE, (nodeId+3*numNodes)*timeSlot+10);
    } else {
        setTimer(SELF_CONSTRUCTION_PHASE, (nodeId+3*numNodes)*timeSlot+10);
    }

    gstebBSPacket = new GSTEBRoutingPacket("GSTEBRouting packet", NETWORK_LAYER_PACKET);
    gstebBSPacket->setPacketType(BS_BROADCAST_PACKET);
    gstebBSPacket->setTtl(pkt->getTtl());
    gstebBSPacket->setBSBroadcastData(bSBroadcastData);
    gstebBSPacket->setSource(SELF_NETWORK_ADDRESS);
    if (gstebBSPacket->getTtl() > 0) {
        GSTEBNodeInfo* sentNode = getGSTEBNodeInfo(self);
        sentNode->consumtion += calcTxEnergy(2000, 80);
        sentNode->numSent ++;
        sentNode->tNumSent ++;
        setTimer(FORWARD_BC_PKT, timeSlot);
        //toMacLayer(dupPkt, BROADCAST_MAC_ADDRESS);
    }
    g_receivedGSTEBBC.push_back(self);
}

void GSTEBRouting::computeEL()
{
    // TODO: compute el
    // EL(i) = [residual energy(i) / alpha]
    myEL = energy / alpha;
}

void GSTEBRouting::sendSensorBroadcast()
{
    // TODO: sensor BC
    // Each node sends its packet in a circle with a certain
    // radius during its own time slot
        // This packet contains a preamble
        // and the information such as coordinates and
        // EL of node i
    if (!usingHTCRDC){
    GSTEBRoutingPacket *netPacket = new GSTEBRoutingPacket("GSTEBRouting packet", NETWORK_LAYER_PACKET);
    netPacket->setPacketType(SENSOR_BROADCAST_PACKET);
    GSTEBSensorBroadcastInfo sensorBroadcastData;
    sensorBroadcastData.nX = xCoor;
    sensorBroadcastData.nY = yCoor;
    sensorBroadcastData.nEL = myEL;
    netPacket->setSensorBroadcastData(sensorBroadcastData);
    netPacket->setSource(SELF_NETWORK_ADDRESS);
    netPacket->setDestination(BROADCAST_NETWORK_ADDRESS);
    netPacket->setTtl(1);
    GSTEBNodeInfo* sentNode = getGSTEBNodeInfo(self);
            sentNode->consumtion += calcTxEnergy(2000, 80);
            sentNode->numSent ++;
            sentNode->tNumSent ++;
     toMacLayer(netPacket, BROADCAST_MAC_ADDRESS);}
    //if ( traceMode == 0 ) trace()  << "******";
}

void GSTEBRouting::handleSensorBroadcast(GSTEBRoutingPacket* pkt)
{
    // neighbors of node i, they can receive this packet
    // and record the information of node i in memory
    int sourceId = atoi(pkt->getSource());
    for (const auto& neighbor : tableI) {
        if (neighbor.nId == sourceId) {
            return; // already recorded
        }
    }

    GSTEBSensorBroadcastInfo sensorBroadcastData = pkt->getSensorBroadcastData();
    GSTEBNeighbors newNeighbor;
    newNeighbor.nId = sourceId;
    newNeighbor.nX = sensorBroadcastData.nX;
    newNeighbor.nY = sensorBroadcastData.nY;
    newNeighbor.nEL = sensorBroadcastData.nEL;
    // if neighbor is far away, consider it for removal
    if (calculateDistance(xCoor, yCoor, newNeighbor.nX, newNeighbor.nY) > communicationRadius) {
        return;
    }
    tableI.push_back(newNeighbor);
    getGSTEBNodeInfo(self)->neighborIds.push_back(sourceId);
    if ( traceMode == 0 ) trace()  << "#NEIGHBOR " << self << ": " << sourceId;
}

double GSTEBRouting::calculateDistance(double x1, double y1, double x2, double y2)
{
    return sqrt(pow(x1 - x2, 2.0) + pow(y1 - y2, 2.0));
}

double GSTEBRouting::calculateDistance(int n)
{
    for (const auto& nodeI : g_GSTEBNodesInfo) {
        if (nodeI.id == n) {
            return calculateDistance(nodeI.x, nodeI.y, xCoor, yCoor);
        }
    }
    return DBL_MAX;
}

void GSTEBRouting::sendNeighborsTable()
{
    // TODO: neighbor of neighbor
    // Each node sends a packet which contains all its
    // neighborsâ€™ information during its own time slot
    // when Step 2 is over.

    for (const auto& neighb : tableI){
        if (!usingHTCRDC) {
        GSTEBRoutingPacket *netPacket = new GSTEBRoutingPacket("GSTEBRouting packet", NETWORK_LAYER_PACKET);
        netPacket->setPacketType(NEIGHBOR_BROADCAST_PACKET);

        netPacket->setNnNumber(tableI.size());
        for (size_t i = 0; i < tableI.size(); ++i) {
            netPacket->setNnId(i, tableI[i].nId);
            netPacket->setNnXCoor(i, tableI[i].nX);
            netPacket->setNnYCoor(i, tableI[i].nY);
            netPacket->setNnEL(i, tableI[i].nEL);
        }
        netPacket->setSource(SELF_NETWORK_ADDRESS);
        netPacket->setDestination(BROADCAST_NETWORK_ADDRESS);
        netPacket->setTtl(1);
        double dist;
        for (auto& neiNode : g_GSTEBNodesInfo){
            if (neiNode.id == neighb.nId){
                dist = calculateDistance(neiNode.x, neiNode.y, xCoor, yCoor);
            }
        }

        GSTEBNodeInfo* sentNode = getGSTEBNodeInfo(self);
                    sentNode->consumtion += calcTxEnergy(2000, dist);
                    sentNode->numSent ++;
                    sentNode->tNumSent ++;
        toMacLayer(netPacket, neighb.nId);
        }
    }
}

void GSTEBRouting::handleNeighborsTable(GSTEBRoutingPacket* pkt)
{
    // Then its neighbors can receive
    // this packet and record the information in memory.
    int senderId = atoi(pkt->getSource());

    int nnNumber = pkt->getNnNumber();
    for (int i = 0; i < nnNumber; ++i) {
        GSTEBNeighborsOfNeighbors nnInfo;
        nnInfo.neighborId = senderId;
        nnInfo.nnId = pkt->getNnId(i);
        nnInfo.nnX = pkt->getNnXCoor(i);
        nnInfo.nnY = pkt->getNnYCoor(i);
        nnInfo.nnEL = pkt->getNnEL(i);
        tableII.push_back(nnInfo);
        if ( traceMode == 0 ) trace()  << "#NN " << self << ": "<< nnInfo.nnId;
    }

}

double GSTEBRouting::calcTxEnergy(int kBits, double distance) {
    // TODO: consumtion calculation
    // These params should exist as members or be read from par()
    // e.g., E_elec = par("E_elec"); eps_fs = par("eps_fs"); ...
    double E_elec = 50e-9;       // example: 50 nJ/bit
    double eps_fs  = 10e-12;     // example: 10 pJ/bit/m^2
    double eps_mp  = 0.0013e-12; // example for mp (you should set real values)
    double d0 = std::sqrt(eps_fs/eps_mp);

    if (distance < d0) {
        double cE = kBits * (E_elec + eps_fs * distance * distance);
        //if ( traceMode == 0 ) trace()  << "+++++++++++ " << cE;
        return cE;
    } else {
        double cE = kBits * (E_elec + eps_mp * pow(distance, 4));
        //if ( traceMode == 0 ) trace()  << "+++++++++++ " << cE;
        return cE;
    }
}

void GSTEBRouting::chooseRelayNode() {
    // TODO: relay
    // For each neighbor, check if it satisfies the conditions
    // If yes, calculate the energy consumption to send data via this neighbor
    // Choose the neighbor with minimum energy consumption as relay node

    for (const auto& neighbor : tableI) {
        double distToSink = calculateDistance(xCoor, yCoor, chX, chY);
        double myDistToSink = calculateDistance(xCoor, yCoor, chX, chY);
        if (distToSink >= myDistToSink) {
            continue; // Condition 1 not satisfied
        }

        bool isRelayCandidate = true;
        double highestEL = neighbor.nEL;

        for (const auto& nnRecord : tableII) {
            if (nnRecord.neighborId == neighbor.nId) {
                if (nnRecord.nnEL > highestEL) {
                    isRelayCandidate = false;
                    highestEL = nnRecord.nnEL;
                    break;
                }
            }
        }

        if (!isRelayCandidate) {
            continue; // Condition 2 not satisfied
        }

        relayCandidates.push_back(neighbor);
    }

    if (relayCandidates.empty()) {
        // if BS in range, send directly to BS
        for (const auto& neighbor : tableI) {
            if (neighbor.nId == chId) {
                parentId = chId;
                return;
            }
        }
        // else, choose the closest neighbor to BS
        double minDistToSink = 9999.0;
        int bestNeighborId = -1;
        for (const auto& neighbor : tableI) {
            double distToSink = calculateDistance(neighbor.nX, neighbor.nY, chX, chY);
            if (distToSink < minDistToSink) {
                minDistToSink = distToSink;
                bestNeighborId = neighbor.nId;
            }
        }
        if (bestNeighborId != -1) {
            parentId = bestNeighborId;
        }
    }
}

void GSTEBRouting::chooseParentNode() {
    // For each relay candidate, calculate the energy consumption
    // Choose the one with minimum energy consumption as parent node
    int minConsumption = 9999;
    int bestParentId = -1;

    for (const auto& candidate : relayCandidates) {
        double distanceToCandidate = calculateDistance(xCoor, yCoor, candidate.nX, candidate.nY);
        double distanceCandidateToSink = calculateDistance(candidate.nX, candidate.nY, chX, chY);
        double consumption = calcTxEnergy(2000, distanceToCandidate) + calcTxEnergy(2000, distanceCandidateToSink); // assuming 2000 bits

        if (consumption < minConsumption) {
            minConsumption = consumption;
            bestParentId = candidate.nId;
        }
    }

    if (bestParentId != -1) {
        parentId = bestParentId;
    }
}

void GSTEBRouting::sendInfoToBS()
{
    // TODO: Centralize
    GSTEBRoutingPacket *netPacket = new GSTEBRoutingPacket("GSTEBRouting packet", NETWORK_LAYER_PACKET);
    netPacket->setPacketType(NODE_INFO_PACKET);

    netPacket->setNNumber(tableI.size());
    netPacket->setNnNumber(tableII.size());
    for (size_t i = 0; i < tableI.size(); ++i) {
        netPacket->setNId(i, tableI[i].nId);
        netPacket->setNXCoor(i, tableI[i].nX);
        netPacket->setNYCoor(i, tableI[i].nY);
        netPacket->setNEL(i, tableI[i].nEL);
    }
    for (size_t i = 0; i < tableII.size(); ++i) {
        netPacket->setNnId(i, tableII[i].nnId);
        netPacket->setNNeighbor(i, tableII[i].neighborId);
        netPacket->setNnXCoor(i, tableII[i].nnX);
        netPacket->setNnYCoor(i, tableII[i].nnY);
        netPacket->setNnEL(i, tableII[i].nnEL);
    }
    netPacket->setSource(SELF_NETWORK_ADDRESS);
    double maxDistance = -1;
    for (const auto& node : tableI) {
        double distance = calculateDistance(xCoor, yCoor, node.nX, node.nY);
        if (distance > maxDistance) {
            maxDistance = distance;
        }
    }
    std::stringstream dest_addr;
    dest_addr << chId;
    netPacket->setDestination(dest_addr.str().c_str());
    netPacket->setTtl(numNodes);
    double dist;
            for (auto& neiNode : g_GSTEBNodesInfo){
                if (neiNode.id == chId){
                    dist = calculateDistance(neiNode.x, neiNode.y, xCoor, yCoor);
                }
            }

            //g_networkConsumption[self][0] += calcTxEnergy(2000, dist);
            GSTEBNodeInfo* sentNode = getGSTEBNodeInfo(self);
                        sentNode->consumtion += calcTxEnergy(2000, dist);
                        sentNode->numSent ++;
                        sentNode->tNumSent ++;
            if (prevHop != -1) {
        toMacLayer(netPacket, prevHop);
    } else {
        toMacLayer(netPacket, chId);
    }
    //if ( traceMode == 0 ) trace()  << "send info to " << chId;
}

void GSTEBRouting::handleInfoFromNode(GSTEBRoutingPacket* pkt)
{
    // BS receives the packets from all the nodes
    // and records all the information in memory.
    if (isCH){
        int senderId = atoi(pkt->getSource());
        int nNumber = pkt->getNNumber();

        for (int i = 0; i < nNumber; ++i) {
            GSTEBNeighbors newNode;
            newNode.nodeId = senderId;
            newNode.nNumber = nNumber;
            newNode.nId = pkt->getNId(i);
            newNode.nX = pkt->getNXCoor(i);
            newNode.nY = pkt->getNYCoor(i);
            newNode.nEL = pkt->getNEL(i);
            newNode.distanceToCH = calculateDistance(newNode.nX, newNode.nY, xCoor, yCoor);
            networkTableI.push_back(newNode);
        }
    } else {
        GSTEBRoutingPacket* dupPkt = pkt->dup();

        if (prevHop != -1) {
                toMacLayer(dupPkt, prevHop);
                GSTEBNodeInfo* sentNode = getGSTEBNodeInfo(self);
                                                sentNode->consumtion += calcTxEnergy(2000, calculateDistance(prevHop));
                                                sentNode->numSent ++;
                                                sentNode->tNumSent ++;
            } else {
                toMacLayer(dupPkt, chId);
                GSTEBNodeInfo* sentNode = getGSTEBNodeInfo(self);
                                                sentNode->consumtion += calcTxEnergy(2000, calculateDistance(chId));
                                                sentNode->numSent ++;
                                                sentNode->tNumSent ++;
            }
    }

    //if ( traceMode == 0 ) trace()  << "Set of neighbor " << networkTableI.size();
}

void GSTEBRouting::calculateRoutingTree()
{
    // TODO: calculate routing tree
    // BS builds the routing tree and the schedule of the
    // network by using the EL and coordinates information
    // collected in Initial Phase.
    // The routing tree is built in a top-down manner,
    // starting from BS. BS first selects its child nodes
    // from all the nodes in the network, then each child
    // node selects its own child nodes from its neighbors,
    // and so on and so forth until all the nodes are
    // included in the tree.

    // The selection criteria are:
        // 1. distance between its parent node and the root
        // should be shorter than that between itself and the
        // root

        // 2. only the
        // nodes with the largest EL of all its neighbors
        // and itself can act as relay nodes
        // The relay node which causes
        // minimum consumption will be chosen as the
        // parent node.

    // Because every node chooses the parent from its
        // neighbors and every node records its neighbors’
        // information in Table II, each node can
        // know all its neighbors’ parent nodes by computing,
        // and it can also know all its child nodes. If a node
        // has no child node, it defines itself as a leaf node,
        // from which the data transmitting begins.
    if ( traceMode == 0 ) trace()  << "*****Start calc routing tree";
    for (auto& node : g_GSTEBNodesInfo){
        node.distanceToCH = calculateDistance(node.chx, node.chy, node.x, node.y);
        if (usingHTCRDC) {
            for (auto& nn : g_GSTEBNodesInfo){
                double dis = calculateDistance(node.x, node.y, nn.x, nn.y);
                if (dis <= communicationRadius){
                    node.neighborIds.push_back(nn.id);
                }
            }
        }
    }
    SortByDistance sortByDistance;
    std::sort(g_GSTEBNodesInfo.begin(), g_GSTEBNodesInfo.end(), sortByDistance);
    getGSTEBNodeInfo(self)->hopToCH = 0;
    for (const auto& curNode : g_GSTEBNodesInfo) {
        if (curNode.chId != self || curNode.id == self) continue;
        vector<int> nRelayCandidates;
        int nParentId = -1;
        // find relay candidates
        for (const int nNode : curNode.neighborIds){
            if (nNode == curNode.chId) {
                nParentId = nNode;
                break;
            }
            GSTEBNodeInfo* neighNode = getGSTEBNodeInfo(nNode);
            double distToSink = calculateDistance(xCoor, yCoor, curNode.x, curNode.y);
            double nDistToSink = calculateDistance(xCoor, yCoor, neighNode->x, neighNode->y);
                            if (distToSink <= nDistToSink) {
                                continue; // Condition 1 not satisfied
                            }

            bool isRelayCandidate = true;
            double highestEL = curNode.el;

            for (const int curNNode : neighNode->neighborIds) {
                //if (curNNode == curNode.id) continue;
                if (getGSTEBNodeInfo(curNNode)->el > highestEL) {
                      isRelayCandidate = false;
                      break;
                }

             }

             if (!isRelayCandidate) {
                continue; // Condition 2 not satisfied
             }

            nRelayCandidates.push_back(nNode);
        }

        if (nParentId == -1){
            if (nRelayCandidates.empty()) {
                // if BS in range, send directly to BS
                if (calculateDistance(xCoor, yCoor, curNode.x, curNode.y) <= communicationRadius)
                {
                    nParentId = chId;
                    continue;
                }

                // else, choose the closest neighbor to BS
                double minDistToSink = 9999.0;
                int bestNeighborId = -1;
                for (const int curNNodeId : curNode.neighborIds) {
                    //if (curNNodeId == curNode.id) continue;
                    GSTEBNodeInfo* curNNode = getGSTEBNodeInfo(curNNodeId);
                    double distToSink = calculateDistance(curNNode->x, curNNode->y, xCoor, yCoor);
                    if (distToSink < minDistToSink) {
                        minDistToSink = distToSink;
                        bestNeighborId = curNNodeId;
                    }
                }
                if (bestNeighborId != -1) {
                    nParentId = bestNeighborId;
                }
            } else {
                // For each relay candidate, calculate the energy consumption
                // Choose the one with minimum energy consumption as parent node
                int minConsumption = 9999;
                int bestParentId = -1;

                for (const int curNNodeId : nRelayCandidates) {
                    //if (curNNodeId == curNode.id) continue;
                    GSTEBNodeInfo* curNNode = getGSTEBNodeInfo(curNNodeId);
                    double distanceToCandidate = calculateDistance(curNode.x, curNode.y, curNNode->x, curNNode->y);
                    double distanceCandidateToSink = calculateDistance(xCoor, yCoor, curNNode->x, curNNode->y);
                    double consumption = calcTxEnergy(2000, distanceToCandidate) + calcTxEnergy(2000, distanceCandidateToSink); // assuming 2000 bits

                    if (consumption < minConsumption) {
                        minConsumption = consumption;
                        bestParentId = curNNodeId;
                    }
                }

                if (bestParentId != -1) {
                    nParentId = bestParentId;
                }
            }
        }

        networkParentTable[curNode.id] = nParentId;
        g_GSTEBParentTable[curNode.id] = nParentId;
        getGSTEBNodeInfo(nParentId)->childrenIds.push_back(curNode.id);
        getGSTEBNodeInfo(curNode.id)->hopToCH = getGSTEBNodeInfo(nParentId)->hopToCH + 1;
        //if ( traceMode == 0 ) trace()  << "#PARENT_OF " << curNode.id << ": " << nParentId;
        if ( traceMode == 0 ) trace()  << "#ROUTING_TABLE " << curNode.id << " (1) -> " << nParentId << " (0)";
        if ( traceMode == 0 ) trace()  << "#HOP_COUNT " << getGSTEBNodeInfo(curNode.id)->hopToCH;
    }
    if (usingHTCRDC){
        for (const auto& curNode : g_GSTEBNodesInfo){
            int newParent = -1;
            int minDegree = numNodes;
            if (curNode.parentId == -1) continue;
            GSTEBNodeInfo* curNodeParent = getGSTEBNodeInfo(curNode.parentId);
            double ca = calculateDistance(curNode.x, curNode.y, curNodeParent->x, curNodeParent->y);
            for (const int curNodeNeighId : curNode.neighborIds){
                GSTEBNodeInfo* curNodeNeigh = getGSTEBNodeInfo(curNodeNeighId);

                if(curNodeNeigh->el/curNode.el < 1.5) continue;

                if (curNodeNeigh->hopToCH == curNode.hopToCH){
                    if (curNodeNeigh->el > curNodeParent->el){
                        double cd = calculateDistance(curNode.x, curNode.y, curNodeNeigh->x, curNodeNeigh->y);
                        if (cd<=ca){
                            int degree = curNodeNeigh->childrenIds.size();
                            if (degree < minDegree){
                                minDegree = degree;
                                newParent = curNodeNeighId;
                            }
                        }
                    }
                }
            }
            if (newParent != -1){
                g_GSTEBParentTable[curNode.id] = newParent;
            }
        }
    }
}

void GSTEBRouting::broadcastRoutingTree()
{
    // TODO: BC routing tree
    // BS broadcasts the routing tree to all the nodes
    // in the network.

    for (const auto& nodeIdd : getGSTEBNodeInfo(self)->neighborIds){
        GSTEBNodeInfo* node = getGSTEBNodeInfo(nodeIdd);
        int des = node->id;
        int xCoord = node->x;
        int yCoord = node->y;

        if (calculateDistance(xCoor, yCoor, xCoord, yCoord) <= communicationRadius){
            GSTEBRoutingPacket *netPacket = new GSTEBRoutingPacket("GSTEBRouting packet", NETWORK_LAYER_PACKET);
            netPacket->setPacketType(NODE_CONTROL_PACKET);
            // set routing tree data
            for (int i=0; i<numNodes; i++) {
                netPacket->setRoutingTree(i, networkParentTable[i]);
            }
            netPacket->setSource(SELF_NETWORK_ADDRESS);


            std::stringstream dest_addr;
            dest_addr << des;
            netPacket->setDestination(dest_addr.str().c_str());
            netPacket->setTtl(numNodes);
            double dist;
                        for (auto& neiNode : g_GSTEBNodesInfo){
                            if (neiNode.id == des){
                                dist = calculateDistance(neiNode.x, neiNode.y, xCoor, yCoor);
                            }
                        }

                        GSTEBNodeInfo* sentNode = getGSTEBNodeInfo(self);
                        sentNode->consumtion += calcTxEnergy(2000, calculateDistance(des));
                        sentNode->numSent ++;
                        sentNode->tNumSent ++;
                        if ( traceMode == 0 ) trace() << "BC ROUTING TREE " << des ;
            toMacLayer(netPacket, des);
        }
    }
}

void GSTEBRouting::handleRoutingTree(GSTEBRoutingPacket* pkt)
{
    // each node receives the routing tree
    // and records its parent node and child nodes
    // in memory.
    //if ( traceMode == 0 ) trace()  << "routing tree";
    if (parentId != -1) return;

    parentId = pkt->getRoutingTree(self);
    //if ( traceMode == 0 ) trace()  << "routing tree" << parentId;
    for (int i=0; i<numNodes; i++){
        int nParentId = pkt->getRoutingTree(i);
        if (nParentId == self){
            myChild.push_back(i);
            //if ( traceMode == 0 ) trace()  << "child" << i;
            for (const auto& nNodeIdd : getGSTEBNodeInfo(self)->neighborIds){
                GSTEBNodeInfo* nNode = getGSTEBNodeInfo(nNodeIdd);
                if (nNode->id == i){
                    int des = i;
                    int xCoord = nNode->x;
                    int yCoord = nNode->y;

                    if (calculateDistance(xCoor, yCoor, xCoord, yCoord) <= communicationRadius){
                        GSTEBRoutingPacket *dupPkt = new GSTEBRoutingPacket("GSTEBRouting packet", NETWORK_LAYER_PACKET);
                        dupPkt->setPacketType(NODE_CONTROL_PACKET);
                        for (int i=0; i<numNodes; i++) {
                            dupPkt->setRoutingTree(i, pkt->getRoutingTree(i));
                        }
                        std::stringstream dest_addr;
                        dest_addr << des;
                        dupPkt->setDestination(dest_addr.str().c_str());
                        dupPkt->setTtl(numNodes);
                        double dist;
                                                for (auto& neiNode : g_GSTEBNodesInfo){
                                                    if (neiNode.id == des){
                                                        dist = calculateDistance(neiNode.x, neiNode.y, xCoor, yCoor);
                                                    }
                                                }

                                                GSTEBNodeInfo* sentNode = getGSTEBNodeInfo(self);
                                                                        sentNode->consumtion += calcTxEnergy(2000, calculateDistance(des));
                                                                        sentNode->numSent ++;
                                                                        sentNode->tNumSent ++;
                                                                        toMacLayer(dupPkt, des);
                    }
                }

            }
        }
    }
    if ( traceMode == 0 ) trace() << "HANDLE ROUTING TREE " ;
    setTimer(DATA_COLLECTING_PHASE, 100);
}

void GSTEBRouting::sendDataPacket()
{
    // TODO: Sensing
    // If a node has no child node, it defines itself as a leaf node,
    // from which the data transmitting begins.
    getGSTEBNodeInfo(self)->controlPacketCount = getGSTEBNodeInfo(self)->numSent;
    getGSTEBNodeInfo(self)->controlPacketsConsumtion = resModule->getSpentEnergy() - getGSTEBNodeInfo(self)->castaliaConsumtion;
    if (parentId == -1 || parentId == chId) return;
    GSTEBRoutingPacket *netPacket = new GSTEBRoutingPacket("GSTEBRouting packet", NETWORK_LAYER_PACKET);
    netPacket->setPacketType(DATA_PACKET);
    netPacket->setSource(SELF_NETWORK_ADDRESS);
    std::stringstream dest_addr;
    dest_addr << parentId;
    netPacket->setDestination(dest_addr.str().c_str());
    netPacket->setTtl(numNodes);
    double dist;
                            for (auto& neiNode : g_GSTEBNodesInfo){
                                if (neiNode.id == parentId){
                                    dist = calculateDistance(neiNode.x, neiNode.y, xCoor, yCoor);
                                }
                            }

                            GSTEBNodeInfo* sentNode = getGSTEBNodeInfo(self);
                                                    sentNode->consumtion += calcTxEnergy(2000, calculateDistance(parentId));

    if ( traceMode == 0 ) trace()  << "#SENSOR_DATA: " << self << " -> " << parentId;
    //toMacLayer(netPacket, parentId);
    gstebTDMAQueue.push(netPacket);
//    for (int i=0; i<numNodes; i++){
//        if (g_GSTEBParentTable[i]==self){
//            GSTEBRoutingPacket *dupPkt = netPacket->dup();
//            gstebTDMAQueue.push(dupPkt);
//        }
//    }
        int myTimeSlot = (getGSTEBNodeInfo(self)->hopToCH % 2 ) + 1;
        double slotNow = ceil(simTime().dbl()/myTimeSlot);
        double nextTimeSlot = (slotNow + 1) * myTimeSlot;
        setTimer(TDMA_DATA, nextTimeSlot-simTime().dbl());
}

void GSTEBRouting::handleDataPacket(GSTEBRoutingPacket* pkt)
{
    if (parentId == -1 || parentId == chId || dataFusion) return;
    if ( traceMode == 0 ) trace()  << "Data packet from " << pkt->getSource() << " to " << parentId;
    // Create new packet to forward to parent
    GSTEBRoutingPacket *netPacket = new GSTEBRoutingPacket("GSTEBRouting packet", NETWORK_LAYER_PACKET);
    netPacket->setPacketType(DATA_PACKET);
    netPacket->setSource(SELF_NETWORK_ADDRESS);
    std::stringstream dest_addr;
    dest_addr << parentId;
    netPacket->setDestination(dest_addr.str().c_str());
    netPacket->setTtl(numNodes);
    double dist;
                                for (auto& neiNode : g_GSTEBNodesInfo){
                                    if (neiNode.id == parentId){
                                        dist = calculateDistance(neiNode.x, neiNode.y, xCoor, yCoor);
                                    }
                                }

                                GSTEBNodeInfo* sentNode = getGSTEBNodeInfo(self);
                                                        sentNode->consumtion += calcTxEnergy(2000, calculateDistance(parentId));
                                                        sentNode->numSent ++;
                                //if ( traceMode == 0 ) trace()  << "#SENSOR_DATA: " << self << " -> " << parentId;
                                toMacLayer(netPacket, parentId);
}


void GSTEBRouting::sendGSTEBTDMA(){
    if (!gstebTDMAQueue.empty()) {
        double firstE = resModule->getSpentEnergy();
        GSTEBRoutingPacket* dupPkt =  gstebTDMAQueue.front();
            gstebTDMAQueue.pop();
            if ( traceMode == 0 ) trace()  << "#SENSOR_DATA: " << self << " -> " << parentId;
            if (parentId == -1) {
                double minDis = 99999;
                for(const int nId : getGSTEBNodeInfo(self)->neighborIds){
                    GSTEBNodeInfo* nInfo = getGSTEBNodeInfo(nId);
                    if(nInfo->distanceToCH < minDis){
                        minDis = nInfo->distanceToCH;
                        parentId = nId;
                    }
                }
            }
            toMacLayer(dupPkt, parentId);
            getGSTEBNodeInfo(self)->numSent ++;
            getGSTEBNodeInfo(self)->tNumSent ++;
            getGSTEBNodeInfo(self)->tNumDataSent ++;
            double secondE = resModule->getSpentEnergy();
                if ((secondE - firstE) != 0){
                    if (g_gstebEPerPkt == 0){
                        g_gstebEPerPkt = secondE - firstE;
                    } else {
                        g_gstebEPerPkt = (g_gstebEPerPkt + secondE - firstE)/2;
                    }
                }
            int myTimeSlot = (getGSTEBNodeInfo(self)->hopToCH % 2 ) + 1;
                double slotNow = ceil(simTime().dbl()/myTimeSlot);
                double nextTimeSlot = (slotNow + 1) * myTimeSlot;
                setTimer(TDMA_DATA, nextTimeSlot-simTime().dbl());
                if (dataFusion){
                    while (!gstebTDMAQueue.empty()){
                        delete gstebTDMAQueue.front();
                        gstebTDMAQueue.pop();
                    }
                }

        }

}

void GSTEBRouting::rotationCH()
{
    // TODO: rotation
        g_isCHsRotated = true;
        g_GSTEBTimeNow = simTime().dbl() + 1000;
            for (auto& node : g_GSTEBNodesInfo){
                node.chId = -1;
                node.isCH = false;
                node.chx = -1;
                node.chy = -1;
                node.neighborIds.clear();
                node.parentId = -1;
            }
            g_gstebRotationCount++;

            double top1 = -1, top2 = -1, top3 = -1;
                int id1 = -1, id2 = -1, id3 = -1;
                double low1 = 999999, low2 = 999999, low3 = 999999;
                int lid1 = -1, lid2 = -1, lid3 = -1;
                double totalEnergy = 0.0;
                double totalCastaliaEnergy = 0.0;
                int n = numNodes/33;
                int totalControl = 0;
                double totalControlCons = 0.0;
                int totalPkt = 0;

                for (auto& gstebNode : g_GSTEBNodesInfo){
                    gstebNode.isCH = false;
                    double eLeft = (initEnergy - gstebNode.castaliaConsumtion);
                    double gstebEL = (eLeft/initEnergy)*100;
                    gstebNode.el = gstebEL;
                    totalCastaliaEnergy += gstebEL;
                    totalControl += gstebNode.controlPacketCount;
                    totalControlCons += gstebNode.controlPacketsConsumtion;
                    totalPkt += gstebNode.numSent;
                    gstebNode.numSent = 0;
                    totalEnergy += gstebNode.el;
                    if (gstebNode.el > top1) {
                            top3 = top2; id3 = id2;
                            top2 = top1; id2 = id1;
                            top1 = gstebNode.el;   id1 = gstebNode.id;
                        } else if (gstebNode.el > top2) {
                            top3 = top2; id3 = id2;
                            top2 = gstebNode.el;   id2 = gstebNode.id;
                        } else if (gstebNode.el > top3) {
                            top3 = gstebNode.el;   id3 = gstebNode.id;
                        }
                    if (gstebNode.el < low1) {
                            low3 = low2; lid3 = lid2;
                            low2 = low1; lid2 = lid1;
                            low1 = gstebNode.el; lid1 = gstebNode.id;
                        } else if (gstebNode.el < low2) {
                            low3 = low2; lid3 = lid2;
                            low2 = gstebNode.el; lid2 = gstebNode.id;
                        } else if (gstebNode.el < low3) {
                            low3 = gstebNode.el; lid3 = gstebNode.id;
                        }
                    }
                    double avgCasEn = totalCastaliaEnergy / numNodes;
                    double avgEnergy = totalEnergy / numNodes;
                    double rctControl = ((double)totalControl / (double)totalPkt)*100;

                    std::vector<GSTEBNodeInfo> gsteb_CHcandidates;
                    for (const auto& node : g_GSTEBNodesInfo) {
                        if (node.el > avgEnergy)
                            gsteb_CHcandidates.push_back(node);
                    }

                    if (gsteb_CHcandidates.empty()) {
                        getGSTEBNodeInfo(id1)->isCH = true;
                        getGSTEBNodeInfo(id2)->isCH = true;
                        getGSTEBNodeInfo(id3)->isCH = true;
                        return;
                    }

                    // Farthest-Point Sampling ---
                    std::vector<int> selectedIds;

                    auto first = std::max_element(gsteb_CHcandidates.begin(), gsteb_CHcandidates.end(),
                                                  [](const GSTEBNodeInfo& a, const GSTEBNodeInfo& b) {
                                                      return a.el < b.el;
                                                  });
                    selectedIds.push_back(first->id);

                    while (selectedIds.size() < static_cast<size_t>(n) && selectedIds.size() < g_GSTEBNodesInfo.size()) {
                        double bestMinDist = -1.0;
                        int bestNodeId = -1;

                        for (const auto& cand : gsteb_CHcandidates) {
                            if (std::find(selectedIds.begin(), selectedIds.end(), cand.id) != selectedIds.end())
                                continue;

                            double minDistToSelected = DBL_MAX;
                            for (int selId : selectedIds) {
                                auto it = std::find_if(gsteb_CHcandidates.begin(), gsteb_CHcandidates.end(),
                                                       [selId](const GSTEBNodeInfo& nd){ return nd.id == selId; });
                                if (it != gsteb_CHcandidates.end()) {
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

                    if (traceMode == 1) trace() << "******************** Round " << g_gstebRotationCount << " ********************";
                    for (int id : selectedIds) {
                        const auto it = std::find_if(g_GSTEBNodesInfo.begin(), g_GSTEBNodesInfo.end(),
                                                     [id](const GSTEBNodeInfo& n){ return n.id == id; });
                        if (it != g_GSTEBNodesInfo.end()) {
                            getGSTEBNodeInfo(id)->isCH = true;
                            if ( traceMode == 1 ) trace()  << "New CHs: " << id
                                      << " | EL: " << std::fixed << std::setprecision(2) << it->el << "%"
                                      << " | Pos(" << it->x << ", " << it->y << ")";
                        }
                    }


                if ( traceMode == 1 ) trace()  << "MIN: " << lid1 << std::setprecision(5) << "(" << (initEnergy - getGSTEBNodeInfo(lid1)->castaliaConsumtion)/initEnergy * 100
                        << "), AVG: "  << avgCasEn << " reception: " << 100.0 - rctControl << " Control: " << (totalControlCons / totalCastaliaEnergy)*100;


                double mnc = getGSTEBNodeInfo(lid1)->controlPacketCount*calcTxEnergy(100, 80) + (getGSTEBNodeInfo(lid1)->numSent - getGSTEBNodeInfo(lid1)->controlPacketCount)*calcTxEnergy(2000, 80);
                double m = ((0.1-mnc)/0.1)*100;
                double tc = (double)totalControl*calcTxEnergy(100, 80) + ((double)totalPkt - (double)totalControl)*calcTxEnergy(2000, 80);
                double a = (((0.1*numNodes-tc)/numNodes)/0.1)*100;
                double tcc = (double)totalControl*calcTxEnergy(100, 80);
                double tpc = (double)totalControl*calcTxEnergy(100, 80) + ((double)totalPkt - (double)totalControl)*calcTxEnergy(2000, 80);
                double c = (tcc/tpc)*100;

                double minNodeConsumsion = 1.0;
                double minNodeId = -1;

                for (auto& gstebNode : g_GSTEBNodesInfo){
                   int numberSentPkt = gstebNode.tNumSent;
                   int numberDataSent = gstebNode.tNumDataSent;
                   int numberControlSent = numberSentPkt - numberDataSent;
                   int numberReceivedPkt = gstebNode.tNumRecv;
                   double txDataConsumtion = (double)numberDataSent*calcTxEnergy(2000, 80);
                   double txControlConsumtion = (double)numberControlSent*calcTxEnergy(2000, 80);
                   double txConsumtion = txDataConsumtion + txControlConsumtion;
                   double rxConsumtion = numberReceivedPkt*calcTxEnergy(2000, 80);


                }
                if ( traceMode == 1 ) trace()  << "MIN: " << lid1 << std::setprecision(5) << "(" << m
                                        << "), AVG: "  << a << " reception: " << ((double)g_GstebControlPkt*100 /((double)g_GstebDataPkt*2000+(double)g_GstebControlPkt*100))*100;
                //if ( traceMode == 1 ) trace() <<
}
