#include "wsn-routing-header.h"

namespace ns3 {
namespace wsn {

NS_OBJECT_ENSURE_REGISTERED(WsnRoutingHeader);

WsnRoutingHeader::WsnRoutingHeader()
{
}

TypeId
WsnRoutingHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}

TypeId
WsnRoutingHeader::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::wsn::WsnRoutingHeader")
    .SetParent<Header>()
    .SetGroupName("Wsn")
    .AddConstructor<WsnRoutingHeader>();
  return tid;
}

uint32_t
WsnRoutingHeader::GetSerializedSize() const
{
    return 2 + 2 + 2 + 8 + 8 + 2 + 2;
}

void
WsnRoutingHeader::Serialize(Buffer::Iterator i) const
{
    i.WriteU16(m_source);
    i.WriteU16(m_destination);
    i.WriteU16(m_sequenceNumber);
    i.WriteU32(static_cast<uint32_t>(m_netMacInfoExchange.RSSI));
    i.WriteU32(static_cast<uint32_t>(m_netMacInfoExchange.LQI));
    i.WriteU16(m_netMacInfoExchange.nextHop);
    i.WriteU16(m_netMacInfoExchange.lastHop);
}

uint32_t
WsnRoutingHeader::Deserialize(Buffer::Iterator i)
{
    m_source = i.ReadU16();
    m_destination = i.ReadU16();
    m_sequenceNumber = i.ReadU16();
    m_netMacInfoExchange.RSSI = static_cast<double>(i.ReadU32());
    m_netMacInfoExchange.LQI = static_cast<double>(i.ReadU32());
    m_netMacInfoExchange.nextHop = i.ReadU16();
    m_netMacInfoExchange.lastHop = i.ReadU16();
  return GetSerializedSize();
}


void
WsnRoutingHeader::Print(std::ostream &os) const
{
os << "[WSN-ROUTING hdr] "
<< " seq=" << m_sequenceNumber
<< " srcNode=" << m_source;
}


} // namespace wsn
} // namespace ns3