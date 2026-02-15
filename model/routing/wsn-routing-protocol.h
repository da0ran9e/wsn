#ifndef WSN_ROUTING_PROTOCOL_H
#define WSN_ROUTING_PROTOCOL_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/log.h"
#include "ns3/mac16-address.h"
#include "ns3/simulator.h"

#include "wsn-forwarder.h"

#include "wsn-routing-header.h"

namespace ns3 {
namespace wsn {

class WsnForwarder;

class WsnRoutingProtocol : public Object,
public ns3::wsn::WsnForwarder::Listener
{
public:
  struct NodeProperties {
    uint16_t nodeId;
    double xCoord;
    double yCoord;
    double zCoord;
  };

  static TypeId GetTypeId();

  WsnRoutingProtocol();
  virtual ~WsnRoutingProtocol();

  //void SetNode(Ptr<Node> node){ m_node = node; }
  void SetForwarder(Ptr<WsnForwarder> forwarder){ 
    m_forwarder = forwarder; 
  m_forwarder->AddListener(this);
  }

  void ToMacLayer(Ptr<Packet> packet, const uint16_t dst){
    m_forwarder->ToMacLayer(packet, dst);
  }

  void FromMacLayer(Ptr<Packet> pkt,
                    const uint16_t src) override;

  void HandlePacket(Ptr<Packet> packet,
                  const WsnRoutingHeader &header);

  void SetSelfNodeProperties(const NodeProperties &props) {
    m_selfNodeProps = props;
  }

  virtual void Start();
private:
  Ptr<WsnForwarder> m_forwarder;
protected:
  NodeProperties m_selfNodeProps;
};

} // namespace wsn
} // namespace ns3

#endif // WSN_ROUTING_PROTOCOL_H
