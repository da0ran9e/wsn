/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "fragment.h"

#include "ns3/log.h"

namespace ns3
{
namespace wsn
{
namespace scenario1
{

NS_LOG_COMPONENT_DEFINE("Scenario1Fragment");

NS_OBJECT_ENSURE_REGISTERED(FragmentHeader);

FragmentHeader::FragmentHeader()
    : m_data()
{
}

FragmentHeader::FragmentHeader(const FragmentData& data)
    : m_data(data)
{
}

FragmentHeader::~FragmentHeader()
{
}

void
FragmentHeader::SetFragmentData(const FragmentData& data)
{
    m_data = data;
}

FragmentData
FragmentHeader::GetFragmentData() const
{
    return m_data;
}

TypeId
FragmentHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::wsn::scenario1::FragmentHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wsn")
                            .AddConstructor<FragmentHeader>();
    return tid;
}

TypeId
FragmentHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
FragmentHeader::Print(std::ostream& os) const
{
    os << "FragmentHeader(id=" << m_data.fragmentId
       << " type=" << m_data.sensorType
       << " conf=" << m_data.baseConfidence
       << " seq=" << m_data.sequenceNumber
       << " txPow=" << m_data.txPowerDbm << ")";
}

uint32_t
FragmentHeader::GetSerializedSize() const
{
    // fragmentId(4) + sensorType(4) + baseConfidence(8) + sequenceNumber(4) + txPowerDbm(4)
    return 4 + 4 + 8 + 4 + 4;
}

void
FragmentHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU32(m_data.fragmentId);
    start.WriteHtonU32(m_data.sensorType);
    start.WriteHtonU64(static_cast<uint64_t>(m_data.baseConfidence * 1e9)); // Convert to int for network
    start.WriteHtonU32(m_data.sequenceNumber);
    
    // Convert float to uint32 for network transmission
    union {
        float f;
        uint32_t u;
    } txPowerConv;
    txPowerConv.f = m_data.txPowerDbm;
    start.WriteHtonU32(txPowerConv.u);
}

uint32_t
FragmentHeader::Deserialize(Buffer::Iterator start)
{
    m_data.fragmentId = start.ReadNtohU32();
    m_data.sensorType = start.ReadNtohU32();
    m_data.baseConfidence = static_cast<double>(start.ReadNtohU64()) / 1e9;
    m_data.sequenceNumber = start.ReadNtohU32();
    
    // Convert uint32 back to float
    union {
        float f;
        uint32_t u;
    } txPowerConv;
    txPowerConv.u = start.ReadNtohU32();
    m_data.txPowerDbm = txPowerConv.f;
    
    return GetSerializedSize();
}

} // namespace scenario1
} // namespace wsn
} // namespace ns3
