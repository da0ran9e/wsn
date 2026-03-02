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
#include "ns3/lr-wpan-module.h"
#include "ns3/spectrum-module.h"

#include "../model/uav/uav-mac.h"
#include "../model/uav/ground-node-mac.h"
#include "../model/uav/cell-forming.h"
#include "../model/uav/cell-forming-mac.h"
#include "../model/uav/uav-mac-cc2420.h"
#include "../model/uav/wsn-radio-mac.h"

#include <iomanip>
#include <cmath>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("UavExample");

/**
 * Print Phase 0 cell formation statistics
 */
void
PrintCellFormingStatistics(std::map<uint32_t, Ptr<CellForming>>& nodeCellFormingMap)
{
    NS_LOG_INFO("\n========================================");
    NS_LOG_INFO("    Cell Forming Statistics");
    NS_LOG_INFO("========================================");
    
    uint32_t clCount = 0;
    uint32_t routingReadyCount = 0;
    std::map<int32_t, std::vector<uint32_t>> cellMembers;
    std::map<int32_t, uint32_t> cellLeaders;
    
    NS_LOG_INFO("\n  [NodeID] CellID | Color | CLId | Neighbors | Status");
    NS_LOG_INFO("  " << std::string(55, '-'));
    
    for (auto& entry : nodeCellFormingMap)
    {
        uint32_t nodeId = entry.first;
        Ptr<CellForming> cf = entry.second;
        
        int32_t cellId = cf->GetCellId();
        int32_t color = cf->GetColor();
        uint32_t clId = cf->GetCellLeaderId();
        uint32_t neighbors = cf->GetNeighbors().size();
        std::string status;
        
        if (cf->IsCellLeader())
        {
            clCount++;
            status = "CL";
            cellLeaders[cellId] = nodeId;
            if (cf->IsCellFormationComplete()) {
                routingReadyCount++;
                status = "CL-Ready";
            }
        }
        else if (cf->IsCellFormationComplete())
        {
            status = "Member";
        }
        else
        {
            status = "Discovering";
        }
        
        cellMembers[cellId].push_back(nodeId);
        
        NS_LOG_INFO("  [" << std::setw(3) << nodeId << "]     " 
                    << std::setw(5) << cellId << " | " << std::setw(5) << color 
                    << " | " << std::setw(4) << clId << " | " << std::setw(9) << neighbors
                    << " | " << status);
    }
    
    NS_LOG_INFO("  " << std::string(55, '-'));
    NS_LOG_INFO("\n  Cell Summary:");
    for (auto& entry : cellMembers)
    {
        int32_t cellId = entry.first;
        std::vector<uint32_t>& members = entry.second;
        uint32_t clId = (cellLeaders.count(cellId) > 0) ? cellLeaders[cellId] : 0xFFFFFFFF;
        NS_LOG_INFO("    Cell " << cellId << ": " << members.size() << " members, CL=" << clId);
    }
    
    NS_LOG_INFO("\n  Formation Status:");
    NS_LOG_INFO("    CLs Elected: " << clCount << "/" << nodeCellFormingMap.size());
    NS_LOG_INFO("    Routing Ready: " << routingReadyCount << "/" << clCount << " (CLs only)");
    NS_LOG_INFO("========================================\n");
}

/**
 * Print final statistics
 */
