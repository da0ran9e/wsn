#include "peece-routing-protocol.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <string>
#include <iomanip>

namespace ns3 {
namespace wsn {

NS_LOG_COMPONENT_DEFINE("PeeceRoutingProtocol");
NS_OBJECT_ENSURE_REGISTERED(PeeceRoutingProtocol);

TypeId
PeeceRoutingProtocol::GetTypeId()
{
    static TypeId tid = TypeId("ns3::wsn::PeeceRoutingProtocol")
        .SetParent<WsnRoutingProtocol>()
        .SetGroupName("Wsn")
        .AddAttribute("HelloInterval",
                      "Hello packet send interval (seconds)",
                      DoubleValue(1.0),
                      MakeDoubleAccessor(&PeeceRoutingProtocol::m_helloInterval),
                      MakeDoubleChecker<double>())
        .AddAttribute("CellRadius",
                      "Radius of hexagonal cell (meters)",
                      DoubleValue(25.0),
                      MakeDoubleAccessor(&PeeceRoutingProtocol::m_cellRadius),
                      MakeDoubleChecker<double>())
        .AddAttribute("MaxHopCount",
                      "Maximum hop count for routing",
                      IntegerValue(10),
                                            MakeIntegerAccessor(&PeeceRoutingProtocol::m_maxHopCount),
                                            MakeIntegerChecker<int>())
        .AddTraceSource("SensingTrace",
                        "Emitted when a sensing packet is sent/received/forwarded",
                                                MakeTraceSourceAccessor(&PeeceRoutingProtocol::m_sensingTrace),
                                                "ns3::TracedCallback::StringUint16Uint16Uint16Int");
    return tid;
}

PeeceRoutingProtocol::PeeceRoutingProtocol()
    : WsnRoutingProtocol(),
      m_helloInterval(1.0),
      m_clElectionDelayInterval(0.5),
      m_cellRadius(25.0),
      m_gridOffset(100),
      m_maxNeighborNumber(20),
      m_maxHopCount(10),
    m_numberHelloIntervals(5),
    m_helloSentCount(0),
      m_clCalculationTime(1000),
      m_routingTableUpdateTime(1000),
      m_state1Time(2000),
      m_sensingStageTime(5000),
      m_reconfigurationTime(10000),
      m_clConfirmationTime(500),
      m_myCellId(-1),
      m_myColor(-1),
      m_myRole(NORMAL_NODE),
      m_myClId(0xFFFF),
      m_myChId(0xFFFF),
      m_fitnessScore(-1.0),
      m_clFitnessScore(-1.0),
      m_receivedCLAnnouncement(false),
      m_gatewayTowardsCH(0xFFFF),
      m_myNextHopId(0xFFFF),
      m_myNextCellHop(-1),
      m_myNextNextCellHop(-1)
{
    // Initialize arrays
    std::fill(m_neighborCells, m_neighborCells + 7, -1);
    std::fill(m_cellGateways, m_cellGateways + 6, 0xFFFF);
    std::fill(m_neighborCellGateways, m_neighborCellGateways + 6, 0xFFFF);
    std::fill(m_myCellPathToCH, m_myCellPathToCH + 100, -1);
}

PeeceRoutingProtocol::~PeeceRoutingProtocol()
{
    ClearNeighborTable();
}

void
PeeceRoutingProtocol::Start()
{
    NS_LOG_FUNCTION(this);
    
    // Initialize local node properties
    CalculateCellInfo();
    
    NodeData nodeData;
    nodeData.nodeId = m_selfNodeProps.nodeId;
    nodeData.x = m_selfNodeProps.xCoord;
    nodeData.y = m_selfNodeProps.yCoord;
    nodeData.role = m_myRole;
    nodeData.cellId = m_myCellId;
    nodeData.color = m_myColor;
    nodeData.clId = m_myClId;
    nodeData.chId = m_myChId;
    nodeData.nextHopId = m_myNextHopId;
    nodeData.neighbors.clear();

    NS_LOG_INFO("Node " << m_selfNodeProps.nodeId << " started at (" 
                << m_selfNodeProps.xCoord << ", " << m_selfNodeProps.yCoord 
                << ") with cellId=" << m_myCellId << " color=" << m_myColor);
    LogStage("START", "pos=(" + std::to_string(m_selfNodeProps.xCoord) + "," + std::to_string(m_selfNodeProps.yCoord) + ") cell=" + std::to_string(m_myCellId) + " color=" + std::to_string(m_myColor));

    // Write comprehensive initialization info to node-init-state.txt with fitness
    static std::ofstream initFile("node-init-state.txt", std::ios::app);
    initFile << "Node " << m_selfNodeProps.nodeId
            << " pos=(" << m_selfNodeProps.xCoord << "," << m_selfNodeProps.yCoord << ")"
            << " cell=" << m_myCellId << " color=" << m_myColor
            << " fitness=" << std::fixed << std::setprecision(6) << m_fitnessScore << std::endl;
    initFile.flush();

    // Schedule initial state machine
    double initialDelayMs = rand() % 10;
    Simulator::Schedule(MilliSeconds(initialDelayMs), 
                       &PeeceRoutingProtocol::TimerCallback_State0, this);
}

void
PeeceRoutingProtocol::TimerCallback_State0()
{
    NS_LOG_FUNCTION(this);
    LogStage("HELLO_PHASE_START", "interval=" + std::to_string(m_helloInterval) + "s rounds=" + std::to_string((int)m_numberHelloIntervals));
    
    // Start hello phase
    m_helloSentCount = 0;
    Simulator::Schedule(MilliSeconds(m_helloInterval * 1000), 
                       &PeeceRoutingProtocol::SendHelloPacket, this);
    
    // Schedule hello timeout
    Simulator::Schedule(MilliSeconds(m_helloInterval * m_numberHelloIntervals * 1000), 
                       &PeeceRoutingProtocol::TimerCallback_HelloTimeout, this);
}

void
PeeceRoutingProtocol::SendHelloPacket()
{
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Sending HELLO packet - Cell:" << m_myCellId << " Color:" << m_myColor);
    LogStage("HELLO_SEND", "neighbors=" + std::to_string(m_neighborTable.size()));
    m_helloSentCount++;
    
    // Trace HELLO send and neighbor table broadcast
    if (m_trace)
    {
        double now = Simulator::Now().GetSeconds();
        std::string msg = "[" + std::to_string(now) + "s] Node " + std::to_string(m_selfNodeProps.nodeId)
                        + " HELLO_SEND neighbors=" + std::to_string(m_neighborTable.size())
                        + " cell=" + std::to_string(m_myCellId) + " color=" + std::to_string(m_myColor);
        m_trace->Trace(msg);
        // Prepare neighbor table summary (all neighbors, no limit)
        std::string tbl = "[";
        size_t n = m_neighborTable.size();
        for (size_t i = 0; i < n; ++i)
        {
            tbl += std::to_string(m_neighborTable[i].nodeId);
            if (i + 1 < n) tbl += ",";
        }
        tbl += "]";
        std::string msg2 = "[" + std::to_string(now) + "s] Node " + std::to_string(m_selfNodeProps.nodeId)
                         + " NEIGHBOR_TABLE_SEND count=" + std::to_string(n) + " ids=" + tbl;
        m_trace->Trace(msg2);
    }
    
    PeeceHeader header;
    header.SetPacketType(PEECE_HELLO_PACKET);
    header.SetSource(m_selfNodeProps.nodeId);
    header.SetDestination(0xFFFF);

    HelloPacketInfo helloData;
    helloData.x = m_selfNodeProps.xCoord;
    helloData.y = m_selfNodeProps.yCoord;
    helloData.cellId = m_myCellId;
    helloData.color = m_myColor;
    // Send all discovered neighbors (no limit)
    helloData.neighborCount = (uint8_t)std::min((size_t)255, m_neighborTable.size());  // Max 255 per uint8_t
    
    // Include all neighbors (dynamically sized)
    for (uint8_t i = 0; i < helloData.neighborCount && i < m_neighborTable.size(); i++) {
        helloData.neighborIds[i] = m_neighborTable[i].nodeId;
    }
    // Clear remaining unused slots to avoid garbage
    for (uint8_t i = helloData.neighborCount; i < 255; i++) {
        helloData.neighborIds[i] = 0xFFFF;
    }

    header.SetHelloData(helloData);

    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(header);

    ToMacLayer(packet, 0xFFFF);

    // Schedule next hello only if we haven't reached the round limit
    if (m_helloSentCount < (int)m_numberHelloIntervals)
    {
        Simulator::Schedule(MilliSeconds(m_helloInterval * 1000), 
                           &PeeceRoutingProtocol::SendHelloPacket, this);
    }
}

void
PeeceRoutingProtocol::TimerCallback_HelloTimeout()
{
    double now = Simulator::Now().GetSeconds();
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Hello phase timeout - Discovered " << m_neighborTable.size() << " neighbors");
    LogStage("HELLO_PHASE_END", "discovered=" + std::to_string(m_neighborTable.size()));
    
    // Trace to file
    if (m_trace) {
        std::string msg = "[" + std::to_string(now) + "s] Node " + std::to_string(m_selfNodeProps.nodeId) 
                        + " HELLO_PHASE_END neighbors=" + std::to_string(m_neighborTable.size());
        m_trace->Trace(msg);

        // Snapshot 1-hop neighbor table
        std::string oneHop = "[";
        for (size_t i = 0; i < m_neighborTable.size(); ++i) {
            oneHop += std::to_string(m_neighborTable[i].nodeId);
            if (i + 1 < m_neighborTable.size()) oneHop += ",";
        }
        oneHop += "]";
        std::string snap = "[" + std::to_string(now) + "s] Node " + std::to_string(m_selfNodeProps.nodeId)
                        + " NEIGHBOR_TABLE_SNAPSHOT count=" + std::to_string(m_neighborTable.size())
                        + " ids=" + oneHop;
        m_trace->Trace(snap);

        // Snapshot 2-hop neighbors per 1-hop neighbor
        for (const auto &nbr : m_neighborTable) {
            std::string twoHop = "[";
            for (size_t j = 0; j < nbr.neighborNodeIds.size(); ++j) {
                twoHop += std::to_string(nbr.neighborNodeIds[j]);
                if (j + 1 < nbr.neighborNodeIds.size()) twoHop += ",";
            }
            twoHop += "]";
            std::string snap2 = "[" + std::to_string(now) + "s] Node " + std::to_string(m_selfNodeProps.nodeId)
                             + " NEIGHBOR_2HOP_SNAPSHOT via=" + std::to_string(nbr.nodeId)
                             + " count=" + std::to_string(nbr.neighborNodeIds.size())
                             + " ids=" + twoHop;
            m_trace->Trace(snap2);
        }
    }

    // Log 2-hop topology for this node
    if (m_neighborTable.size() > 0) {
        std::string twoHopInfo = "[Node " + std::to_string(m_selfNodeProps.nodeId) + "] 2-HOP TOPOLOGY: ";
        for (size_t i = 0; i < m_neighborTable.size(); i++) {
            twoHopInfo += "N" + std::to_string(m_neighborTable[i].nodeId);
            if (m_neighborTable[i].neighborNodeIds.size() > 0) {
                twoHopInfo += "{";
                for (size_t j = 0; j < m_neighborTable[i].neighborNodeIds.size() && j < 8; j++) {
                    twoHopInfo += std::to_string(m_neighborTable[i].neighborNodeIds[j]);
                    if (j < m_neighborTable[i].neighborNodeIds.size() - 1 && j < 7) twoHopInfo += ",";
                }
                twoHopInfo += "}";
            }
            if (i < m_neighborTable.size() - 1) twoHopInfo += "|";
        }
        NS_LOG_INFO(twoHopInfo);
    }
    
    // Calculate fitness score after hello phase completes
    CalculateFitnessScore();
    
    // Determine the best known candidate within our cell
    double bestFitness = 0.0;
    uint16_t bestId = FindBestCandidateInCell(m_myCellId, bestFitness);
    bool iAmBestCandidate = (bestId == m_selfNodeProps.nodeId);
    
    if (m_trace) {
        double nowEval = Simulator::Now().GetSeconds();
        std::string evalMsg = "[" + std::to_string(nowEval) + "s] Node " + std::to_string(m_selfNodeProps.nodeId)
                            + " BEST_CANDIDATE_EVAL bestId=" + std::to_string(bestId)
                            + " bestFitness=" + std::to_string(bestFitness)
                            + " myFitness=" + std::to_string(m_fitnessScore)
                            + " iAmBest=" + (iAmBestCandidate ? "true" : "false");
        m_trace->Trace(evalMsg);
    }
    
    // Schedule CL announcement AND election for ALL nodes (even if no neighbors)
    // This ensures election happens even for isolated nodes
    
    // Schedule election timeout FIRST (always schedule this)
    // Election will happen even if node doesn't send announcement (no neighbors case)
    double electionDelay = 5000 + (rand() % 500);  // 5-5.5s after HELLO phase
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Scheduling CL election in " << electionDelay << " ms");
    if (m_trace) {
        double now = Simulator::Now().GetSeconds();
        std::string msg = "[" + std::to_string(now) + "s] Node " + std::to_string(m_selfNodeProps.nodeId)
                        + " ELECTION_SCHEDULED delay=" + std::to_string((int)electionDelay) + "ms";
        m_trace->Trace(msg);
    }
    Simulator::Schedule(MilliSeconds(electionDelay), 
                       &PeeceRoutingProtocol::TimerCallback_CLElection, this);
    
    // Only send announcement if we have neighbors AND we are the best known candidate
    if (m_fitnessScore > 0 && m_neighborTable.size() > 0 && iAmBestCandidate) {
        // Fitness-scaled announcement delay: higher fitness = shorter delay
        // Delay = baseDelay + (1 - fitnessScore) * maxScaleMs + tieBreaker
        // For fitnessScore in [0,1]: delay ranges from baseDelay to baseDelay+maxScaleMs
        // Example: fitnessScore=0.9 (high) → delay ≈ 200ms (announces early)
        //          fitnessScore=0.1 (low)  → delay ≈ 1900ms (waits much longer to listen)
        // Tie-breaker: Add micro-offset based on node ID (0-50ms) so lower IDs announce first
        double baseDelayMs = 100;
        double maxScaleMs = 1900;  // Total range: 100-2000ms (increased from 950)
        double tieBreaker = (m_selfNodeProps.nodeId % 100) * 0.5;  // 0-50ms spread
        double delay = baseDelayMs + (1.0 - m_fitnessScore) * maxScaleMs + tieBreaker;
        NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Scheduling CL announcement in " << delay 
                    << " ms (fitness=" << m_fitnessScore << ", tieBreaker=" << tieBreaker << "ms)");
        LogStage("CL_ANNOUNCE_SCHEDULED", "delayMs=" + std::to_string((int)delay) + 
                                          " fitness=" + std::to_string(m_fitnessScore) +
                                          " nodeId=" + std::to_string(m_selfNodeProps.nodeId));
        Simulator::Schedule(MilliSeconds(delay), 
                           &PeeceRoutingProtocol::SendCLAnnouncement, this);
    } else {
        // Skip announcement; wait to receive better candidates
        if (m_trace) {
            double nowSkip = Simulator::Now().GetSeconds();
            std::string reason;
            if (m_neighborTable.empty()) {
                reason = "no_neighbors";
            } else if (!iAmBestCandidate) {
                reason = "not_best_candidate";
            } else {
                reason = "unknown";
            }
            std::string skipMsg = "[" + std::to_string(nowSkip) + "s] Node " + std::to_string(m_selfNodeProps.nodeId)
                                + " CL_ANNOUNCEMENT_SKIPPED reason=" + reason
                                + " bestId=" + std::to_string(bestId)
                                + " bestFitness=" + std::to_string(bestFitness);
            m_trace->Trace(skipMsg);
        }
    }
}

void
PeeceRoutingProtocol::TimerCallback_CLElection()
{
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] === CL ELECTION DECISION ===");
    LogStage("CL_ELECTION_DECISION");
    
