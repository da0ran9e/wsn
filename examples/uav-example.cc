/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * UAV Broadcast Example with CC2420
 *
 * Topology:
 * =========
 * Ground Nodes (NxN grid, configurable spacing)
 * UAV Flight Path (circular, configurable altitude)
 *
 * Simulation:
 * ===========
 * - UAV broadcasts packets periodically
 * - Ground nodes receive and log packets
 * - Statistics collected via MAC layers
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"

#include "../model/uav/uav-mac.h"
#include "../model/uav/ground-node-mac.h"

#include <iomanip>
#include <cmath>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("UavExample");

/**
 * Print final statistics
 */
void
PrintStatistics(Ptr<UavMac> uavMac, NodeContainer groundNodes)
{
    NS_LOG_INFO("\n========================================");
    NS_LOG_INFO("         Simulation Statistics");
    NS_LOG_INFO("========================================");
    NS_LOG_INFO("Total Broadcasts: " << uavMac->GetTotalBroadcasts());
    NS_LOG_INFO("Total Receptions: " << uavMac->GetTotalReceptions());
    NS_LOG_INFO("Total Fragments Sent: " << uavMac->GetFragmentsSent());
    
    if (uavMac->GetTotalBroadcasts() > 0)
    {
        NS_LOG_INFO("Average Receptions per Broadcast: " 
                    << std::fixed << std::setprecision(2)
                    << (double)uavMac->GetTotalReceptions() / uavMac->GetTotalBroadcasts());
    }
    
    NS_LOG_INFO("\nPer-Node Reception Statistics:");
    NS_LOG_INFO("  [NodeID] Packets | Coverage% | Avg RSSI | Min Distance | Frags | Conf | Alert");
    NS_LOG_INFO("  " << std::string(90, '-'));
    
    uint32_t nodesWithReception = 0;
    for (uint32_t i = 0; i < groundNodes.GetN(); i++)
    {
        Ptr<Node> node = groundNodes.Get(i);
        Ptr<GroundNodeMac> mac = node->GetObject<GroundNodeMac>();
        
        if (mac)
        {
            uint32_t count = mac->GetPacketsReceived();
            double percentage = (uavMac->GetTotalBroadcasts() > 0) ?
                               (double)count / uavMac->GetTotalBroadcasts() * 100.0 : 0.0;
            double avgRssi = mac->GetAverageRssi();
            double minDist = mac->GetMinDistance();
            uint32_t fragments = mac->GetFragmentsReceived();
            double confidence = mac->GetConfidence();
            bool alerted = mac->HasAlerted();
            
            if (count > 0)
            {
                nodesWithReception++;
                NS_LOG_INFO("  [" << std::setw(3) << node->GetId() << "]    "
                            << std::setw(4) << count << "/" << std::setw(3) << uavMac->GetTotalBroadcasts()
                            << "   |   " << std::setw(5) << std::fixed << std::setprecision(1) << percentage << "%"
                            << "   |  " << std::setw(6) << std::fixed << std::setprecision(1) << avgRssi << " dBm"
                            << "  |    " << std::setw(5) << std::fixed << std::setprecision(1) << minDist << " m"
                            << "  |  " << std::setw(4) << fragments
                            << "  | " << std::setw(4) << std::fixed << std::setprecision(2) << confidence
                            << " |  " << (alerted ? "YES" : "NO"));
            }
        }
        else
        {
            NS_LOG_WARN("Node " << node->GetId() << " has no GroundNodeMac!");
        }
    }
    NS_LOG_INFO("  " << std::string(90, '-'));
    NS_LOG_INFO("  Nodes with reception: " << nodesWithReception << "/" << groundNodes.GetN());
    NS_LOG_INFO("========================================\n");
}

// ============================================================================
// Main
// ============================================================================

