/*
 * Scenario 4 - Ground Node Routing Implementation
 */

#include "ground-node-routing.h"
#include "../packet-header.h"
#include "../helper/calc-utils.h"
#include "cell-cooperation.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/mobility-model.h"
#include "ns3/node-list.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("GroundNodeRouting");

namespace wsn {
namespace scenario4 {
namespace routing {

// Global state storage
std::map<uint32_t, GroundNetworkState> g_groundNetworkPerNode;

void
InitializeGroundNodeRouting(NodeContainer nodes, uint32_t numFragments)
{
    NS_LOG_FUNCTION(nodes.GetN() << numFragments);
    
    // Initialize state for each ground node
    for (uint32_t i = 0; i < nodes.GetN(); ++i) {
        Ptr<Node> node = nodes.Get(i);
        uint32_t nodeId = node->GetId();
        
        GroundNetworkState state;
        state.nodeId = nodeId;
        state.fragments = GenerateFragments(numFragments);
        state.confidence = state.fragments.totalConfidence;
        state.packetCount = 0;
        
        // Compute cell ID from position
        Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
        if (mobility) {
            Vector pos = mobility->GetPosition();
            state.cellId = helper::ComputeCellId(pos.x, pos.y, 80.0);
        } else {
            state.cellId = 0;
        }
        
        g_groundNetworkPerNode[nodeId] = state;
        
        NS_LOG_DEBUG("Ground node " << nodeId << " initialized with " 
                     << numFragments << " fragments, confidence=" 
                     << state.confidence << ", cell=" << state.cellId);
    }
    
    NS_LOG_INFO("Ground node routing initialized for " << nodes.GetN() << " nodes");
}

void
OnGroundNodeReceivePacket(uint32_t nodeId, Ptr<const Packet> packet, double rssiDbm)
{
    NS_LOG_FUNCTION(nodeId << packet->GetSize() << rssiDbm);
    
    if (g_groundNetworkPerNode.find(nodeId) == g_groundNetworkPerNode.end()) {
        NS_LOG_WARN("Node " << nodeId << " not initialized");
        return;
    }
    
    GroundNetworkState& state = g_groundNetworkPerNode[nodeId];
    state.packetCount++;
    
    // Try to extract packet header
    Ptr<Packet> copy = packet->Copy();
    PacketHeader header;
    copy->RemoveHeader(header);
    
    switch (header.GetType()) {
        case PACKET_TYPE_STARTUP:
            NS_LOG_DEBUG("Node " << nodeId << " received STARTUP packet");
            // Handle startup phase (will be implemented in startup-phase module)
            break;
            
        case PACKET_TYPE_FRAGMENT:
            NS_LOG_DEBUG("Node " << nodeId << " received FRAGMENT packet");
            // Handle fragment reception
            {
                FragmentPacket fragPkt;
                copy->RemoveHeader(fragPkt);
                
                uint32_t fragId = fragPkt.GetFragmentId();
                double confidence = fragPkt.GetConfidence();
                
                // Update fragment if new or higher confidence
                if (!state.fragments.HasFragment(fragId) ||
                    state.fragments.GetFragment(fragId)->confidence < confidence) {
                    
                    Fragment frag;
                    frag.fragmentId = fragId;
                    frag.confidence = confidence;
                    frag.size = 1024;
                    
                    state.fragments.AddFragment(frag);
                    state.confidence = state.fragments.totalConfidence;
                    
                    NS_LOG_INFO("Node " << nodeId << " updated fragment " << fragId 
                                << " with confidence " << confidence);
                }

                // Trigger in-cell cooperation when threshold condition is met
                RequestFragmentSharing(nodeId, state.cellId);
            }
            break;
            
        case PACKET_TYPE_COOPERATION:
            NS_LOG_DEBUG("Node " << nodeId << " received COOPERATION packet");
            // Handle cooperation (will be implemented in cell-cooperation module)
            break;
            
        default:
            NS_LOG_WARN("Node " << nodeId << " received unknown packet type");
            break;
    }
}

GlobalTopology
BuildTopologySnapshot()
{
    NS_LOG_FUNCTION_NOARGS();
    
    GlobalTopology topology;
    topology.timestamp = Simulator::Now().GetSeconds();
    
    for (const auto& entry : g_groundNetworkPerNode) {
        uint32_t nodeId = entry.first;
        const GroundNetworkState& state = entry.second;
        
        NodeInfo info;
        info.nodeId = nodeId;
        info.neighbors = state.neighbors;
        info.avgConfidence = state.confidence;
        info.packetCount = state.packetCount;
        
        // Get position from node
        Ptr<Node> node = NodeList::GetNode(nodeId);
        if (node) {
            Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
            if (mobility) {
                info.position = mobility->GetPosition();
            }
        }
        
        topology.nodes[nodeId] = info;
    }
    
    NS_LOG_INFO("Built topology snapshot with " << topology.nodes.size() 
                << " nodes at t=" << topology.timestamp);
    
    return topology;
}

void
SendTopologyToBS()
{
    NS_LOG_FUNCTION_NOARGS();
    
    GlobalTopology topology = BuildTopologySnapshot();
    
    // Send to BS via callback
    if (g_bsTopologyCallback) {
        g_bsTopologyCallback(topology);
        NS_LOG_INFO("Topology sent to BS with " << topology.nodes.size() << " nodes");
    } else {
        NS_LOG_WARN("BS topology callback not registered");
    }
}

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3