    // Decision: Become CL only if no better CL was selected
    // If m_myClId == 0xFFFF, it means no announcement had better fitness than ours
    if (m_myClId == 0xFFFF) {
        // No CL selected = no better candidate found
        // Elect ourselves as CL
        m_myRole = CELL_LEADER;
        m_myClId = m_selfNodeProps.nodeId;
        m_myChId = m_selfNodeProps.nodeId;
        NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Elected SELF as CELL_LEADER for cell " << m_myCellId 
                << " (fitness: " << m_fitnessScore << ", no better candidate)");
        LogStage("BECOME_CL", "fitness=" + std::to_string(m_fitnessScore));
        // File trace: CL elected
        if (m_trace) {
            double now = Simulator::Now().GetSeconds();
            std::string msg = "[" + std::to_string(now) + "s] Node " + std::to_string(m_selfNodeProps.nodeId)
                            + " CL_ELECTED cell=" + std::to_string(m_myCellId)
                            + " fitness=" + std::to_string(m_fitnessScore);
            m_trace->Trace(msg);
        }
    } else if (m_myClId != m_selfNodeProps.nodeId) {
        // A different CL was selected via announcement
        m_myChId = m_myClId;
        NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Accepting CL Node " << m_myClId 
                << " (their fitness: " << m_clFitnessScore << ", my fitness: " << m_fitnessScore << ")");
        LogStage("SELECT_CL", "cl=" + std::to_string(m_myClId) + " clFitness=" + std::to_string(m_clFitnessScore));
        // File trace: selected CL
        if (m_trace) {
            double now = Simulator::Now().GetSeconds();
            std::string msg = "[" + std::to_string(now) + "s] Node " + std::to_string(m_selfNodeProps.nodeId)
                            + " CL_SELECTED cl=" + std::to_string(m_myClId)
                            + " clFitness=" + std::to_string(m_clFitnessScore);
            m_trace->Trace(msg);
        }
    } else {
        // m_myClId == m_selfNodeProps.nodeId: I was already elected as CL
        m_myChId = m_selfNodeProps.nodeId;
        NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Confirming SELF as CELL_LEADER (fitness: " << m_fitnessScore << ")");
    }
    
    // Schedule next phase: CL confirmation and routing calculation
    // Wait for CL confirmations to be received, then proceed to routing
    double clCalcDelay = m_clCalculationTime + (rand() % 500);
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Scheduling CL calculation in " << clCalcDelay << " ms");
    Simulator::Schedule(MilliSeconds(clCalcDelay), 
                       &PeeceRoutingProtocol::TimerCallback_CLCalculation, this);
}