void
PrintStatistics(Ptr<UavMacCc2420> uavMac, NodeContainer groundNodes)
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
    LogComponentEnable("CellForming", LOG_LEVEL_INFO);
    // LogComponentEnable("LrWpanMac", LOG_LEVEL_INFO);
    // LogComponentEnable("LrWpanPhy", LOG_LEVEL_INFO);
    
    NS_LOG_INFO("\n========================================");
    NS_LOG_INFO("  UAV Broadcast + Phase 0 CellForming");
    NS_LOG_INFO("  WITH LR-WPAN (CC2420) Radio");
    NS_LOG_INFO("========================================");
    NS_LOG_INFO("Grid: " << gridSize << "x" << gridSize << " nodes");
    NS_LOG_INFO("Grid Spacing: " << gridSpacing << " m");
    NS_LOG_INFO("UAV Altitude: " << uavAltitude << " m");
    NS_LOG_INFO("UAV Speed: " << uavSpeed << " m/s");
    NS_LOG_INFO("Broadcast Interval: " << broadcastInterval << " s");
    NS_LOG_INFO("Simulation Duration: " << simDuration << " s");
    NS_LOG_INFO("Radio: LR-WPAN (IEEE 802.15.4, CC2420 model)");
    NS_LOG_INFO("========================================\n");
    
    // ========================================================================
    // 0. Setup LR-WPAN Channel and Devices
    // ========================================================================
    
    NS_LOG_INFO("Setting up LR-WPAN channel and devices...");
    
    // Create channel
    Ptr<SpectrumChannel> channel = CreateObject<MultiModelSpectrumChannel>();
    
    // Add propagation loss model
    Ptr<LogDistancePropagationLossModel> lossModel = 
        CreateObject<LogDistancePropagationLossModel>();
    lossModel->SetPathLossExponent(3.0); // Outdoor path loss exponent
    lossModel->SetReference(1.0, -75.0); // Reference distance 1m gives -75dBm
    channel->AddPropagationLossModel(lossModel);
    
    // Add propagation delay model
    Ptr<ConstantSpeedPropagationDelayModel> delayModel = 
        CreateObject<ConstantSpeedPropagationDelayModel>();
    delayModel->SetSpeed(3.0e8); // Speed of light
    channel->SetPropagationDelayModel(delayModel);
    
    NS_LOG_INFO("  Channel: MultiModelSpectrumChannel");
    NS_LOG_INFO("  Path Loss Model: LogDistance (exponent=3.0)");
    NS_LOG_INFO("  Propagation Delay: ConstantSpeed");
    
    NS_LOG_INFO("========================================\n");

    NS_LOG_INFO("Cell Formation: ENABLED (Phase 0)");
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
    
    // UAV: waypoint mobility
    MobilityHelper uavMobility;
    uavMobility.SetMobilityModel("ns3::WaypointMobilityModel");
    uavMobility.Install(uavNode);
    
    NS_LOG_INFO("  Ground node positions (grid " << gridSize << "x" << gridSize << "):");
    
    // ========================================================================
    // 2.5. Install LR-WPAN Devices
    // ========================================================================
    
    NS_LOG_INFO("\nInstalling LR-WPAN devices...");
    
    // Create LR-WPAN helper
    LrWpanHelper lrWpanHelper;
    
    // Create all nodes together for device installation
    NodeContainer allNodes;
    allNodes.Add(groundNodes);
    allNodes.Add(uavNode);
    
    // Install on ground nodes and UAV
    NetDeviceContainer allDevices = lrWpanHelper.Install(allNodes);
    
    // Extract device containers
    NetDeviceContainer groundDevices;
    NetDeviceContainer uavDevices;
    for (uint32_t i = 0; i < groundNodes.GetN(); i++)
    {
        groundDevices.Add(allDevices.Get(i));
    }
    uavDevices.Add(allDevices.Get(groundNodes.GetN()));
    
    NS_LOG_INFO("  Ground node devices: " << groundDevices.GetN());
    NS_LOG_INFO("  UAV device: " << uavDevices.GetN() << "\n");
    
    // Create a map to store CellFormingMac for each ground node (extends CC2420 MAC)
    std::map<uint32_t, Ptr<CellFormingMac>> nodeFormingMacMap;
    
    // Map to track CellForming modules for later statistics
    std::map<uint32_t, Ptr<CellForming>> nodeCellFormingMap;

    
    for (uint32_t i = 0; i < gridSize; i++)
    {
        for (uint32_t j = 0; j < gridSize; j++)
        {
            uint32_t nodeIdx = i * gridSize + j;
            Ptr<Node> node = groundNodes.Get(nodeIdx);
            Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
            Vector pos(j * gridSpacing, i * gridSpacing, 0.0);
            mob->SetPosition(pos);
            
            // Attach GroundNodeMac to each node
            Ptr<GroundNodeMac> groundMac = CreateObject<GroundNodeMac>();
            node->AggregateObject(groundMac);
            
            // Create CellFormingMac that extends CC2420Mac
            Ptr<CellFormingMac> formingMac = CreateObject<CellFormingMac>();
            
            // Set up MAC configuration
            MacConfig config;
            config.panId = 0x1234;
            char addrStr[6];
            std::snprintf(addrStr, sizeof(addrStr), "%02x:%02x", 
                         static_cast<unsigned>((nodeIdx >> 8) & 0xff),
                         static_cast<unsigned>(nodeIdx & 0xff));
            config.shortAddress = Mac16Address(addrStr);
            config.macMinBE = 3;
            config.macMaxBE = 5;
            config.macMaxCSMABackoffs = 4;
            config.macMaxFrameRetries = 3;
            config.txAckRequest = false;
            config.rxOnWhenIdle = true;
            formingMac->SetMacConfig(config);
            formingMac->Start();
            
            node->AggregateObject(formingMac);
            nodeFormingMacMap[node->GetId()] = formingMac;

            
            // Initialize Phase 0: CellForming module
            Ptr<CellForming> cellForming = CreateObject<CellForming>();
            cellForming->SetNodeParams(node->GetId(), pos, 150.0, 1000); // nodeId, pos, cellRadius=150m, gridOffset=1000
            cellForming->SetTimingParams(1.0, 0.5, 2.0); // helloInterval, clElectionDelay, clCalculationTime
            node->AggregateObject(cellForming);
            nodeCellFormingMap[node->GetId()] = cellForming;
            
            // Link CellForming to CellFormingMac
            formingMac->SetCellFormingModule(cellForming);
            
            NS_LOG_INFO("    Node " << node->GetId() 
                        << ": (" << pos.x << ", " << pos.y << ", " << pos.z << ")");
        }
    }
    
    // Set up packet delivery for Phase 0 (now directly via CellFormingMac extending CC2420)
    for (auto& [nodeId, formingMac] : nodeFormingMacMap)
    {
        Ptr<CellForming> cellForming = nodeCellFormingMap[nodeId];
        
        // HELLO packet transmission callback (via CellFormingMac->CC2420)
        cellForming->SetHelloCallback([formingMac](const HelloPacket& hello) {
            formingMac->SendHelloPacket(hello.sourceId, 
                                       Vector(hello.position.x, hello.position.y, hello.position.z));
        });
        
        // CL announcement transmission callback (via CellFormingMac->CC2420)
        cellForming->SetCLAnnouncementCallback([formingMac](const CLAnnouncementPacket& announcement) {
            formingMac->SendCLAnnouncement(announcement.sourceId, announcement.cellId, 
                                          announcement.cellLeaderFitness);
        });
        
        // Member feedback transmission callback (via CellFormingMac->CC2420)
        cellForming->SetMemberFeedbackCallback([formingMac](const CLMemberFeedbackPacket& feedback) {
            formingMac->SendMemberFeedback(feedback.memberId, feedback.cellLeaderId, 
                                          feedback.cellId, feedback.feedbackValue);
        });
    }
    
    // Initialize all CellForming modules (now that callbacks are set)
    for (auto& [nodeId, cellForming] : nodeCellFormingMap)
    {
        cellForming->Initialize();
    }
    
    // ========================================================================
    // 3. Setup UAV Node
    // ========================================================================

    Ptr<WaypointMobilityModel> uavWaypoint = 
        uavNode.Get(0)->GetObject<WaypointMobilityModel>();
    
    // Create UavMacCc2420 that extends CC2420Mac for UAV Phase 1 broadcasting
    Ptr<UavMacCc2420> uavMacCc2420 = CreateObject<UavMacCc2420>();
    
    // Set up MAC configuration for UAV
    MacConfig uavConfig;
    uavConfig.panId = 0x1234;
    uavConfig.shortAddress = Mac16Address("ff:ff"); // UAV uses broadcast address
    uavConfig.macMinBE = 3;
    uavConfig.macMaxBE = 5;
    uavConfig.macMaxCSMABackoffs = 4;
    uavConfig.macMaxFrameRetries = 3;
    uavConfig.txAckRequest = false;
    uavConfig.rxOnWhenIdle = true;
    uavMacCc2420->SetMacConfig(uavConfig);
    uavMacCc2420->Start();
    
    // Initialize with ground nodes
    uavMacCc2420->Initialize(uavNode.Get(0), groundNodes);
    uavNode.Get(0)->AggregateObject(uavMacCc2420);
    
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
    // 4. Setup UAV MAC and Start Broadcasting (via CC2420)
    // ========================================================================
    
    NS_LOG_INFO("Setting up UAV MAC layer (UavMacCc2420) with CC2420 transmission...");
    
    // Configure TX power and sensitivity
    uavMacCc2420->SetTxPower(txPowerDbm);
    uavMacCc2420->SetRxSensitivity(rxSensitivity);
    
    // Generate fragment set (partition file into n fragments with total confidence = 1.0)
    uavMacCc2420->GenerateFragmentSet(numFragments, 1.0);
    
    NS_LOG_INFO("  TX Power: " << txPowerDbm << " dBm");
    NS_LOG_INFO("  RX Sensitivity: " << rxSensitivity << " dBm");
    NS_LOG_INFO("  Broadcast Interval: " << broadcastInterval << " s");
    NS_LOG_INFO("  Number of Fragments: " << numFragments);
    NS_LOG_INFO("  Expected broadcasts: ~" << (uint32_t)(simDuration / broadcastInterval) << "\n");
    
    // ========================================================================
    // 4. Initialize Cell Formation (Phase 0) before UAV broadcasting
    // ========================================================================
    
    NS_LOG_INFO("\nInitializing Phase 0 (Cell Forming)...");
    NS_LOG_INFO("  Cell Radius: 150 m");
    NS_LOG_INFO("  HELLO Interval: 1.0 s");
    NS_LOG_INFO("  Cell Leaders will be elected automatically\n");
    
    // Schedule callback to show when cell formation completes
    Simulator::Schedule(Seconds(5.0), [&nodeCellFormingMap]() {
        NS_LOG_INFO("\n[t=5.0s] Cell Formation Status Check:");
        uint32_t readyCount = 0;
        for (auto& entry : nodeCellFormingMap)
        {
            if (entry.second->IsCellFormationComplete())
            {
                readyCount++;
            }
        }
        NS_LOG_INFO("  Nodes with cell assignment: " << readyCount << "/" 
                    << nodeCellFormingMap.size());
    });
    
    // Start broadcasting (after Phase 0 has time to initialize)
    NS_LOG_INFO("\nScheduling UAV broadcasting to start at t=6s (after Phase 0 initialization)...");
    Simulator::Schedule(Seconds(6.0), &UavMacCc2420::StartBroadcast, uavMacCc2420, 
                       Time(Seconds(broadcastInterval)), Seconds(simDuration));
    
    // ========================================================================
    // 5. Run Simulation
    // ========================================================================
    
    NS_LOG_INFO("\nStarting simulation...\n");
    
    Simulator::Stop(Seconds(simDuration));
    Simulator::Run();
    
    // Print Phase 0 statistics
    PrintCellFormingStatistics(nodeCellFormingMap);
    
    // Print UAV statistics
    PrintStatistics(uavMacCc2420, groundNodes);
    
    Simulator::Destroy();
    
    return 0;
}
            