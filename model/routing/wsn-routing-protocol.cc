#include "wsn-routing-protocol.h"

#include "ns3/simulator.h"

namespace ns3 {
namespace wsn {

NS_LOG_COMPONENT_DEFINE("WsnCellularRouting");
NS_OBJECT_ENSURE_REGISTERED(WsnRoutingProtocol);

TypeId
WsnRoutingProtocol::GetTypeId()
{
  static TypeId tid = TypeId("ns3::wsn::WsnRoutingProtocol")
      .SetParent<Object>()
      .SetGroupName("Wsn")
      .AddConstructor<WsnRoutingProtocol>();
  return tid;
}

WsnRoutingProtocol::WsnRoutingProtocol() {}
WsnRoutingProtocol::~WsnRoutingProtocol() {}

void
WsnRoutingProtocol::HandlePacket(Ptr<Packet> packet,
                               const WsnRoutingHeader &header)
{
  int srcNodeId = header.GetSource();
  std::cout << "[Routing] Node " << m_selfNodeProps.nodeId
              << " DELIVER packet from " << srcNodeId << std::endl;
}

void WsnRoutingProtocol::FromMacLayer(Ptr<Packet> pkt, const uint16_t src)
{
}

void WsnRoutingProtocol::Start()
{
    std::cout << "[Routing] Starting base routing" << std::endl;
}

} // namespace wsn
} // namespace ns3