#ifndef _SSCELLULARROUTING_H_
#define _SSCELLULARROUTING_H_

#include "node/communication/routing/VirtualRouting.h"
#include "node/communication/routing/ssCellularRouting/SSCellularRouting_m.h"
#include <algorithm>

using namespace std;

struct Point {
    double x;
    double y;
};

enum SSCellularRoutingTimers {
    PRECALCULATE_TIMERS =1,
    CH_ROTATION_SSCR,
    CH_ADVERTISMENT_TIMER,
    SEND_ANNOUNCEMENT_QUEUE,
    ANNOUNCE_CELL_HOP_TIMER,
    COLOR_SCHEDULING_TIMER,
    SENSING_STATE,
    SEND_CELL_PACKET,
	RECONFIGURATION_TIMER,
	OVERHEARING_TIMER,
    CLEANUP,
};

struct SSCRCellData {
    int cellId;
    int color;
    int clId;  
    int chId;  
    vector<int> members;
    vector<int> neighbors;
    map<int, int> gateways;
};

struct SSCRNodeData {
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
    vector<int> neighbors;
    double controlPacketsConsumtion;
    int controlPacketCount;
    int totalControlPacket;
    int totalSensorPacket;
    int totalPacket;
};

struct SSCRPacket {
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
static bool g_isPrecalculated = false;
static int g_rotationCount = 1;
static bool g_isCHRotation = true;
static vector<SSCRNodeData> g_ssNodesDataList;
static vector<SSCRCellData> g_ssCellDataList;
static map <int, map<int, int>> g_ssRoutingTable; // <node, <next-cell, next-hop>>

static int g_ssSensorData[1050]; // < data>
static int g_ssSensorDataArr[1050]; // < data>
static vector<int> g_ssSensorDataSent;
static vector<int> g_ssSensorDataReceived;
static int g_ssSensorDataSentCount = 0;
static int g_ssSensorDataReceivedCount = 0;
static map<int, vector<SSCRPacket>> g_ssSensorDataOverheared;
static vector<double> g_ssCastaliaEnergy;
static double ePerPkt = 0;

class SSCellularRouting: public VirtualRouting {
private:
    bool dataFusion = true;
    int traceMode = 0;
	int grid_offset = 100;
	double cellRadius = 80.0;
	int sensingDuration = 100;
	int reconfigurationTime = 10000;
	double colorTimeSlot = 100.0;
	int sensorDataDub = 1;
	int numberOfNodes;
    int maxHopCount;
    double initEnergy = 0.1;

	bool isCH;
	bool isCL;
	int myCellId = -1;
	int myColor = -1;
	double myX = 0.0, myY = 0.0;
	int myCLId = -1;
	int myCHId = -1;

	map <int, map<int, int>> intraCellRoutingTable; // <node, <next-cell, next-hop>>

	//vector<CellMemberRecord> cellMembers;
	int neighborCells[7] = {-1, -1, -1, -1, -1, -1};
	int cellGateways[6] = {-1, -1, -1, -1, -1, -1};
	int neighborCellGateways[6] = {-1, -1, -1, -1, -1, -1};
	queue<pair<SSCellularRoutingPacket*, int>> announcementQueue;
	queue<int> boardcastAnnouncementQueue;
	struct ComparePacketsPriority {
		bool operator()(const pair<SSCellularRoutingPacket*, int>& a, const pair<SSCellularRoutingPacket*, int>& b) {
			return a.first->getSensorData().dataId < b.first->getSensorData().dataId;
		}
	};
	priority_queue<pair<SSCellularRoutingPacket*, int>, vector<pair<SSCellularRoutingPacket*, int>>, ComparePacketsPriority> cellPacketQueue;
	
	int myCellPathToCH[1050] = {-1};

	int levelInCell = -1;

	int ssSentHop = -1;
	vector<SSCRPacket> ssSentPacket;
	ResourceManager *resModule;
protected:
	void startup() override;
    void timerFiredCallback(int) override;
    void fromApplicationLayer(cPacket *, const char *) override;
    void fromMacLayer(cPacket *, int, double, double) override;
    double calculateDistance(double x1, double y1, double x2, double y2);

    void PrecalculateSimulationResults();
	SSCRNodeData* getNodeData(int nodeId);
    SSCRCellData* getCellData(int cellId);
    bool isNodeInList(int nodeId, const vector<int>& nodeList);
    void rotationCH();

    Point calculateCellCenter(int cell_id);
    void calculateCellInfo();
    void forwardPacket(SSCellularRoutingPacket* pkt, int nextHopId);
    void sendHelloPacket();
    void handleHelloPacket(SSCellularRoutingPacket* pkt);
    void calculateFitnessScore();
    void sendCLAnnouncement();
    void handleCLAnnouncementPacket(SSCellularRoutingPacket* pkt);
    void sendCLConfirmationPacket();
    void handleCLConfirmationPacket(SSCellularRoutingPacket* pkt);
    void gatewaySelection();
    void sendGatewaySelectionPacket();
    void handleGatewaySelectionPacket(SSCellularRoutingPacket* pkt);
    void sendLinkRequestPacket();
    void handleLinkRequestPacket(SSCellularRoutingPacket* pkt);
    void sendLinkConfirmationPacket(SSCellularRoutingPacket* pkt);
    void handleLinkConfirmationPacket(SSCellularRoutingPacket* pkt);
    void calculateRoutingTree();
    void announceRoutingTable();
    void sendRoutingTableAnnouncementPacket();
    void handleRoutingTableAnnouncementPacket(SSCellularRoutingPacket* pkt);
    void finalizeRouting();
    void sendCHAnnouncement();
    void sendAnnouncementQueue();
    void sendCellPacket();
    void handleCHAnnouncementPacket(SSCellularRoutingPacket* pkt);
    void selectClusterHead();
    void sendCellHopAnnouncementPacket();
    void handleCellHopAnnouncementPacket(SSCellularRoutingPacket* pkt);
    void sendSensorDataPacket();
    void handleSensorDataPacket(SSCellularRoutingPacket* pkt);
    double calculateConsumption(int desNode, int type);
    void savePacketCopy(SSCellularRoutingPacket* pkt, int des);
    void overhearingPacket();
};

#endif		