void
PeeceRoutingProtocol::SendCLAnnouncement()
{
    double now = Simulator::Now().GetSeconds();
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Sending CL_ANNOUNCEMENT BROADCAST (fitness: " << m_fitnessScore << ")");
    LogStage("CL_ANNOUNCEMENT_SEND", "toNeighbors=" + std::to_string(m_neighborTable.size()) + " fitness=" + std::to_string(m_fitnessScore));
    
    // Trace to file
    if (m_trace) {
        std::string msg = "[" + std::to_string(now) + "s] Node " + std::to_string(m_selfNodeProps.nodeId) 
                        + " CL_ANNOUNCEMENT_SEND neighbors=" + std::to_string(m_neighborTable.size()) 
                        + " fitness=" + std::to_string(m_fitnessScore);
        m_trace->Trace(msg);
    }
    
    // CHANGE: Use BROADCAST instead of unicast to ensure all nodes in range receive
    // This solves asymmetric neighbor table issues
    PeeceHeader header;
    header.SetPacketType(PEECE_CL_ANNOUNCEMENT);
    header.SetSource(m_selfNodeProps.nodeId);
    header.SetDestination(0xFFFF);  // Broadcast to all (0xFFFF = broadcast address)
    header.SetTtl(2);  // TTL=2 allows 2-hop propagation within cell

    CLAnnouncementInfo clInfo;
    clInfo.x = m_selfNodeProps.xCoord;
    clInfo.y = m_selfNodeProps.yCoord;
    clInfo.cellId = m_myCellId;
    clInfo.color = m_myColor;
    clInfo.fitnessScore = m_fitnessScore;

    header.SetCLAnnouncementData(clInfo);

    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(header);
    ToMacLayer(packet, 0xFFFF);
}

void
PeeceRoutingProtocol::TimerCallback_CLConfirmation()
{
    NS_LOG_FUNCTION(this);
    SendCLConfirmationPacket();
}

void
PeeceRoutingProtocol::SendCLConfirmationPacket()
{
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Sending CL_CONFIRMATION to CL Node " << m_myClId);
    LogStage("CL_CONFIRMATION_SEND", "to=" + std::to_string(m_myClId));
    
    if (m_myClId != 0xFFFF) {
        PeeceHeader header;
        header.SetPacketType(PEECE_CL_CONFIRMATION);
        header.SetSource(m_selfNodeProps.nodeId);
        header.SetDestination(m_myClId);

        CLConfirmationInfo confirmInfo;
        confirmInfo.clId = m_myClId;
        confirmInfo.cellId = m_myCellId;
        confirmInfo.nodeId = m_selfNodeProps.nodeId;

        header.SetCLConfirmationData(confirmInfo);

        Ptr<Packet> packet = Create<Packet>();
        packet->AddHeader(header);

        ToMacLayer(packet, m_myClId);
    }
}

void
PeeceRoutingProtocol::TimerCallback_CLCalculation()
{
    NS_LOG_FUNCTION(this);
    
    if (m_myRole == CELL_LEADER) {
        CalculateRoutingTree();
        
        double delayMs = m_routingTableUpdateTime + (rand() % 100);
        Simulator::Schedule(MilliSeconds(delayMs), 
                   &PeeceRoutingProtocol::SendRoutingTableAnnouncementPacket, this);
    } else {
        // Non-CH nodes calculate their own next hop
        CalculateMyNextHop();
    }
    
    // Schedule next phase: sensing operation for all nodes
    // Give time for routing table to propagate, then start sensing
    double sensingDelay = m_routingTableUpdateTime + 1000 + (rand() % 500);
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Scheduling sensing phase in " << sensingDelay << " ms");
    Simulator::Schedule(MilliSeconds(sensingDelay), 
                       &PeeceRoutingProtocol::TimerCallback_SensingStart, this);
}

void
PeeceRoutingProtocol::CalculateRoutingTree()
{
    NS_LOG_FUNCTION(this);
    LogStage("ROUTING_TREE_CALCULATE", "members=" + std::to_string(m_cellMembers.size()));
    // File trace: emit members snapshot for this CL
    if (m_trace && m_myRole == CELL_LEADER) {
        double now = Simulator::Now().GetSeconds();
        std::string ids = "";
        for (size_t i = 0; i < m_cellMembers.size(); ++i) {
            ids += std::to_string(m_cellMembers[i].nodeId);
            if (i + 1 < m_cellMembers.size()) ids += ",";
        }
        std::string msg = "[" + std::to_string(now) + "s] Node " + std::to_string(m_selfNodeProps.nodeId)
                        + " CL_MEMBERS_SNAPSHOT cell=" + std::to_string(m_myCellId)
                        + " count=" + std::to_string(m_cellMembers.size())
                        + " ids=[" + ids + "]";
        m_trace->Trace(msg);
    }
    
    // Simplified routing tree calculation
    // In full implementation, calculate intra-cell routing with gateways and CL as roots
    
    for (const auto& member : m_cellMembers) {
        m_intraCellRoutingTable[member.nodeId].clear();
        
        // Set next hop toward cell leader (simplified)
        uint16_t nextHop = m_myClId;
        for (const auto& neighbor : member.neighbors) {
            if (neighbor.cellId == m_myCellId) {
                nextHop = neighbor.nodeId;
                break;
            }
        }
        
        m_intraCellRoutingTable[member.nodeId][m_myCellId] = nextHop;
    }
    
    AnnounceRoutingTable();
}

void
PeeceRoutingProtocol::AnnounceRoutingTable()
{
    NS_LOG_FUNCTION(this);
    
    // Announce routing table to cell members
    m_routingUpdates.clear();
    
    for (const auto& member : m_cellMembers) {
        RoutingUpdateInfo updateInfo;
        updateInfo.nodeId = member.nodeId;
        updateInfo.fromCell = m_myCellId;
        updateInfo.toCell = m_myCellId;
        updateInfo.nextHop = m_intraCellRoutingTable[member.nodeId][m_myCellId];
        
        m_routingUpdates.push_back(updateInfo);
    }
}

