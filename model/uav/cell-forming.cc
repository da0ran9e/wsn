/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Cell Forming Module Implementation (Phase 0)
 */

#include "cell-forming.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include <cmath>
#include <algorithm>

NS_LOG_COMPONENT_DEFINE("CellForming");

namespace ns3
{
    NS_OBJECT_ENSURE_REGISTERED(CellForming);

TypeId
CellForming::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CellForming")
        .SetParent<Object>()
        .SetGroupName("Wsn")
        .AddConstructor<CellForming>();
    return tid;
}

CellForming::CellForming()
    : m_nodeId(0),
      m_cellRadius(100.0),
      m_gridOffset(100),
      m_helloInterval(1.0),
      m_clElectionDelayInterval(0.5),
      m_clCalculationTime(2.0),
      m_state(DISCOVERING),
      m_cellId(-1),
      m_color(-1),
      m_cellLeaderId(0),
      m_myCellLeaderFitness(0.0)
{
}

CellForming::~CellForming()
{
    if (!m_helloEvent.IsExpired())
        m_helloEvent.Cancel();
    if (!m_clElectionEvent.IsExpired())
        m_clElectionEvent.Cancel();
    if (!m_clCalculationEvent.IsExpired())
        m_clCalculationEvent.Cancel();
}

void
CellForming::SetNodeParams(uint32_t nodeId, Vector position, double cellRadius, int32_t gridOffset)
{
    m_nodeId = nodeId;
    m_position = position;
    m_cellRadius = cellRadius;
    m_gridOffset = gridOffset;
}

void
CellForming::SetTimingParams(double helloInterval, double clElectionDelayInterval, double clCalculationTime)
{
    m_helloInterval = helloInterval;
    m_clElectionDelayInterval = clElectionDelayInterval;
    m_clCalculationTime = clCalculationTime;
}

void
CellForming::Initialize()
{
    CalculateCellInfo();
    NS_LOG_INFO("Node " << m_nodeId << " initialized in cell " << m_cellId << " color " << m_color);
    SetState(DISCOVERING, "Initialized, starting neighbor discovery");
    ScheduleHello();
}

void
CellForming::CalculateCellInfo()
{
    // Phase 0.1: Calculate hex cell coordinates from position
    double x = m_position.x;
    double y = m_position.y;
    
    // Fractional axial coordinates
    double q_f = (std::sqrt(3.0) / 3.0 * x - 1.0 / 3.0 * y) / m_cellRadius;
    double r_f = (2.0 / 3.0 * y) / m_cellRadius;
    double s_f = -q_f - r_f;
    
    // Round to nearest integer cube coords with error correction
    int32_t q = static_cast<int32_t>(std::round(q_f));
    int32_t r = static_cast<int32_t>(std::round(r_f));
    int32_t s = static_cast<int32_t>(std::round(s_f));
    
    // Error correction
    double q_err = std::abs(q - q_f);
    double r_err = std::abs(r - r_f);
    double s_err = std::abs(s - s_f);
    
    if (q_err > r_err && q_err > s_err)
        q = -r - s;
    else if (r_err > s_err)
        r = -q - s;
    else
        s = -q - r;
    
    m_cellId = q + r * m_gridOffset;
    m_color = ((q - r) % 3 + 3) % 3;
    
    NS_LOG_DEBUG("Node " << m_nodeId << " at (" << x << ", " << y << ") → cellId=" << m_cellId 
                 << " color=" << m_color << " (q=" << q << " r=" << r << ")");
}

double
CellForming::CalculateFitness()
{
    // Phase 0.3: Calculate fitness based on distance to cell center
    Vector center = GetCellCenter(m_cellId);
    double distance = std::sqrt(std::pow(m_position.x - center.x, 2) + std::pow(m_position.y - center.y, 2));
    double fitness = 1.0 / (1.0 + distance);
    return fitness;
}

