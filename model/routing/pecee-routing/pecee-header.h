#ifndef PECEE_HEADER_H
#define PECEE_HEADER_H

#include "ns3/wsn-routing-header.h"
#include "ns3/buffer.h"

namespace ns3 {
namespace wsn {

enum PeceePacketType {
    HELLO_PACKET = 0,         
    CL_ANNOUNCEMENT = 1,    
    CL_CONFIRMATION = 2,
    BS_UPDATE_PACKET = 3,       
    LINK_REQUEST = 4,        
    LINK_ACK = 5,             
    LINK_ESTABLISHED = 6,      
    INTRA_CELL_ROUTING_UPDATE = 7,
    CL_COMMAND_PACKET = 8,
    NCL_CONFIRM_PACKET = 9,
    ROUTING_TREE_UPDATE_PACKET = 10,
    CH_ANNOUNCEMENT_PACKET = 11,
    DATA_PACKET = 12,
    ANNOUNCE_CELL_HOP = 13,
    SENSOR_DATA = 14,
    FINALIZE_PKT = 15
};

struct SSCHAnnouncementInfo {
    int chId;
};

struct SSCellHopAnnouncementInfo {
    int nextCell;
    int cellPath[3];
};

struct SSSensorInfo {
    int destinationCH;
    double dataId;
    int sensorId;
    int hopCount;
};

class PeceeHeader : public WsnRoutingHeader
{
public:
    PeceeHeader();
    virtual ~PeceeHeader();
    
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    
    // Setters
    void SetPacketType(PeceePacketType type) { m_packetType = type; }
    void SetClusterHead(int ch) { m_clusterHead = ch; }
    void SetCellSent(int cell) { m_cellSent = cell; }
    void SetCellNext(int cell) { m_cellNext = cell; }
    void SetCellNextNext(int cell) { m_cellNextNext = cell; }
    void SetCellSource(int cell) { m_cellSource = cell; }
    void SetCellDestination(int cell) { m_cellDestination = cell; }
    void SetCellHopCount(int count) { m_cellHopCount = count; }
    void SetTtl(int ttl) { m_ttl = ttl; }
    void SetCellPath(int index, int value) { if (index < 3) m_cellPath[index] = value; }
    void SetSensorData(const SSSensorInfo& data) { m_sensorData = data; }
    void SetCHAnnouncementData(const SSCHAnnouncementInfo& data) { m_chAnnouncementData = data; }
    void SetSSCellHopAnnouncementData(const SSCellHopAnnouncementInfo& data) { m_cellHopAnnouncementData = data; }
    
    // Getters
    PeceePacketType GetPacketType() const { return m_packetType; }
    int GetClusterHead() const { return m_clusterHead; }
    int GetCellSent() const { return m_cellSent; }
    int GetCellNext() const { return m_cellNext; }
    int GetCellNextNext() const { return m_cellNextNext; }
    int GetCellSource() const { return m_cellSource; }
    int GetCellDestination() const { return m_cellDestination; }
    int GetCellHopCount() const { return m_cellHopCount; }
    int GetTtl() const { return m_ttl; }
    int GetCellPath(int index) const { return (index < 3) ? m_cellPath[index] : -1; }
    SSSensorInfo GetSensorData() const { return m_sensorData; }
    SSCHAnnouncementInfo GetCHAnnouncementData() const { return m_chAnnouncementData; }
    SSCellHopAnnouncementInfo GetSSCellHopAnnouncementData() const { return m_cellHopAnnouncementData; }
    
    // Header interface
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream &os) const override;
    
private:
    PeceePacketType m_packetType;
    int m_clusterHead; 
    int m_cellSent;
    int m_cellNext;
    int m_cellNextNext;
    int m_cellSource;
    int m_cellDestination;
    int m_cellHopCount;
    int m_cellPath[3];
    int m_ttl;
    
    SSSensorInfo m_sensorData;
    SSCHAnnouncementInfo m_chAnnouncementData;
    SSCellHopAnnouncementInfo m_cellHopAnnouncementData;
};

}
}
#endif // PECEE_HEADER_H