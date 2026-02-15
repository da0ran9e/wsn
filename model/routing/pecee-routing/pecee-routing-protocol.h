#ifndef PECEE_ROUTING_PROTOCOL_H
#define PECEE_ROUTING_PROTOCOL_H

#include "ns3/wsn-routing-protocol.h"
#include "pecee-header.h"
#include <vector>
#include <map>
#include <queue>
#include <set>
#include <cmath>
#include <algorithm>
#include <cfloat>
#include <iomanip>

using std::vector;
using std::map;
using std::queue;
using std::priority_queue;
using std::pair;
using std::set;

namespace ns3 {
namespace wsn {

// Configuration and data structures

struct Point {
    double x;
    double y;
};

struct PeceeCellData {
    int cellId;
    int color;
    int clId;  
    int chId;  
    vector<int> members;
    vector<int> neighbors;
    map<int, int> gateways;
};

struct PeceeNodeData {
    int id;
    double x;
    double y;
    bool isCH;
    bool isCL;
    int cellId;
    int color;
    int clId;
    int chId;
    int numSent;
    int numRecv;
    double energyConsumtion;
    double castaliaConsumtion;
    double el;
    int controlPacketCount;
    double controlPacketsConsumtion;
    vector<int> neighbors;
};

struct PeceePacket {
    int nextHop;
    int sensorId;
    int dataId;
    int desCH;
    int hopCount;
    int cellSource;
    int cellSent;
    int cellDes;
    int ttl;
    int source;
    vector<int> cellPath;
};
// Global state for fast algorithm testing
static bool g_isPrecalculated = false;
static bool g_isCHRotation = true;
static int g_rotationCount = 0;
static bool g_enableDetailedDebug = false;  // Set to true to enable debug output (except rotation)
static double g_rotationStartTime = 0.0;  // Track rotation start time for measurement
static set<int> g_CHsProcessedCHA;  // Track which CHs have completed their CHA
static double g_lastCHACompleteTime = 0.0;  // Track when last CHA was sent
static vector<PeceeNodeData> g_ssNodesDataList;
static vector<PeceeCellData> g_ssCellDataList;
static map <int, map<int, int>> g_ssRoutingTable; // <node, <next-cell, next-hop>>

static int g_ssSensorData[1000]; // sensor data
static int g_ssSensorDataArr[1000]; // received sensor data
static vector<int> g_ssSensorDataSent;
static vector<int> g_ssSensorDataReceived;
static int g_ssSensorDataSentCount = 0;
static int g_ssSensorDataReceivedCount = 0;
static map<int, vector<PeceePacket>> g_ssSensorDataOverheared;
static map<int, vector<int>> g_CHANodeTraces; // CH announcements traced by nodes
static map<int, vector<int>> g_CHACellTraces; // CH announcements traced by cells

// Timer types
enum PeceeTimerType {
    SEND_ANNOUNCEMENT_QUEUE = 0,
    ANNOUNCE_CELL_HOP_TIMER = 1,
    COLOR_SCHEDULING_TIMER = 2,
    SEND_CELL_PACKET = 3,
    CH_ROTATION_TIMER = 4
};


class PeceeRoutingProtocol : public WsnRoutingProtocol
{
public:
    PeceeRoutingProtocol();
    virtual ~PeceeRoutingProtocol();

    static TypeId GetTypeId();
    void FromMacLayer(Ptr<Packet> pkt,
                      const uint16_t src) override;
    void Start() override;
    
    void SetIsCH(bool value) { isCH = value; }
    
private:
    // Configuration parameters
    int traceMode = 0;
    int grid_offset = 100;
    double cellRadius = 80.0;
    int sensingDuration = 100;
    int reconfigurationTime = 10000;
    double colorTimeSlot = 100.0;
    int sensorDataDub = 1;
    int numberOfNodes;
    int maxHopCount;
    double initEnergy = 2;
    bool dataFusion = false;

    // Node state
    bool isCH;
    bool isCL;
    int myCellId = -1;
    int myColor = -1;
    double myX = 0.0, myY = 0.0;
    int myCLId = -1;
    int myCHId = -1;
    int myNextHopToCH = -1;  // Next hop toward CH (for members: their CL; for CL: gateway to next cell)
    int self;

    // Routing tables and queues
    map <int, map<int, int>> intraCellRoutingTable; 
    int neighborCells[7] = {-1, -1, -1, -1, -1, -1};
    int cellGateways[6] = {-1, -1, -1, -1, -1, -1};
    int neighborCellGateways[6] = {-1, -1, -1, -1, -1, -1};
    queue<pair<Ptr<Packet>, int>> announcementQueue;
    queue<int> boardcastAnnouncementQueue;
    map<int, set<int>> receivedCHAFromCells;  // Track: chId -> set of cellIds we've forwarded from
    
    struct ComparePacketsPriority {
        bool operator()(const pair<Ptr<Packet>, int>& a, const pair<Ptr<Packet>, int>& b) {
            // Note: Need to extract dataId from packet for proper comparison
            return false; // Simplified for now
        }
    };
    priority_queue<pair<Ptr<Packet>, int>, vector<pair<Ptr<Packet>, int>>, ComparePacketsPriority> cellPacketQueue;
    
    int myCellPathToCH[1000];
    int levelInCell = -1;
    int ssSentHop = -1;
    vector<PeceePacket> ssSentPacket;

    // Helper methods
    double calculateDistance(double x1, double y1, double x2, double y2);
    void PrecalculateSimulationResults();
    PeceeNodeData* getNodeData(int nodeId);
    PeceeCellData* getCellData(int cellId);
    bool isNodeInList(int nodeId, const vector<int>& nodeList);
    void rotationCH();

    Point calculateCellCenter(int cell_id);
    void calculateCellInfo();
    void sendCHAnnouncement();
    void sendAnnouncementQueue();
    void sendCellPacket();
    void handleCHAnnouncementPacket(Ptr<Packet> pkt);
    void selectClusterHead();
    void sendCellHopAnnouncementPacket();
    void handleCellHopAnnouncementPacket(Ptr<Packet> pkt);
    void sendSensorDataPacket();
    void handleSensorDataPacket(Ptr<Packet> pkt);
    void checkCHACompletion();  // Check when CHA phase completes after rotation
    double calculateConsumption(int desNode, int type);
    void savePacketCopy(Ptr<Packet> pkt, int des);
    void overhearingPacket();
    
    // Helper methods for ns-3 API wrapping
    int GetPathLength();
    double GetRandomDelay(double minMs, double maxMs);
    void ScheduleTimer(PeceeTimerType timerType, double delaySeconds);
    Ptr<Packet> CreatePeceePacket(PeceePacketType type);
    PeceeHeader ExtractPeceeHeader(Ptr<Packet> packet);
    void SendPacket(Ptr<Packet> packet, PeceeHeader& header, uint16_t destination);
    
    // Timer callbacks
    void HandleTimer(PeceeTimerType timerType);
};
}
}


#endif