void
PeeceRoutingProtocol::CalculateMyNextHop()
{
    NS_LOG_FUNCTION(this);
    double now = Simulator::Now().GetSeconds();
    
    NS_LOG_INFO("[t=" << now << "s] [Node " << m_selfNodeProps.nodeId << "] ===== CALCULATING MY NEXT HOP =====");
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] m_myChId=" << m_myChId 
                << " m_myRole=" << (int)m_myRole);
    
    // If I'm the CH, no next hop needed
    if (m_myChId == m_selfNodeProps.nodeId) {
        m_myNextHopId = m_selfNodeProps.nodeId;
        NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] I AM CH - next hop is myself");
        return;
    }
    
    // If CH not assigned, cannot calculate
    if (m_myChId == 0xFFFF) {
        NS_LOG_WARN("[Node " << m_selfNodeProps.nodeId << "] CH not assigned yet - cannot calculate next hop");
        return;
    }
    
    // Find best next hop toward CH from my neighbors
    m_myNextHopId = 0xFFFF;
    double bestDistance = 1e9;
    
    // First, check if CH is direct neighbor
    for (const auto& neighbor : m_neighborTable) {
        if (neighbor.nodeId == m_myChId) {
            m_myNextHopId = m_myChId;
            NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] CH is direct neighbor - next hop = " << m_myNextHopId);
            return;
        }
    }
    
    // Otherwise, find neighbor with best metric toward CH
    // Simple heuristic: choose neighbor closest to me (best connectivity)
    for (const auto& neighbor : m_neighborTable) {
        // Prefer neighbors in same cell
        if (neighbor.cellId == m_myCellId) {
            // Calculate distance to neighbor
            double dx = neighbor.x - m_selfNodeProps.xCoord;
            double dy = neighbor.y - m_selfNodeProps.yCoord;
            double distance = std::sqrt(dx*dx + dy*dy);
            
            if (distance < bestDistance) {
                bestDistance = distance;
                m_myNextHopId = neighbor.nodeId;
            }
        }
    }
    
    // If no same-cell neighbor found, use any neighbor
    if (m_myNextHopId == 0xFFFF && !m_neighborTable.empty()) {
        m_myNextHopId = m_neighborTable[0].nodeId;
        NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] No same-cell neighbor - using first neighbor " << m_myNextHopId);
    }
    
    if (m_myNextHopId != 0xFFFF) {
        NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] *** NEXT HOP CALCULATED: " << m_myNextHopId << " ***");
        LogStage("NEXT_HOP_CALCULATED", "nextHop=" + std::to_string(m_myNextHopId));
    } else {
        NS_LOG_WARN("[Node " << m_selfNodeProps.nodeId << "] *** FAILED TO CALCULATE NEXT HOP ***");
    }
}

void
PeeceRoutingProtocol::SendRoutingTableAnnouncementPacket()
{
    double now = Simulator::Now().GetSeconds();
    NS_LOG_INFO("[t=" << now << "s] [Node " << m_selfNodeProps.nodeId << "] Broadcasting ROUTING_TABLE with " << m_routingUpdates.size() << " entries");
    LogStage("ROUTING_TABLE_BROADCAST", "entries=" + std::to_string(m_routingUpdates.size()));
    
    if (m_routingUpdates.empty()) {
        NS_LOG_WARN("[Node " << m_selfNodeProps.nodeId << "] No routing updates to send");
        return;
    }
    
    PeeceHeader header;
    header.SetPacketType(PEECE_ROUTING_TREE_UPDATE);
    header.SetSource(m_selfNodeProps.nodeId);
    header.SetDestination(0xFFFF);

    for (size_t i = 0; i < m_routingUpdates.size() && i < 7; i++) {
        RoutingUpdateData updateData;
        updateData.nodeId = m_routingUpdates[i].nodeId;
        updateData.fromCell = m_routingUpdates[i].fromCell;
        updateData.toCell = m_routingUpdates[i].toCell;
        updateData.nextHop = m_routingUpdates[i].nextHop;
        
        header.SetRoutingUpdateData(updateData, i);
    }

    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(header);

    ToMacLayer(packet, 0xFFFF);
}

double
PeeceRoutingProtocol::ComputeFitness(double x, double y, int cellId)
{
    Point cellCenter = CalculateCellCenter(cellId);
    double distanceToCenter = CalculateDistance(x, y, cellCenter.x, cellCenter.y);
    return 1.0 / (1.0 + distanceToCenter);
}

void
PeeceRoutingProtocol::CalculateFitnessScore()
{
    NS_LOG_DEBUG("[Node " << m_selfNodeProps.nodeId << "] Calculating fitness score");
    // Calculate fitness score based on distance to cell center
    Point cellCenter = CalculateCellCenter(m_myCellId);
    double distanceToCenter = CalculateDistance(m_selfNodeProps.xCoord, m_selfNodeProps.yCoord, 
                                               cellCenter.x, cellCenter.y);
    m_fitnessScore = 1.0 / (1.0 + distanceToCenter);

    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Fitness score: " << m_fitnessScore 
                << " (distance to center: " << distanceToCenter << ")");
    LogStage("FITNESS_CALCULATED", "score=" + std::to_string(m_fitnessScore));
    
    // Trace fitness calculation to file
    if (m_trace) {
        double now = Simulator::Now().GetSeconds();
        std::string msg = "[" + std::to_string(now) + "s] Node " + std::to_string(m_selfNodeProps.nodeId)
                        + " FITNESS_CALCULATED"
                        + " cell=" + std::to_string(m_myCellId)
                        + " fitness=" + std::to_string(m_fitnessScore)
                        + " distToCenter=" + std::to_string(distanceToCenter)
                        + " centerPos=(" + std::to_string(cellCenter.x) + "," + std::to_string(cellCenter.y) + ")";
        m_trace->Trace(msg);

        // Calculate and trace fitness for known neighbors in same cell
        double bestNeighborFitness = -1.0;
        uint16_t bestNeighborId = 0xFFFF;
        for (const auto &neighbor : m_neighborTable) {
            if (neighbor.cellId != m_myCellId) {
                continue; // Only compare within same cell
            }
            double nf = ComputeFitness(neighbor.x, neighbor.y, m_myCellId);
            
            // Update known nodes map with fitness
            auto &knownNode = m_knownNodes[neighbor.nodeId];
            knownNode.x = neighbor.x;
            knownNode.y = neighbor.y;
            knownNode.cellId = neighbor.cellId;
            knownNode.fitness = nf;
            knownNode.hasFitness = true;
            knownNode.lastSeen = Simulator::Now();
            
            double nd = CalculateDistance(neighbor.x, neighbor.y, cellCenter.x, cellCenter.y);
            std::string nmsg = "[" + std::to_string(now) + "s] Node " + std::to_string(m_selfNodeProps.nodeId)
                             + " NEIGHBOR_FITNESS_CALCULATED neighbor=" + std::to_string(neighbor.nodeId)
                             + " cell=" + std::to_string(m_myCellId)
                             + " fitness=" + std::to_string(nf)
                             + " distToCenter=" + std::to_string(nd);
            m_trace->Trace(nmsg);
            
            if (nf > bestNeighborFitness || (nf == bestNeighborFitness && neighbor.nodeId < bestNeighborId)) {
                bestNeighborFitness = nf;
                bestNeighborId = neighbor.nodeId;
            }
        }
        
        // Trace best known candidate
        std::string bmsg = "[" + std::to_string(now) + "s] Node " + std::to_string(m_selfNodeProps.nodeId)
                         + " BEST_KNOWN_CANDIDATE candidate=" + (bestNeighborId == 0xFFFF ? std::string("none") : std::to_string(bestNeighborId))
                         + " fitness=" + (bestNeighborFitness < 0 ? std::string("n/a") : std::to_string(bestNeighborFitness));
        m_trace->Trace(bmsg);
    }
}

uint16_t
PeeceRoutingProtocol::FindBestCandidateInCell(int cellId, double &outFitness)
{
    double bestFitness = m_fitnessScore;
    uint16_t bestId = m_selfNodeProps.nodeId;
    
    // Compare against all known nodes in same cell with fitness data
    for (const auto &kv : m_knownNodes) {
        uint16_t nodeId = kv.first;
        const KnownNodeInfo &info = kv.second;
        
        if (info.cellId != cellId || !info.hasFitness) {
            continue;
        }
        
        // Apply tie-breaker: higher fitness wins, or if equal, lower node ID wins
        if (info.fitness > bestFitness || (info.fitness == bestFitness && nodeId < bestId)) {
            bestFitness = info.fitness;
            bestId = nodeId;
        }
    }
    
    outFitness = bestFitness;
    return bestId;
}

Point
PeeceRoutingProtocol::CalculateCellCenter(int cellId)
{
    int r = (int)std::round((double)cellId / m_gridOffset);
    int q = cellId - r * m_gridOffset;

    Point center;
    center.x = m_cellRadius * (std::sqrt(3.0) * q + std::sqrt(3.0) / 2.0 * r);
    center.y = m_cellRadius * (3.0 / 2.0 * r);

    return center;
}

