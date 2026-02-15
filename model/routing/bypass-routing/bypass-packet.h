#ifndef BYPASS_PACKET_H
#define BYPASS_PACKET_H

#include "ns3/wsn-routing-header.h"

namespace ns3 {
namespace wsn {

class BypassPacket : public WsnRoutingHeader
{
public:
    BypassPacket() : WsnRoutingHeader() {}
    virtual ~BypassPacket() {}
};

} // namespace wsn
} // namespace ns3

#endif // BYPASS_PACKET_H