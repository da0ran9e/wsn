#include "sensor-network.h"
#include "wsn-object.h"
#include "wsn-node.h"
#include "../routing/pecee-routing/pecee-routing-protocol.h"
#include "ns3/ptr.h"
 
namespace ns3 {
namespace wsn {

SensorNetwork::~SensorNetwork() = default;

bool SensorNetwork::SetProperty(const std::string &key, const std::string &value)
{
    if (key == "field_x") {
        field_x = std::stoi(value);
    }
    else if (key == "field_y") {
        field_y = std::stoi(value);
    }
    else if (key == "field_z") {
        field_z = std::stoi(value);
    }
    else if (key == "numNodes") {
        numNodes = std::stoi(value);
    }
    else if (key == "deployment") {
        deployment = value;
    }
    else if (key == "numPhysicalProcesses") {
        numPhysicalProcesses = std::stoi(value);
    }
    else if (key == "wirelessChannel") {
        wirelessChannelName = value;
    }
    else {
        return false;
    }
    
    NotifyAttributeChanged(key, value);
    return true;
}

void SensorNetwork::Build(BuildContext& ctx)
{
    std::cout << "=== Building Sensor Network ===" << std::endl;
    std::cout << "Field size: "
                << field_x << " x "
                << field_y << " x "
                << field_z << std::endl;
    std::cout << "Number of nodes: " << numNodes << std::endl;
// Create nodes
    ctx.nodes.Create(numNodes);

    // Setup mobility
    ctx.mobility.SetMobilityModel("ns3::wsn::WsnMobilityModel");
    ctx.mobility.Install(ctx.nodes);

    // Set positions for all nodes 
    for (uint32_t i = 0; i < ctx.nodes.GetN(); ++i) {
        ns3::wsn::WsnObjectPtr wsnObj = this->GetChildIndexed("node", i);
        double propsx = dynamic_cast<ns3::wsn::Node*>(wsnObj.get())->GetNodeProperties().xCoord;
        double propsy = dynamic_cast<ns3::wsn::Node*>(wsnObj.get())->GetNodeProperties().yCoord;
        double propsz = dynamic_cast<ns3::wsn::Node*>(wsnObj.get())->GetNodeProperties().zCoord;
        //struct NodeProperties props = {propsx, propsy, propsz};
        ctx.nodes.Get(i)->GetObject<ns3::MobilityModel>()->SetPosition(
            Vector(propsx, propsy, propsz));
    }

    // Setup spectrum channel and propagation models
    ctx.spectrumChannel = CreateObject<ns3::SingleModelSpectrumChannel>();
    ctx.lossModel = CreateObject<ns3::LogDistancePropagationLossModel>();
    ctx.delayModel = CreateObject<ns3::ConstantSpeedPropagationDelayModel>();
    ctx.spectrumChannel->AddPropagationLossModel(ctx.lossModel);
    ctx.spectrumChannel->SetPropagationDelayModel(ctx.delayModel);
    
    // Setup LR-WPAN helper
    ctx.lrwpan.SetChannel(ctx.spectrumChannel);

    // Install network devices
    ctx.netDevices = ctx.lrwpan.Install(ctx.nodes);
    
    // Configure MAC addresses to match node IDs (node i → MAC i+1)
    // Also set PAN ID and start RX for proper LR-WPAN operation
    for (uint32_t i = 0; i < ctx.netDevices.GetN(); ++i) {
        Ptr<ns3::lrwpan::LrWpanNetDevice> dev =
            ns3::DynamicCast<ns3::lrwpan::LrWpanNetDevice>(ctx.netDevices.Get(i));
        dev->GetMac()->SetShortAddress(ns3::Mac16Address(i+1));
        dev->GetMac()->SetPanId(0);  // Set PAN ID to 0 for all nodes
        
        // Start PHY in RX mode to listen for packets
        dev->GetPhy()->PlmeSetTRXStateRequest(ns3::lrwpan::IEEE_802_15_4_PHY_RX_ON);
    }
    
    
    // Configure each node
    for (uint32_t i = 0; i < ctx.nodes.GetN(); ++i)
    {
        // Create forwarder and routing protocol
        Ptr<wsn::WsnForwarder> forwarder = CreateObject<wsn::WsnForwarder>();
        Ptr<wsn::PeceeRoutingProtocol> routing = CreateObject<wsn::PeceeRoutingProtocol>();
        
        Ptr<ns3::Node> node = ctx.nodes.Get(i);
        Ptr<ns3::NetDevice> dev = ctx.netDevices.Get(i);

        routing->SetSelfNodeProperties({
            static_cast<uint16_t>(i),
            node->GetObject<ns3::MobilityModel>()->GetPosition().x,
            node->GetObject<ns3::MobilityModel>()->GetPosition().y,
            node->GetObject<ns3::MobilityModel>()->GetPosition().z
        });

        routing->SetForwarder(forwarder);
        forwarder->SetNetDevice(dev);

        node->AggregateObject(forwarder);
        node->AggregateObject(routing);

        // Set CH status based on node ID AFTER aggregating to node
        // Nodes 0 and 99 are CHs as defined in input-pecee.ini
        bool isClusterHead = (i == 0 || i == 99);
        routing->SetAttribute("isCH", ns3::BooleanValue(isClusterHead));
        routing->SetAttribute("cellRadius", ns3::DoubleValue(20.0));
        
        if (isClusterHead) {
            std::cout << "Set Node " << i << " as Cluster Head" << std::endl;
        }

        routing->Start();
    }
}

} // namespace wsn
} // namespace ns3
