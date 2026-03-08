/*
 * Scenario 4 - Base Station Node
 * 
 * BS node uses callbacks for control plane communication.
 * Does not use network packets for control messages.
 */

#ifndef SCENARIO4_BASE_STATION_NODE_H
#define SCENARIO4_BASE_STATION_NODE_H

#include "ns3/node.h"
#include "ns3/vector.h"
#include <map>
#include <set>
#include <vector>
#include <functional>

namespace ns3 {
namespace wsn {
namespace scenario4 {
namespace routing {

/**
 * Node information for topology snapshot.
 */
struct NodeInfo
{
    uint32_t nodeId;
    Vector position;
    std::set<uint32_t> neighbors;
    double avgConfidence;
    uint32_t packetCount;
};

/**
 * Global topology snapshot.
 */
struct GlobalTopology
{
    std::map<uint32_t, NodeInfo> nodes;
    double timestamp;
};

/**
 * UAV waypoint.
 */
struct Waypoint
{
    Vector position;
    double arrivalTime;
};

/**
 * UAV flight path.
 */
struct UavFlightPath
{
    std::vector<Waypoint> waypoints;
    double totalTime;
};

// ===== Global Callbacks =====
// These are used by ground/UAV nodes to communicate with BS
extern std::function<void(const GlobalTopology&)> g_bsTopologyCallback;
extern std::function<void(uint32_t, const UavFlightPath&)> g_bsUavCommandCallback;

/**
 * Base Station Node.
 * 
 * Responsibilities:
 * - Receive topology from ground nodes
 * - Select suspicious region
 * - Plan UAV flight path
 * - Send commands to UAV
 */
class BaseStationNode
{
public:
    /**
     * Constructor.
     * 
     * \param nodeId Base station node ID
     */
    explicit BaseStationNode(uint32_t nodeId);
    
    /**
     * Destructor.
     */
    ~BaseStationNode();
    
    /**
     * Initialize BS and register callbacks.
     */
    void Initialize();
    
    /**
     * Receive topology snapshot from ground network.
     * Called via callback.
     * 
     * \param topology Global topology information
     */
    void ReceiveTopology(const GlobalTopology& topology);
    
    /**
     * Select suspicious region based on topology.
     * 
     * \return Set of suspicious node IDs
     */
    std::set<uint32_t> SelectSuspiciousRegion();
    
    /**
     * Calculate UAV flight path for suspicious region.
     * 
     * \param suspiciousNodes Target node IDs
     * \return Planned flight path
     */
    UavFlightPath CalculateFlightPath(const std::set<uint32_t>& suspiciousNodes);
    
    /**
     * Send waypoint command to UAV.
     * 
     * \param uavNodeId UAV node ID
     * \param path Flight path
     */
    void SendWaypointCommand(uint32_t uavNodeId, const UavFlightPath& path);
    
    /**
     * Get node ID.
     */
    uint32_t GetNodeId() const;
    
private:
    uint32_t m_nodeId;
    GlobalTopology m_topology;
    std::set<uint32_t> m_suspiciousNodes;
    bool m_topologyReceived;
};

// Global storage for suspicious nodes (used by FinalizeGroundNodeStateFields)
extern std::set<uint32_t> g_suspiciousNodes;

/**
 * Get the set of suspicious nodes detected by base station.
 * 
 * \return Set of suspicious node IDs
 */
const std::set<uint32_t>& GetSuspiciousNodes();

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3

#endif // SCENARIO4_BASE_STATION_NODE_H
