/*
 * Scenario 4 - Ground Node Routing Implementation
 */

#include "ground-node-routing.h"
#include "../packet-header.h"
#include "../helper/calc-utils.h"
#include "../node-routing.h"
#include "cell-cooperation.h"
#include "../base-station-node/base-station-node.h"
#include "../../../radio/cc2420/cc2420-net-device.h"
#include "../../../radio/cc2420/cc2420-mac.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/mobility-model.h"
#include "ns3/node-list.h"
#include <algorithm>
#include <limits>
#include "../../../../examples/scenarios/scenario4/scenario4-params.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("GroundNodeRouting");

namespace wsn {
namespace scenario4 {
namespace routing {

// Global state storage
std::map<uint32_t, GroundNetworkState> g_groundNetworkPerNode;
GlobalTopology g_latestTopologySnapshot;
bool g_hasLatestTopologySnapshot = false;

namespace
{
void
HandleGroundMacIndication(uint32_t nodeId,
                          Ptr<Packet> packet,
                          Mac16Address source,
                          double rssiDbm)
{
    (void)source;
    OnGroundNodeReceivePacket(nodeId, packet, rssiDbm);
}

void
AttachGroundRxCallback(Ptr<Node> node)
{
    if (!node || node->GetNDevices() == 0)
    {
        return;
    }

    Ptr<::ns3::wsn::Cc2420NetDevice> cc2420Dev =
        DynamicCast<::ns3::wsn::Cc2420NetDevice>(node->GetDevice(0));
    if (!cc2420Dev)
    {
        return;
    }

    Ptr<::ns3::wsn::Cc2420Mac> mac = cc2420Dev->GetMac();
    if (!mac)
    {
        return;
    }

    mac->SetMcpsDataIndicationCallback(
        MakeBoundCallback(&HandleGroundMacIndication, node->GetId()));
}
}

void
InitializeGroundNodeRouting(NodeContainer nodes, uint32_t numFragments)
{
    NS_LOG_FUNCTION(nodes.GetN() << numFragments);
    
    // Initialize state for each ground node
    for (uint32_t i = 0; i < nodes.GetN(); ++i) {
        Ptr<Node> node = nodes.Get(i);
        uint32_t nodeId = node->GetId();
        
        GroundNetworkState state;
        
        // === Node Identity ===
        state.nodeId = nodeId;
        state.isCellLeader = false; // Sẽ được xác định trong startup phase
        state.cellId = -1; // Chưa xác định cell ID
        state.cellColor = -1; // Chưa xác định màu cell
        state.isTimeSynchronized = false;
        state.clockOffsetSec = 0.0;
        state.lastSyncTime = 0.0;
        
        // === Fragment Management ===
        state.fragments.fragments.clear();
        state.confidence = 0.0; // Tính toán sau khi nhận fragment
        state.expectedFragmentCount = numFragments;
        state.fragmentCoverageRatio = 0.0;
        state.fragmentsReceivedFromUav = 0;
        state.fragmentsReceivedFromPeers = 0;
        state.duplicateFragmentsDiscarded = 0;
        
        // === Neighbor Discovery ===
        state.startupComplete = false;
        state.isIsolated = false;
        state.lifecyclePhase = GroundNodeLifecyclePhase::DISCOVERY;
        // neighbors, neighborRssi, neighborDistance sẽ được fill trong startup
        
        // === Cell Info ===
        // Compute cell ID from position
        Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
        if (mobility) {
            Vector pos = mobility->GetPosition();
            state.position = pos;
        } else {
            state.cellId = -1;
            state.cellColor = -1;
        }
        
        // === Communication Statistics ===
        state.packetCount = 0;
        state.startupPacketsReceived = 0;
        state.fragmentPacketsReceived = 0;
        state.cooperationPacketsReceived = 0;
        state.packetsSent = 0;
        state.totalBytesReceived = 0.0;
        state.totalBytesSent = 0.0;
        state.lastPacketRssiDbm = 0.0;
        state.avgPacketRssiDbm = 0.0;
        state.rssiSampleCount = 0;
        
        // === Cell Cooperation ===
        state.cooperationRequestsSent = 0;
        state.cooperationRequestsReceived = 0;
        state.lastCooperationTime = 0.0;
        state.cooperationEnabled = true; // Mặc định enable cooperation
        state.cooperationTimeoutScheduled = false;
        state.cooperationTimeoutTime = 0.0;
        
        // === UAV Interaction ===
        state.lastUavContactTime = 0.0;
        state.uavEncounters = 0;
        
        // === Topology Reporting ===
        state.lastTopologyReportTime = 0.0;
        state.topologyReportCount = 0;
        
        // === Energy & Resource ===
        state.remainingEnergy = 1000.0; // 1000 joules mặc định
        state.energyConsumedTx = 0.0;
        state.energyConsumedRx = 0.0;
        
        // === Timing ===
        state.initializationTime = Simulator::Now().GetSeconds();
        state.lastActivityTime = state.initializationTime;
        
        g_groundNetworkPerNode[nodeId] = state;
        
    }
    
    NS_LOG_INFO("Ground node routing initialized for " << nodes.GetN() << " nodes");
}

void
AttachGroundRxCallbacks(NodeContainer nodes)
{
    for (uint32_t i = 0; i < nodes.GetN(); ++i)
    {
        AttachGroundRxCallback(nodes.Get(i));
    }
}

void
OnGroundNodeReceivePacket(uint32_t nodeId, Ptr<const Packet> packet, double rssiDbm)
{
    NS_LOG_FUNCTION(nodeId << packet->GetSize() << rssiDbm);
    
    if (g_groundNetworkPerNode.find(nodeId) == g_groundNetworkPerNode.end()) {
        NS_LOG_WARN("Node " << nodeId << " not initialized");
        return;
    }
    
    GroundNetworkState& state = g_groundNetworkPerNode[nodeId];
    
    // Update statistics
    state.packetCount++;
    state.totalBytesReceived += packet->GetSize();
    state.lastActivityTime = Simulator::Now().GetSeconds();
    state.lastPacketRssiDbm = rssiDbm;
    state.rssiSampleCount++;
    state.avgPacketRssiDbm += (rssiDbm - state.avgPacketRssiDbm) / state.rssiSampleCount;
    state.lifecyclePhase = (state.remainingEnergy > 0.0)
                           ? GroundNodeLifecyclePhase::ACTIVE
                           : GroundNodeLifecyclePhase::DEAD;
    
    // Estimate energy consumption for reception (simplified model)
    const double rxEnergyCost = 0.001 * packet->GetSize() / 1024.0; // ~1mJ per KB
    state.energyConsumedRx += rxEnergyCost;
    state.remainingEnergy = std::max(0.0, state.remainingEnergy - rxEnergyCost);
    
    // Try to extract packet header
    // If this node is the suspicious seed point, log its current confidence
    // and fragment counts (from UAV vs from peers) for debugging/analysis.
    if (nodeId == ::ns3::wsn::scenario4::routing::g_suspiciousSeedNodeId)
    {
        NS_LOG_INFO("[SUSPICIOUS-POINT] Node " << nodeId
                    << " received packet | confidence=" << state.confidence
                    << " | totalFrags=" << state.fragments.fragments.size()
                    << " | fromUAV=" << state.fragmentsReceivedFromUav
                    << " | fromPeers=" << state.fragmentsReceivedFromPeers
                    << " | t=" << Simulator::Now().GetSeconds() << "s");

        if (ns3::wsn::scenario4::params::g_resultFileStream)
        {
            *ns3::wsn::scenario4::params::g_resultFileStream
                << "[SUSPICIOUS-POINT-RECV] " << Simulator::Now().GetSeconds()
                << " | nodeId=" << nodeId
                << " | confidence=" << state.confidence
                << " | totalFrags=" << state.fragments.fragments.size()
                << " | fromUAV=" << state.fragmentsReceivedFromUav
                << " | fromPeers=" << state.fragmentsReceivedFromPeers
                << " | fragments=";
            for (const auto &p : state.fragments.fragments)
            {
                *ns3::wsn::scenario4::params::g_resultFileStream << p.first << " ";
            }
            *ns3::wsn::scenario4::params::g_resultFileStream << "\n";
        }
    }
    Ptr<Packet> copy = packet->Copy();
    PacketHeader header;
    copy->RemoveHeader(header);
    
    switch (header.GetType()) {
        case PACKET_TYPE_STARTUP:
            NS_LOG_DEBUG("Node " << nodeId << " received STARTUP packet");
            state.startupPacketsReceived++;
            {
                StartupPhasePacket startupPkt;
                copy->RemoveHeader(startupPkt);
                uint32_t srcNodeId = startupPkt.GetNodeId();
                if (srcNodeId != nodeId)
                {
                    state.neighbors.insert(srcNodeId);
                    state.neighborRssi[srcNodeId] = rssiDbm;

                    double srcX = 0.0;
                    double srcY = 0.0;
                    startupPkt.GetPosition(srcX, srcY);
                    state.neighborDistance[srcNodeId] =
                        helper::CalculateDistance(state.position.x, state.position.y, srcX, srcY);
                }
            }
            // Handle startup phase (will be implemented in startup-phase module)
            break;
            
        case PACKET_TYPE_FRAGMENT:
            NS_LOG_DEBUG("Node " << nodeId << " received FRAGMENT packet");
            state.fragmentPacketsReceived++;

            // Handle fragment reception
            {
                FragmentPacket fragPkt;
                if (copy->RemoveHeader(fragPkt) == 0)
                {
                    NS_LOG_WARN("Node " << nodeId << " received malformed FRAGMENT packet");
                    break;
                }
                
                uint32_t fragId = fragPkt.GetFragmentId();
                double confidence = std::clamp(fragPkt.GetConfidence(), 0.0, 1.0);
                uint32_t srcNodeId = fragPkt.GetSourceId();
                const double now = Simulator::Now().GetSeconds();

                bool fromUav = false;
                if (srcNodeId < NodeList::GetNNodes())
                {
                    Ptr<Node> srcNode = NodeList::GetNode(srcNodeId);
                    if (srcNode)
                    {
                        Ptr<MobilityModel> srcMobility = srcNode->GetObject<MobilityModel>();
                        if (srcMobility)
                        {
                            fromUav = srcMobility->GetPosition().z > 1.0;
                        }
                    }
                }
                
                // Update fragment if new or higher confidence
                bool updated = false;
                if (!state.fragments.HasFragment(fragId) ||
                    state.fragments.GetFragment(fragId)->confidence < confidence) {
                    
                    Fragment frag;
                    frag.fragmentId   = fragId;
                    frag.strideOffset = fragId;  // stride slot = fragment ID (N-way uniform split)
                    // Payload bytes / bytesPerPixel gives the pixel count for this region.
                    frag.pixelCount   = copy->GetSize()
                                        / ::ns3::wsn::scenario4::params::DEFAULT_BYTES_PER_PIXEL;
                    frag.confidence   = confidence;
                    
                    state.fragments.AddFragment(frag);
                    state.confidence = state.fragments.totalConfidence;
                    state.fragmentLastUpdateTime[fragId] = now;
                    state.fragmentCoverageRatio = (state.expectedFragmentCount > 0)
                                                  ? static_cast<double>(state.fragments.fragments.size()) /
                                                        state.expectedFragmentCount
                                                  : 0.0;
                    
                    if (fromUav)
                    {
                        state.fragmentsReceivedFromUav++;
                        state.lastUavContactTime = now;
                        state.uavEncounters++;
                    }
                    else
                    {
                        state.fragmentsReceivedFromPeers++;
                    }
                    updated = true;

                    // Early-complete UAV2 mission when suspicious-point node reaches alert threshold.
                    const uint32_t suspiciousSeedNodeId = GetSuspiciousSeedNodeId();
                    if (suspiciousSeedNodeId != std::numeric_limits<uint32_t>::max() &&
                        nodeId == suspiciousSeedNodeId &&
                        state.confidence >= ::ns3::wsn::scenario4::params::ALERT_THRESHOLD)
                    {
                        MarkUav2MissionCompleted(nodeId, state.confidence);
                    }
                    
                    NS_LOG_INFO("Node " << nodeId << " updated fragment " << fragId 
                                << " with confidence " << confidence);
                }
                else
                {
                    state.duplicateFragmentsDiscarded++;
                }

                // Format: srcNodeId1-R-nodeId1 (fragId1) srcNodeId2-R-nodeId2 (fragId2) ...
                if (ns3::wsn::scenario4::params::g_resultFileStream)
                {
                    *ns3::wsn::scenario4::params::g_resultFileStream << srcNodeId
                        << "-R-" << nodeId
                        << "(" << fragId << ") ";
                }

                // Schedule per-node cooperation timeout after first fragment.
                // Delay = numFragments × broadcast interval + margin so that the node waits
                // until the UAV has broadcast its full fragment set before cooperating.
                // This ensures the node cooperates with its complete fragment subset rather
                // than prematurely sharing only 1-2 fragments.
                if (state.cooperationEnabled && updated && !state.cooperationTimeoutScheduled)
                {
                    const double cooperationDelay =
                        static_cast<double>(state.expectedFragmentCount) *
                            ns3::wsn::scenario4::params::FRAGMENT_BROADCAST_INTERVAL +
                        0.5; // 0.5s margin after UAV finishes broadcasting
                    const double timeoutTime = now + cooperationDelay;

                    state.cooperationTimeoutScheduled = true;
                    state.cooperationTimeoutTime = timeoutTime;

                    NS_LOG_DEBUG("Node " << nodeId << " scheduled cooperation timeout"
                                << " | delay=" << cooperationDelay << "s"
                                << " | timeout_at=" << timeoutTime << "s");

                    // Schedule timeout callback
                    Simulator::Schedule(Seconds(cooperationDelay), [nodeId]() {
                        if (g_groundNetworkPerNode.find(nodeId) == g_groundNetworkPerNode.end())
                        {
                            return;
                        }

                        auto& state = g_groundNetworkPerNode[nodeId];
                        const bool hasAllFragments =
                            (state.expectedFragmentCount > 0) &&
                            (state.fragments.fragments.size() >= state.expectedFragmentCount);

                        // Emit a snapshot immediately after the UAV departure (i.e., when
                        // this cooperation-timeout callback fires). This mirrors the
                        // per-packet suspicious logging but is timed to the UAV finish
                        // so we can observe the node's state right after UAV leaves.
                        if (nodeId == ::ns3::wsn::scenario4::routing::g_suspiciousSeedNodeId)
                        {
                            NS_LOG_INFO("[SUSPICIOUS-POINT-UAV-DEPART] Node " << nodeId
                                        << " | confidence=" << state.confidence
                                        << " | totalFrags=" << state.fragments.fragments.size()
                                        << " | fromUAV=" << state.fragmentsReceivedFromUav
                                        << " | fromPeers=" << state.fragmentsReceivedFromPeers
                                        << " | t=" << Simulator::Now().GetSeconds() << "s");

                            if (ns3::wsn::scenario4::params::g_resultFileStream)
                            {
                                *ns3::wsn::scenario4::params::g_resultFileStream
                                    << "[SUSPICIOUS-POINT-UAV-DEPART] "
                                    << Simulator::Now().GetSeconds()
                                    << " | nodeId=" << nodeId
                                    << " | confidence=" << state.confidence
                                    << " | totalFrags=" << state.fragments.fragments.size()
                                    << " | fromUAV=" << state.fragmentsReceivedFromUav
                                    << " | fromPeers=" << state.fragmentsReceivedFromPeers
                                    << " | fragments=";
                                for (const auto &p : state.fragments.fragments)
                                {
                                    *ns3::wsn::scenario4::params::g_resultFileStream << p.first << " ";
                                }
                                *ns3::wsn::scenario4::params::g_resultFileStream << "\n";
                            }
                        }

                        // Cooperate whenever the node is incomplete — regardless of confidence.
                        // The old belowReliableReconstruction gate (confidence < COOPERATION_THRESHOLD=0.3)
                        // was incorrectly suppressing cooperation for nodes with 2+ fragments
                        // (confidence ≈ 0.37), leaving zero-fragment cell-mates without help.
                        //
                        // Reset the flag so that subsequent UAV fragments (from later broadcast
                        // cycles) can re-trigger cooperation — each new fragment subset is
                        // more complete and worth spreading to cell-mates.
                        state.cooperationTimeoutScheduled = false;
                        if (state.cooperationEnabled && state.cellId >= 0 && !hasAllFragments)
                        {
                            const double staggerDelay =
                                ComputeCellCooperationStaggerDelay(nodeId, state.cellId);
                            NS_LOG_INFO("Node " << nodeId << " cooperation timeout triggered"
                                       << " | confidence=" << state.confidence
                                       << " | frags=" << state.fragments.fragments.size()
                                       << "/" << state.expectedFragmentCount
                                       << " | level_stagger=" << staggerDelay << "s");
                            ScheduleFragmentSharingRequest(nodeId, state.cellId);
                        }
                    });
                }
            }
            break;
            
        case PACKET_TYPE_COOPERATION:
            NS_LOG_DEBUG("Node " << nodeId << " received COOPERATION packet");
            state.cooperationPacketsReceived++;
            state.cooperationRequestsReceived++;
            state.lastCooperationTime = Simulator::Now().GetSeconds();
            // Handle cooperation (will be implemented in cell-cooperation module)
            break;
            
        default:
            NS_LOG_WARN("Node " << nodeId << " received unknown packet type");
            break;
    }
}

GlobalTopology
BuildTopologySnapshot()
{
    NS_LOG_FUNCTION_NOARGS();
    
    GlobalTopology topology;
    topology.timestamp = Simulator::Now().GetSeconds();
    
    for (const auto& entry : g_groundNetworkPerNode) {
        uint32_t nodeId = entry.first;
        const GroundNetworkState& state = entry.second;
        
        NodeInfo info;
        info.nodeId = nodeId;
        info.neighbors = state.neighbors;
        info.avgConfidence = state.confidence;
        info.packetCount = state.packetCount;
        
        // Get position from node
        Ptr<Node> node = NodeList::GetNode(nodeId);
        if (node) {
            Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
            if (mobility) {
                info.position = mobility->GetPosition();
            }
        }
        
        topology.nodes[nodeId] = info;
    }
    
    NS_LOG_INFO("Built topology snapshot with " << topology.nodes.size() 
                << " nodes at t=" << topology.timestamp);
    
    return topology;
}

void
SendTopologyToBS()
{
    NS_LOG_FUNCTION_NOARGS();

    // Cache topology snapshot for pull-based access by BS
    g_latestTopologySnapshot = BuildTopologySnapshot();
    g_hasLatestTopologySnapshot = true;
    NS_LOG_INFO("Topology cached locally with " << g_latestTopologySnapshot.nodes.size() << " nodes");
}

const GlobalTopology*
GetLatestTopologySnapshotPtr()
{
    if (!g_hasLatestTopologySnapshot)
    {
        return nullptr;
    }
    return &g_latestTopologySnapshot;
}

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3
