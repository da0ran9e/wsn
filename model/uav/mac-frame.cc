/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * MAC Frame Implementation
 */

#include "mac-frame.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE("WsnMacFrame");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(WsnMacHeader);

TypeId
WsnMacHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::WsnMacHeader")
        .SetParent<Header>()
        .SetGroupName("Wsn")
        .AddConstructor<WsnMacHeader>();
    return tid;
}

WsnMacHeader::WsnMacHeader()
    : m_type(0), m_seqNum(0), m_srcId(0), m_dstId(0)
{
}

WsnMacHeader::~WsnMacHeader()
{
}

TypeId
WsnMacHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
WsnMacHeader::GetSerializedSize() const
{
    return 1 + 2 + 2 + 2; // type + seqNum + srcId + dstId
}

void
WsnMacHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(m_type);
    start.WriteHtonU16(m_seqNum);
    start.WriteHtonU16(m_srcId);
    start.WriteHtonU16(m_dstId);
}

uint32_t
WsnMacHeader::Deserialize(Buffer::Iterator start)
{
    m_type = start.ReadU8();
    m_seqNum = start.ReadNtohU16();
    m_srcId = start.ReadNtohU16();
    m_dstId = start.ReadNtohU16();
    return GetSerializedSize();
}

void
WsnMacHeader::Print(std::ostream& os) const
{
    os << "WsnMacHeader(type=" << (int)m_type << ", seq=" << m_seqNum
       << ", src=" << m_srcId << ", dst=" << m_dstId << ")";
}

// ============================================================================
// Serialization functions
// ============================================================================

Ptr<Packet>
SerializeHelloPacket(const HelloPacket& hello)
{
    // Create packet with: senderId + cellId + position + neighborList
    Ptr<Packet> pkt = Create<Packet>();
    
    // This is a simplified serialization - full implementation would serialize all fields
    // For now, we use callback-based delivery as fallback
    
    NS_LOG_DEBUG("Serialized HELLO packet from node " << hello.senderId);
    return pkt;
}

bool
DeserializeHelloPacket(Ptr<Packet> pkt, HelloPacket& hello)
{
    // Deserialize packet fields
    // For now, simplified
    return true;
}

Ptr<Packet>
SerializeCLAnnouncement(const CLAnnouncementPacket& announcement)
{
    Ptr<Packet> pkt = Create<Packet>();
    NS_LOG_DEBUG("Serialized CL announcement from node " << announcement.senderId);
    return pkt;
}

bool
DeserializeCLAnnouncement(Ptr<Packet> pkt, CLAnnouncementPacket& announcement)
{
    return true;
}

Ptr<Packet>
SerializeMemberFeedback(const CLMemberFeedbackPacket& feedback)
{
    Ptr<Packet> pkt = Create<Packet>();
    NS_LOG_DEBUG("Serialized member feedback from node " << feedback.senderId);
    return pkt;
}

bool
DeserializeMemberFeedback(Ptr<Packet> pkt, CLMemberFeedbackPacket& feedback)
{
    return true;
}

Ptr<Packet>
SerializeFragment(const Fragment& frag)
{
    // Fragment serialization
    Ptr<Packet> pkt = Create<Packet>();
    
    // Serialize fragment fields to packet
    // fragmentId (4B) + sensorType (1B) + baseConfidence (8B) + position (12B) + timestamp (8B) + txPowerDbm (8B)
    uint8_t buffer[41];
    int offset = 0;
    
    // Fragment ID (4 bytes)
    std::memcpy(buffer + offset, &frag.fragmentId, 4);
    offset += 4;
    
    // Sensor Type (1 byte)
    buffer[offset] = frag.sensorType;
    offset += 1;
    
    // Base Confidence (8 bytes)
    std::memcpy(buffer + offset, &frag.baseConfidence, 8);
    offset += 8;
    
    // Position (12 bytes: 3 * 4 bytes floats)
    float x = frag.broadcastPosition.x;
    float y = frag.broadcastPosition.y;
    float z = frag.broadcastPosition.z;
    std::memcpy(buffer + offset, &x, 4);
    std::memcpy(buffer + offset + 4, &y, 4);
    std::memcpy(buffer + offset + 8, &z, 4);
    offset += 12;
    
    // Timestamp (8 bytes)
    std::memcpy(buffer + offset, &frag.timestamp, 8);
    offset += 8;
    
    // TX Power (8 bytes)
    std::memcpy(buffer + offset, &frag.txPowerDbm, 8);
    offset += 8;
    
    pkt->AddAtEnd(Create<Packet>(buffer, offset));
    
    NS_LOG_DEBUG("Serialized fragment " << frag.fragmentId << " (size: " << offset << " bytes)");
    return pkt;
}

bool
DeserializeFragment(Ptr<Packet> pkt, Fragment& frag)
{
    if (pkt->GetSize() < 41)
    {
        NS_LOG_WARN("Fragment packet too small: " << pkt->GetSize());
        return false;
    }
    
    uint8_t buffer[41];
    pkt->CopyData(buffer, 41);
    
    int offset = 0;
    
    // Fragment ID
    std::memcpy(&frag.fragmentId, buffer + offset, 4);
    offset += 4;
    
    // Sensor Type
    frag.sensorType = buffer[offset];
    offset += 1;
    
    // Base Confidence
    std::memcpy(&frag.baseConfidence, buffer + offset, 8);
    offset += 8;
    
    // Position
    float x, y, z;
    std::memcpy(&x, buffer + offset, 4);
    std::memcpy(&y, buffer + offset + 4, 4);
    std::memcpy(&z, buffer + offset + 8, 4);
    frag.broadcastPosition = Vector(x, y, z);
    offset += 12;
    
    // Timestamp
    std::memcpy(&frag.timestamp, buffer + offset, 8);
    offset += 8;
    
    // TX Power
    std::memcpy(&frag.txPowerDbm, buffer + offset, 8);
    offset += 8;
    
    NS_LOG_DEBUG("Deserialized fragment " << frag.fragmentId);
    return true;
}

} // namespace ns3
