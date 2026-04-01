#ifndef _GSTEBROUTING_H_
#define _GSTEBROUTING_H_

#include <map>
#include <set>
#include <unordered_set>
#include <queue>
#include <limits>
#include <cmath>
#include <algorithm>
#include <memory>
#include "node/communication/routing/VirtualRouting.h"
#include "node/communication/routing/gstebRouting/GSTEBRouting_m.h"

using namespace std;

struct GSTEBNodeInfo {
    int id;
    int x;
    int y;
    bool isCH;
    int parentId;
    int chId;
    double chx;
    double chy;
    int numSent;
    int numRecv;
    double distanceToCH;
    double el;
    double consumtion;
    vector<int> childrenIds;
    vector<int> neighborIds;
    int hopToCH = 0;
    double castaliaConsumtion;
    vector<int> g_gstebSentPkt;
    double controlPacketsConsumtion;
    int controlPacketCount;
    int tNumDataSent = 0;
    int tNumSent = 0;
    int tNumRecv = 0;
};
static vector<GSTEBNodeInfo> g_GSTEBNodesInfo;
static bool g_isCHsRotated = true;
static bool g_isVariableInitiated = false;
static vector<int> g_GSTEBPacketSent;
static vector<int> g_GSTEBPacketRecv;
static map<int, int> g_GSTEBParentTable;
static double g_GSTEBTimeNow = 1000;
static vector<int> g_receivedGSTEBBC;
static int g_gstebRotationCount = 1;
static double g_gstebEPerPkt = 0;

enum GSTEBRoutingTimers {
    INITIAL_PHRASE = 0,
    BS_BROADCAST,
    FORWARD_BC_PKT,
    PHRASE_I_TIMESLOT,
    SENSOR_BROADCAST_TIMEOUT,

    TREE_CONSTRUCTION_PHASE,

    SELF_CONSTRUCTION_PHASE,

    DATA_COLLECTING_PHASE,
    DATA_PACKET,
    TDMA_DATA,

    CH_ROTATION,
    CLEANUP,
};

struct GSTEBNeighborsOfNeighbors {
    int neighborId;
    int nnId;
    int nnX;
    int nnY;
    int nnEL;
};

struct GSTEBNeighbors {
    int nodeId; // neighbor of
    int nNumber;
    int nId; // this node ID
    int nX;
    int nY;
    int nEL;
    int consumption;
    double distanceToCH;
};

struct SortByNumber { //number of neighbor
        bool operator() (const GSTEBNeighbors& a, const GSTEBNeighbors& b) const {
            if (a.nNumber != b.nNumber) return a.nNumber < b.nNumber;
            return a.nId < b.nId;
        }
    };

struct SortByDistance { //number of neighbor
        bool operator() (const GSTEBNodeInfo& a, const GSTEBNodeInfo& b) const {
            if (a.distanceToCH != b.distanceToCH) return a.distanceToCH < b.distanceToCH;
            if (a.id != b.id) return a.id < b.id;
        }
    };

struct SortByHopCount { //number of neighbor
        bool operator() (const GSTEBNodeInfo& a, const GSTEBNodeInfo& b) const {
            return a.hopToCH > b.hopToCH;
        }
    };

class GSTEBRouting: public VirtualRouting {
    private:
    bool dataFusion = true;
    bool usingHTCRDC = false;
    int traceMode = 1;
    int nodeId;
    double xCoor;
    double yCoor;
    double energy;
    bool isCH;
    bool isSink;
    int myCHId;
    double initEnergy = 0.1;
    double alpha = 0.1;
    double communicationRadius = 80.0;

    int chId = -1;
    int chX;
    int chY;
    int numNodes;
    int timeStart;
    int timeSlot = 1;

    int prevHop = -1;

    vector<GSTEBNeighbors> tableI;
    vector<GSTEBNeighborsOfNeighbors> tableII;
    double myEL;
    int phase;

    vector<GSTEBNeighbors> networkTableI;
    map<int, int> networkParentTable;

    vector<GSTEBNeighbors> relayCandidates;
    int parentId = -1;
    vector<int> myChild;
    GSTEBRoutingPacket* gstebBSPacket;

    int phaseITimeslot;
    int sensorBroadcastTimeout;
    int initialPhraseTimeout; //The length of time slots in Steps 2 and 3 is predefined
    queue<GSTEBRoutingPacket*> gstebTDMAQueue;
    ResourceManager *resModule;
 protected:
    void startup() override;
    void timerFiredCallback(int) override;
    void fromApplicationLayer(cPacket *, const char *) override;
    void fromMacLayer(cPacket *, int, double, double) override;

    GSTEBNodeInfo* getGSTEBNodeInfo(int nodeId);
    void sendBSBroadcast();
    void handleBSBroadcast(GSTEBRoutingPacket* pkt);
    void computeEL();
    void sendSensorBroadcast();
    void handleSensorBroadcast(GSTEBRoutingPacket* pkt);
    double calculateDistance(double x1, double y1, double x2, double y2);
    double calculateDistance(int n);
    void sendNeighborsTable();
    void handleNeighborsTable(GSTEBRoutingPacket* pkt);
    double calcTxEnergy(int kBits, double distance);
    void chooseRelayNode();
    void chooseParentNode();
    void sendInfoToBS();
    void handleInfoFromNode(GSTEBRoutingPacket* pkt);
    void calculateRoutingTree();
    void broadcastRoutingTree();
    void handleRoutingTree(GSTEBRoutingPacket* pkt);
    void sendDataPacket();
    void handleDataPacket(GSTEBRoutingPacket* pkt);
    void sendGSTEBTDMA();
    void rotationCH();
    void savePacketCopy(GSTEBRoutingPacket* pkt, int des);
    void overhearingPacket();
};

#endif              //BYPASSROUTINGMODULE
