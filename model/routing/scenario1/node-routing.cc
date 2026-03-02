/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Scenario1 Node Routing: Network layer routing and scheduling logic
 */

#include "node-routing.h"

#include "ns3/cc2420-net-device.h"
#include "ns3/cc2420-mac.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"

#include <map>

namespace ns3
{
namespace wsn
{
namespace scenario1
{

NS_LOG_COMPONENT_DEFINE("Scenario1NodeRouting");

namespace
{
uint32_t g_totalTx = 0;
uint32_t g_totalRx = 0;
std::map<uint32_t, uint32_t> g_rxPerNode;
} // anonymous namespace

/**
 * @brief Callback for received packets at network layer
 */
static void
OnNodeReceivePacket(uint32_t nodeId, Ptr<Packet> packet, Mac16Address src, double rssiDbm)
{
    g_totalRx++;
    g_rxPerNode[nodeId]++;

    NS_LOG_INFO("t=" << Simulator::Now().GetSeconds() << "s Node " << nodeId 
                     << " RX from " << src << " size=" << packet->GetSize()
                     << " RSSI=" << rssiDbm << " dBm");
}

/**
 * @brief Attach RX callback to a network device
 */
static void
AttachRxCallback(Ptr<NetDevice> dev, uint32_t nodeId)
{
    Ptr<Cc2420NetDevice> cc2420Dev = DynamicCast<Cc2420NetDevice>(dev);
    if (cc2420Dev)
    {
        Ptr<Cc2420Mac> mac = cc2420Dev->GetMac();
        if (mac)
        {
            mac->SetMcpsDataIndicationCallback(MakeBoundCallback(&OnNodeReceivePacket, nodeId));
        }
    }
}

/**
 * @brief Internal function to send packet from a node
 */
static void
SendPacketFrom(NodeContainer nodes, uint32_t srcNodeId, Mac16Address dst, uint32_t packetSize)
{
    Ptr<Node> srcNode = nodes.Get(srcNodeId);
    Ptr<Cc2420NetDevice> dev = DynamicCast<Cc2420NetDevice>(srcNode->GetDevice(0));
    if (!dev)
    {
        NS_LOG_WARN("Node " << srcNodeId << " has no Cc2420NetDevice");
        return;
    }

    Ptr<Packet> p = Create<Packet>(packetSize);
    if (dev->Send(p, dst, 0))
    {
        g_totalTx++;
        NS_LOG_DEBUG("Node " << srcNodeId << " TX to " << dst << " size=" << packetSize);
    }
}

void
InitializeNodeRouting(NetDeviceContainer devices, uint32_t packetSize)
{
    NS_LOG_FUNCTION_NOARGS();

    // Reset statistics
    ResetRoutingStatistics();

    // Attach RX callbacks to all devices
    for (uint32_t i = 0; i < devices.GetN(); ++i)
    {
        AttachRxCallback(devices.Get(i), i);
    }

    NS_LOG_INFO("Node routing initialized for " << devices.GetN() << " devices");
}

void
ScheduleTransmission(NodeContainer nodes,
                    uint32_t srcNodeId,
                    Mac16Address dst,
                    uint32_t packetSize,
                    Time time)
{
    NS_LOG_FUNCTION(srcNodeId << dst << packetSize << time);

    Simulator::Schedule(time, &SendPacketFrom, nodes, srcNodeId, dst, packetSize);
}

void
ScheduleScenario1TrafficPattern(NodeContainer nodes,
                               uint32_t gridSize,
                               uint32_t packetSize,
                               double firstTxTime)
{
    NS_LOG_FUNCTION(gridSize << packetSize << firstTxTime);

    // Calculate node positions in grid
    const uint32_t center = (gridSize / 2) * gridSize + (gridSize / 2);
    const uint32_t topLeft = 0;
    const uint32_t topRight = gridSize - 1;
    const uint32_t bottomLeft = gridSize * (gridSize - 1);
    const uint32_t bottomRight = gridSize * gridSize - 1;

    Mac16Address broadcast = Mac16Address("FF:FF");

    // Schedule broadcast from center node
    ScheduleTransmission(nodes, center, broadcast, packetSize, Seconds(firstTxTime));

    // Schedule broadcasts from four corner nodes
    ScheduleTransmission(nodes, topLeft, broadcast, packetSize, Seconds(firstTxTime + 1.0));
    ScheduleTransmission(nodes, topRight, broadcast, packetSize, Seconds(firstTxTime + 1.2));
    ScheduleTransmission(nodes, bottomLeft, broadcast, packetSize, Seconds(firstTxTime + 1.4));
    ScheduleTransmission(nodes, bottomRight, broadcast, packetSize, Seconds(firstTxTime + 1.6));

    NS_LOG_INFO("Scenario1 traffic pattern scheduled: center + 4 corners");
}

uint32_t
GetTotalTransmissions()
{
    return g_totalTx;
}

uint32_t
GetTotalReceptions()
{
    return g_totalRx;
}

const std::map<uint32_t, uint32_t>&
GetPerNodeReceptions()
{
    return g_rxPerNode;
}

void
ResetRoutingStatistics()
{
    g_totalTx = 0;
    g_totalRx = 0;
    g_rxPerNode.clear();
}

} // namespace scenario1
} // namespace wsn
} // namespace ns3
