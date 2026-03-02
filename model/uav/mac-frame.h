/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * MAC Frame Definitions for Phase 0 and Phase 1 Packets
 */

#ifndef MAC_FRAME_H
#define MAC_FRAME_H

#include "ns3/packet.h"
#include "ns3/header.h"
#include "ns3/buffer.h"

#include "fragment.h"
#include "cell-forming-packet.h"

#include <vector>
#include <cstring>

namespace ns3
{

/**
 * @brief MAC Frame Header for WSN packets
 * 
 * Frame Format:
 * | Type (1B) | SeqNum (2B) | SrcId (2B) | DstId (2B) | Payload |
 */
class WsnMacHeader : public Header
{
public:
    // Packet types
    enum PacketType : uint8_t
    {
        HELLO_PACKET = 1,              // Phase 0 HELLO discovery
        CL_ANNOUNCEMENT = 2,            // Phase 0 CL announcement
        MEMBER_FEEDBACK = 3,            // Phase 0 Member feedback
        UAV_FRAGMENT = 4,              // Phase 1 Fragment broadcast
    };

    WsnMacHeader();
    ~WsnMacHeader() override;

    /**
     * Set packet type
     */
    void SetPacketType(PacketType type) { m_type = type; }
    PacketType GetPacketType() const { return (PacketType)m_type; }

    /**
     * Set sequence number
     */
    void SetSeqNum(uint16_t seq) { m_seqNum = seq; }
    uint16_t GetSeqNum() const { return m_seqNum; }

    /**
     * Set source and destination
     */
    void SetSource(uint16_t src) { m_srcId = src; }
    uint16_t GetSource() const { return m_srcId; }

    void SetDestination(uint16_t dst) { m_dstId = dst; }
    uint16_t GetDestination() const { return m_dstId; }

    // Inherited methods
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

private:
    uint8_t m_type;
    uint16_t m_seqNum;
    uint16_t m_srcId;
    uint16_t m_dstId;
};

/**
 * @brief Serialize HELLO packet to MAC payload
 */
Ptr<Packet> SerializeHelloPacket(const HelloPacket& hello);
bool DeserializeHelloPacket(Ptr<Packet> pkt, HelloPacket& hello);

/**
 * @brief Serialize CL Announcement packet to MAC payload
 */
Ptr<Packet> SerializeCLAnnouncement(const CLAnnouncementPacket& announcement);
bool DeserializeCLAnnouncement(Ptr<Packet> pkt, CLAnnouncementPacket& announcement);

/**
 * @brief Serialize Member Feedback packet to MAC payload
 */
Ptr<Packet> SerializeMemberFeedback(const CLMemberFeedbackPacket& feedback);
bool DeserializeMemberFeedback(Ptr<Packet> pkt, CLMemberFeedbackPacket& feedback);

/**
 * @brief Serialize Fragment to MAC payload
 */
Ptr<Packet> SerializeFragment(const Fragment& frag);
bool DeserializeFragment(Ptr<Packet> pkt, Fragment& frag);

} // namespace ns3

#endif /* MAC_FRAME_H */