double
PeeceRoutingProtocol::CalculateDistance(double x1, double y1, double x2, double y2)
{
    return std::sqrt(std::pow(x1 - x2, 2) + std::pow(y1 - y2, 2));
}

void
PeeceRoutingProtocol::CalculateCellInfo()
{
    NS_LOG_FUNCTION(this);
    
    double frac_q = (std::sqrt(3.0) / 3.0 * m_selfNodeProps.xCoord - 1.0 / 3.0 * m_selfNodeProps.yCoord) / m_cellRadius;
    double frac_r = (2.0 / 3.0 * m_selfNodeProps.yCoord) / m_cellRadius;
    double frac_s = -frac_q - frac_r;

    int q = (int)std::round(frac_q);
    int r = (int)std::round(frac_r);
    int s = (int)std::round(frac_s);

    double q_diff = std::abs(q - frac_q);
    double r_diff = std::abs(r - frac_r);
    double s_diff = std::abs(s - frac_s);

    if (q_diff > r_diff && q_diff > s_diff) {
        q = -r - s;
    } else if (r_diff > s_diff) {
        r = -q - s;
    } else {
        s = -q - r;
    }

    m_myCellId = q + r * m_gridOffset;
    m_myColor = ((q - r) % 3 + 3) % 3;

    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Cell assignment: cellId=" << m_myCellId 
                << " color=" << m_myColor << " (q=" << q << " r=" << r << ")");
    
    // Write to initialization file
    static std::ofstream initFile("node-init-state.txt", std::ios::app);
    if (!initFile.is_open()) {
        initFile.open("node-init-state.txt", std::ios::app);
    }
    if (initFile.is_open()) {
        initFile << "Node " << m_selfNodeProps.nodeId
                << " pos=(" << m_selfNodeProps.xCoord << "," << m_selfNodeProps.yCoord << ")"
                << " cell=" << m_myCellId
                << " color=" << m_myColor << std::endl;
    }
}

void
PeeceRoutingProtocol::HandleHelloPacket(PeeceHeader &header, uint16_t src)
{
    // TODO: Optimize neighbor table management: save neighbor info (include 2-hop neighbors): x,y,cellId, neighborIds[]
    // TODO: Memory all kwown nodes info to allow fitness calculation for all known nodes (x,y,cellId,fitness)
    // Use MAC layer source (src parameter)
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Received HELLO from Node " << src);
    if (m_trace)
    {
        double now = Simulator::Now().GetSeconds();
        std::string msg = "[" + std::to_string(now) + "s] Node " + std::to_string(m_selfNodeProps.nodeId)
                        + " HELLO_RECV from=" + std::to_string(src);
        m_trace->Trace(msg);
    }
    
    HelloPacketInfo helloData = header.GetHelloData();
    // Trace received neighbor table
    if (m_trace)
    {
        double nowTbl = Simulator::Now().GetSeconds();
        std::string tbl = "[";
        for (uint8_t i = 0; i < helloData.neighborCount; i++) {
            tbl += std::to_string(helloData.neighborIds[i]);
            if (i + 1 < helloData.neighborCount) tbl += ",";
        }
        tbl += "]";
        std::string msgTbl = "[" + std::to_string(nowTbl) + "s] Node " + std::to_string(m_selfNodeProps.nodeId)
                           + " RECV_NEIGHBOR_TABLE from=" + std::to_string(src)
                           + " count=" + std::to_string(helloData.neighborCount)
                           + " ids=" + tbl;
        m_trace->Trace(msgTbl);
    }
    
    // Check if neighbor already exists
    for (auto& neighbor : m_neighborTable) {
        if (neighbor.nodeId == src) {
            // Existing neighbor: update its 2-hop neighbor list from received HELLO
            std::vector<uint16_t> oldTwoHop = neighbor.neighborNodeIds;

            neighbor.x = header.GetHelloData().x;
            neighbor.y = header.GetHelloData().y;
            neighbor.cellId = header.GetHelloData().cellId;
            neighbor.neighborNodeIds.clear();
            HelloPacketInfo helloDataUpd = header.GetHelloData();
            for (uint8_t i = 0; i < helloDataUpd.neighborCount && neighbor.neighborNodeIds.size() < 255; i++) {
                uint16_t nid = helloDataUpd.neighborIds[i];
                if (nid != 0xFFFF && nid != src) {
                    neighbor.neighborNodeIds.push_back(nid);
                }
            }

            // Trace update if changed
            if (m_trace) {
                bool changed = (oldTwoHop.size() != neighbor.neighborNodeIds.size());
                if (!changed) {
                    for (size_t k = 0; k < oldTwoHop.size(); ++k) {
                        if (oldTwoHop[k] != neighbor.neighborNodeIds[k]) { changed = true; break; }
                    }
                }
                if (changed) {
                    double nowUpd = Simulator::Now().GetSeconds();
                    std::string oldStr = "[";
                    for (size_t k = 0; k < oldTwoHop.size(); ++k) {
                        oldStr += std::to_string(oldTwoHop[k]);
                        if (k + 1 < oldTwoHop.size()) oldStr += ",";
                    }
                    oldStr += "]";
                    std::string newStr = "[";
                    for (size_t k = 0; k < neighbor.neighborNodeIds.size(); ++k) {
                        newStr += std::to_string(neighbor.neighborNodeIds[k]);
                        if (k + 1 < neighbor.neighborNodeIds.size()) newStr += ",";
                    }
                    newStr += "]";
                    std::string msgUpd = "[" + std::to_string(nowUpd) + "s] Node " + std::to_string(m_selfNodeProps.nodeId)
                                       + " NEIGHBOR_TABLE_UPDATE via=" + std::to_string(src)
                                       + " old=" + oldStr + " new=" + newStr;
                    m_trace->Trace(msgUpd);
                }
            }
            return;  // Already in table; updated
        }
    }

    // Check distance threshold
    double distance = CalculateDistance(m_selfNodeProps.xCoord, m_selfNodeProps.yCoord,
                                       helloData.x, helloData.y);
    if (distance > m_cellRadius) {
        return;  // Too far away
    }

    // Add new neighbor
    NeighborRecord newNeighbor;
    newNeighbor.nodeId = src;
    newNeighbor.x = helloData.x;
    newNeighbor.y = helloData.y;
    newNeighbor.cellId = helloData.cellId;
    
    // Update known nodes map for this 1-hop neighbor
    auto &knownNode = m_knownNodes[src];
    knownNode.x = helloData.x;
    knownNode.y = helloData.y;
    knownNode.cellId = helloData.cellId;
    knownNode.hasFitness = false;  // Will be computed in CalculateFitnessScore
    knownNode.lastSeen = Simulator::Now();
    
    // Learn about neighbor's neighbors (2-hop discovery)
    // Include all neighbors from the HELLO, except the sender's own ID (to avoid self-loops)
    // Limit to 255 max to prevent memory issues
    for (uint8_t i = 0; i < helloData.neighborCount && newNeighbor.neighborNodeIds.size() < 255; i++) {
        if (helloData.neighborIds[i] != 0xFFFF && helloData.neighborIds[i] != src) {
            newNeighbor.neighborNodeIds.push_back(helloData.neighborIds[i]);
            
            // Register 2-hop node in known nodes (without position yet)
            if (m_knownNodes.find(helloData.neighborIds[i]) == m_knownNodes.end()) {
                KnownNodeInfo twoHopInfo;
                twoHopInfo.hasFitness = false;
                twoHopInfo.cellId = -1;  // Unknown until we receive their HELLO
                twoHopInfo.lastSeen = Simulator::Now();
                m_knownNodes[helloData.neighborIds[i]] = twoHopInfo;
            }
        }
    }
    
    m_neighborTable.push_back(newNeighbor);

    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Added neighbor Node " << src 
                << " at distance " << distance << " with " << newNeighbor.neighborNodeIds.size() 
                << " neighbors (total neighbors: " << m_neighborTable.size() << ")");
    if (m_trace)
    {
        double now2 = Simulator::Now().GetSeconds();
        std::string msg3 = "[" + std::to_string(now2) + "s] Node " + std::to_string(m_selfNodeProps.nodeId)
                         + " NEIGHBOR_ADDED id=" + std::to_string(src)
                         + " total=" + std::to_string(m_neighborTable.size())
                         + " dist=" + std::to_string(distance);
        m_trace->Trace(msg3);
    }
}