Vector
CellForming::GetCellCenter(int32_t cellId)
{
    // Reverse: from cellId = q + r * gridOffset, extract q and r
    // Then compute hex center: centerX = cellRadius(√3*q + √3/2*r), centerY = cellRadius(3/2*r)
    int32_t q = cellId % m_gridOffset;
    int32_t r = cellId / m_gridOffset;
    
    double centerX = m_cellRadius * (std::sqrt(3.0) * q + std::sqrt(3.0) / 2.0 * r);
    double centerY = m_cellRadius * (3.0 / 2.0 * r);
    
    return Vector(centerX, centerY, 0.0);
}

void
CellForming::BroadcastHello()
{
    // Phase 0.2: Broadcast HELLO with current neighbors
    HelloPacket hello;
    hello.senderId = m_nodeId;
    hello.senderPosition = m_position;
    hello.senderCellId = m_cellId;
    
    // Include current neighbor list
    for (const auto& neighbor : m_neighbors)
    {
        hello.neighborList.push_back(neighbor);
    }
    
    NS_LOG_INFO("Node " << m_nodeId << " broadcasting HELLO with " << hello.neighborList.size() 
                << " neighbors");
    
    if (!m_helloCallback.IsNull())
    {
        m_helloCallback(hello);
    }
    
    ScheduleHello();
}

void
CellForming::ScheduleHello()
{
    if (!m_helloEvent.IsExpired())
        m_helloEvent.Cancel();
    
    m_helloEvent = Simulator::Schedule(Seconds(m_helloInterval), &CellForming::BroadcastHello, this);
}

void
CellForming::HandleHelloPacket(const HelloPacket& hello)
{
    // Phase 0.2: Process HELLO from neighbor
    double distance = std::sqrt(std::pow(m_position.x - hello.senderPosition.x, 2) + 
                               std::pow(m_position.y - hello.senderPosition.y, 2));
    
    if (distance > m_cellRadius)
    {
        // Too far, not a neighbor
        return;
    }
    
    // Add or update neighbor
    NeighborInfo neighbor;
    neighbor.nodeId = hello.senderId;
    neighbor.position = hello.senderPosition;
    neighbor.cellId = hello.senderCellId;
    neighbor.distance = distance;
    
    auto it = std::find_if(m_neighbors.begin(), m_neighbors.end(),
                          [&](const NeighborInfo& n) { return n.nodeId == hello.senderId; });
    
    if (it != m_neighbors.end())
    {
        // Update existing
        it->position = neighbor.position;
        it->distance = neighbor.distance;
        it->cellId = neighbor.cellId;
    }
    else
    {
        // Add new neighbor
        m_neighbors.push_back(neighbor);
    }
    
    m_neighborMap[hello.senderId] = neighbor;
    
    // Process sender's neighbor list to build 2-hop info
    for (const auto& senderNeighbor : hello.neighborList)
    {
        if (senderNeighbor.nodeId == m_nodeId)
            continue; // Skip self
        
        // Check if already known as 1-hop
        auto it1hop = std::find_if(m_neighbors.begin(), m_neighbors.end(),
                                  [&](const NeighborInfo& n) { return n.nodeId == senderNeighbor.nodeId; });
        if (it1hop != m_neighbors.end())
            continue; // Already 1-hop
        
        // Add as 2-hop
        auto it2hop = std::find_if(m_twoHopNeighbors.begin(), m_twoHopNeighbors.end(),
                                   [&](const NeighborInfo& n) { return n.nodeId == senderNeighbor.nodeId; });
        
        if (it2hop != m_twoHopNeighbors.end())
        {
            // Update
            it2hop->distance = senderNeighbor.distance;
            it2hop->cellId = senderNeighbor.cellId;
        }
        else
        {
            m_twoHopNeighbors.push_back(senderNeighbor);
        }
    }
    
    // Check if in same cell → schedule CL election
    if (hello.senderCellId == m_cellId && m_state == DISCOVERING)
    {
        ScheduleCLElection();
    }
    
    NS_LOG_DEBUG("Node " << m_nodeId << " got HELLO from " << hello.senderId 
                 << " (1-hop: " << m_neighbors.size() << ", 2-hop: " << m_twoHopNeighbors.size() << ")");
}

