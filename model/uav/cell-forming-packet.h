/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Cell Forming Protocol Packets (Phase 0)
 */

#ifndef CELL_FORMING_PACKET_H
#define CELL_FORMING_PACKET_H

#include "ns3/vector.h"
#include <vector>
#include <map>

namespace ns3
{

/**
 * @brief Neighbor info in HELLO or feedback packet
 */
struct NeighborInfo
{
    uint32_t nodeId;
    Vector position;
    int32_t cellId;
    double distance;
};

/**
 * @brief HELLO beacon packet (phase 0.2)
 * Broadcast periodically to discover 1-hop and 2-hop neighbors
 */
struct HelloPacket
{
    uint32_t senderId;
    Vector senderPosition;
    int32_t senderCellId;
    std::vector<NeighborInfo> neighborList;  // 1-hop neighbors already discovered by sender
    
    HelloPacket() : senderId(0), senderCellId(-1) {}
};

/**
 * @brief Cell Leader Announcement packet (phase 0.5)
 * Sent by CL to announce itself
 */
struct CLAnnouncementPacket
{
    uint32_t senderId;
    Vector senderPosition;
    int32_t cellId;
    double fitnessScore;
    
    CLAnnouncementPacket() : senderId(0), cellId(-1), fitnessScore(0.0) {}
};

/**
 * @brief Cell Member Feedback packet (phase 0.6)
 * Sent by cell members to CL after receiving CL announcement
 */
struct CLMemberFeedbackPacket
{
    uint32_t senderId;
    Vector senderPosition;
    int32_t cellId;
    std::vector<NeighborInfo> neighbors;      // 1-hop neighbors
    std::vector<NeighborInfo> twoHopNeighbors; // 2-hop neighbors
    
    CLMemberFeedbackPacket() : senderId(0), cellId(-1) {}
};

/**
 * @brief Cell routing info (for CL use)
 * Stored in CL's routing table
 */
struct RoutingEntry
{
    uint32_t memberId;
    int32_t targetNeighborCellId;
    uint32_t nextHopNodeId;
    
    RoutingEntry() : memberId(0), targetNeighborCellId(-1), nextHopNodeId(0) {}
};

} // namespace ns3

#endif /* CELL_FORMING_PACKET_H */
