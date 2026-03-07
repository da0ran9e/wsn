/*
 * Scenario 4 - Base Station Node Implementation
 */

#include "base-station-node.h"
#include "../helper/calc-utils.h"
#include "region-selection.h"
#include "uav-control.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("BaseStationNode");

namespace wsn {
namespace scenario4 {
namespace routing {

// Global callback definitions
std::function<void(const GlobalTopology&)> g_bsTopologyCallback;
std::function<void(uint32_t, const UavFlightPath&)> g_bsUavCommandCallback;

BaseStationNode::BaseStationNode(uint32_t nodeId)
    : m_nodeId(nodeId),
      m_topologyReceived(false)
{
    NS_LOG_FUNCTION(this << nodeId);
}

BaseStationNode::~BaseStationNode()
{
    NS_LOG_FUNCTION(this);
}

void
BaseStationNode::Initialize()
{
    NS_LOG_FUNCTION(this);
    
    // Register callback for topology reception
    g_bsTopologyCallback = [this](const GlobalTopology& topo) {
        this->ReceiveTopology(topo);
    };
    
    NS_LOG_INFO("BS Node " << m_nodeId << " initialized with callback registration");
}

void
BaseStationNode::ReceiveTopology(const GlobalTopology& topology)
{
    NS_LOG_FUNCTION(this);
    
    m_topology = topology;
    m_topologyReceived = true;
    
    NS_LOG_INFO("BS received topology with " << topology.nodes.size() 
                << " nodes at t=" << topology.timestamp);
    
    // Trigger region selection after receiving topology
    if (m_topologyReceived) {
        m_suspiciousNodes = SelectSuspiciousRegion();
        
        if (!m_suspiciousNodes.empty()) {
            UavFlightPath path = CalculateFlightPath(m_suspiciousNodes);
            // UAV node ID will be assigned when UAV is created
            // Send command via callback path
            SendWaypointCommand(0, path);
            NS_LOG_INFO("UAV flight path calculated: " << path.waypoints.size() 
                        << " waypoints, total time: " << path.totalTime << "s");
        }
    }
}

std::set<uint32_t>
BaseStationNode::SelectSuspiciousRegion()
{
    NS_LOG_FUNCTION(this);
    
    if (!m_topologyReceived || m_topology.nodes.empty()) {
        NS_LOG_WARN("Cannot select suspicious region: no topology available");
        return std::set<uint32_t>();
    }
    
    // Calculate suspicious score for each node
    std::set<uint32_t> suspiciousNodes = SelectSuspiciousRegionFromTopology(m_topology, 0.30);
    
    NS_LOG_INFO("Selected " << suspiciousNodes.size() << " suspicious nodes out of " 
                << m_topology.nodes.size() << " total nodes");
    
    return suspiciousNodes;
}

UavFlightPath
BaseStationNode::CalculateFlightPath(const std::set<uint32_t>& suspiciousNodes)
{
    NS_LOG_FUNCTION(this << suspiciousNodes.size());
    
    UavFlightPath path;
    
    if (suspiciousNodes.empty()) {
        NS_LOG_WARN("No suspicious nodes to visit");
        return path;
    }
    
    path = BuildGreedyFlightPath(m_topology, suspiciousNodes, 50.0, 10.0);
    
    NS_LOG_INFO("Flight path calculated: " << path.waypoints.size() 
                << " waypoints, total time: " << path.totalTime << "s");
    
    return path;
}

void
BaseStationNode::SendWaypointCommand(uint32_t uavNodeId, const UavFlightPath& path)
{
    NS_LOG_FUNCTION(this << uavNodeId << path.waypoints.size());
    
    // Use callback to send command to UAV
    if (g_bsUavCommandCallback) {
        g_bsUavCommandCallback(uavNodeId, path);
        NS_LOG_INFO("BS sent waypoint command to UAV " << uavNodeId);
    } else {
        NS_LOG_WARN("UAV command callback not registered");
    }
}

uint32_t
BaseStationNode::GetNodeId() const
{
    return m_nodeId;
}

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3
