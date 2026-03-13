/*
 * Scenario 4 - API Runner Implementation
 */

#include "scenario4-api.h"
#include "scenario4-params.h"
#include "scenario4-scheduler.h"
#include "scenario4-metrics.h"
#include "scenario4-visualizer.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/mobility-helper.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/waypoint-mobility-model.h"
#include "ns3/callback.h"
#include "../../../helper/cc2420-helper.h"
#include "../../../model/radio/cc2420/cc2420-net-device.h"
#include "../../../model/radio/cc2420/cc2420-phy.h"
#include "../../../model/propagation/cc2420-spectrum-propagation-loss-model.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Scenario4Api");

namespace wsn {
namespace scenario4 {

namespace
{
void
HandleRadioDebugTrace(Ptr<wsn::Cc2420NetDevice> dev,
                      std::string eventName,
                      Ptr<const Packet> packet)
{
    if (!dev || !ns3::wsn::scenario4::params::g_resultFileStream)
    {
        return;
    }

    const uint32_t packetSize = packet ? packet->GetSize() : 0u;
    const uint32_t nodeId = dev->GetNode() ? dev->GetNode()->GetId() : 0u;

    std::string link = std::to_string(nodeId) + "-D-?";
    std::string reason;
    std::string meta;

    const std::size_t firstBar = eventName.find('|');
    if (firstBar != std::string::npos)
    {
        std::string head = eventName.substr(0, firstBar);
        if (head.rfind("MAC::", 0) == 0 || head.rfind("PHY::", 0) == 0)
        {
            head = head.substr(5);
        }

        if (head.find("-D-") != std::string::npos)
        {
            link = head;
        }

        const std::size_t secondBar = eventName.find('|', firstBar + 1);
        if (secondBar != std::string::npos)
        {
            reason = eventName.substr(firstBar + 1, secondBar - firstBar - 1);
            meta = eventName.substr(secondBar + 1);
        }
        else
        {
            reason = eventName.substr(firstBar + 1);
        }
    }

    // Keep only the aggregated contact-window summary (one line per TX attempt).
    if (reason != "DropContactWindowSummary")
    {
        return;
    }

    *ns3::wsn::scenario4::params::g_resultFileStream
        << "\n[EVENT] " << Simulator::Now().GetSeconds()
        << " | event=ContactDrop"
        << " | link=" << link
        << " | packetSize=" << packetSize;

    if (!meta.empty())
    {
        *ns3::wsn::scenario4::params::g_resultFileStream
            << " | " << meta;
    }

    *ns3::wsn::scenario4::params::g_resultFileStream << std::endl;
}
} // namespace

// External functions from routing layer (implemented in wsn module)
namespace routing {
    extern void InitializeGroundNodeRouting(NodeContainer nodes, uint32_t numFragments);
    extern void AttachGroundRxCallbacks(NodeContainer nodes);
    extern void SendTopologyToBS();
    extern void InitializeBaseStation(uint32_t nodeId);
    extern void InitializeUavRouting(Ptr<Node> uavNode);
}

Scenario4Runner::Scenario4Runner(const Scenario4RunConfig& config)
    : m_config(config)
{
    NS_LOG_FUNCTION(this);
    
    // Set random seed for reproducibility
    RngSeedManager::SetSeed(m_config.seed);
    RngSeedManager::SetRun(m_config.runId);
}

Scenario4Runner::~Scenario4Runner()
{
    NS_LOG_FUNCTION(this);
}

void
Scenario4Runner::Build()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Building Scenario 4 network...");
    
    BuildGroundNetwork();
    BuildBaseStation();
    BuildUavNodes();

    InstallProtocolStack();
    
    // Initialize routing layers
    routing::InitializeGroundNodeRouting(m_groundNodes, m_config.numFragments);
    routing::InitializeBaseStation(m_bsNode->GetId());

