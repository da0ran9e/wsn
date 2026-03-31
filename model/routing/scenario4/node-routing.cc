/*
 * Scenario 4 - Initialization Helper
 * 
 * Helper functions to initialize routing components from scenario layer.
 */

#include "base-station-node/base-station-node.h"
#include "base-station-node/fragment-generator.h"
#include "node-routing.h"
#include "ground-node-routing/ground-node-routing.h"
#include "ground-node-routing/cell-cooperation.h"
#include "helper/calc-utils.h"
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
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <unordered_set>

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
static double g_scenarioStopTimeSec = -1.0;

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

void
TryMarkUav2CompletionFromSuspiciousSeed(const char* source)
{
    if (g_uav2MissionCompleted)
    {
        return;
    }

    const uint32_t suspiciousSeedNodeId = GetSuspiciousSeedNodeId();
    if (suspiciousSeedNodeId == std::numeric_limits<uint32_t>::max())
    {
        return;
    }

    auto it = g_groundNetworkPerNode.find(suspiciousSeedNodeId);
    if (it == g_groundNetworkPerNode.end())
    {
        return;
    }

    const double confidence = it->second.confidence;
    if (confidence >= ::ns3::wsn::scenario4::params::ALERT_THRESHOLD)
    {
        NS_LOG_INFO("[UAV2-MISSION-FALLBACK] Triggered by " << source
                    << " | seedNodeId=" << suspiciousSeedNodeId
                    << " | confidence=" << std::fixed << std::setprecision(3) << confidence
                    << " | threshold=" << ::ns3::wsn::scenario4::params::ALERT_THRESHOLD
                    << " | t=" << Simulator::Now().GetSeconds() << "s");
        MarkUav2MissionCompleted(suspiciousSeedNodeId, confidence);
    }
}

void
SchedulePeriodicUav2CompletionCheck(double intervalSec, double endTimeSec)
{
    if (g_uav2MissionCompleted || intervalSec <= 0.0)
    {
        return;
    }

    const double nowSec = Simulator::Now().GetSeconds();
    if (endTimeSec > 0.0 && nowSec > endTimeSec)
    {
        return;
    }

    TryMarkUav2CompletionFromSuspiciousSeed("PeriodicCheck");

    if (g_uav2MissionCompleted)
    {
        return;
    }

    const double nextSec = nowSec + intervalSec;
    if (endTimeSec > 0.0 && nextSec > endTimeSec)
    {
        return;
    }

    Simulator::Schedule(Seconds(intervalSec),
                        &SchedulePeriodicUav2CompletionCheck,
                        intervalSec,
                        endTimeSec);
}

void
TryMarkUav1CompletionFromHoveringTrace(const char* source)
{
    if (g_uav1MissionCompleted)
    {
        return;
    }

    const uint32_t suspiciousSeedNodeId = GetSuspiciousSeedNodeId();
    if (suspiciousSeedNodeId == std::numeric_limits<uint32_t>::max())
    {
        return;
    }

    const auto& hoveringTrace = ::ns3::wsn::scenario4::params::g_uav1HoveringNodeIds;
    const bool foundSuspiciousPoint =
        std::find(hoveringTrace.begin(), hoveringTrace.end(), suspiciousSeedNodeId) != hoveringTrace.end();
    if (!foundSuspiciousPoint)
    {
        return;
    }

    g_uav1MissionCompleted = true;
    g_uav1MissionCompletedTime = Simulator::Now().GetSeconds();

    NS_LOG_WARN("[UAV1-MISSION] Completed from hovering trace"
                << " | source=" << source
                << " | suspiciousSeedNodeId=" << suspiciousSeedNodeId
                << " | traceSize=" << hoveringTrace.size()
                << " | t=" << g_uav1MissionCompletedTime << "s");

    if (ns3::wsn::scenario4::params::g_resultFileStream)
    {
        *ns3::wsn::scenario4::params::g_resultFileStream
            << "\n[EVENT] " << g_uav1MissionCompletedTime
            << " | event=UAV1MissionComplete"
            << " | reason=SuspiciousPointInHoveringTrace"
            << " | source=" << source
            << " | suspiciousSeedNodeId=" << suspiciousSeedNodeId
            << std::endl;
    }

    if (g_uav1MissionCompleted && g_uav2MissionCompleted)
    {
        constexpr double kEarlyStopDelay = 1.0;
        NS_LOG_WARN("[EARLY-STOP] Both UAV missions complete, stopping simulation in "
                    << kEarlyStopDelay << "s");
        Simulator::Stop(Seconds(kEarlyStopDelay));
    }
}

