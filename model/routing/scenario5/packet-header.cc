/*
 * Scenario 4 - Packet Header Implementation
 */

#include "packet-header.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Scenario5PacketHeader");

namespace wsn {
namespace scenario5 {
namespace routing {

// ===== PacketHeader =====

NS_OBJECT_ENSURE_REGISTERED(PacketHeader);

PacketHeader::PacketHeader()
    : m_type(PACKET_TYPE_STARTUP)
{
}

PacketHeader::~PacketHeader()
{
}

void
PacketHeader::SetType(PacketType type)
{
    m_type = static_cast<uint8_t>(type);
}

PacketType
PacketHeader::GetType() const
{
    return static_cast<PacketType>(m_type);
}

TypeId
PacketHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::wsn::scenario5::routing::PacketHeader")
        .SetParent<Header>()
        .SetGroupName("Wsn")
        .AddConstructor<PacketHeader>();
    return tid;
}

TypeId
PacketHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
PacketHeader::Print(std::ostream& os) const
{
    os << "PacketHeader(type=" << static_cast<int>(m_type) << ")";
}

uint32_t
PacketHeader::GetSerializedSize() const
{
    return 1; // 1 byte for type
}

void
PacketHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(m_type);
}

uint32_t
PacketHeader::Deserialize(Buffer::Iterator start)
{
    m_type = start.ReadU8();
    return GetSerializedSize();
}

// ===== StartupPhasePacket =====

NS_OBJECT_ENSURE_REGISTERED(StartupPhasePacket);

StartupPhasePacket::StartupPhasePacket()
    : m_nodeId(0), m_posX(0.0), m_posY(0.0), m_timestamp(0)
{
}

StartupPhasePacket::~StartupPhasePacket()
{
}

void
StartupPhasePacket::SetNodeId(uint32_t nodeId)
{
    m_nodeId = nodeId;
}

uint32_t
StartupPhasePacket::GetNodeId() const
{
    return m_nodeId;
}

void
StartupPhasePacket::SetPosition(double x, double y)
{
    m_posX = x;
    m_posY = y;
}

void
StartupPhasePacket::GetPosition(double& x, double& y) const
{
    x = m_posX;
    y = m_posY;
}

void
StartupPhasePacket::SetTimestamp(Time timestamp)
{
    m_timestamp = timestamp.GetNanoSeconds();
}

Time
StartupPhasePacket::GetTimestamp() const
{
    return NanoSeconds(m_timestamp);
}

TypeId
StartupPhasePacket::GetTypeId()
{
    static TypeId tid = TypeId("ns3::wsn::scenario5::routing::StartupPhasePacket")
        .SetParent<Header>()
        .SetGroupName("Wsn")
        .AddConstructor<StartupPhasePacket>();
    return tid;
}

TypeId
StartupPhasePacket::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
StartupPhasePacket::Print(std::ostream& os) const
{
    os << "StartupPhasePacket(node=" << m_nodeId 
       << ", pos=(" << m_posX << "," << m_posY << ")";
}

uint32_t
StartupPhasePacket::GetSerializedSize() const
{
    return 4 + 8 + 8 + 8; // nodeId + posX + posY + timestamp
}

void
StartupPhasePacket::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU32(m_nodeId);
    start.WriteHtonU64(*reinterpret_cast<const uint64_t*>(&m_posX));
    start.WriteHtonU64(*reinterpret_cast<const uint64_t*>(&m_posY));
    start.WriteHtonU64(m_timestamp);
}

uint32_t
StartupPhasePacket::Deserialize(Buffer::Iterator start)
{
    m_nodeId = start.ReadNtohU32();
    uint64_t xBits = start.ReadNtohU64();
    uint64_t yBits = start.ReadNtohU64();
    m_posX = *reinterpret_cast<double*>(&xBits);
    m_posY = *reinterpret_cast<double*>(&yBits);
    m_timestamp = start.ReadNtohU64();
    return GetSerializedSize();
}

// ===== FragmentPacket =====

NS_OBJECT_ENSURE_REGISTERED(FragmentPacket);

FragmentPacket::FragmentPacket()
    : m_fragmentId(0), m_confidence(0.0), m_sourceId(0)
{
}

FragmentPacket::~FragmentPacket()
{
}

void
FragmentPacket::SetFragmentId(uint32_t fragmentId)
{
    m_fragmentId = fragmentId;
}