int
main(int argc, char* argv[])
{
    // Simulation parameters
    uint32_t gridSize = 3;           // 3x3 grid
    double gridSpacing = 100.0;      // 100m between nodes
    double uavAltitude = 10.0;       // 10m flight height
    double uavSpeed = 5.0;           // 5 m/s
    double broadcastInterval = 0.25; // 0.25s between broadcasts (increased frequency)
    double simDuration = 60.0;       // 60s simulation
    double txPowerDbm = 0.0;         // 0 dBm TX power
    double rxSensitivity = -95.0;    // -95 dBm RX sensitivity
    uint32_t numFragments = 10;      // Number of fragments to partition file into
    
    // Parse command line
    CommandLine cmd;
    cmd.AddValue("gridSize", "Grid size (N x N)", gridSize);
    cmd.AddValue("gridSpacing", "Distance between grid nodes (m)", gridSpacing);
    cmd.AddValue("uavAltitude", "UAV flight altitude (m)", uavAltitude);
    cmd.AddValue("uavSpeed", "UAV speed (m/s)", uavSpeed);
    cmd.AddValue("interval", "Broadcast interval (s)", broadcastInterval);
    cmd.AddValue("duration", "Simulation duration (s)", simDuration);
    cmd.AddValue("numFragments", "Number of fragments in file", numFragments);
    cmd.Parse(argc, argv);
    
    // Enable logging
    LogComponentEnable("UavExample", LOG_LEVEL_INFO);
    
    NS_LOG_INFO("\n========================================");
    NS_LOG_INFO("    UAV Broadcast Example - CC2420");
    NS_LOG_INFO("========================================");
    NS_LOG_INFO("Grid: " << gridSize << "x" << gridSize << " nodes");
    NS_LOG_INFO("Grid Spacing: " << gridSpacing << " m");
    NS_LOG_INFO("UAV Altitude: " << uavAltitude << " m");
    NS_LOG_INFO("UAV Speed: " << uavSpeed << " m/s");
    NS_LOG_INFO("Broadcast Interval: " << broadcastInterval << " s");
    NS_LOG_INFO("Simulation Duration: " << simDuration << " s");
    NS_LOG_INFO("========================================\n");
    
    // ========================================================================
    // 1. Create Nodes
    // ========================================================================
    
    NS_LOG_INFO("Creating nodes...");
    
    // Ground nodes (grid)
    NodeContainer groundNodes;
    groundNodes.Create(gridSize * gridSize);
    
    // UAV node
    NodeContainer uavNode;
    uavNode.Create(1);
    
    NS_LOG_INFO("  Ground nodes: " << groundNodes.GetN());
    NS_LOG_INFO("  UAV node: 1\n");
    
    // ========================================================================
    // 2. Setup Mobility
    // ========================================================================
    
    NS_LOG_INFO("Setting up mobility models...");
    
    // Ground nodes: static grid positions
    MobilityHelper groundMobility;
    groundMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    groundMobility.Install(groundNodes);
    
    NS_LOG_INFO("  Ground node positions (grid " << gridSize << "x" << gridSize << "):");
    for (uint32_t i = 0; i < gridSize; i++)
    {
        for (uint32_t j = 0; j < gridSize; j++)
        {
            uint32_t nodeIdx = i * gridSize + j;
            Ptr<MobilityModel> mob = groundNodes.Get(nodeIdx)->GetObject<MobilityModel>();
            Vector pos(j * gridSpacing, i * gridSpacing, 0.0);
            mob->SetPosition(pos);
            
            // Attach GroundNodeMac to each node
            Ptr<GroundNodeMac> groundMac = CreateObject<GroundNodeMac>();
            groundNodes.Get(nodeIdx)->AggregateObject(groundMac);
            
            NS_LOG_INFO("    Node " << groundNodes.Get(nodeIdx)->GetId() 
                        << ": (" << pos.x << ", " << pos.y << ", " << pos.z << ")");
        }
    }
    
    // UAV: waypoint mobility (circular path)
    MobilityHelper uavMobility;
    uavMobility.SetMobilityModel("ns3::WaypointMobilityModel");
    uavMobility.Install(uavNode);
    
    Ptr<WaypointMobilityModel> uavWaypoint = 
        uavNode.Get(0)->GetObject<WaypointMobilityModel>();
    
    // Calculate waypoints (expanded circular path based on grid density)
    double centerX = (gridSize - 1) * gridSpacing / 2.0;
    double centerY = (gridSize - 1) * gridSpacing / 2.0;
    // Expand radius to cover more area: base radius + 30% for larger grids
    double radius = gridSpacing * (1.0 + 0.3 * (gridSize / 3.0));
    
    // Calculate time for each segment (distance / speed)
    double segmentDistance = 2.0 * M_PI * radius / 4.0; // Quarter circle approximation
    double segmentTime = segmentDistance / uavSpeed;
    
    NS_LOG_INFO("\n  UAV waypoint path (altitude: " << uavAltitude << " m):");
    
    // Initial position (North)
    Vector wp0(centerX, centerY - radius, uavAltitude);
    uavWaypoint->AddWaypoint(Waypoint(Seconds(0), wp0));
    NS_LOG_INFO("    t=0s: (" << wp0.x << ", " << wp0.y << ", " << wp0.z << ")");
    
    // East
    Vector wp1(centerX + radius, centerY, uavAltitude);
    uavWaypoint->AddWaypoint(Waypoint(Seconds(segmentTime), wp1));
    NS_LOG_INFO("    t=" << segmentTime << "s: (" << wp1.x << ", " << wp1.y << ", " << wp1.z << ")");
    
    // South
    Vector wp2(centerX, centerY + radius, uavAltitude);
    uavWaypoint->AddWaypoint(Waypoint(Seconds(2 * segmentTime), wp2));
    NS_LOG_INFO("    t=" << 2*segmentTime << "s: (" << wp2.x << ", " << wp2.y << ", " << wp2.z << ")");
    
    // West
    Vector wp3(centerX - radius, centerY, uavAltitude);
    uavWaypoint->AddWaypoint(Waypoint(Seconds(3 * segmentTime), wp3));
    NS_LOG_INFO("    t=" << 3*segmentTime << "s: (" << wp3.x << ", " << wp3.y << ", " << wp3.z << ")");
    
    // End at West (single pass, no loop back)
    double pathTime = 3 * segmentTime;
    NS_LOG_INFO("\n  Path duration: " << std::fixed << std::setprecision(1) << pathTime << " s");
    
    // ========================================================================
    // 3. Setup UAV MAC and Start Broadcasting
    // ========================================================================
    
    NS_LOG_INFO("Setting up UAV MAC layer...");
    
    // Create and attach UAV MAC
    Ptr<UavMac> uavMac = CreateObject<UavMac>();
    uavMac->SetTxPower(txPowerDbm);
    uavMac->SetRxSensitivity(rxSensitivity);
    uavMac->Initialize(uavNode.Get(0), groundNodes);
    uavNode.Get(0)->AggregateObject(uavMac);
    
    // Generate fragment set (partition file into n fragments with total confidence = 1.0)
    uavMac->GenerateFragmentSet(numFragments, 1.0);
    
    NS_LOG_INFO("  TX Power: " << txPowerDbm << " dBm");
    NS_LOG_INFO("  RX Sensitivity: " << rxSensitivity << " dBm");
    NS_LOG_INFO("  Broadcast Interval: " << broadcastInterval << " s");
    NS_LOG_INFO("  Number of Fragments: " << numFragments);
    NS_LOG_INFO("  Expected broadcasts: ~" << (uint32_t)(simDuration / broadcastInterval) << "\n");
    
    // Start broadcasting
    uavMac->StartBroadcast(Seconds(broadcastInterval), Seconds(simDuration));
    
    // ========================================================================
    // 4. Run Simulation
    // ========================================================================
    
    NS_LOG_INFO("Starting simulation...\n");
    
    Simulator::Stop(Seconds(simDuration));
    Simulator::Run();
    
    // Print statistics
    PrintStatistics(uavMac, groundNodes);
    
    Simulator::Destroy();
    
    return 0;
}
            