void
PeeceRoutingProtocol::HandleCLAnnouncementPacket(PeeceHeader &header, uint16_t src)
{
    double now = Simulator::Now().GetSeconds();
    
    // IMPORTANT: Use header.GetSource() for original sender, not MAC src (which could be forwarder)
    uint16_t originalSrc = header.GetSource();
    
    // Ignore packets from self (broadcast loopback)
    if (originalSrc == m_selfNodeProps.nodeId) {
        return;
    }
    
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Received CL_ANNOUNCEMENT from Node " << originalSrc 
                << " (MAC src=" << src << ")");
    
    CLAnnouncementInfo clInfo = header.GetCLAnnouncementData();
    LogStage("CL_ANNOUNCEMENT_RECV", "from=" + std::to_string(originalSrc) + " fitness=" + std::to_string(clInfo.fitnessScore));
    
    // Trace to file
    if (m_trace) {
        std::string msg = "[" + std::to_string(now) + "s] Node " + std::to_string(m_selfNodeProps.nodeId) 
                        + " CL_ANNOUNCEMENT_RECV from=" + std::to_string(originalSrc) 
                        + " fitness=" + std::to_string(clInfo.fitnessScore);
        m_trace->Trace(msg);
    }
    
    if (clInfo.cellId != m_myCellId) {
        NS_LOG_DEBUG("[Node " << m_selfNodeProps.nodeId << "] Ignoring CL from different cell (" << clInfo.cellId << ")");
        return;  // Not for our cell
    }

    // Mark that we received at least one announcement
    m_receivedCLAnnouncement = true;

    // Select this CL if:
    // 1. Their fitness is HIGHER than ours, OR
    // 2. Fitness is EQUAL but their node ID is LOWER (tie-breaker)
    // This ensures only one CL per cell when multiple nodes have same fitness
    bool isBetterCandidate = (clInfo.fitnessScore > m_fitnessScore) || 
                            (clInfo.fitnessScore == m_fitnessScore && originalSrc < m_selfNodeProps.nodeId);
    
    if (isBetterCandidate) {
        // Only update if this CL is better than what we have
        bool shouldUpdate = (m_myClId == 0xFFFF) || 
                           (clInfo.fitnessScore > m_clFitnessScore) ||
                           (clInfo.fitnessScore == m_clFitnessScore && originalSrc < m_myClId);
        
        if (shouldUpdate) {
            m_myClId = originalSrc;
            m_clFitnessScore = clInfo.fitnessScore;
            std::string reason = (clInfo.fitnessScore > m_fitnessScore) ? "higher fitness" : "same fitness, lower ID";
            NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Selected CL: Node " << m_myClId 
                        << " with fitness " << m_clFitnessScore << " (my fitness: " << m_fitnessScore 
                        << ", reason: " << reason << ")");
            
            // Schedule confirmation
            double delayMs = m_clConfirmationTime + (rand() % 100);
            Simulator::Schedule(MilliSeconds(delayMs), 
                       &PeeceRoutingProtocol::TimerCallback_CLConfirmation, this);
        }
    } else {
        NS_LOG_DEBUG("[Node " << m_selfNodeProps.nodeId << "] Ignoring CL announcement: their fitness (" 
                    << clInfo.fitnessScore << ") is not better than mine (" << m_fitnessScore << ")");
    }
    
    // FORWARD/RELAY: If TTL > 1, rebroadcast to help propagate to entire cell
    uint8_t ttl = header.GetTtl();
    if (ttl > 1) {
        NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Forwarding CL announcement (TTL=" << (int)ttl << " -> " << (int)(ttl-1) << ")");
        
        // Preserve original CL announcer as source (not MAC forwarder)
        // This ensures tie-breaker logic uses correct node ID
        PeeceHeader fwdHeader;
        fwdHeader.SetPacketType(PEECE_CL_ANNOUNCEMENT);
        fwdHeader.SetSource(originalSrc);  // Preserve original announcer, not MAC src
        fwdHeader.SetDestination(0xFFFF);
        fwdHeader.SetTtl(ttl - 1);
        fwdHeader.SetCLAnnouncementData(clInfo);
        
        // Trace forwarding event
        if (m_trace) {
            std::string fwdMsg = "[" + std::to_string(now) + "s] Node " + std::to_string(m_selfNodeProps.nodeId)
                               + " CL_ANNOUNCEMENT_FWD orig=" + std::to_string(originalSrc)
                               + " macSrc=" + std::to_string(src)
                               + " ttl=" + std::to_string((int)(ttl-1));
            m_trace->Trace(fwdMsg);
        }
        
        // Optional: Unicast to neighbors not in CL's neighbor list (reduces redundancy)
        // Check if we know CL's neighbors from 2-hop discovery
        bool canOptimize = false;
        std::vector<uint16_t> clNeighbors;
        for (const auto &neighbor : m_neighborTable) {
            if (neighbor.nodeId == originalSrc) {
                clNeighbors = neighbor.neighborNodeIds;
                canOptimize = true;
                break;
            }
        }
        
        if (canOptimize && !clNeighbors.empty()) {
            // Unicast only to neighbors NOT in CL's neighbor list
            std::vector<uint16_t> targets;
            for (const auto &neighbor : m_neighborTable) {
                if (neighbor.cellId == m_myCellId &&
                    std::find(clNeighbors.begin(), clNeighbors.end(), neighbor.nodeId) == clNeighbors.end()) {
                    targets.push_back(neighbor.nodeId);
                }
            }
            
            if (!targets.empty()) {
                NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Optimized forwarding to " << targets.size() << " targets");
                for (uint16_t targetId : targets) {
                    Ptr<Packet> fwdPacket = Create<Packet>();
                    fwdPacket->AddHeader(fwdHeader);
                    double fwdDelay = 5 + (rand() % 10);
                    Simulator::Schedule(MilliSeconds(fwdDelay),
                                       &PeeceRoutingProtocol::ToMacLayer, this, fwdPacket, targetId);
                }
                return;  // Skip broadcast fallback
            }
        }
        
        // Fallback: Broadcast to all neighbors
        Ptr<Packet> fwdPacket = Create<Packet>();
        fwdPacket->AddHeader(fwdHeader);
        
        // Small random delay to avoid collision
        double fwdDelay = 5 + (rand() % 10);
        Simulator::Schedule(MilliSeconds(fwdDelay), 
                           &PeeceRoutingProtocol::ToMacLayer, this, fwdPacket, (uint16_t)0xFFFF);
    }
}

void
PeeceRoutingProtocol::HandleCLConfirmationPacket(PeeceHeader &header, uint16_t src)
{
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Received CL_CONFIRMATION from Node " << src);
    LogStage("CL_CONFIRMATION_RECV", "from=" + std::to_string(src));
    
    CLConfirmationInfo confirmInfo = header.GetCLConfirmationData();
    
    // Check if this is directed to us (we are the CL)
    if (confirmInfo.clId != m_selfNodeProps.nodeId) {
        return;  // Not directed to us
    }
    
    // Add confirmed member to cell members (even if not yet officially CL, buffer for later)
    bool exists = false;
    for (const auto &m : m_cellMembers) {
        if (m.nodeId == confirmInfo.nodeId) { exists = true; break; }
    }
    if (!exists) {
        CellMemberRecord member;
        member.nodeId = confirmInfo.nodeId;
        member.x = m_selfNodeProps.xCoord;
        member.y = m_selfNodeProps.yCoord;
        member.energy = 1.0;
        m_cellMembers.push_back(member);
    }

    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Added cell member Node " << confirmInfo.nodeId 
                << " (total members: " << m_cellMembers.size() << ")");
    // File trace: member added to this CL's member list (if we are CL)
    if (m_trace) {
        double now = Simulator::Now().GetSeconds();
        std::string msg = "[" + std::to_string(now) + "s] Node " + std::to_string(m_selfNodeProps.nodeId)
                        + " CL_MEMBER_ADDED cl=" + std::to_string(confirmInfo.clId)
                        + " member=" + std::to_string(confirmInfo.nodeId)
                        + " total=" + std::to_string(m_cellMembers.size());
        m_trace->Trace(msg);
    }
}