void
CellForming::ScheduleCLElection()
{
    if (!m_clElectionEvent.IsExpired())
        return; // Already scheduled
    
    double myFitness = CalculateFitness();
    double delay = m_clElectionDelayInterval * (1.0 - myFitness);
    
    m_clElectionEvent = Simulator::Schedule(Seconds(delay), &CellForming::PerformCLElection, this);
    
    NS_LOG_DEBUG("Node " << m_nodeId << " scheduled CL election in " << delay 
                 << "s (fitness=" << myFitness << ")");
}

void
CellForming::PerformCLElection()
{
    // Phase 0.4: Determine if this node should be CL
    double myFitness = CalculateFitness();
    
    // Compare with best neighbor in same cell
    double bestNeighborFitness = -1.0;
    
    for (const auto& neighbor : m_neighbors)
    {
        if (neighbor.cellId != m_cellId)
            continue;
        
        // Estimate neighbor's fitness based on distance to cell center
        Vector center = GetCellCenter(m_cellId);
        double neighborDist = std::sqrt(std::pow(neighbor.position.x - center.x, 2) + 
                                       std::pow(neighbor.position.y - center.y, 2));
        double neighborFitness = 1.0 / (1.0 + neighborDist);
        
        if (neighborFitness > bestNeighborFitness)
        {
            bestNeighborFitness = neighborFitness;
        }
    }
    
    // Also check 2-hop neighbors in same cell
    for (const auto& neighbor2hop : m_twoHopNeighbors)
    {
        if (neighbor2hop.cellId != m_cellId)
            continue;
        
        Vector center = GetCellCenter(m_cellId);
        double neighborDist = std::sqrt(std::pow(neighbor2hop.position.x - center.x, 2) + 
                                       std::pow(neighbor2hop.position.y - center.y, 2));
        double neighborFitness = 1.0 / (1.0 + neighborDist);
        
        if (neighborFitness > bestNeighborFitness)
        {
            bestNeighborFitness = neighborFitness;
        }
    }
    
    bool shouldBeCL = (bestNeighborFitness < 0) || (myFitness > bestNeighborFitness) ||
                      (myFitness == bestNeighborFitness && m_nodeId < m_cellLeaderId);
    
    if (shouldBeCL)
    {
        m_cellLeaderId = m_nodeId;
        SetState(ELECTED_CL, "Elected as Cell Leader");
        SendCLAnnouncement();
    }
    else
    {
        SetState(AWAITING_CL, "Waiting for CL announcement");
    }
}

void
CellForming::SendCLAnnouncement()
{
    // Phase 0.5: Send CL announcement
    CLAnnouncementPacket announcement;
    announcement.senderId = m_nodeId;
    announcement.senderPosition = m_position;
    announcement.cellId = m_cellId;
    announcement.fitnessScore = CalculateFitness();
    
    NS_LOG_INFO("Node " << m_nodeId << " sending CL announcement (fitness=" << announcement.fitnessScore << ")");
    
    if (!m_clAnnouncementCallback.IsNull())
    {
        m_clAnnouncementCallback(announcement);
    }
    
    // Schedule calculation after delay
    if (!m_clCalculationEvent.IsExpired())
        m_clCalculationEvent.Cancel();
    
    m_clCalculationEvent = Simulator::Schedule(Seconds(m_clCalculationTime), 
                                              &CellForming::ScheduleCLCalculation, this);
}

