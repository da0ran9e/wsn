#ifndef BYPASS_ROUTING_PROTOCOL_H
#define BYPASS_ROUTING_PROTOCOL_H

#include "ns3/wsn-routing-protocol.h"
#include "bypass-packet.h"

namespace ns3 {
namespace wsn {

class BypassRoutingProtocol : public WsnRoutingProtocol
{
public:
    BypassRoutingProtocol();
    virtual ~BypassRoutingProtocol();

    static TypeId GetTypeId();
    void FromMacLayer(Ptr<Packet> pkt,
                      const uint16_t src) override;
    void Start() override;
private:
    void SendBeacon();
};

} // namespace wsn
} // namespace ns3

#endif