void
PeeceRoutingProtocol::HandleRoutingTableAnnouncementPacket(PeeceHeader &header, uint16_t src)
{
    double now = Simulator::Now().GetSeconds();
    uint16_t actualSrc = src;
    NS_LOG_INFO("[t=" << now << "s] [Node " << m_selfNodeProps.nodeId << "] ===== ROUTING TABLE RECV =====");
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Received ROUTING_TABLE from Node " << actualSrc);
    LogStage("ROUTING_TABLE_RECV", "from=" + std::to_string(actualSrc));
    
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] ---- CHECKING ROUTING UPDATES ----");
    
    bool foundMyEntry = false;
    // Update routing table based on received announcement
    for (int i = 0; i < 7; i++) {
        RoutingUpdateData updateData = header.GetRoutingUpdateData(i);
        
        NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Entry " << i 
                    << ": nodeId=" << updateData.nodeId << " nextHop=" << updateData.nextHop);
        
        if (updateData.nodeId == m_selfNodeProps.nodeId) {
            NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] *** FOUND MY ROUTING ENTRY ***");
            NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] OLD m_myNextHopId=" << m_myNextHopId 
                        << " NEW m_myNextHopId=" << updateData.nextHop);
            m_myNextHopId = updateData.nextHop;
            LogStage("ROUTING_TABLE_UPDATE", "nextHop=" + std::to_string(m_myNextHopId));
            NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] *** NEXT HOP UPDATED TO NODE " << m_myNextHopId << " ***");
            foundMyEntry = true;
            break;
        }
    }
    
    // If no entry found for me, calculate my own next hop
    if (!foundMyEntry && m_myNextHopId == 0xFFFF) {
        NS_LOG_WARN("[Node " << m_selfNodeProps.nodeId << "] No routing entry found for me - calculating own next hop");
        CalculateMyNextHop();
    }
    
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] ===== ROUTING TABLE RECV COMPLETE =====");
}

void
PeeceRoutingProtocol::HandleCHAnnouncementPacket(PeeceHeader &header, uint16_t src)
{
    uint16_t actualSrc = src;
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Received CH_ANNOUNCEMENT from Node " << actualSrc);
    LogStage("CH_ANNOUNCEMENT_RECV", "from=" + std::to_string(actualSrc));
    
    CHAnnouncementInfo chInfo = header.GetCHAnnouncementData();
    m_myChId = chInfo.chId;

    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Registered CH Node " << m_myChId);
}

void
PeeceRoutingProtocol::TimerCallback_FinalizeRouting()
{
    NS_LOG_FUNCTION(this);
    FinalizeRouting();
}

void
PeeceRoutingProtocol::FinalizeRouting()
{
    NS_LOG_FUNCTION(this);
    // Finalize routing table and prepare for data phase
    NS_LOG_INFO("Node " << m_selfNodeProps.nodeId << " finalized routing");
}

void
PeeceRoutingProtocol::SendSensorDataPacket()
{
    NS_LOG_FUNCTION(this);
    double now = Simulator::Now().GetSeconds();
    LogStage("SENSING_SEND");
    
    NS_LOG_INFO("[t=" << now << "s] [Node " << m_selfNodeProps.nodeId << "] ===== SENSOR DATA SEND =====");
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] m_myNextHopId=" << m_myNextHopId 
                << " m_myChId=" << m_myChId << " m_myCellId=" << m_myCellId);
    
    PeeceHeader header;
    header.SetPacketType(PEECE_SENSOR_DATA);
    header.SetSource(m_selfNodeProps.nodeId);
    // Prefer unicast to next hop if known; else broadcast
    uint16_t dst = (m_myNextHopId != 0xFFFF) ? m_myNextHopId : 0xFFFF;
    header.SetDestination(dst);
    
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Destination: " << (dst == 0xFFFF ? "BROADCAST" : std::to_string(dst)));

    SensorDataInfo sensorData;
    sensorData.dataId = (int)(Simulator::Now().GetSeconds() * 1000);
    sensorData.sensorId = m_selfNodeProps.nodeId;
    sensorData.destinationCH = m_myChId;
    sensorData.hopCount = 0;

    header.SetSensorData(sensorData);
    header.SetCellDestination(m_myCellId);

    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(header);

    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Sent SENSOR_DATA packet (dataId: " << sensorData.dataId 
                << " sensorID: " << sensorData.sensorId << ")");
    
    TraceSensingPacket("SEND", m_selfNodeProps.nodeId, m_selfNodeProps.nodeId, dst, sensorData.dataId);
    ToMacLayer(packet, dst);
    
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] ===== SENSOR DATA SEND COMPLETE =====");
}

void
PeeceRoutingProtocol::HandleSensorDataPacket(PeeceHeader &header, uint16_t src)
{
    double now = Simulator::Now().GetSeconds();
    uint16_t actualSrc = src;
    uint16_t pktSrc = header.GetSource();
    uint16_t pktDst = header.GetDestination();
    uint16_t pktCell = header.GetCellDestination();
    
    NS_LOG_INFO("[t=" << now << "s] [Node " << m_selfNodeProps.nodeId << "] ===== SENSOR DATA RECV =====");
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] MAC Src=" << actualSrc << " Header Src=" << pktSrc 
                << " Header Dst=" << pktDst << " Cell=" << pktCell);
    
    SensorDataInfo sensorData = header.GetSensorData();
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] SensorID=" << sensorData.sensorId 
                << " DataID=" << sensorData.dataId << " HopCount=" << (int)sensorData.hopCount);
    
    // Check if this packet is for me
    bool isForMe = (pktDst == m_selfNodeProps.nodeId) || (pktDst == 0xFFFF);
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] *** Packet destined for me? " 
                << (isForMe ? "YES" : "NO") << " (pktDst=" << pktDst 
                << ", myNode=" << m_selfNodeProps.nodeId << ") ***");
    
    if (!isForMe) {
        NS_LOG_WARN("[Node " << m_selfNodeProps.nodeId << "] *** DROPPING: Packet not for me (dst=" 
                    << pktDst << ", I am " << m_selfNodeProps.nodeId << ") ***");
        return;
    }
    
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] *** TRACING RECV EVENT ***");
    TraceSensingPacket("RECV", m_selfNodeProps.nodeId, actualSrc, 0xFFFF, sensorData.dataId);
    
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] ---- CHECKING RECEPTION ----");
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] m_myChId=" << m_myChId 
                << " m_selfNodeProps.nodeId=" << m_selfNodeProps.nodeId 
                << " m_myCellId=" << m_myCellId << " pktCell=" << pktCell);
    
    // If I'm the CH, accept the data
    bool iAmCH = (m_myChId == m_selfNodeProps.nodeId);
    bool cellMatches = (pktCell == m_myCellId);
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] I am CH? " << (iAmCH ? "YES" : "NO"));
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Cell matches? " << (cellMatches ? "YES" : "NO"));
    
    if (iAmCH && cellMatches) {
        NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] *** I AM CH - ACCEPTING DATA ***");
        LogStage("SENSING_RECV_ACCEPTED", "from=" + std::to_string(actualSrc) + " sensorID=" + std::to_string(sensorData.sensorId));
        NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] ===== SENSOR DATA RECV COMPLETE =====");
        return;
    }
    
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] ---- FORWARDING CHECK ----");
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] m_myNextHopId=" << m_myNextHopId 
                << " (0xFFFF=" << 0xFFFF << ") m_myChId=" << m_myChId);
    
    // Check if I should forward this packet
    bool shouldForward = false;
    std::string noForwardReason = "";
    
    if (m_myNextHopId == 0xFFFF) {
        noForwardReason = "no_nexthop";
    } else if (m_myNextHopId == m_selfNodeProps.nodeId) {
        noForwardReason = "nexthop_is_myself";
    } else if (m_myChId == m_selfNodeProps.nodeId) {
        noForwardReason = "i_am_ch";
    } else {
        shouldForward = true;
    }
    
    if (!shouldForward) {
        NS_LOG_WARN("[Node " << m_selfNodeProps.nodeId << "] *** NO FORWARD: " << noForwardReason << " ***");
        LogStage("SENSING_RECV_NO_FORWARD", "reason=" + noForwardReason + " sensorID=" + std::to_string(sensorData.sensorId));
        NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] ===== SENSOR DATA RECV COMPLETE =====");
        return;
    }

    // Forward toward next hop
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] *** FORWARDING TO NEXT HOP " << m_myNextHopId << " ***");
    sensorData.hopCount++;
    header.SetSensorData(sensorData);

    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(header);
    
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Forwarding SENSOR_DATA to Node " 
                << m_myNextHopId << " (new hop count: " << (int)sensorData.hopCount << ")");
    
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] *** TRACING FWD EVENT ***");
    TraceSensingPacket("FWD", m_selfNodeProps.nodeId, actualSrc, m_myNextHopId, sensorData.dataId);
    LogStage("SENSING_RECV_FORWARD", "to=" + std::to_string(m_myNextHopId) + " hopCount=" + std::to_string(sensorData.hopCount));
    
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Sending to MAC layer: dst=" << m_myNextHopId);
    ToMacLayer(packet, m_myNextHopId);
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] ===== SENSOR DATA RECV COMPLETE =====");
}

