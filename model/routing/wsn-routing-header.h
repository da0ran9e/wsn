// wsn-routing-header.h
#ifndef WSN_ROUTING_HEADER_H
#define WSN_ROUTING_HEADER_H

#include "ns3/header.h"
#include "ns3/nstime.h"
#include "ns3/address.h"


namespace ns3 {
namespace wsn {

class WsnRoutingHeader : public Header
{
public:
    WsnRoutingHeader();

    struct NetMacInfoExchange_type {
        double RSSI;
        double LQI;
        uint16_t nextHop;
        uint16_t lastHop;
    };

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    void SetNetMacInfoExchange(const NetMacInfoExchange_type& info){ m_netMacInfoExchange = info; };
    void SetSource(uint16_t src){ m_source = src; };
    void SetDestination(uint16_t dst){ m_destination = dst; };
    void SetSequenceNumber(uint16_t seq){ m_sequenceNumber = seq; };

    NetMacInfoExchange_type GetNetMacInfoExchange() const { return m_netMacInfoExchange; };
    uint16_t GetSource() const { return m_source; };
    uint16_t GetDestination() const { return m_destination; };
    uint16_t GetSequenceNumber() const { return m_sequenceNumber; };

    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream &os) const override;

private:
	uint16_t m_source;
	uint16_t m_destination;
	uint16_t m_sequenceNumber;
    NetMacInfoExchange_type m_netMacInfoExchange;   
};



} // namespace wsn
} // namespace ns3


#endif // WSN_CELLULAR_HEADER_H