void
ExpandUav1HoveringNodeIdsOnce(const char* source)
{
    if (g_uav1MissionCompleted)
    {
        return;
    }

    const auto snapshot = ::ns3::wsn::scenario4::params::g_uav1HoveringNodeIds;
    if (snapshot.empty())
    {
        return;
    }

    std::unordered_set<uint32_t> currentIds(snapshot.begin(), snapshot.end());
    std::vector<uint32_t> additions;
    additions.reserve(snapshot.size());

    for (uint32_t nodeId : snapshot)
    {
        auto it = g_groundNetworkPerNode.find(nodeId);
        if (it == g_groundNetworkPerNode.end() || it->second.neighbors.empty())
        {
            continue;
        }

        const auto& neighbors = it->second.neighbors;
        uint32_t selectedNeighbor = *neighbors.begin();
        bool foundUnseenNeighbor = false;

        for (uint32_t neighborId : neighbors)
        {
            if (!currentIds.count(neighborId))
            {
                selectedNeighbor = neighborId;
                foundUnseenNeighbor = true;
                break;
            }
        }

        additions.push_back(selectedNeighbor);
        if (foundUnseenNeighbor)
        {
            currentIds.insert(selectedNeighbor);
        }
    }

    if (additions.empty())
    {
        return;
    }

    auto& hoveringTrace = ::ns3::wsn::scenario4::params::g_uav1HoveringNodeIds;
    hoveringTrace.insert(hoveringTrace.end(), additions.begin(), additions.end());

    NS_LOG_INFO("[UAV1-HOVER-EXPAND] source=" << source
                << " | baseSize=" << snapshot.size()
                << " | added=" << additions.size()
                << " | totalSize=" << hoveringTrace.size()
                << " | t=" << Simulator::Now().GetSeconds() << "s");

    if (ns3::wsn::scenario4::params::g_resultFileStream)
    {
        *ns3::wsn::scenario4::params::g_resultFileStream
            << "\n[EVENT] " << Simulator::Now().GetSeconds()
            << " | event=UAV1HoverExpand"
            << " | source=" << source
            << " | baseSize=" << snapshot.size()
            << " | added=" << additions.size()
            << " | totalSize=" << hoveringTrace.size()
            << std::endl;
    }

    TryMarkUav1CompletionFromHoveringTrace(source);
}

