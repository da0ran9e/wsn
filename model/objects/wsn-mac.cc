#include "wsn-mac.h"
#include "wsn-node.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/lr-wpan-mac.h"
#include "ns3/mac16-address.h"

namespace ns3 {
namespace wsn {
bool WsnMac::SetProperty(const std::string &key, const std::string &value)
{
    if (key == "macMaxPacketSize") {
        macMaxPacketSize = std::stoi(value);
    }
    else if (key == "macBufferSize") {
        macBufferSize = std::stoi(value);
    }
    else if (key == "macPacketOverhead") {
        macPacketOverhead = std::stoi(value);
    }
    else {
        return false;
    }

    NotifyAttributeChanged(key, value);
    return true;
}
void WsnMac::Build(BuildContext& ctx)
{
    if(m_built) {
        return;
    }
    m_built = true;
    std::cout << "Building MAC: " << GetInstanceName() << std::endl;
    
    // std::shared_ptr<ns3::wsn::Node> node = FindAncestor<ns3::wsn::Node>();
    // NS_ASSERT(node);
    // uint32_t nodeId = node->GetAddr();
    // std::cout << "MAC built for Node ID: " << nodeId;
    // Ptr<NetDevice> dev = ctx.netDevices.Get(nodeId);
    // Ptr<lrwpan::LrWpanNetDevice> lrwpan = DynamicCast<lrwpan::LrWpanNetDevice>(dev);
    // lrwpan->GetMac()->SetShortAddress(Mac16Address(nodeId));  
    // std::cout << " with Short Address: " << ns3::Mac16Address(nodeId) << std::endl;
}

} // namespace wsn
} // namespace ns3