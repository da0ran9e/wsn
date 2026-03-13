/*
 * Scenario 4 - Initialization Helper
 * 
 * Helper functions to initialize routing components from scenario layer.
 */

#include "base-station-node/base-station-node.h"
#include "base-station-node/fragment-generator.h"
#include "ground-node-routing/ground-node-routing.h"
#include "ground-node-routing/cell-cooperation.h"
#include "packet-header.h"
#include "../../radio/cc2420/cc2420-net-device.h"
#include "ns3/mac16-address.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/node-list.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/waypoint-mobility-model.h"
#include "../../../examples/scenarios/scenario4/scenario4-params.h"
#include <cmath>
#include <iomanip>
#include <limits>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Scenario4NodeRouting");

namespace wsn {
namespace scenario4 {
namespace routing {

// Global BS instance
static BaseStationNode* g_baseStation = nullptr;
static double g_lastProcessedTopologyTs = -1.0;
static bool g_uav1MissionCompleted = false;
static double g_uav1MissionCompletedTime = -1.0;
static bool g_uav2MissionCompleted = false;
static double g_uav2MissionCompletedTime = -1.0;

namespace
{
Ptr<wsn::Cc2420NetDevice>
GetCc2420Device(Ptr<Node> node)
{
    if (!node || node->GetNDevices() == 0)
    {
        return nullptr;
    }
    return DynamicCast<wsn::Cc2420NetDevice>(node->GetDevice(0));
}
}

void InitializeBaseStation(uint32_t nodeId)
{
    if (g_baseStation == nullptr) {
        g_baseStation = new BaseStationNode(nodeId);
        g_baseStation->Initialize();
    }
}

void
MarkUav2MissionCompleted(uint32_t triggerNodeId, double triggerConfidence)
{
    if (g_uav2MissionCompleted)
    {
        return;
    }

    g_uav2MissionCompleted = true;
    g_uav2MissionCompletedTime = Simulator::Now().GetSeconds();

    NS_LOG_WARN("[UAV2-MISSION] Early completion triggered"
                << " | triggerNodeId=" << triggerNodeId
                << " | confidence=" << std::fixed << std::setprecision(3) << triggerConfidence
                << " | t=" << g_uav2MissionCompletedTime << "s");

    if (ns3::wsn::scenario4::params::g_resultFileStream)
    {
        *ns3::wsn::scenario4::params::g_resultFileStream
            << "\n[EVENT] " << g_uav2MissionCompletedTime
            << " | event=UAV2MissionComplete"
            << " | triggerNodeId=" << triggerNodeId
            << " | confidence=" << std::fixed << std::setprecision(3) << triggerConfidence
            << std::endl;
    }
}

bool
IsUav1MissionCompleted()
{
    return g_uav1MissionCompleted;
}

bool
IsUav2MissionCompleted()
{
    return g_uav2MissionCompleted;
}

double
GetUav1MissionCompletedTime()
{
    return g_uav1MissionCompletedTime;
}

double
GetUav2MissionCompletedTime()
{
    return g_uav2MissionCompletedTime;
}

void TickBaseStationControl()
{
    if (g_baseStation == nullptr)
    {
        return;
    }

    const GlobalTopology* topo = GetLatestTopologySnapshotPtr();
    if (topo == nullptr)
    {
        return;
    }

    // Process only new snapshots
    if (topo->timestamp <= g_lastProcessedTopologyTs)
    {
        return;
    }

    g_lastProcessedTopologyTs = topo->timestamp;
    g_baseStation->ReceiveTopology(*topo);
    NS_LOG_INFO("BS pulled shared topology at t=" << topo->timestamp);
}

void InitializeUavFlight()
{
    NS_LOG_FUNCTION_NOARGS();
    
    const auto& uavPaths = GetUavFlightPaths();
    
    if (uavPaths.empty())
    {
        NS_LOG_WARN("[UAV-FLIGHT] No UAV flight paths available");
        return;
    }

    // UAV1 is defined as the first UAV in the planned path map.
    const uint32_t uav1NodeId = uavPaths.begin()->first;

    // Suspicious point node position (seed node selected by BS).
    Vector suspiciousPointPos{0.0, 0.0, 0.0};
    bool hasSuspiciousPointPos = false;
    const uint32_t suspiciousSeedNodeId = GetSuspiciousSeedNodeId();
    if (suspiciousSeedNodeId != std::numeric_limits<uint32_t>::max())
    {
        auto itState = g_groundNetworkPerNode.find(suspiciousSeedNodeId);
        if (itState != g_groundNetworkPerNode.end())
        {
            suspiciousPointPos = itState->second.position;
            hasSuspiciousPointPos = true;
        }
    }
    
    NS_LOG_INFO("[UAV-FLIGHT] Initializing flight for " << uavPaths.size() << " UAVs");
    
    // Schedule waypoint movements for each UAV
    for (const auto& [uavNodeId, path] : uavPaths)
    {
        Ptr<Node> uavNode = NodeList::GetNode(uavNodeId);
        if (!uavNode)
        {
            NS_LOG_WARN("[UAV-FLIGHT] UAV node " << uavNodeId << " not found");
            continue;
        }
        
        Ptr<MobilityModel> mobility = uavNode->GetObject<MobilityModel>();
        if (!mobility)
        {
            NS_LOG_WARN("[UAV-FLIGHT] UAV node " << uavNodeId << " has no MobilityModel");
            continue;
        }

        Ptr<WaypointMobilityModel> waypointMobility = DynamicCast<WaypointMobilityModel>(mobility);
        
        NS_LOG_INFO("[UAV-FLIGHT] Scheduling " << path.waypoints.size() 
                    << " waypoints for UAV " << uavNodeId
                    << " | totalTime=" << path.totalTime << "s");
        
        // Schedule movement to each waypoint
        for (size_t i = 0; i < path.waypoints.size(); ++i)
        {
            const Waypoint& wp = path.waypoints[i];
            const bool isUav1 = (uavNodeId == uav1NodeId);
            const bool hasPreviousWaypoint = (i > 0);
            const Waypoint prevWp = hasPreviousWaypoint ? path.waypoints[i - 1] : wp;

            if (waypointMobility)
            {
                waypointMobility->AddWaypoint(
                    ns3::Waypoint(Simulator::Now() + Seconds(wp.arrivalTime), wp.position));
            }
            
            // Schedule position update at waypoint arrival time
            Simulator::Schedule(Seconds(wp.arrivalTime),
                                [uavNodeId,
                                 wp,
                                 i,
                                 isUav1,
                                 hasPreviousWaypoint,
                                 prevWp,
                                 hasSuspiciousPointPos,
                                 suspiciousPointPos]() {
                Ptr<Node> node = NodeList::GetNode(uavNodeId);
                if (!node) return;
                
                Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
                if (!mob) return;

                const Vector actualPos = mob->GetPosition();
                
                NS_LOG_INFO("[UAV-FLIGHT] UAV " << uavNodeId 
                            << " arrived at waypoint " << (i + 1)
                            << " | pos=(" << actualPos.x << "," << actualPos.y << "," << actualPos.z << ")"
                            << " | t=" << Simulator::Now().GetSeconds() << "s");

                // TODO:  in log vào `g_resultFileStream` tại đây
                // Format: [EVENT] time | event=UAVWaypointArrival | nodeId=... | pos=(x,y,z)
                if (ns3::wsn::scenario4::params::g_resultFileStream)
                {
                    *ns3::wsn::scenario4::params::g_resultFileStream
                        << "\n[EVENT] " << Simulator::Now().GetSeconds()
                        << " | event=UAVWaypointArrival"
                        << " | nodeId=" << uavNodeId
                        << " | pos=(" << actualPos.x << "," << actualPos.y << "," << actualPos.z << ")"
                        << std::endl;
                }

                // UAV1 mission completion: considered done right after leaving suspicious point.
                // In discrete waypoint events, we detect this at arrival of the waypoint
                // immediately after the suspicious-point waypoint.
                if (!g_uav1MissionCompleted && isUav1 && hasPreviousWaypoint && hasSuspiciousPointPos)
                {
                    constexpr double kPosEps = 1.0;
                    const bool prevIsSuspiciousPoint =
                        (std::abs(prevWp.position.x - suspiciousPointPos.x) <= kPosEps) &&
                        (std::abs(prevWp.position.y - suspiciousPointPos.y) <= kPosEps);

                    if (prevIsSuspiciousPoint)
                    {
                        g_uav1MissionCompleted = true;
                        g_uav1MissionCompletedTime = Simulator::Now().GetSeconds();

                        NS_LOG_WARN("[UAV1-MISSION] Completed after leaving suspicious point"
                                    << " | t=" << g_uav1MissionCompletedTime << "s");

                        if (ns3::wsn::scenario4::params::g_resultFileStream)
                        {
                            *ns3::wsn::scenario4::params::g_resultFileStream
                                << "\n[EVENT] " << g_uav1MissionCompletedTime
                                << " | event=UAV1MissionComplete"
                                << " | reason=LeftSuspiciousPoint"
                                << std::endl;
                        }
                    }
                }
            });
        }
    }
}

void InitializeUavBroadcast()
{
    NS_LOG_FUNCTION_NOARGS();
    
    // Get UAV flight paths
    const auto& uavPaths = GetUavFlightPaths();
    
    if (uavPaths.size() < 2)
    {
        NS_LOG_WARN("[UAV-BROADCAST] UAV2 not available (need at least 2 UAVs)");
        return;
    }
    
    // Get UAV2 (second UAV in the map)
    auto it = uavPaths.begin();
    ++it; // Move to second UAV
    const uint32_t uav2NodeId = it->first;
    const UavFlightPath& uav2Path = it->second;
    
    if (uav2Path.waypoints.empty())
    {
        NS_LOG_WARN("[UAV-BROADCAST] UAV2 has no waypoints");
        return;
    }
    
    // Get fragments to broadcast
    const FragmentCollection& fragments = GetBsGeneratedFragments();
    
    if (fragments.fragments.empty())
    {
        NS_LOG_WARN("[UAV-BROADCAST] No fragments available for broadcast");
        return;
    }
    
    // UAV2 broadcasts from first waypoint until last waypoint
    const double broadcastStartTime = uav2Path.waypoints[0].arrivalTime;
    const double broadcastEndTime = uav2Path.waypoints.back().arrivalTime;
    const double broadcastInterval = ns3::wsn::scenario4::params::FRAGMENT_BROADCAST_INTERVAL;
    
    // Calculate total broadcast duration and number of cycles
    const double totalBroadcastDuration = broadcastEndTime - broadcastStartTime;
    const double singleCycleDuration = fragments.fragments.size() * broadcastInterval;
    const uint32_t numBroadcastCycles = static_cast<uint32_t>(
        std::ceil(totalBroadcastDuration / singleCycleDuration));
    
    NS_LOG_INFO("[UAV-BROADCAST] Initializing UAV2 fragment broadcast"
                << " | uavNodeId=" << uav2NodeId
                << " | numFragments=" << fragments.fragments.size()
                << " | startTime=" << broadcastStartTime << "s"
                << " | endTime=" << broadcastEndTime << "s"
                << " | duration=" << totalBroadcastDuration << "s"
                << " | cycleDuration=" << singleCycleDuration << "s"
                << " | numCycles=" << numBroadcastCycles
                << " | interval=" << broadcastInterval << "s");
    
    // Schedule broadcast cycles - repeat until reaching last waypoint
    double currentTime = broadcastStartTime;
    uint32_t totalBroadcasts = 0;
    
    for (uint32_t cycle = 0; cycle < numBroadcastCycles; ++cycle)
    {
        for (const auto& [fragmentId, fragment] : fragments.fragments)
        {
            // Stop scheduling if we've reached the last waypoint time
            if (currentTime > broadcastEndTime)
            {
                break;
            }
            
            const uint32_t cycleNum = cycle; // Capture for lambda
            Simulator::Schedule(Seconds(currentTime), [uav2NodeId, fragmentId, fragment, cycleNum]() {
                if (g_uav2MissionCompleted)
                {
                    return;
                }

                NS_LOG_INFO("[UAV-BROADCAST] UAV " << uav2NodeId 
                            << " broadcasting fragment " << fragmentId
                            << " (cycle " << (cycleNum + 1) << ")"
                            << " | confidence=" << std::fixed << std::setprecision(3) << fragment.confidence
                            << " | size=" << fragment.size << " bytes"
                            << " | t=" << Simulator::Now().GetSeconds() << "s");
                
                // Log to result file
                if (ns3::wsn::scenario4::params::g_resultFileStream)
                {
                    *ns3::wsn::scenario4::params::g_resultFileStream
                        << "\n[EVENT] " << Simulator::Now().GetSeconds()
                        << " | event=UAVFragmentBroadcast"
                        << " | nodeId=" << uav2NodeId
                        << " | fragmentId=" << fragmentId
                        << " | cycle=" << (cycleNum + 1)
                        << " | confidence=" << std::fixed << std::setprecision(3) << fragment.confidence
                        << " | size=" << fragment.size
                        << std::endl;
                }
                
                // Broadcast fragment through CC2420 MAC/PHY.
                // Receiver filtering is handled by CC2420 PHY link evaluation.
                Ptr<Node> uavNode = NodeList::GetNode(uav2NodeId);
                if (!uavNode) return;

                Ptr<wsn::Cc2420NetDevice> uavDev = GetCc2420Device(uavNode);
                if (!uavDev)
                {
                    NS_LOG_WARN("[UAV-BROADCAST] UAV node " << uav2NodeId << " has no Cc2420NetDevice");
                    return;
                }
                
                Ptr<Packet> pkt = Create<Packet>(fragment.size);

                FragmentPacket fragHeader;
                fragHeader.SetFragmentId(fragmentId);
                fragHeader.SetConfidence(fragment.confidence);
                fragHeader.SetSourceId(uav2NodeId);
                pkt->AddHeader(fragHeader);

                PacketHeader typeHeader;
                typeHeader.SetType(PACKET_TYPE_FRAGMENT);
                pkt->AddHeader(typeHeader);

                if (!uavDev->Send(pkt, Mac16Address("FF:FF"), 0))
                {
                    NS_LOG_WARN("[UAV-BROADCAST] Broadcast send failed for UAV " << uav2NodeId
                                << " fragment=" << fragmentId);
                }
            });
            
            currentTime += broadcastInterval;
            totalBroadcasts++;
        }
        
        if (currentTime > broadcastEndTime)
        {
            break;
        }
    }
    
    NS_LOG_INFO("[UAV-BROADCAST] Scheduled " << totalBroadcasts 
                << " total fragment broadcasts in " << numBroadcastCycles << " cycles"
                << " | actualEndTime=" << (currentTime - broadcastInterval) << "s");
}

void InitializeCellCooperationTimeout()
{
    NS_LOG_FUNCTION_NOARGS();
    
    // Force cooperation timeout: Triggered when UAV2 reaches its last waypoint.
    // At this point, all fragments have been broadcast and nodes should share
    // fragments regardless of whether they've reached the cooperation threshold.
    
    // Get UAV flight paths
    const auto& uavPaths = GetUavFlightPaths();
    
    if (uavPaths.size() < 2)
    {
        NS_LOG_WARN("[CELL-COOPERATION-TIMEOUT] UAV2 not available (need at least 2 UAVs)");
        return;
    }
    
    // Get UAV2 (second UAV in the map)
    auto it = uavPaths.begin();
    ++it; // Move to second UAV
    const UavFlightPath& uav2Path = it->second;
    
    if (uav2Path.waypoints.empty())
    {
        NS_LOG_WARN("[CELL-COOPERATION-TIMEOUT] UAV2 has no waypoints");
        return;
    }
    
    // Timeout = time when UAV2 reaches last waypoint
    const double cooperationTimeoutSec = uav2Path.waypoints.back().arrivalTime;
    
    NS_LOG_INFO("[CELL-COOPERATION-TIMEOUT] Scheduling forced cooperation trigger"
                << " | uav2LastWaypointTime=" << cooperationTimeoutSec << "s"
                << " | numWaypoints=" << uav2Path.waypoints.size());
    
    Simulator::Schedule(Seconds(cooperationTimeoutSec), []() {
        NS_LOG_INFO("[CELL-COOPERATION-TIMEOUT] Global timeout triggered for fallback cooperation"
                    << " | t=" << Simulator::Now().GetSeconds() << "s");
        
        uint32_t triggeredCount = 0;
        uint32_t skippedCount = 0;
        
        // Trigger cooperation only for nodes that haven't triggered per-node timeout
        for (auto& [nodeId, state] : g_groundNetworkPerNode)
        {
            const bool hasAllFragments =
                (state.expectedFragmentCount > 0) &&
                (state.fragments.fragments.size() >= state.expectedFragmentCount);
            if (state.cooperationEnabled && state.cellId >= 0 && !state.cooperationTimeoutScheduled && !hasAllFragments)
            {
                RequestFragmentSharing(nodeId, state.cellId);
                triggeredCount++;
            }
            else if (state.cooperationEnabled && state.cellId >= 0 && state.cooperationTimeoutScheduled)
            {
                skippedCount++;
            }
        }
        
        NS_LOG_INFO("[CELL-COOPERATION-TIMEOUT] Global timeout completed"
                    << " | triggered=" << triggeredCount 
                    << " | skipped=" << skippedCount << " (already cooperated)");
        
        // Log to result file
        if (ns3::wsn::scenario4::params::g_resultFileStream)
        {
            *ns3::wsn::scenario4::params::g_resultFileStream
                << "[EVENT] " << Simulator::Now().GetSeconds()
                << " | event=CellCooperationTimeout"
                << " | triggeredNodes=" << triggeredCount
                << std::endl;
        }
    });
}

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3