void
SchedulePeriodicUav1HoverExpansion(double intervalSec, double endTimeSec)
{
    if (g_uav1MissionCompleted || intervalSec <= 0.0)
    {
        return;
    }

    const double nowSec = Simulator::Now().GetSeconds();
    if (endTimeSec > 0.0 && nowSec > endTimeSec)
    {
        return;
    }

    ExpandUav1HoveringNodeIdsOnce("PeriodicExpand");

    if (g_uav1MissionCompleted)
    {
        return;
    }

    const double nextSec = nowSec + intervalSec;
    if (endTimeSec > 0.0 && nextSec > endTimeSec)
    {
        return;
    }

    Simulator::Schedule(Seconds(intervalSec),
                        &SchedulePeriodicUav1HoverExpansion,
                        intervalSec,
                        endTimeSec);
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
RunBaseStationPostInitTasks()
{
    if (g_baseStation == nullptr)
    {
        NS_LOG_WARN("RunBaseStationPostInitTasks called before InitializeBaseStation");
        return;
    }
    g_baseStation->RunPostInitScheduledTasks();
}

void
SetScenarioStopTime(double stopTimeSec)
{
    g_scenarioStopTimeSec = stopTimeSec;
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

    // Also emit a snapshot of the suspicious seed node state at mission completion
    // so downstream analysis can see fragment origin counts and fragment IDs.
    const uint32_t suspiciousSeed = GetSuspiciousSeedNodeId();
    if (suspiciousSeed != std::numeric_limits<uint32_t>::max())
    {
        auto it = g_groundNetworkPerNode.find(suspiciousSeed);
        if (it != g_groundNetworkPerNode.end())
        {
            const auto &state = it->second;
            NS_LOG_INFO("[SUSPICIOUS-POINT-MISSION-COMPLETE] Node " << suspiciousSeed
                        << " | confidence=" << state.confidence
                        << " | totalFrags=" << state.fragments.fragments.size()
                        << " | fromUAV=" << state.fragmentsReceivedFromUav
                        << " | fromPeers=" << state.fragmentsReceivedFromPeers
                        << " | t=" << g_uav2MissionCompletedTime << "s");

            if (ns3::wsn::scenario4::params::g_resultFileStream)
            {
                *ns3::wsn::scenario4::params::g_resultFileStream
                    << "[SUSPICIOUS-POINT-MISSION-COMPLETE] " << g_uav2MissionCompletedTime
                    << " | nodeId=" << suspiciousSeed
                    << " | confidence=" << std::fixed << std::setprecision(3) << state.confidence
                    << " | totalFrags=" << state.fragments.fragments.size()
                    << " | fromUAV=" << state.fragmentsReceivedFromUav
                    << " | fromPeers=" << state.fragmentsReceivedFromPeers
                    << " | fragments=";
                for (const auto &p : state.fragments.fragments)
                {
                    *ns3::wsn::scenario4::params::g_resultFileStream << p.first << " ";
                }
                *ns3::wsn::scenario4::params::g_resultFileStream << std::endl;
            }
        }
    }

    if (g_uav1MissionCompleted && g_uav2MissionCompleted)
    {
        constexpr double kEarlyStopDelay = 1.0;
        NS_LOG_WARN("[EARLY-STOP] Both UAV missions complete, stopping simulation in "
                    << kEarlyStopDelay << "s");
        Simulator::Stop(Seconds(kEarlyStopDelay));
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

    g_uav1MissionCompleted = false;
    g_uav1MissionCompletedTime = -1.0;

    // Reset runtime trace for UAV1 hovered node IDs.
    ::ns3::wsn::scenario4::params::g_uav1HoveringNodeIds.clear();
    
    const auto& uavPaths = GetUavFlightPaths();
    
    if (uavPaths.empty())
    {
        NS_LOG_WARN("[UAV-FLIGHT] No UAV flight paths available");
        return;
    }

    // UAV1 is defined as the first UAV in the planned path map.
    const uint32_t uav1NodeId = uavPaths.begin()->first;

    // Fallback checker:
    // periodically evaluate seed-node confidence to avoid missing UAV2 completion
    // when threshold crossing happens via non-direct reception paths.
    constexpr double kUav2CompletionCheckIntervalSec = 0.5;
    const double checkEndTimeSec = (g_scenarioStopTimeSec > 0.0)
        ? g_scenarioStopTimeSec
        : 0.0;
    Simulator::Schedule(Seconds(kUav2CompletionCheckIntervalSec),
                        &SchedulePeriodicUav2CompletionCheck,
                        kUav2CompletionCheckIntervalSec,
                        checkEndTimeSec);

    // UAV1 hovering-trace expansion:
    // repeat every 2.7 * UAV1 hovering time.
    const double uav1HoverExpandIntervalSec = 2.7 * ::ns3::wsn::scenario4::params::UAV1_HOVER_TIME;
    if (uav1HoverExpandIntervalSec > 0.0)
    {
        Simulator::Schedule(Seconds(uav1HoverExpandIntervalSec),
                            &SchedulePeriodicUav1HoverExpansion,
                            uav1HoverExpandIntervalSec,
                            checkEndTimeSec);
    }
    
    NS_LOG_INFO("[UAV-FLIGHT] Initializing flight for " << uavPaths.size() << " UAVs");

    const double scenarioStopTimeSec = g_scenarioStopTimeSec;
    
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
        const bool isUav1Path = (uavNodeId == uav1NodeId);

        uint32_t repeatCount = 1;
        if (!isUav1Path && path.totalTime > 0.0 && scenarioStopTimeSec > 0.0)
        {
            repeatCount = static_cast<uint32_t>(std::ceil(scenarioStopTimeSec / path.totalTime));
            repeatCount = std::max(1u, repeatCount);
        }
        
        NS_LOG_INFO("[UAV-FLIGHT] Scheduling " << path.waypoints.size() 
                    << " waypoints for UAV " << uavNodeId
                    << " | totalTime=" << path.totalTime << "s"
                    << " | repeatCount=" << repeatCount);
        
        // Schedule movement to each waypoint
        for (uint32_t repeatIdx = 0; repeatIdx < repeatCount; ++repeatIdx)
        {
            const double cycleOffsetSec = repeatIdx * path.totalTime;

            for (size_t i = 0; i < path.waypoints.size(); ++i)
            {
                const Waypoint& wp = path.waypoints[i];
                const double scheduledArrivalSec = wp.arrivalTime + cycleOffsetSec;
                if (scenarioStopTimeSec > 0.0 && scheduledArrivalSec > scenarioStopTimeSec)
                {
                    break;
                }

                const bool isUav1 = (uavNodeId == uav1NodeId);
                const uint32_t hoverNodeId = wp.hoverNodeId;
                const uint32_t cycleNum = repeatIdx + 1;

                if (waypointMobility)
                {
                    waypointMobility->AddWaypoint(
                        ns3::Waypoint(Simulator::Now() + Seconds(scheduledArrivalSec), wp.position));
                }
            
                // Schedule position update at waypoint arrival time
                Simulator::Schedule(Seconds(scheduledArrivalSec),
                                    [uavNodeId,
                                     i,
                                     cycleNum,
                                     isUav1,
                                     hoverNodeId
                                     ]() {
                    Ptr<Node> node = NodeList::GetNode(uavNodeId);
                    if (!node) return;
                    
                    Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
                    if (!mob) return;

                    const Vector actualPos = mob->GetPosition();
                    
                    NS_LOG_INFO("[UAV-FLIGHT] UAV " << uavNodeId 
                                << " arrived at waypoint " << (i + 1)
                                << " | cycle=" << cycleNum
                                << " | pos=(" << actualPos.x << "," << actualPos.y << "," << actualPos.z << ")"
                                << " | t=" << Simulator::Now().GetSeconds() << "s");

                    if (ns3::wsn::scenario4::params::g_resultFileStream)
                    {
                        *ns3::wsn::scenario4::params::g_resultFileStream
                            << "\n[EVENT] " << Simulator::Now().GetSeconds()
                            << " | event=UAVWaypointArrival"
                            << " | nodeId=" << uavNodeId
                            << " | cycle=" << cycleNum
                            << " | hoverNodeId=" << hoverNodeId
                            << " | pos=(" << actualPos.x << "," << actualPos.y << "," << actualPos.z << ")"
                            << std::endl;
                    }

                    // Global trace: record UAV1 hovered node id at each waypoint arrival.
                    if (isUav1 && hoverNodeId != std::numeric_limits<uint32_t>::max())
                    {
                        ::ns3::wsn::scenario4::params::g_uav1HoveringNodeIds.push_back(hoverNodeId);
                        TryMarkUav1CompletionFromHoveringTrace("WaypointArrival");
                    }
                });
            }
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
    
    // Per-fragment radio contact time helper:
    // t_tx(fragment_i) = (fragment_i_bytes * 8) / RADIO_BITRATE_BPS
    auto fragmentTxTimeSec = [](uint32_t pixelCount) {
        return helper::CalculateFragmentTransmissionTime(
            pixelCount,
            ns3::wsn::scenario4::params::DEFAULT_BYTES_PER_PIXEL,
            ns3::wsn::scenario4::params::RADIO_BITRATE_BPS);
    };
    
    // UAV2 broadcasts from first waypoint until the repeated-flight horizon.
    const double broadcastStartTime = uav2Path.waypoints[0].arrivalTime;
    const double onePassEndTime = uav2Path.waypoints.back().arrivalTime;
    const double broadcastEndTime = (g_scenarioStopTimeSec > broadcastStartTime)
        ? g_scenarioStopTimeSec
        : onePassEndTime;
    
    // Calculate total broadcast duration and number of cycles
    const double totalBroadcastDuration = broadcastEndTime - broadcastStartTime;
    double singleCycleDuration = 0.0;
    for (const auto& [fragmentId, fragment] : fragments.fragments)
    {
        (void)fragmentId;
        const double txTime = fragmentTxTimeSec(fragment.pixelCount);
        singleCycleDuration += (txTime > 0.0)
            ? txTime
            : ns3::wsn::scenario4::params::FRAGMENT_BROADCAST_INTERVAL;
    }
    if (singleCycleDuration <= 0.0)
    {
        singleCycleDuration = fragments.fragments.size() * ns3::wsn::scenario4::params::FRAGMENT_BROADCAST_INTERVAL;
    }
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
                << " | txFormula=(bytes*8)/" << ns3::wsn::scenario4::params::RADIO_BITRATE_BPS << "bps");
    
    // Schedule broadcast cycles - repeat until reaching last waypoint
    double currentTime = broadcastStartTime;
    double lastScheduledTime = broadcastStartTime;
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
                            << " | pixels=" << fragment.pixelCount
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
                        << " | pixels=" << fragment.pixelCount
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
                
                Ptr<Packet> pkt = Create<Packet>(fragment.pixelCount
                                                   * ::ns3::wsn::scenario4::params::DEFAULT_BYTES_PER_PIXEL);

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
            
            const double perFragmentInterval = std::max(
                fragmentTxTimeSec(fragment.pixelCount),
                ns3::wsn::scenario4::params::FRAGMENT_BROADCAST_INTERVAL);
            lastScheduledTime = currentTime;
            currentTime += perFragmentInterval;
            totalBroadcasts++;
        }
        
        if (currentTime > broadcastEndTime)
        {
            break;
        }
    }
    
    NS_LOG_INFO("[UAV-BROADCAST] Scheduled " << totalBroadcasts 
                << " total fragment broadcasts in " << numBroadcastCycles << " cycles"
                << " | actualEndTime=" << lastScheduledTime << "s");
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
    
    // Timeout = time when UAV2 finishes its repeated mission horizon.
    const double cooperationTimeoutSec = (g_scenarioStopTimeSec > 0.0)
        ? g_scenarioStopTimeSec
        : uav2Path.waypoints.back().arrivalTime;
    
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
                ScheduleFragmentSharingRequest(nodeId, state.cellId);
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

        // One-shot fallback check right after global cooperation trigger.
        TryMarkUav2CompletionFromSuspiciousSeed("CellCooperationTimeout");
        
        // Log to result file
        if (ns3::wsn::scenario4::params::g_resultFileStream)
        {
            *ns3::wsn::scenario4::params::g_resultFileStream
                << "\n[EVENT] " << Simulator::Now().GetSeconds()
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