uint32_t
FragmentPacket::GetFragmentId() const
{
    return m_fragmentId;
}

void
FragmentPacket::SetConfidence(double confidence)
{
    m_confidence = confidence;
}

double
FragmentPacket::GetConfidence() const
{
    return m_confidence;
}

void
FragmentPacket::SetSourceId(uint32_t sourceId)
{
    m_sourceId = sourceId;
}

uint32_t
FragmentPacket::GetSourceId() const
{
    return m_sourceId;
}

TypeId
FragmentPacket::GetTypeId()
{
    static TypeId tid = TypeId("ns3::wsn::scenario5::routing::FragmentPacket")
        .SetParent<Header>()
        .SetGroupName("Wsn")
        .AddConstructor<FragmentPacket>();
    return tid;
}

TypeId
FragmentPacket::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
FragmentPacket::Print(std::ostream& os) const
{
    os << "FragmentPacket(id=" << m_fragmentId 
       << ", confidence=" << m_confidence
       << ", source=" << m_sourceId << ")";
}

uint32_t
FragmentPacket::GetSerializedSize() const
{
    return 4 + 8 + 4; // fragmentId + confidence + sourceId
}

void
FragmentPacket::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU32(m_fragmentId);
    start.WriteHtonU64(*reinterpret_cast<const uint64_t*>(&m_confidence));
    start.WriteHtonU32(m_sourceId);
}

uint32_t
FragmentPacket::Deserialize(Buffer::Iterator start)
{
    m_fragmentId = start.ReadNtohU32();
    uint64_t confBits = start.ReadNtohU64();
    m_confidence = *reinterpret_cast<double*>(&confBits);
    m_sourceId = start.ReadNtohU32();
    return GetSerializedSize();
}

// ===== CooperationPacket =====

NS_OBJECT_ENSURE_REGISTERED(CooperationPacket);

CooperationPacket::CooperationPacket()
    : m_requesterId(0), m_cellId(0)
{
}

CooperationPacket::~CooperationPacket()
{
}

void
CooperationPacket::SetRequesterId(uint32_t requesterId)
{
    m_requesterId = requesterId;
}

uint32_t
CooperationPacket::GetRequesterId() const
{
    return m_requesterId;
}

void
CooperationPacket::SetCellId(int32_t cellId)
{
    m_cellId = cellId;
}

int32_t
CooperationPacket::GetCellId() const
{
    return m_cellId;
}

void
CooperationPacket::SetNeededFragments(const std::vector<uint32_t>& fragmentIds)
{
    m_neededFragments = fragmentIds;
}

const std::vector<uint32_t>&
CooperationPacket::GetNeededFragments() const
{
    return m_neededFragments;
}

TypeId
CooperationPacket::GetTypeId()
{
    static TypeId tid = TypeId("ns3::wsn::scenario5::routing::CooperationPacket")
        .SetParent<Header>()
        .SetGroupName("Wsn")
        .AddConstructor<CooperationPacket>();
    return tid;
}

TypeId
CooperationPacket::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
CooperationPacket::Print(std::ostream& os) const
{
    os << "CooperationPacket(requester=" << m_requesterId
       << ", cell=" << m_cellId
       << ", fragments=" << m_neededFragments.size() << ")";
}

uint32_t
CooperationPacket::GetSerializedSize() const
{
    return 4 + 4 + 4 + (m_neededFragments.size() * 4);
}

void
CooperationPacket::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU32(m_requesterId);
    start.WriteHtonU32(static_cast<uint32_t>(m_cellId));
    start.WriteHtonU32(static_cast<uint32_t>(m_neededFragments.size()));
    for (uint32_t fragId : m_neededFragments) {
        start.WriteHtonU32(fragId);
    }
}

uint32_t
CooperationPacket::Deserialize(Buffer::Iterator start)
{
    m_requesterId = start.ReadNtohU32();
    m_cellId = static_cast<int32_t>(start.ReadNtohU32());
    uint32_t count = start.ReadNtohU32();
    m_neededFragments.clear();
    for (uint32_t i = 0; i < count; ++i) {
        m_neededFragments.push_back(start.ReadNtohU32());
    }
    return GetSerializedSize();
}

} // namespace routing
} // namespace scenario5
} // namespace wsn
} // namespace ns3
