/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Cell Forming Example (Phase 0) + UAV Scenario
 *
 * Demonstrates:
 * - Phase 0: Cell forming, neighbor discovery, CL election
 * - Phase 1: UAV fragment broadcast with cell-aware routing
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"

#include "../model/uav/uav-mac.h"
#include "../model/uav/ground-node-mac.h"
#include "../model/uav/cell-forming.h"
#include "../model/uav/cell-forming-packet.h"

#include <iomanip>
#include <cmath>
#include <map>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CellFormingExample");

/**
 * @brief Print cell forming statistics
 */
void
PrintCellFormingStats(std::map<uint32_t, Ptr<CellForming>>& nodeCellFormingMap)
{
    NS_LOG_INFO("\n========================================");
    NS_LOG_INFO("       Cell Forming Statistics");
    NS_LOG_INFO("========================================");
    
    // Collect cell info from all nodes
    std::map<int32_t, std::vector<uint32_t>> cellMembers;
    std::map<int32_t, uint32_t> cellLeaders;
    uint32_t completedNodes = 0;
    uint32_t clElectedCount = 0;
    
    for (const auto& [nodeId, cellForming] : nodeCellFormingMap)
    {
        int32_t cellId = cellForming->GetCellId();
        uint32_t clId = cellForming->GetCellLeaderId();
        bool isComplete = cellForming->IsCellFormationComplete();
        bool isCL = cellForming->IsCellLeader();
        
        cellMembers[cellId].push_back(nodeId);
        if (clId > 0)
        {
            cellLeaders[cellId] = clId;
        }
        
        if (isComplete)
        {
            completedNodes++;
        }
        
        if (isCL)
        {
            clElectedCount++;
        }
        
        NS_LOG_INFO("Node " << nodeId << ": cellId=" << cellId 
                   << ", color=" << cellForming->GetColor()
                   << ", clId=" << clId
                   << ", neighbors=" << cellForming->GetNeighbors().size()
                   << ", isCL=" << (isCL ? "YES" : "NO")
                   << ", complete=" << (isComplete ? "YES" : "NO"));
    }
    
    NS_LOG_INFO("\nCell Summary:");
    for (const auto& [cellId, members] : cellMembers)
    {
        uint32_t clId = cellLeaders.count(cellId) ? cellLeaders[cellId] : 0;
        NS_LOG_INFO("Cell " << cellId << ": " << members.size() << " members, CL=" << clId);
    }
    
    NS_LOG_INFO("\nFormation Status:");
    NS_LOG_INFO("  CLs Elected: " << clElectedCount << "/" << nodeCellFormingMap.size());
    NS_LOG_INFO("  Routing Ready: " << completedNodes << "/" << nodeCellFormingMap.size() 
               << " (CLs only)");
    NS_LOG_INFO("=========================================\n");
}

/**
 * @brief Callback for HELLO packet (would be transmitted in real simulation)
 */
void
HandleHelloCallback(uint32_t senderId, const HelloPacket& hello)
{
    NS_LOG_DEBUG("HELLO callback: node " << senderId << " would broadcast HELLO");
}

/**
 * @brief Callback for CL announcement
 */
void
HandleCLAnnouncementCallback(uint32_t senderId, const CLAnnouncementPacket& announcement)
{
    NS_LOG_INFO("CL announcement from node " << senderId 
               << " for cell " << announcement.cellId 
               << " (fitness=" << announcement.fitnessScore << ")");
}

/**
 * @brief Callback for member feedback
 */
void
HandleMemberFeedbackCallback(uint32_t senderId, const CLMemberFeedbackPacket& feedback)
{
    NS_LOG_DEBUG("Member feedback from node " << senderId 
                << " to CL (neighbors=" << feedback.neighbors.size() << ")");
}

