#include "bypass-routing-protocol.h"
#include "ns3/simulator.h"

namespace ns3 {
namespace wsn { 

NS_LOG_COMPONENT_DEFINE("BypassRoutingProtocol");
NS_OBJECT_ENSURE_REGISTERED(BypassRoutingProtocol);

BypassRoutingProtocol::BypassRoutingProtocol() {}
BypassRoutingProtocol::~BypassRoutingProtocol() {}

TypeId BypassRoutingProtocol::GetTypeId()
{
    static TypeId tid = TypeId("ns3::wsn::BypassRoutingProtocol")
        .SetParent<WsnRoutingProtocol>()
        .SetGroupName("Wsn")
        .AddConstructor<BypassRoutingProtocol>();
    return tid;
}

void BypassRoutingProtocol::Start(){
// Implementation of routing start logic
// Beaconing in random statup delay
    std::cout << "[BypassRouting] Starting Bypass Routing Protocol on Node "
              << m_selfNodeProps.nodeId << std::endl;
    ns3::Simulator::Schedule(MilliSeconds(rand() % 1000), &BypassRoutingProtocol::SendBeacon, this);

}

void BypassRoutingProtocol::FromMacLayer(Ptr<Packet> pkt,
                                        const uint16_t src)
{
    std::cout << "[BypassRouting] Node " << m_selfNodeProps.nodeId
              << " received packet from MAC layer, src=" << src << std::endl;

    // Here we would normally parse the packet and decide what to do
    // For simplicity, we just log the reception
}

void BypassRoutingProtocol::SendBeacon()
{
    std::cout << "[BypassRouting] Node " << Mac16Address(m_selfNodeProps.nodeId)   
              << " is sending a beacon." << std::endl;

    // // Create a bypass packet (details omitted for brevity)
    Ptr<Packet> beaconPacket = Create<Packet>(100); // Example size
    ToMacLayer(beaconPacket, 0xFFFF); // Broadcast address
    // // Send the packet to all neighbors (broadcast)
    // m_mac->Send(beaconPacket, 0xFFFF); // 0xFFFF is the broadcast address
    // std::cout << "[BypassRouting] Node " << m_selfNodeProps.nodeId
    //           << " beacon sent." << std::endl;
}

}
}