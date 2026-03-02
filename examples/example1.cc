/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * CC2420 grid scenario example.
 *
 * This example is intentionally lightweight: it only defines a simulation scenario
 * (topology + traffic), and leaves radio behavior details to PHY/MAC modules.
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/cc2420-helper.h"
#include "ns3/cc2420-net-device.h"
#include "ns3/cc2420-mac.h"

#include <map>

using namespace ns3;
using namespace ns3::wsn;

NS_LOG_COMPONENT_DEFINE("Cc2420GridScenarioExample");

namespace
{
uint32_t g_gridSize = 10;
double g_spacing = 5.0;             // tuned for minimum CC2420 TX power
uint32_t g_packetSize = 64;
double g_txPowerDbm = 0.0;          // maximum CC2420 TX power
double g_rxSensitivityDbm = -95.0;

double g_simTimeSeconds = 8.0;
double g_firstTxTimeSeconds = 1.0;

uint32_t g_totalTx = 0;
uint32_t g_totalRx = 0;
std::map<uint32_t, uint32_t> g_rxPerNode;
} // namespace

static void
OnMcpsDataIndication(uint32_t nodeId, Ptr<Packet> packet, Mac16Address src, double rssiDbm)
{
    g_totalRx++;
    g_rxPerNode[nodeId]++;

    NS_LOG_INFO("Node " << nodeId << " RX from " << src << " size=" << packet->GetSize()
                         << " RSSI=" << rssiDbm << " dBm");
}

static void
InstallRxCallback(Ptr<NetDevice> dev, uint32_t nodeId)
{
    Ptr<Cc2420NetDevice> cc2420Dev = DynamicCast<Cc2420NetDevice>(dev);
    if (cc2420Dev)
    {
        Ptr<Cc2420Mac> mac = cc2420Dev->GetMac();
        if (mac)
        {
            mac->SetMcpsDataIndicationCallback(MakeBoundCallback(&OnMcpsDataIndication, nodeId));
        }
    }
}

static void
SendFrom(NodeContainer nodes, uint32_t srcNodeId, Mac16Address dst)
{
    Ptr<Node> srcNode = nodes.Get(srcNodeId);
    Ptr<Cc2420NetDevice> dev = DynamicCast<Cc2420NetDevice>(srcNode->GetDevice(0));
    if (!dev)
    {
        NS_LOG_WARN("Node " << srcNodeId << " has no Cc2420NetDevice");
        return;
    }

    Ptr<Packet> p = Create<Packet>(g_packetSize);
    if (dev->Send(p, dst, 0))
    {
        g_totalTx++;
    }
}

int
main(int argc, char* argv[])
{
    CommandLine cmd;
    cmd.AddValue("gridSize", "Grid size N (N x N)", g_gridSize);
    cmd.AddValue("spacing", "Grid spacing in meters", g_spacing);
    cmd.AddValue("txPower", "CC2420 TX power in dBm", g_txPowerDbm);
    cmd.AddValue("packetSize", "Packet size in bytes", g_packetSize);
    cmd.AddValue("simTime", "Simulation time in seconds", g_simTimeSeconds);
    cmd.Parse(argc, argv);

    LogComponentEnable("Cc2420GridScenarioExample", LOG_LEVEL_INFO);

    const uint32_t numNodes = g_gridSize * g_gridSize;
    NodeContainer nodes;
    nodes.Create(numNodes);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(0.0),
                                  "MinY", DoubleValue(0.0),
                                  "DeltaX", DoubleValue(g_spacing),
                                  "DeltaY", DoubleValue(g_spacing),
                                  "GridWidth", UintegerValue(g_gridSize),
                                  "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // Use Cc2420Helper to configure PHY/MAC stack
    Cc2420Helper cc2420;
    Ptr<SingleModelSpectrumChannel> channel = cc2420.CreateChannel();
    cc2420.SetChannel(channel);
    cc2420.SetPhyAttribute("TxPower", DoubleValue(g_txPowerDbm));
    cc2420.SetPhyAttribute("RxSensitivity", DoubleValue(g_rxSensitivityDbm));

    NetDeviceContainer devices = cc2420.Install(nodes);

    // Attach RX callbacks to all devices
    for (uint32_t i = 0; i < devices.GetN(); ++i)
    {
        InstallRxCallback(devices.Get(i), i);
    }

    // Scenario traffic only (no RF computations in example):
    // 1) center broadcast
    // 2) four-corner broadcasts
    const uint32_t center = (g_gridSize / 2) * g_gridSize + (g_gridSize / 2);
    Simulator::Schedule(Seconds(g_firstTxTimeSeconds), &SendFrom, nodes, center, Mac16Address("FF:FF"));

    const uint32_t topLeft = 0;
    const uint32_t topRight = g_gridSize - 1;
    const uint32_t bottomLeft = g_gridSize * (g_gridSize - 1);
    const uint32_t bottomRight = g_gridSize * g_gridSize - 1;

    Simulator::Schedule(Seconds(g_firstTxTimeSeconds + 1.0), &SendFrom, nodes, topLeft, Mac16Address("FF:FF"));
    Simulator::Schedule(Seconds(g_firstTxTimeSeconds + 1.2), &SendFrom, nodes, topRight, Mac16Address("FF:FF"));
    Simulator::Schedule(Seconds(g_firstTxTimeSeconds + 1.4), &SendFrom, nodes, bottomLeft, Mac16Address("FF:FF"));
    Simulator::Schedule(Seconds(g_firstTxTimeSeconds + 1.6), &SendFrom, nodes, bottomRight, Mac16Address("FF:FF"));

    Simulator::Stop(Seconds(g_simTimeSeconds));
    Simulator::Run();

    NS_LOG_INFO("=== Scenario Summary ===");
    NS_LOG_INFO("Nodes: " << numNodes << " (" << g_gridSize << "x" << g_gridSize << ")");
    NS_LOG_INFO("Spacing: " << g_spacing << " m, TX power: " << g_txPowerDbm << " dBm");
    NS_LOG_INFO("Total TX requests: " << g_totalTx);
    NS_LOG_INFO("Total RX indications: " << g_totalRx);

    Simulator::Destroy();
    return 0;
}