void
CellForming::HandleCLAnnouncement(const CLAnnouncementPacket& announcement)
{
    // Phase 0.5: Receive CL announcement
    if (announcement.cellId != m_cellId)
        return; // Not for my cell
    
    if (m_cellLeaderId == 0 || announcement.fitnessScore > m_myCellLeaderFitness ||
        (announcement.fitnessScore == m_myCellLeaderFitness && announcement.senderId < m_cellLeaderId))
    {
        m_cellLeaderId = announcement.senderId;
        m_myCellLeaderFitness = announcement.fitnessScore;
    }
    
    // If I'm not the CL, send feedback after small delay
    if (m_nodeId != m_cellLeaderId && m_state == AWAITING_CL)
    {
        Simulator::Schedule(Seconds(0.1), &CellForming::SendMemberFeedback, this);
    }
    
    NS_LOG_DEBUG("Node " << m_nodeId << " received CL announcement from node " 
                 << announcement.senderId << " (fitness=" << announcement.fitnessScore << ")");
}

void
CellForming::SendMemberFeedback()
{
    // Phase 0.6: Send feedback to CL
    CLMemberFeedbackPacket feedback;
    feedback.senderId = m_nodeId;
    feedback.senderPosition = m_position;
    feedback.cellId = m_cellId;
    feedback.neighbors = m_neighbors;
    feedback.twoHopNeighbors = m_twoHopNeighbors;
    
    NS_LOG_INFO("Node " << m_nodeId << " sending feedback to CL " << m_cellLeaderId 
                << " (neighbors=" << feedback.neighbors.size() << ")");
    
    if (!m_memberFeedbackCallback.IsNull())
    {
        m_memberFeedbackCallback(feedback);
    }
    
    SetState(CELL_FORMED, "Sent feedback to CL");
}

void
CellForming::HandleMemberFeedback(const CLMemberFeedbackPacket& feedback)
{
    // Phase 0.6: CL receives feedback (CL only)
    if (m_state != ELECTED_CL && m_state != ROUTING_READY)
        return; // Not a CL
    
    m_cellMembers.push_back(feedback.senderId);
    m_memberLocations[feedback.senderId] = feedback.senderPosition;
    m_memberNeighbors[feedback.senderId] = feedback.neighbors;
    
    // Also merge neighbors to identify adjacent cells
    for (const auto& neighbor : feedback.neighbors)
    {
        if (neighbor.cellId != m_cellId)
        {
            m_neighboringCells.insert(neighbor.cellId);
        }
    }
    
    NS_LOG_DEBUG("CL " << m_nodeId << " received feedback from node " 
                 << feedback.senderId << " (neighbors=" << feedback.neighbors.size() << ")");
}

void
CellForming::ScheduleCLCalculation()
{
    // Phase 0.7: Wait a bit more for all feedback, then compute routing table
    if (m_state == ELECTED_CL)
    {
        // Schedule calculation with slight delay
        if (!m_clCalculationEvent.IsExpired())
            m_clCalculationEvent.Cancel();
        
        m_clCalculationEvent = Simulator::Schedule(Seconds(m_clCalculationTime), 
                                                  &CellForming::ComputeRoutingTable, this);
    }
}

void
CellForming::ComputeRoutingTable()
{
    // Phase 0.7: CL computes routing table
    if (m_state != ELECTED_CL)
        return;
    
    m_routingTable.clear();
    
    // For each cell member
    for (uint32_t memberId : m_cellMembers)
    {
        if (memberId == m_nodeId)
            continue; // Skip self
        
        auto it = m_memberNeighbors.find(memberId);
        if (it == m_memberNeighbors.end())
            continue;
        
        const auto& memberNeighbors = it->second;
        
        // For each neighboring cell
        for (int32_t neighborCellId : m_neighboringCells)
        {
            // Find if this member connects to neighborCellId
            uint32_t bestGateway = FindBestCGW(neighborCellId);
            if (bestGateway == 0)
                continue;
            
            // Compute path from member to gateway (simple BFS would go here)
            // For now, use direct neighbor as next-hop if available
            uint32_t nextHop = 0;
            for (const auto& neighbor : memberNeighbors)
            {
                if (neighbor.cellId == m_cellId)
                {
                    nextHop = neighbor.nodeId;
                    break;
                }
            }
            
            if (nextHop > 0)
            {
                m_routingTable[{memberId, neighborCellId}] = nextHop;
            }
        }
    }
    
    SetState(ROUTING_READY, "Routing table computed");
    NS_LOG_INFO("CL " << m_nodeId << " computed routing table with " 
                << m_routingTable.size() << " entries");
}

