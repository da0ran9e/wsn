/*
 * Copyright (c) 2026 WSN Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: WSN Project <support@wsn-project.org>
 */

#include "packet-header.h"

#include "ns3/buffer.h"
#include "ns3/log.h"
#include "ns3/type-id.h"

namespace ns3
{
namespace wsn
{
namespace scenario3
{

NS_LOG_COMPONENT_DEFINE("Scenario3PacketHeader");

// ============================================================================
// Scenario3PacketHeader Implementation
// ============================================================================

Scenario3PacketHeader::Scenario3PacketHeader()
    : m_version(0x01),
      m_messageType(0),
      m_nodeType(0),
      m_reserved1(0),
      m_sequenceNumber(0),
      m_sourceNodeId(0),
      m_destinationNodeId(0xFFFF),
      m_payloadLength(0),
      m_checksumVersion(0),
      m_hopLimit(16),
      m_timestampHigh(0),
      m_timestampLow(0),
      m_confidence(0),
      m_reserved2(0)
{
}

Scenario3PacketHeader::~Scenario3PacketHeader()
{
}

TypeId
Scenario3PacketHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::wsn::scenario3::Scenario3PacketHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wsn")
                            .AddConstructor<Scenario3PacketHeader>();
    return tid;
}

TypeId
Scenario3PacketHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
Scenario3PacketHeader::GetSerializedSize() const
{
    return 24; // Total header size in bytes
}

void
Scenario3PacketHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;

    // Serialize all fields in network byte order (big-endian)
    i.WriteU8(m_version);
    i.WriteU8(m_messageType);
    i.WriteU8(m_nodeType);
    i.WriteU8(m_reserved1);
    i.WriteHtonU16(m_sequenceNumber);
    i.WriteHtonU16(m_sourceNodeId);
    i.WriteHtonU16(m_destinationNodeId);
    i.WriteHtonU16(m_payloadLength);
    i.WriteHtonU16(m_checksumVersion);
    i.WriteHtonU16(m_hopLimit);
    i.WriteHtonU16(m_timestampHigh);
    i.WriteHtonU16(m_timestampLow);
    i.WriteHtonU16(m_confidence);
    i.WriteHtonU16(m_reserved2);
}

uint32_t
Scenario3PacketHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    // Deserialize all fields from network byte order
    m_version = i.ReadU8();
    m_messageType = i.ReadU8();
    m_nodeType = i.ReadU8();
    m_reserved1 = i.ReadU8();
    m_sequenceNumber = i.ReadNtohU16();
    m_sourceNodeId = i.ReadNtohU16();
    m_destinationNodeId = i.ReadNtohU16();
    m_payloadLength = i.ReadNtohU16();
    m_checksumVersion = i.ReadNtohU16();
    m_hopLimit = i.ReadNtohU16();
    m_timestampHigh = i.ReadNtohU16();
    m_timestampLow = i.ReadNtohU16();
    m_confidence = i.ReadNtohU16();
    m_reserved2 = i.ReadNtohU16();

    return GetSerializedSize();
}

void
Scenario3PacketHeader::Print(std::ostream& os) const
{
    os << "Scenario3PacketHeader [" << std::endl
       << "  Version=" << (int)m_version << " MessageType=" << (int)m_messageType
       << " NodeType=" << (int)m_nodeType << " SeqNum=" << m_sequenceNumber << std::endl
       << "  Source=" << m_sourceNodeId << " Destination=" << m_destinationNodeId
       << " PayloadLen=" << m_payloadLength << std::endl
       << "  HopLimit=" << m_hopLimit << " Confidence=" << m_confidence << std::endl
       << "]" << std::endl;
}

// ============================================================================
// Scenario3FragmentHeader Implementation
// ============================================================================

Scenario3FragmentHeader::Scenario3FragmentHeader()
    : m_fragmentId(0),
      m_fragmentSequence(0),
      m_totalFragments(1),
      m_sensorType(0),
      m_flags(0),
      m_dataLength(0),
      m_crc16(0)
{
}

Scenario3FragmentHeader::~Scenario3FragmentHeader()
{
}

TypeId
Scenario3FragmentHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::wsn::scenario3::Scenario3FragmentHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wsn")
                            .AddConstructor<Scenario3FragmentHeader>();
    return tid;
}

TypeId
Scenario3FragmentHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
Scenario3FragmentHeader::GetSerializedSize() const
{
    return 12; // Total fragment header size in bytes
}

void
Scenario3FragmentHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;

    i.WriteHtonU16(m_fragmentId);
    i.WriteHtonU16(m_fragmentSequence);
    i.WriteHtonU16(m_totalFragments);
    i.WriteU8(m_sensorType);
    i.WriteU8(m_flags);
    i.WriteHtonU16(m_dataLength);
    i.WriteHtonU16(m_crc16);
}

uint32_t
Scenario3FragmentHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    m_fragmentId = i.ReadNtohU16();
    m_fragmentSequence = i.ReadNtohU16();
    m_totalFragments = i.ReadNtohU16();
    m_sensorType = i.ReadU8();
    m_flags = i.ReadU8();
    m_dataLength = i.ReadNtohU16();
    m_crc16 = i.ReadNtohU16();

    return GetSerializedSize();
}

void
Scenario3FragmentHeader::Print(std::ostream& os) const
{
    os << "Scenario3FragmentHeader [" << std::endl
       << "  FragmentId=" << m_fragmentId << " Sequence=" << m_fragmentSequence
       << "/" << m_totalFragments << std::endl
       << "  SensorType=" << (int)m_sensorType << " DataLen=" << m_dataLength
       << " CRC16=" << m_crc16 << std::endl
       << "  IsLast=" << (IsLastFragment() ? "true" : "false")
       << " IsCompressed=" << (IsCompressed() ? "true" : "false") << std::endl
       << "]" << std::endl;
}

// ============================================================================
// Scenario3AcknowledgementHeader Implementation
// ============================================================================

Scenario3AcknowledgementHeader::Scenario3AcknowledgementHeader()
    : m_acknowledgedSequence(0),
      m_acknowledgementType(0),
      m_status(0)
{
}

Scenario3AcknowledgementHeader::~Scenario3AcknowledgementHeader()
{
}

TypeId
Scenario3AcknowledgementHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::wsn::scenario3::Scenario3AcknowledgementHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wsn")
                            .AddConstructor<Scenario3AcknowledgementHeader>();
    return tid;
}

TypeId
Scenario3AcknowledgementHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
Scenario3AcknowledgementHeader::GetSerializedSize() const
{
    return 4; // Total acknowledgement header size in bytes
}

void
Scenario3AcknowledgementHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;

    i.WriteHtonU16(m_acknowledgedSequence);
    i.WriteU8(m_acknowledgementType);
    i.WriteU8(m_status);
}

uint32_t
Scenario3AcknowledgementHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    m_acknowledgedSequence = i.ReadNtohU16();
    m_acknowledgementType = i.ReadU8();
    m_status = i.ReadU8();

    return GetSerializedSize();
}

void
Scenario3AcknowledgementHeader::Print(std::ostream& os) const
{
    os << "Scenario3AcknowledgementHeader [" << std::endl
       << "  AcknowledgedSeq=" << m_acknowledgedSequence
       << " Type=" << (int)m_acknowledgementType << " Status=" << (int)m_status
       << std::endl
       << "]" << std::endl;
}

} // namespace scenario3
} // namespace wsn
} // namespace ns3
