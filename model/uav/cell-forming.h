/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Cell Forming Module (Phase 0)
 * Implements hex-grid cell mapping, neighbor discovery, and CL election
 */

#ifndef CELL_FORMING_H
#define CELL_FORMING_H

#include "ns3/object.h"
#include "ns3/vector.h"
#include "ns3/callback.h"
#include "ns3/event-id.h"
#include "cell-forming-packet.h"

#include <vector>
#include <map>
#include <set>
#include <memory>

namespace ns3
{

/**
 * @brief Cell Forming module for Phase 0
 *
 * Implements:
 * - Hex-grid cell coordinate calculation (section 0.1)
 * - HELLO beacon broadcasting (section 0.2)
 * - Fitness score calculation (section 0.3)
 * - Cell Leader election (section 0.4, 0.5)
 * - Cell building via feedback (section 0.6)
 * - Intra-cell routing table computation (section 0.7)
 */
class CellForming : public Object
{
public:
    /**
     * @brief Get the TypeId
     * @return TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Constructor
     */
    CellForming();

    /**
     * @brief Destructor
     */
    ~CellForming() override;

    // === Configuration ===

    /**
     * @brief Set node parameters
     * @param nodeId Unique node identifier
     * @param position Node's position (x, y, z)
     * @param cellRadius Radius for neighbor discovery
     * @param gridOffset Grid offset for cell ID calculation
     */
    void SetNodeParams(uint32_t nodeId, Vector position, double cellRadius, int32_t gridOffset);

    /**
     * @brief Set timing parameters
     * @param helloInterval Interval between HELLO broadcasts
     * @param clElectionDelayInterval Base delay for CL election
     * @param clCalculationTime Delay before CL computes routing table
     */
    void SetTimingParams(double helloInterval, double clElectionDelayInterval, double clCalculationTime);

    // === Core Methods ===

    /**
     * @brief Initialize cell forming (start HELLO broadcasts)
     */
    void Initialize();

    /**
     * @brief Handle received HELLO packet
     * @param hello HELLO packet
     */
    void HandleHelloPacket(const HelloPacket& hello);

    /**
     * @brief Handle received CL announcement
     * @param announcement CL announcement packet
     */
    void HandleCLAnnouncement(const CLAnnouncementPacket& announcement);

    /**
     * @brief Handle feedback from cell member
     * @param feedback Member feedback packet
     */
    void HandleMemberFeedback(const CLMemberFeedbackPacket& feedback);

    // === Getters ===

    /**
     * @brief Get node's cell ID
     * @return Cell ID
     */
    int32_t GetCellId() const;

    /**
     * @brief Get node's TDMA color
     * @return Color [0, 2]
     */
    int32_t GetColor() const;

    /**
     * @brief Get Cell Leader ID for this node's cell
     * @return CL node ID, or 0 if not yet elected
     */
    uint32_t GetCellLeaderId() const;

    /**
     * @brief Check if this node is a Cell Leader
     * @return True if node is CL
     */
    bool IsCellLeader() const;

    /**
     * @brief Get list of 1-hop neighbors
     * @return Vector of neighbor info
     */
    const std::vector<NeighborInfo>& GetNeighbors() const;

    /**
     * @brief Get list of 2-hop neighbors
     * @return Vector of 2-hop neighbor info
     */
    const std::vector<NeighborInfo>& GetTwoHopNeighbors() const;

    /**
     * @brief Get neighboring cell IDs
     * @return Set of adjacent cell IDs
     */
    const std::set<int32_t>& GetNeighboringCells() const;

    /**
     * @brief Get routing table (for CL only)
     * @return Map: (memberId, neighborCellId) → nextHopNodeId
     */
    const std::map<std::pair<uint32_t, int32_t>, uint32_t>& GetRoutingTable() const;

    /**
     * @brief Check if cell formation is complete
     * @return True if CL elected and routing table computed
     */
    bool IsCellFormationComplete() const;

    // === Callbacks for external handlers ===

    /**
     * @brief Callback when HELLO needs to be sent
     * @param hello HelloPacket to send
     */
    typedef Callback<void, const HelloPacket&> HelloCallback;
    void SetHelloCallback(HelloCallback cb);

