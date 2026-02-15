#include "wsn-radio.h"
#include "wsn-node.h"

namespace ns3 {
namespace wsn {
bool Radio::SetProperty(const std::string &key, const std::string &value)
{
    if (key == "radioTxPowerDbm") {
        radioTxPowerDbm = std::stod(value);
    }
    else if (key == "radioRxSensitivityDbm") {
        radioRxSensitivityDbm = std::stod(value);
    }
    else if (key == "radioChannelBandwidthKbps") {
        radioChannelBandwidthKbps = std::stod(value);
    }
    else if (key == "radioHeaderOverhead") {
        radioHeaderOverhead = std::stoi(value);     
    }
    else {
        return false;
    }   
    NotifyAttributeChanged(key, value);
    return true;
}

void Radio::Build(BuildContext& ctx)
{
    if(m_built) {
        return;
    }
    m_built = true;

    std::cout << "Building Radio: " << GetInstanceName() << std::endl;
    // Implementation of the Build method
    // WsnObject::Build(ctx);
    // std::shared_ptr<ns3::wsn::Node> node = FindAncestor<ns3::wsn::Node>();
    // NS_ASSERT(node);
    // uint32_t nodeId = node->GetAddr();
    // m_selfDetDevice = ctx.netDevices.Get(nodeId);
    // std::cout << "Radio built for Node " << nodeId << " with NetDevice ID " << m_selfDetDevice->GetIfIndex() << std::endl;
}

} // namespace wsn
} // namespace ns3