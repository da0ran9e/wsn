/*
 * Scenario 4 - Unified Packet Header Definitions
 */

#ifndef SCENARIO5_PACKET_HEADER_H
#define SCENARIO5_PACKET_HEADER_H

#include "ns3/header.h"
#include "ns3/nstime.h"
#include <vector>

namespace ns3 {
namespace wsn {
namespace scenario5 {
namespace routing {

/**
 * Packet types for scenario5 protocol.
 */
enum PacketType
{
    PACKET_TYPE_STARTUP = 1,     ///< Startup phase discovery
    PACKET_TYPE_FRAGMENT = 2,    ///< Fragment data
    PACKET_TYPE_COOPERATION = 3, ///< Cell cooperation request
    PACKET_TYPE_UAV_COMMAND = 4  ///< UAV command (via callback, not actual network packet)
};

/**
 * Base packet header with type field.
 */
class PacketHeader : public Header
{
public:
    PacketHeader();
    virtual ~PacketHeader();
    
    void SetType(PacketType type);
    PacketType GetType() const;
    
    // Header serialization
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    
private:
    uint8_t m_type;
};

/**
 * Startup phase discovery packet.
 * 
 * Broadcast to neighbors for topology building.
 */
class StartupPhasePacket : public Header
{
public:
    StartupPhasePacket();
    virtual ~StartupPhasePacket();
    
    void SetNodeId(uint32_t nodeId);
    uint32_t GetNodeId() const;
    
    void SetPosition(double x, double y);
    void GetPosition(double& x, double& y) const;
    
    void SetTimestamp(Time timestamp);
    Time GetTimestamp() const;
    
    // Header serialization
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    
private:
    uint32_t m_nodeId;
    double m_posX;
    double m_posY;
    uint64_t m_timestamp; // in nanoseconds
};

/**
 * Fragment data packet.
 * 
 * Contains file fragment with metadata.
 */
class FragmentPacket : public Header
{
public:
    FragmentPacket();
    virtual ~FragmentPacket();
    
    void SetFragmentId(uint32_t fragmentId);
    uint32_t GetFragmentId() const;
    
    void SetConfidence(double confidence);
    double GetConfidence() const;
    
    void SetSourceId(uint32_t sourceId);
    uint32_t GetSourceId() const;
    
    // Header serialization
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    
private:
    uint32_t m_fragmentId;
    double m_confidence;
    uint32_t m_sourceId;
};

/**
 * Cell cooperation request packet.
 * 
 * Request for fragment sharing within cell.
 */
class CooperationPacket : public Header
{
public:
    CooperationPacket();
    virtual ~CooperationPacket();
    
    void SetRequesterId(uint32_t requesterId);
    uint32_t GetRequesterId() const;
    
    void SetCellId(int32_t cellId);
    int32_t GetCellId() const;
    
    void SetNeededFragments(const std::vector<uint32_t>& fragmentIds);
    const std::vector<uint32_t>& GetNeededFragments() const;
    
    // Header serialization
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    
private:
    uint32_t m_requesterId;
    int32_t m_cellId;
    std::vector<uint32_t> m_neededFragments;
};

} // namespace routing
} // namespace scenario5
} // namespace wsn
} // namespace ns3

#endif // SCENARIO5_PACKET_HEADER_H
