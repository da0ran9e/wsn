cplusplus {{
#include "node/communication/routing/RoutingPacket_m.h"
}}

class RoutingPacket;

enum GSTEBRoutingPacketType {
    BS_BROADCAST_PACKET = 0;   
    SENSOR_BROADCAST_PACKET = 1;
    NEIGHBOR_BROADCAST_PACKET = 2;
    NODE_INFO_PACKET = 3;
    NODE_CONTROL_PACKET = 4;
    FINALIZE = 5;
};

struct GSTEBBSBroadcastInfo {
    int senderX;
    int senderY;
    int x;
    int y;
    double timeStart;
    int numNodes;
    int sinkId;
    double timeSlot;
};

struct GSTEBSensorBroadcastInfo {
    int nX;
    int nY;
    double nEL;
};

packet GSTEBRoutingPacket extends RoutingPacket {
    int packetType @enum(GSTEBRoutingPacketType);

    int ttl;
	int gstebDataId;
	
    GSTEBBSBroadcastInfo bSBroadcastData;
    GSTEBSensorBroadcastInfo sensorBroadcastData;

    int nNumber;
    int nId[1050];
    int nXCoor[1050];
    int nYCoor[1050];
    int nEL[1050];

    int nnNumber;
    int nNeighbor[1050];
    int nnId[1050];
    int nnXCoor[1050];
    int nnYCoor[1050];
    int nnEL[1050];

    int routingTree[1050];
}

