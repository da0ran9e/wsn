#include "wsn-routing.h"
#include "wsn-node.h"
#include "ns3/wsn-forwarder.h"
#include "ns3/bypass-routing-protocol.h"

namespace ns3 {
namespace wsn {

bool WsnRouting::SetProperty(const std::string &key, const std::string &value)
{
    if (key == "collectTraceInfo") {
        collectTraceInfo = (value == "true" || value == "1");
    }
    else if (key == "maxNetFrameSize") {
        maxNetFrameSize = std::stoi(value);
    }
    else if (key == "netDataFrameOverhead") {
        netDataFrameOverhead = std::stoi(value);
    }
    else if (key == "netBufferSize") {
        netBufferSize = std::stoi(value);
    }
    else {
        return false;
    }

    NotifyAttributeChanged(key, value);
    return true;
}

void WsnRouting::Build(BuildContext& ctx)
{
    if(m_built) {
        return;
    }
    m_built = true;
    std::cout << "Building Routing: " << GetInstanceName() << std::endl;
    
    // ctx.forwarder = CreateObject<ns3::wsn::WsnForwarder>();
    // ctx.routing = CreateObject<ns3::wsn::BypassRoutingProtocol>();
    
    // std::shared_ptr<ns3::wsn::Node> node = FindAncestor<ns3::wsn::Node>();
    // NS_ASSERT(node);
    // uint16_t nodeId = node->GetAddr();
    // ctx.routing->SetSelfNodeProperties({
    //     nodeId,
    //     node->GetNodeProperties().xCoord,
    //     node->GetNodeProperties().yCoord,
    //     node->GetNodeProperties().zCoord
    // });
    // ctx.forwarder->SetNetDevice(ctx.netDevices.Get(nodeId));
    // ctx.routing->SetForwarder(ctx.forwarder);
    // std::cout << "Routing built for Node ID: " << nodeId << std::endl;

}


} // namespace wsn
} // namespace ns3