int
main(int argc, char* argv[])
{
    // Simulation parameters
    uint32_t gridSize = 3;           // 3x3 grid
    double gridSpacing = 100.0;      // 100m between nodes
    double cellRadius = 150.0;       // Cell radius for neighbor discovery
    double simDuration = 30.0;       // 30s simulation (enough for cell forming)
    
    // Cell forming parameters
    double helloInterval = 1.0;              // HELLO broadcast every 1s
    double clElectionDelayInterval = 0.5;   // Base election delay
    double clCalculationTime = 2.0;          // CL calculation delay
    
    // Parse command line
    CommandLine cmd;
    cmd.AddValue("gridSize", "Grid size (N x N)", gridSize);
    cmd.AddValue("gridSpacing", "Distance between grid nodes (m)", gridSpacing);
    cmd.AddValue("cellRadius", "Cell radius for neighbor discovery (m)", cellRadius);
    cmd.AddValue("duration", "Simulation duration (s)", simDuration);
    cmd.AddValue("helloInterval", "HELLO broadcast interval (s)", helloInterval);
    cmd.Parse(argc, argv);
    
    // Enable logging
    LogComponentEnable("CellFormingExample", LOG_LEVEL_INFO);
    LogComponentEnable("CellForming", LOG_LEVEL_INFO);
    
    NS_LOG_INFO("\n========================================");
    NS_LOG_INFO("    Phase 0: Cell Forming Example");
    NS_LOG_INFO("========================================");
    NS_LOG_INFO("Grid: " << gridSize << "x" << gridSize << " nodes");
    NS_LOG_INFO("Grid Spacing: " << gridSpacing << " m");
    NS_LOG_INFO("Cell Radius: " << cellRadius << " m");
    NS_LOG_INFO("HELLO Interval: " << helloInterval << " s");
    NS_LOG_INFO("Simulation Duration: " << simDuration << " s");
    NS_LOG_INFO("========================================\n");
    
    // ========================================================================
    // 1. Create Ground Nodes
    // ========================================================================
    
    NS_LOG_INFO("Creating ground nodes...");
    NodeContainer groundNodes;
    groundNodes.Create(gridSize * gridSize);
    
    // ========================================================================
    // 2. Set positions (grid layout)
    // ========================================================================
    
    NS_LOG_INFO("Setting node positions (grid)...");
    Ptr<ListPositionAllocator> posAlloc = CreateObject<ListPositionAllocator>();
    
    for (uint32_t i = 0; i < gridSize; i++)
    {
        for (uint32_t j = 0; j < gridSize; j++)
        {
            double x = i * gridSpacing;
            double y = j * gridSpacing;
            posAlloc->Add(Vector(x, y, 0.0));
            NS_LOG_DEBUG("Node " << (i * gridSize + j) << " at (" << x << ", " << y << ")");
        }
    }
    
    MobilityHelper mobilityHelper;
    mobilityHelper.SetPositionAllocator(posAlloc);
    mobilityHelper.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobilityHelper.Install(groundNodes);
    
    // ========================================================================
    // 3. Create and Initialize CellForming modules
    // ========================================================================
    
    NS_LOG_INFO("Creating CellForming modules...");
    std::vector<Ptr<CellForming>> cellFormingModules;
    std::map<uint32_t, Ptr<CellForming>> nodeCellFormingMap;
    
    int32_t gridOffset = 100;
    
    for (uint32_t i = 0; i < groundNodes.GetN(); i++)
    {
        Ptr<Node> node = groundNodes.Get(i);
        Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
        Vector position = mobility->GetPosition();
        
        Ptr<CellForming> cellForming = CreateObject<CellForming>();
        cellForming->SetNodeParams(i, position, cellRadius, gridOffset);
        cellForming->SetTimingParams(helloInterval, clElectionDelayInterval, clCalculationTime);
        
        // Set callbacks
        cellForming->SetHelloCallback([i](const HelloPacket& hello) {
            HandleHelloCallback(i, hello);
        });
        
        cellForming->SetCLAnnouncementCallback([i](const CLAnnouncementPacket& announcement) {
            HandleCLAnnouncementCallback(i, announcement);
        });
        
        cellForming->SetMemberFeedbackCallback([i](const CLMemberFeedbackPacket& feedback) {
            HandleMemberFeedbackCallback(i, feedback);
        });
        
        cellForming->SetStateChangeCallback([i](std::string state) {
            NS_LOG_INFO("Node " << i << " state: " << state);
        });
        
        // Simulate HELLO packet delivery (all nodes in range receive immediately)
        cellForming->SetHelloCallback([i, &nodeCellFormingMap](const HelloPacket& hello) {
            for (const auto& [nodeId, otherCellForming] : nodeCellFormingMap)
            {
                if (i == nodeId) continue;
                
                // Deliver HELLO with small delay (simulating propagation)
                Simulator::Schedule(MilliSeconds(10), [otherCellForming, hello]() {
                    otherCellForming->HandleHelloPacket(hello);
                });
            }
        });
        
        // Simulate CL announcement delivery
        cellForming->SetCLAnnouncementCallback([i, &nodeCellFormingMap](const CLAnnouncementPacket& announcement) {
            for (const auto& [nodeId, otherCellForming] : nodeCellFormingMap)
            {
                if (i == nodeId) continue;
                
                if (otherCellForming->GetCellId() == announcement.cellId)
                {
                    Simulator::Schedule(MilliSeconds(10), [otherCellForming, announcement]() {
                        otherCellForming->HandleCLAnnouncement(announcement);
                    });
                }
            }
        });
        
        // Simulate member feedback delivery (to CL)
        cellForming->SetMemberFeedbackCallback([&nodeCellFormingMap](const CLMemberFeedbackPacket& feedback) {
            // Deliver to CL node
            for (const auto& [nodeId, otherCellForming] : nodeCellFormingMap)
            {
                if (otherCellForming->GetCellLeaderId() == nodeId && otherCellForming->IsCellLeader())
                {
                    Simulator::Schedule(MilliSeconds(10), [otherCellForming, feedback]() {
                        otherCellForming->HandleMemberFeedback(feedback);
                    });
                    break;
                }
            }
        });
        
        nodeCellFormingMap[i] = cellForming;
        cellFormingModules.push_back(cellForming);
        
        NS_LOG_DEBUG("Created CellForming module for node " << i);
    }
    
    // ========================================================================
    // 4. Initialize all CellForming modules
    // ========================================================================
    
    NS_LOG_INFO("Initializing cell forming...");
    for (auto& cf : cellFormingModules)
    {
        cf->Initialize();
    }
    
    // ========================================================================
    // 5. Schedule statistics output
    // ========================================================================
    
    Simulator::Schedule(Seconds(simDuration - 1.0), [&nodeCellFormingMap]() {
        PrintCellFormingStats(nodeCellFormingMap);
    });
    
    // ========================================================================
    // 6. Run Simulation
    // ========================================================================
    
    NS_LOG_INFO("Starting simulation...\n");
    Simulator::Stop(Seconds(simDuration));
    Simulator::Run();
    Simulator::Destroy();
    
    NS_LOG_INFO("\nSimulation completed!");
    return 0;
}
