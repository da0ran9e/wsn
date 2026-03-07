/*
 * Scenario 4 - Ground Node Routing
 */

#ifndef SCENARIO4_GROUND_NODE_ROUTING_H
#define SCENARIO4_GROUND_NODE_ROUTING_H

#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/packet.h"
#include "ns3/net-device.h"
#include "../fragment.h"
#include "../base-station-node/base-station-node.h"
#include <map>
#include <set>

namespace ns3 {
namespace wsn {
namespace scenario4 {
namespace routing {

/**
 * Ground node network state.
 */
struct GroundNetworkState
{
    uint32_t nodeId;
    FragmentCollection fragments;
    std::set<uint32_t> neighbors;
    std::map<uint32_t, double> neighborRssi;
    int32_t cellId;
    double confidence;
    uint32_t packetCount;
};

// Global storage for ground node states
extern std::map<uint32_t, GroundNetworkState> g_groundNetworkPerNode;

/**
 * Initialize ground node routing for all nodes.
 * 
 * \param nodes Ground node container
 * \param numFragments Number of fragments to generate
 */
void InitializeGroundNodeRouting(NodeContainer nodes, uint32_t numFragments);

/**
 * Handle packet reception on ground node.
 * 
 * \param nodeId Receiving node ID
 * \param packet Received packet
 * \param rssiDbm Signal strength
 */
void OnGroundNodeReceivePacket(uint32_t nodeId, Ptr<const Packet> packet, double rssiDbm);

/**
 * Build topology snapshot from ground network.
 * 
 * \return Global topology
 */
GlobalTopology BuildTopologySnapshot();

/**
 * Send topology snapshot to base station.
 */
void SendTopologyToBS();

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3

#endif // SCENARIO4_GROUND_NODE_ROUTING_H
