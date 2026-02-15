#include "pecee-header.h"
#include "ns3/log.h"

namespace ns3 {
namespace wsn {

NS_LOG_COMPONENT_DEFINE("PeceeHeader");
NS_OBJECT_ENSURE_REGISTERED(PeceeHeader);

PeceeHeader::PeceeHeader()
    : m_packetType(HELLO_PACKET),
      m_clusterHead(-1),
      m_cellSent(-1),
      m_cellNext(-1),
      m_cellNextNext(-1),
      m_cellSource(-1),
      m_cellDestination(-1),
      m_cellHopCount(0),
      m_ttl(0)
{
    for (int i = 0; i < 3; i++) {
        m_cellPath[i] = -1;
    }
    m_sensorData.destinationCH = -1;
    m_sensorData.dataId = 0;
    m_sensorData.sensorId = -1;
    m_sensorData.hopCount = 0;
    m_chAnnouncementData.chId = -1;
    m_cellHopAnnouncementData.nextCell = -1;
    for (int i = 0; i < 3; i++) {
        m_cellHopAnnouncementData.cellPath[i] = -1;
    }
}

PeceeHeader::~PeceeHeader()
{
}

TypeId PeceeHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::wsn::PeceeHeader")
        .SetParent<WsnRoutingHeader>()
        .SetGroupName("Wsn")
        .AddConstructor<PeceeHeader>();
    return tid;
}

TypeId PeceeHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t PeceeHeader::GetSerializedSize() const
{
    // Base routing header + compacted 16-bit fields (all IDs/TTL within 0..65535)
    // 9*2 + 3*2 + 4*2 + 2 + (1+3)*2 = 46 bytes over WsnRoutingHeader
    return WsnRoutingHeader::GetSerializedSize() + 46;
}

void PeceeHeader::Serialize(Buffer::Iterator start) const
{
    WsnRoutingHeader::Serialize(start);
    
    start.WriteU16(static_cast<uint16_t>(m_packetType));
    start.WriteU16(static_cast<uint16_t>(m_clusterHead));
    start.WriteU16(static_cast<uint16_t>(m_cellSent));
    start.WriteU16(static_cast<uint16_t>(m_cellNext));
    start.WriteU16(static_cast<uint16_t>(m_cellNextNext));
    start.WriteU16(static_cast<uint16_t>(m_cellSource));
    start.WriteU16(static_cast<uint16_t>(m_cellDestination));
    start.WriteU16(static_cast<uint16_t>(m_cellHopCount));
    start.WriteU16(static_cast<uint16_t>(m_ttl));
    
    // Write cell path array
    for (int i = 0; i < 3; i++) {
        start.WriteU16(static_cast<uint16_t>(m_cellPath[i]));
    }
    
    // Write sensor data
    start.WriteU16(static_cast<uint16_t>(m_sensorData.destinationCH));
    start.WriteU16(static_cast<uint16_t>(m_sensorData.dataId));  // Convert double to uint16 for ns-3 compatibility
    start.WriteU16(static_cast<uint16_t>(m_sensorData.sensorId));
    start.WriteU16(static_cast<uint16_t>(m_sensorData.hopCount));
    
    // Write CH announcement data
    start.WriteU16(static_cast<uint16_t>(m_chAnnouncementData.chId));
    
    // Write cell hop announcement data
    start.WriteU16(static_cast<uint16_t>(m_cellHopAnnouncementData.nextCell));
    for (int i = 0; i < 3; i++) {
        start.WriteU16(static_cast<uint16_t>(m_cellHopAnnouncementData.cellPath[i]));
    }
}

uint32_t PeceeHeader::Deserialize(Buffer::Iterator start)
{
    WsnRoutingHeader::Deserialize(start);
    
    m_packetType = static_cast<PeceePacketType>(start.ReadU16());
    m_clusterHead = start.ReadU16();
    m_cellSent = start.ReadU16();
    m_cellNext = start.ReadU16();
    m_cellNextNext = start.ReadU16();
    m_cellSource = start.ReadU16();
    m_cellDestination = start.ReadU16();
    m_cellHopCount = start.ReadU16();
    m_ttl = start.ReadU16();
    
    // Read cell path array
    for (int i = 0; i < 3; i++) {
        m_cellPath[i] = start.ReadU16();
    }
    
    // Read sensor data
    m_sensorData.destinationCH = start.ReadU16();
    m_sensorData.dataId = (double)start.ReadU16();  // Convert uint16 back to double
    m_sensorData.sensorId = start.ReadU16();
    m_sensorData.hopCount = start.ReadU16();
    
    // Read CH announcement data
    m_chAnnouncementData.chId = start.ReadU16();
    
    // Read cell hop announcement data
    m_cellHopAnnouncementData.nextCell = start.ReadU16();
    for (int i = 0; i < 3; i++) {
        m_cellHopAnnouncementData.cellPath[i] = start.ReadU16();
    }
    
    return GetSerializedSize();
}

void PeceeHeader::Print(std::ostream &os) const
{
    WsnRoutingHeader::Print(os);
    os << " PacketType=" << m_packetType
       << " CellSrc=" << m_cellSource
       << " CellDst=" << m_cellDestination
       << " CH=" << m_clusterHead
       << " TTL=" << m_ttl;
}

}
}