    // Initialize UAV routing callback endpoints
    for (uint32_t i = 0; i < m_uavNodes.GetN(); ++i)
    {
        routing::InitializeUavRouting(m_uavNodes.Get(i));
    }
}

void
Scenario4Runner::BuildGroundNetwork()
{
    NS_LOG_FUNCTION(this);
    
    // Create ground nodes in grid
    uint32_t numNodes = m_config.gridSize * m_config.gridSize;
    m_groundNodes.Create(numNodes);
    
    // Grid mobility
    MobilityHelper mobility;
    mobility.SetPositionAllocator(
        "ns3::GridPositionAllocator",
        "MinX", DoubleValue(0.0),
        "MinY", DoubleValue(0.0),
        "DeltaX", DoubleValue(m_config.gridSpacing),
        "DeltaY", DoubleValue(m_config.gridSpacing),
        "GridWidth", UintegerValue(m_config.gridSize),
        "LayoutType", StringValue("RowFirst")
    );
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(m_groundNodes);
    
    NS_LOG_INFO("Ground network: " << numNodes << " nodes in " 
                << m_config.gridSize << "x" << m_config.gridSize << " grid");
}

void
Scenario4Runner::BuildBaseStation()
{
    NS_LOG_FUNCTION(this);
    
    // Create BS node
    m_bsNode = CreateObject<Node>();
    
    // Position BS far from network
    Ptr<MobilityModel> mobility = CreateObject<ConstantPositionMobilityModel>();
    mobility->SetPosition(Vector(
        m_config.bsPositionX,
        m_config.bsPositionY,
        m_config.bsPositionZ
    ));
    m_bsNode->AggregateObject(mobility);
    
    NS_LOG_INFO("Base station positioned at (" 
                << m_config.bsPositionX << ", "
                << m_config.bsPositionY << ", "
                << m_config.bsPositionZ << ")");
}

void
Scenario4Runner::BuildUavNodes()
{
    NS_LOG_FUNCTION(this);

    // Create UAV nodes (same initial XY position as BS)
    m_uavNodes.Create(m_config.numUavs);
    for (uint32_t i = 0; i < m_uavNodes.GetN(); ++i)
    {
        Ptr<Node> uav = m_uavNodes.Get(i);
        Ptr<WaypointMobilityModel> uavMobility = CreateObject<WaypointMobilityModel>();
        uavMobility->AddWaypoint(ns3::Waypoint(
            Seconds(0.0),
            Vector(m_config.bsPositionX, m_config.bsPositionY, m_config.uavAltitude)));
        uav->AggregateObject(uavMobility);
    }

    NS_LOG_INFO("UAV nodes created: " << m_uavNodes.GetN()
                << " (initial position: " << m_config.bsPositionX << ", "
                << m_config.bsPositionY << ", " << m_config.uavAltitude << ")");
}

void
Scenario4Runner::InstallProtocolStack()
{
    NS_LOG_FUNCTION(this);

    Cc2420Helper cc2420;
    Ptr<SingleModelSpectrumChannel> channel = cc2420.CreateChannel();
    cc2420.SetChannel(channel);

    NetDeviceContainer groundDevices = cc2420.Install(m_groundNodes);
    NetDeviceContainer bsDevices = cc2420.Install(m_bsNode);
    if (m_uavNodes.GetN() > 0)
    {
        NetDeviceContainer uavDevices = cc2420.Install(m_uavNodes);

        Ptr<propagation::Cc2420SpectrumPropagationLossModel> propagationModel =
            CreateObject<propagation::Cc2420SpectrumPropagationLossModel>();

        auto bindPropagation = [&propagationModel](const NetDeviceContainer& devices) {
            for (uint32_t i = 0; i < devices.GetN(); ++i)
            {
                Ptr<wsn::Cc2420NetDevice> dev = DynamicCast<wsn::Cc2420NetDevice>(devices.Get(i));
                if (!dev)
                {
                    continue;
                }
                Ptr<wsn::Cc2420Phy> phy = dev->GetPhy();
                if (phy)
                {
                    phy->SetPropagationLossModel(propagationModel);
                }

                // dev->SetDebugPacketTraceCallback(
                //     MakeBoundCallback(&HandleRadioDebugTrace, dev));
            }
        };

        bindPropagation(groundDevices);
        bindPropagation(bsDevices);
        bindPropagation(uavDevices);
    }
    else
    {
        Ptr<propagation::Cc2420SpectrumPropagationLossModel> propagationModel =
            CreateObject<propagation::Cc2420SpectrumPropagationLossModel>();

        for (uint32_t i = 0; i < groundDevices.GetN(); ++i)
        {
            Ptr<wsn::Cc2420NetDevice> dev = DynamicCast<wsn::Cc2420NetDevice>(groundDevices.Get(i));
            if (dev && dev->GetPhy())
            {
                dev->GetPhy()->SetPropagationLossModel(propagationModel);
                // dev->SetDebugPacketTraceCallback(
                //     MakeBoundCallback(&HandleRadioDebugTrace, dev));
            }
        }
        for (uint32_t i = 0; i < bsDevices.GetN(); ++i)
        {
            Ptr<wsn::Cc2420NetDevice> dev = DynamicCast<wsn::Cc2420NetDevice>(bsDevices.Get(i));
            if (dev && dev->GetPhy())
            {
                dev->GetPhy()->SetPropagationLossModel(propagationModel);
                // dev->SetDebugPacketTraceCallback(
                //     MakeBoundCallback(&HandleRadioDebugTrace, dev));
            }
        }
    }

    routing::AttachGroundRxCallbacks(m_groundNodes);

    NS_LOG_INFO("CC2420 protocol stack installed");
}

void
Scenario4Runner::Schedule()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Scheduling simulation events...");
    ScheduleSingleScenario4Event(m_config);
    
    NS_LOG_INFO("Events scheduled for " << m_config.simTime << " seconds");
}

void
Scenario4Runner::Run()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Running simulation...");
    
    Simulator::Run();
    ReportScenario4Metrics();
    
    NS_LOG_INFO("Simulation complete");
}

} // namespace scenario4
} // namespace wsn
} // namespace ns3
