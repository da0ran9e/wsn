/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "cc2420-header.h"

#include "ns3/log.h"
#include "ns3/buffer.h"

namespace ns3
{
namespace cc2420
{

NS_LOG_COMPONENT_DEFINE("Cc2420Header");

// =============================================================================
// Cc2420Header Implementation
// =============================================================================

Cc2420Header::Cc2420Header()
    : m_frameControl(0),
      m_sequenceNumber(0),
      m_destinationPanId(0),
      m_sourcePanId(0),
      m_destinationAddress(),
      m_sourceAddress()
{
}

Cc2420Header::~Cc2420Header()
{
}

TypeId
Cc2420Header::GetTypeId()
{
    static TypeId tid = TypeId("ns3::cc2420::Cc2420Header")
        .SetParent<Header>()
        .SetGroupName("Cc2420")
        .AddConstructor<Cc2420Header>();
    return tid;
}

TypeId
Cc2420Header::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
Cc2420Header::GetSerializedSize() const
{
    // Frame Control (2) + DSN (1) + DestPAN (2) + DestAddr (2) +
    // SrcPAN (2) + SrcAddr (2) = 11 bytes
    return 11;
}

void
Cc2420Header::Serialize(Buffer::Iterator start) const
{
    // TODO: Implement serialization
    // Write frame control
    // Write sequence number
    // Write PAN IDs
    // Write addresses
}

uint32_t
Cc2420Header::Deserialize(Buffer::Iterator start)
{
    // TODO: Implement deserialization
    // Read frame control
    // Read sequence number
    // Read PAN IDs
    // Read addresses
    return GetSerializedSize();
}

void
Cc2420Header::Print(std::ostream& os) const
{
    // TODO: Implement print
    os << "Cc2420Header(FCF=" << std::hex << m_frameControl << std::dec
       << " DSN=" << (uint16_t)m_sequenceNumber
       << " DestAddr=" << m_destinationAddress
       << " SrcAddr=" << m_sourceAddress << ")";
}

// =============================================================================
// Frame Control Field Accessors
// =============================================================================

void
Cc2420Header::SetFrameType(FrameType type)
{
    // FCF bits 0-2
    m_frameControl &= 0xFFF8;
    m_frameControl |= (type & 0x07);
}

Cc2420Header::FrameType
Cc2420Header::GetFrameType() const
{
    return static_cast<FrameType>(m_frameControl & 0x07);
}

void
Cc2420Header::SetSecurityEnabled(bool enabled)
{
    if (enabled)
        m_frameControl |= 0x0008;
    else
        m_frameControl &= ~0x0008;
}

bool
Cc2420Header::GetSecurityEnabled() const
{
    return (m_frameControl & 0x0008) != 0;
}

void
Cc2420Header::SetFramePending(bool pending)
{
    if (pending)
        m_frameControl |= 0x0010;
    else
        m_frameControl &= ~0x0010;
}

bool
Cc2420Header::GetFramePending() const
{
    return (m_frameControl & 0x0010) != 0;
}

void
Cc2420Header::SetAckRequest(bool ackReq)
{
    if (ackReq)
        m_frameControl |= 0x0020;
    else
        m_frameControl &= ~0x0020;
}

bool
Cc2420Header::GetAckRequest() const
{
    return (m_frameControl & 0x0020) != 0;
}

void
Cc2420Header::SetPanIdCompression(bool compression)
{
    if (compression)
        m_frameControl |= 0x0040;
    else
        m_frameControl &= ~0x0040;
}

bool
Cc2420Header::GetPanIdCompression() const
{
    return (m_frameControl & 0x0040) != 0;
}

void
Cc2420Header::SetDestinationAddressingMode(uint8_t mode)
{
    m_frameControl &= 0xFF3F;
    m_frameControl |= ((mode & 0x03) << 6);
}

uint8_t
Cc2420Header::GetDestinationAddressingMode() const
{
    return (m_frameControl >> 6) & 0x03;
}

void
Cc2420Header::SetSourceAddressingMode(uint8_t mode)
{
    m_frameControl &= 0xCFFF;
    m_frameControl |= ((mode & 0x03) << 8);
}

uint8_t
Cc2420Header::GetSourceAddressingMode() const
{
    return (m_frameControl >> 8) & 0x03;
}

// =============================================================================
// Sequence Number
// =============================================================================

void
Cc2420Header::SetSequenceNumber(uint8_t dsn)
{
    m_sequenceNumber = dsn;
}

uint8_t
Cc2420Header::GetSequenceNumber() const
{
    return m_sequenceNumber;
}

// =============================================================================
// PAN IDs
// =============================================================================

void
Cc2420Header::SetDestinationPanId(uint16_t panId)
{
    m_destinationPanId = panId;
}

uint16_t
Cc2420Header::GetDestinationPanId() const
{
    return m_destinationPanId;
}

void
Cc2420Header::SetSourcePanId(uint16_t panId)
{
    m_sourcePanId = panId;
}

uint16_t
Cc2420Header::GetSourcePanId() const
{
    return m_sourcePanId;
}

// =============================================================================
// Addresses
// =============================================================================

void
Cc2420Header::SetDestinationAddress(Mac16Address addr)
{
    m_destinationAddress = addr;
}

Mac16Address
Cc2420Header::GetDestinationAddress() const
{
    return m_destinationAddress;
}

void
Cc2420Header::SetSourceAddress(Mac16Address addr)
{
    m_sourceAddress = addr;
}

Mac16Address
Cc2420Header::GetSourceAddress() const
{
    return m_sourceAddress;
}

} // namespace cc2420
} // namespace ns3
