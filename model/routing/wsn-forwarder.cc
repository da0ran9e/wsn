#include "wsn-forwarder.h"
#include "ns3/node.h"
#include "ns3/mac16-address.h"


namespace ns3 {
namespace wsn {

NS_LOG_COMPONENT_DEFINE("WsnForwarder");
NS_OBJECT_ENSURE_REGISTERED(WsnForwarder);

TypeId
WsnForwarder::GetTypeId()
{
  static TypeId tid = TypeId("ns3::wsn::WsnForwarder")
    .SetParent<Object>()
    .SetGroupName("Wsn")
    .AddConstructor<ns3::wsn::WsnForwarder>();
  return tid;
}

WsnForwarder::WsnForwarder() {}
WsnForwarder::~WsnForwarder() {}

void
WsnForwarder::SetNetDevice(Ptr<NetDevice> dev)
{
  m_dev = dev;

  Ptr<lrwpan::LrWpanNetDevice> lrDev =
      DynamicCast<lrwpan::LrWpanNetDevice>(dev);

  NS_ABORT_MSG_IF(!lrDev, "Not an LR-WPAN device");

  Ptr<lrwpan::LrWpanMac> mac = lrDev->GetMac();

  mac->SetMcpsDataIndicationCallback(
      MakeCallback(&WsnForwarder::ReceiveFromMac, this));
}

void
WsnForwarder::ToMacLayer(Ptr<Packet> packet, const uint16_t dst)
{
  NS_ASSERT(m_dev);
  Address macAddr = ResolveMACAddress(dst);
  // std::cout << "[Forwarder] ToMacLayer dst:" << dst
  //           << " MAC:" << Mac16Address::ConvertFrom(macAddr)
  //           << " pktSize:" << packet->GetSize() << std::endl;
  m_dev->Send(packet, macAddr, 0);
}

void
WsnForwarder::ReceiveFromMac(
  lrwpan::McpsDataIndicationParams params,
  Ptr<Packet> pkt)
{
  //std::cout << "[Forwarder] ReceiveFromMac pktSize:" << pkt->GetSize() << std::endl;
  // Peek routing header
  WsnRoutingHeader hdr;
  pkt->PeekHeader(hdr);

    uint16_t srcNodeId = hdr.GetSource();

    for (auto l : m_listeners)
    {
        l->FromMacLayer(pkt->Copy(), srcNodeId);
    }

    // std::cout << "[Forwarder] Node received pkt from node "
    //           << srcNodeId << std::endl;
}

Address
WsnForwarder::ResolveMACAddress(uint16_t nodeId)
{
    if (nodeId == 0xFFFF) {
        return Mac16Address::GetBroadcast();
    }
    return Mac16Address(nodeId+1);
}

void
WsnForwarder::AddListener(Listener* listener)
{
  NS_ASSERT(listener);
  m_listeners.push_back(listener);
}


} // namespace wsncellular
} // namespace ns3
