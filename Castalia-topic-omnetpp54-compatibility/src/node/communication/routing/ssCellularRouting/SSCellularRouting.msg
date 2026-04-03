import node.communication.routing.RoutingPacket;

enum SSCellularRoutingPacketType {
    HELLO_PACKET = 0;         
    CL_ANNOUNCEMENT = 1;    
    CL_CONFIRMATION = 2;
    BS_UPDATE_PACKET = 3;       
    LINK_REQUEST = 4;        
    LINK_ACK = 5;             
    LINK_ESTABLISHED = 6;      
    INTRA_CELL_ROUTING_UPDATE = 7;
    CL_COMMAND_PACKET = 8;
    NCL_CONFIRM_PACKET = 9;
    ROUTING_TREE_UPDATE_PACKET = 10;
    CH_ANNOUNCEMENT_PACKET = 11;
    DATA_PACKET = 12;
    ANNOUNCE_CELL_HOP = 13;
    SENSOR_DATA = 14;
    FINALIZE_PKT = 15;
};

struct SSCHAnnouncementInfo{
    int chId;
};

struct SSCellHopAnnouncementInfo{
    int nextCell;
    int cellPath[1050];
};

struct SSSensorInfo{
    int destinationCH;
    int dataId;
    int sensorId;
    int hopCount;
}

packet SSCellularRoutingPacket extends RoutingPacket {
    int packetType @enum(SSCellularRoutingPacketType);
    int clusterHead; 
    int cellSent;
    int cellNext;
    int cellNextNext;
    int cellSource;
    int cellDestination;
    int cellHopCount;
    int cellPath[1050];
    int cellPathToDestination[1050];
    int ttl;
    
    SSSensorInfo sensorData;
    SSCHAnnouncementInfo cHAnnouncementData;
    SSCellHopAnnouncementInfo sSCellHopAnnouncementData;
}

