cplusplus {{
#include "node/communication/routing/RoutingPacket_m.h"
}}

class RoutingPacket;

enum CellularRoutingPacketType {
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
};

struct HelloInfo {
    int cellId;
    int color;
    double energyLevel;
    double x;
    double y;
};

struct CLAnnouncementInfo {
    int cellId;
    int color;
    double fitnessScore;
    double x;
    double y;
};

struct NeighborInfo {
    int nodeId;
    double x;
    double y;
    int cellId;
}

struct NodeInfo {
    int nodeId;
    double x;
    double y;
    double energyLevel;
    NeighborInfo neighbors[500];
};

struct LinkRequestInfo {
    int source_cell_id;
    int source_cgw_id;
    int target_ngw_id;
    int target_cell_id;
    int final_ch_id; 
};

struct CLCommandInfo {
    int source_cgw_id;
    int target_ngw_id;
    int target_cell_id;
    int final_ch_id;
};

struct NCLConfirmInfo{
    int cgw_id;
    int ngw_id; 
};

struct RoutingUpdateInfo{
    int nodeId;
    int fromCell;
    int toCell;
    int nextHop;
};

struct CHAnnouncementInfo{
    int chId;
};

struct CellHopAnnouncementInfo{
    int nextCell;
    int cellPath[100];
};

struct SensorData{
    int destinationCH;
    int dataId;
    int sensorId;
    int hopCount;
};

packet CellularRoutingPacket extends RoutingPacket {
    int packetType @enum(CellularRoutingPacketType);

    int clusterHead; 
    int cellSent;
    int cellNext;
    int cellNextNext;
    int cellSource;
    int cellDestination;
    int cellHopCount;
    int cellPath[100];
    int cellPathToDestination[100];

    int ttl;
    HelloInfo helloData;
    CLAnnouncementInfo clAnnouncementData;
    NodeInfo nodeInfoData;
    
    LinkRequestInfo linkRequestData;
    CLCommandInfo clCommandData;
    NCLConfirmInfo nclCommandData;
    RoutingUpdateInfo routingUpdateData[7];
    CHAnnouncementInfo chAnnouncementData;
    int nextHopId; 
    CellHopAnnouncementInfo cellHopAnnouncementData;
    SensorData sensorData;
};