uint32_t
CellForming::FindBestCGW(int32_t neighborCellId)
{
    // Find best node to gateway to neighborCellId
    uint32_t bestCGW = 0;
    double bestScore = -1.0;
    
    for (const auto& [memberId, neighbors] : m_memberNeighbors)
    {
        // Check if this member has neighbor in targetCell
        for (const auto& neighbor : neighbors)
        {
            if (neighbor.cellId == neighborCellId)
            {
                // Good candidate - prefer closest to border
                Vector myCenter = GetCellCenter(m_cellId);
                Vector memberLoc = m_memberLocations[memberId];
                double distToCenter = std::sqrt(std::pow(memberLoc.x - myCenter.x, 2) + 
                                              std::pow(memberLoc.y - myCenter.y, 2));
                double score = 1.0 / (1.0 + distToCenter); // Higher score = closer to center = better positioned
                
                if (score > bestScore || (score == bestScore && memberId < bestCGW))
                {
                    bestScore = score;
                    bestCGW = memberId;
                }
                break;
            }
        }
    }
    
    return bestCGW;
}

void
CellForming::SetState(State newState, const std::string& reason)
{
    if (m_state == newState)
        return;
    
    m_state = newState;
    
    std::string stateName;
    switch (newState)
    {
        case DISCOVERING: stateName = "DISCOVERING"; break;
        case ELECTED_CL: stateName = "ELECTED_CL"; break;
        case AWAITING_CL: stateName = "AWAITING_CL"; break;
        case CELL_FORMED: stateName = "CELL_FORMED"; break;
        case ROUTING_READY: stateName = "ROUTING_READY"; break;
    }
    
    NS_LOG_INFO("Node " << m_nodeId << " state change to " << stateName << " (" << reason << ")");
    
    if (!m_stateChangeCallback.IsNull())
    {
        m_stateChangeCallback(stateName);
    }
}

// === Getters ===

int32_t
CellForming::GetCellId() const
{
    return m_cellId;
}

int32_t
CellForming::GetColor() const
{
    return m_color;
}

uint32_t
CellForming::GetCellLeaderId() const
{
    return m_cellLeaderId;
}

bool
CellForming::IsCellLeader() const
{
    return (m_state == ELECTED_CL || m_state == ROUTING_READY) && m_nodeId == m_cellLeaderId;
}

const std::vector<NeighborInfo>&
CellForming::GetNeighbors() const
{
    return m_neighbors;
}

const std::vector<NeighborInfo>&
CellForming::GetTwoHopNeighbors() const
{
    return m_twoHopNeighbors;
}

const std::set<int32_t>&
CellForming::GetNeighboringCells() const
{
    return m_neighboringCells;
}

const std::map<std::pair<uint32_t, int32_t>, uint32_t>&
CellForming::GetRoutingTable() const
{
    return m_routingTable;
}

bool
CellForming::IsCellFormationComplete() const
{
    return m_state == ROUTING_READY;
}

// === Callback Setters ===

void
CellForming::SetHelloCallback(HelloCallback cb)
{
    m_helloCallback = cb;
}

void
CellForming::SetCLAnnouncementCallback(CLAnnouncementCallback cb)
{
    m_clAnnouncementCallback = cb;
}

void
CellForming::SetMemberFeedbackCallback(MemberFeedbackCallback cb)
{
    m_memberFeedbackCallback = cb;
}

void
CellForming::SetStateChangeCallback(StateChangeCallback cb)
{
    m_stateChangeCallback = cb;
}

} // namespace ns3
