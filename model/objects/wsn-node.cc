#include "wsn-node.h"
namespace ns3 {
namespace wsn {

bool Node::SetProperty(const std::string &key, const std::string &value)
{
    if (key == "nodeAddr") {
        nodeAddr = static_cast<uint16_t>(std::stoi(value));
    }
    else if (key == "xCoor") {
        xCoor = std::stod(value);
    }
    else if (key == "yCoor") {
        yCoor = std::stod(value);
    }
    else if (key == "zCoor") {
        zCoor = std::stod(value);
    }
    else if (key == "phi") {
        phi = std::stod(value);
    }
    else if (key == "theta") {
        theta = std::stod(value);
    }
    else if (key == "startupOffset") {
        startupOffset = std::stod(value);
    }
    else if (key == "startupRandomization") {
        startupRandomization = std::stod(value);
    }
    else if (key == "ApplicationName") {
        ApplicationName = value;
    }
    else if (key == "MACProtocolName") {
        MACProtocolName = value;
    }
    else if (key == "RadioProtocolName") {
        RadioProtocolName = value;
    }
    else if (key == "RoutingProtocolName") {
        RoutingProtocolName = value;
    }
    else {
        return false;
    }

    NotifyAttributeChanged(key, value);
    return true;
}

void Node::Build(BuildContext& ctx)
{
    if(m_built) {
        return;
    }
    m_built = true;

    int nodeIndex = GetAddr();
    std::cout << "---------Building node: " << nodeIndex << "---------" << std::endl;
    int objId = ctx.nodeAddr[nodeIndex];
    if(ctx.nodes.Contains(nodeIndex)) {
        m_selfNode = ctx.nodes.Get(nodeIndex);
        //std::cout << "Node " << nodeIndex << " found with ID " << m_selfNode->GetId() << std::endl;
    } else {
        std::cerr << "Error: Node with index " << nodeIndex << " not found in context." << std::endl;
    }

    m_selfNode->GetObject<ns3::MobilityModel>()->SetPosition(
        ns3::Vector(xCoor, yCoor, zCoor)
    );
    std::cout << "Node " << nodeIndex << " position set to ("
              << xCoor << ", " << yCoor << ", " << zCoor << ")" << std::endl;
    // GetChild("Radio")->Build(ctx);
    // GetChild("MAC")->Build(ctx);
    // GetChild("Routing")->Build(ctx);
    // m_selfNode->AggregateObject(ctx.forwarder);
    // m_selfNode->AggregateObject(ctx.routing);

    // ctx.routing->Start();
}

} // namespace wsn
} // namespace ns3