void
PeeceRoutingProtocol::SendCellPacket()
{
    NS_LOG_FUNCTION(this);
    // Send queued cell packets
}

void
PeeceRoutingProtocol::SendCellHopAnnouncementPacket()
{
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Sending CELL_HOP_ANNOUNCEMENT (nextCell: " << m_myNextCellHop << ")");
    LogStage("CELL_HOP_ANNOUNCE_SEND", "nextCell=" + std::to_string(m_myNextCellHop));
    
    if (m_myRole == CELL_LEADER) {
        PeeceHeader header;
        header.SetPacketType(PEECE_ANNOUNCE_CELL_HOP);
        header.SetSource(m_selfNodeProps.nodeId);
        header.SetDestination(0xFFFF);

        CellHopAnnouncementInfo cellHopInfo;
        cellHopInfo.nextCell = m_myNextCellHop;
        std::fill(cellHopInfo.cellPath, cellHopInfo.cellPath + 100, -1);

        header.SetCellHopAnnouncementData(cellHopInfo);

        Ptr<Packet> packet = Create<Packet>();
        packet->AddHeader(header);

        ToMacLayer(packet, 0xFFFF);
    }
}

void
PeeceRoutingProtocol::HandleCellHopAnnouncementPacket(PeeceHeader &header, uint16_t src)
{
    uint16_t actualSrc = src;
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Received CELL_HOP_ANNOUNCEMENT from Node " << actualSrc);
    LogStage("CELL_HOP_ANNOUNCE_RECV", "from=" + std::to_string(actualSrc));
    
    CellHopAnnouncementInfo cellHopInfo = header.GetCellHopAnnouncementData();
    m_myNextCellHop = cellHopInfo.nextCell;
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Updated next cell hop to cell " << m_myNextCellHop);
}

void
PeeceRoutingProtocol::FromMacLayer(Ptr<Packet> pkt, const uint16_t src)
{
    double now = Simulator::Now().GetSeconds();
    uint32_t pktSize = pkt->GetSize();
    
    NS_LOG_INFO("[t=" << now << "s] [Node " << m_selfNodeProps.nodeId << "] ===== RX PACKET ===== "
                << "from Node " << src << " (" << pktSize << " bytes)");
    
    PeeceHeader header;
    pkt->RemoveHeader(header);

    uint8_t packetType = header.GetPacketType();
    const char* typeNames[] = {"HELLO", "CL_ANNOUNCEMENT", "CL_CONFIRMATION", "ROUTING_TREE_UPDATE", 
                               "CH_ANNOUNCEMENT", "ANNOUNCE_CELL_HOP", "SENSOR_DATA"};
    
    std::string typeName = (packetType < 7) ? typeNames[packetType] : "UNKNOWN";
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Packet Type: " << typeName 
                << " (Type=" << (int)packetType << ")");
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Header: Src=" << header.GetSource() 
                << " Dst=" << header.GetDestination() 
                << " Cell=" << header.GetCellDestination());
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] MAC Src=" << src 
                << " My Node=" << m_selfNodeProps.nodeId);

    switch (packetType) {
        case PEECE_HELLO_PACKET:
            NS_LOG_DEBUG("[Node " << m_selfNodeProps.nodeId << "] Processing HELLO packet");
            HandleHelloPacket(header, src);
            break;
        case PEECE_CL_ANNOUNCEMENT:
            NS_LOG_DEBUG("[Node " << m_selfNodeProps.nodeId << "] Processing CL_ANNOUNCEMENT packet");
            HandleCLAnnouncementPacket(header, src);
            break;
        case PEECE_CL_CONFIRMATION:
            NS_LOG_DEBUG("[Node " << m_selfNodeProps.nodeId << "] Processing CL_CONFIRMATION packet");
            HandleCLConfirmationPacket(header, src);
            break;
        case PEECE_ROUTING_TREE_UPDATE:
            NS_LOG_DEBUG("[Node " << m_selfNodeProps.nodeId << "] Processing ROUTING_TREE_UPDATE packet");
            HandleRoutingTableAnnouncementPacket(header, src);
            break;
        case PEECE_CH_ANNOUNCEMENT:
            NS_LOG_DEBUG("[Node " << m_selfNodeProps.nodeId << "] Processing CH_ANNOUNCEMENT packet");
            HandleCHAnnouncementPacket(header, src);
            break;
        case PEECE_ANNOUNCE_CELL_HOP:
            NS_LOG_DEBUG("[Node " << m_selfNodeProps.nodeId << "] Processing CELL_HOP_ANNOUNCEMENT packet");
            HandleCellHopAnnouncementPacket(header, src);
            break;
        case PEECE_SENSOR_DATA:
            NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] *** DISPATCHING TO HandleSensorDataPacket ***");
            NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] Packet Dst=" << header.GetDestination() 
                        << " (BROADCAST=" << 0xFFFF << ")");
            HandleSensorDataPacket(header, src);
            break;
        default:
            NS_LOG_WARN("[Node " << m_selfNodeProps.nodeId << "] Unknown packet type: " << (int)packetType);
            break;
    }
    
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] ===== RX COMPLETE =====");
}

void
PeeceRoutingProtocol::ClearNeighborTable()
{
    m_neighborTable.clear();
}

void
PeeceRoutingProtocol::LogStage(const std::string &stage, const std::string &details)
{
    double t = Simulator::Now().GetSeconds();
    std::string msg = "[t=" + std::to_string(t) + "s] [Node " + std::to_string(m_selfNodeProps.nodeId) + "] >>> STAGE: " + stage;
    if (!details.empty()) {
        msg += " - " + details;
    }
    NS_LOG_INFO(msg);
}

void
PeeceRoutingProtocol::InitializeNeighborRecord(NeighborRecord &record, uint16_t nodeId,
                                               double x, double y, int cellId)
{
    record.nodeId = nodeId;
    record.x = x;
    record.y = y;
    record.cellId = cellId;
}

void
PeeceRoutingProtocol::TimerCallback_State1()
{
    NS_LOG_FUNCTION(this);
}

void
PeeceRoutingProtocol::TimerCallback_SensingStart()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("[Node " << m_selfNodeProps.nodeId << "] === SENSING PHASE START ===");
    LogStage("SENSING_START", "cl=" + std::to_string(m_myClId));
    
    // Start periodic sensing
    double sensingInterval = 5.0;  // seconds between sensor readings
    Simulator::Schedule(MilliSeconds(sensingInterval * 1000), 
                       &PeeceRoutingProtocol::TimerCallback_SensingState, this);
}

void
PeeceRoutingProtocol::TimerCallback_SensingState()
{
    NS_LOG_FUNCTION(this);
    SendSensorDataPacket();
    
    // Schedule next sensing round
    double sensingInterval = 5.0;  // seconds between sensor readings
    Simulator::Schedule(MilliSeconds(sensingInterval * 1000), 
                       &PeeceRoutingProtocol::TimerCallback_SensingState, this);
}

void
PeeceRoutingProtocol::TimerCallback_Reconfiguration()
{
    NS_LOG_FUNCTION(this);
}

void
PeeceRoutingProtocol::TimerCallback_AnnounceCellHop()
{
    NS_LOG_FUNCTION(this);
    SendCellHopAnnouncementPacket();
}

void
PeeceRoutingProtocol::TimerCallback_SendCellPacket()
{
    NS_LOG_FUNCTION(this);
    SendCellPacket();
}

void
PeeceRoutingProtocol::TraceSensingPacket(const std::string &action, uint16_t nodeId, uint16_t src, uint16_t dst, int dataId)
{
    double t = Simulator::Now().GetSeconds();
    std::string msg = "[t=" + std::to_string(t) + "s] [Node " + std::to_string(nodeId) + "] >>> SENSING_TRACE: " 
                      + action + " dataId=" + std::to_string(dataId) + " src=" + std::to_string(src)
                      + " dst=" + std::to_string(dst);
    NS_LOG_INFO(msg);
    m_sensingTrace(action, nodeId, src, dst, dataId);
}

} // namespace wsn
} // namespace ns3
