/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "cc2420-trailer.h"

#include "ns3/log.h"
#include "ns3/buffer.h"

namespace ns3
{
namespace wsn
{

NS_LOG_COMPONENT_DEFINE("Cc2420Trailer");

// =============================================================================
// Cc2420Trailer Implementation
// =============================================================================

Cc2420Trailer::Cc2420Trailer()
    : m_fcs(0),
      m_lqi(0)
{
}

Cc2420Trailer::~Cc2420Trailer()
{
}

TypeId
Cc2420Trailer::GetTypeId()
{
    static TypeId tid = TypeId("ns3::cc2420::Cc2420Trailer")
        .SetParent<Trailer>()
        .SetGroupName("Cc2420")
        .AddConstructor<Cc2420Trailer>();
    return tid;
}

TypeId
Cc2420Trailer::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
Cc2420Trailer::GetSerializedSize() const
{
    // FCS (2 bytes) + LQI (1 byte) = 3 bytes
    // Note: LQI is optional, added on reception only
    return 3;
}

void
Cc2420Trailer::Serialize(Buffer::Iterator start) const
{
    // TODO: Implement serialization
    // Write FCS (2 bytes)
    // Write LQI (1 byte)
}

uint32_t
Cc2420Trailer::Deserialize(Buffer::Iterator start)
{
    // TODO: Implement deserialization
    // Read FCS (2 bytes)
    // Read LQI (1 byte)
    return GetSerializedSize();
}

void
Cc2420Trailer::Print(std::ostream& os) const
{
    // TODO: Implement print
    os << "Cc2420Trailer(FCS=" << std::hex << m_fcs << std::dec
       << " LQI=" << (uint16_t)m_lqi << ")";
}

// =============================================================================
// FCS Accessors
// =============================================================================

void
Cc2420Trailer::SetFrameCheckSequence(uint16_t fcs)
{
    m_fcs = fcs;
}

uint16_t
Cc2420Trailer::GetFrameCheckSequence() const
{
    return m_fcs;
}

// =============================================================================
// LQI Accessors
// =============================================================================

void
Cc2420Trailer::SetLinkQualityIndicator(uint8_t lqi)
{
    m_lqi = lqi;
}

uint8_t
Cc2420Trailer::GetLinkQualityIndicator() const
{
    return m_lqi;
}

} // namespace cc2420
} // namespace ns3