    /**
     * @brief Callback when CL announcement needs to be sent
     * @param announcement CLAnnouncementPacket to send
     */
    typedef Callback<void, const CLAnnouncementPacket&> CLAnnouncementCallback;
    void SetCLAnnouncementCallback(CLAnnouncementCallback cb);

    /**
     * @brief Callback when member feedback needs to be sent
     * @param feedback CLMemberFeedbackPacket to send
     */
    typedef Callback<void, const CLMemberFeedbackPacket&> MemberFeedbackCallback;
    void SetMemberFeedbackCallback(MemberFeedbackCallback cb);

    /**
     * @brief Callback when cell formation state changes
     * @param state State description
     */
    typedef Callback<void, std::string> StateChangeCallback;
    void SetStateChangeCallback(StateChangeCallback cb);

private:
    // === Node Parameters ===
    uint32_t m_nodeId;
    Vector m_position;
    double m_cellRadius;
    int32_t m_gridOffset;

    // === Timing Parameters ===
    double m_helloInterval;
    double m_clElectionDelayInterval;
    double m_clCalculationTime;

    // === State ===
    enum State
    {
        DISCOVERING,      // Phase 0.1-0.2: discovering neighbors
        ELECTED_CL,       // Phase 0.4: this node elected CL
        AWAITING_CL,      // Phase 0.5: waiting for CL announcement
        CELL_FORMED,      // Phase 0.6: cell topology established
        ROUTING_READY     // Phase 0.7: routing table computed
    };
    State m_state;

    // === Cell Information ===
    int32_t m_cellId;
    int32_t m_color;
    uint32_t m_cellLeaderId;
    double m_myCellLeaderFitness;

    // === Neighbor Information ===
    std::vector<NeighborInfo> m_neighbors;         // 1-hop
    std::vector<NeighborInfo> m_twoHopNeighbors;   // 2-hop
    std::map<uint32_t, NeighborInfo> m_neighborMap; // For quick lookup
    std::set<int32_t> m_neighboringCells;           // Adjacent cell IDs

    // === CL Information (CL only) ===
    std::vector<uint32_t> m_cellMembers;
    std::map<uint32_t, Vector> m_memberLocations;
    std::map<uint32_t, std::vector<NeighborInfo>> m_memberNeighbors;
    std::map<std::pair<uint32_t, int32_t>, uint32_t> m_routingTable; // (memberId, neighborCellId) → nextHopNodeId

    // === Callbacks ===
    HelloCallback m_helloCallback;
    CLAnnouncementCallback m_clAnnouncementCallback;
    MemberFeedbackCallback m_memberFeedbackCallback;
    StateChangeCallback m_stateChangeCallback;

    // === Events ===
    EventId m_helloEvent;
    EventId m_clElectionEvent;
    EventId m_clCalculationEvent;

    // === Private Methods ===

    /**
     * @brief Calculate cell ID and color from position
     */
    void CalculateCellInfo();

    /**
     * @brief Calculate fitness score (distance to cell center)
     * @return Fitness value [0, 1]
     */
    double CalculateFitness();

    /**
     * @brief Get hex cell center from cell ID
     * @param cellId Cell ID
     * @return Position of cell center
     */
    Vector GetCellCenter(int32_t cellId);

    /**
     * @brief Broadcast HELLO packet
     */
    void BroadcastHello();

    /**
     * @brief Schedule next HELLO broadcast
     */
    void ScheduleHello();

    /**
     * @brief Schedule CL election
     */
    void ScheduleCLElection();

    /**
     * @brief Perform CL election (compare fitness with neighbors)
     */
    void PerformCLElection();

    /**
     * @brief Send CL announcement
     */
    void SendCLAnnouncement();

    /**
     * @brief Send member feedback to CL
     */
    void SendMemberFeedback();

    /**
     * @brief Schedule CL calculation (after sufficient feedback collected)
     */
    void ScheduleCLCalculation();

    /**
     * @brief Compute routing table from collected feedback (CL only)
     */
    void ComputeRoutingTable();

    /**
     * @brief Update state and notify listeners
     * @param newState New state
     */
    void SetState(State newState, const std::string& reason);

    /**
     * @brief Find best neighbor in same cell for CGW selection
     * @param neighborCellId Target neighboring cell
     * @return Node ID of best gateway candidate
     */
    uint32_t FindBestCGW(int32_t neighborCellId);
};

} // namespace ns3

#endif /* CELL_FORMING_H */
