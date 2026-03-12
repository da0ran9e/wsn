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
#include "../../../helper/cc2420-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Scenario4Api");

namespace wsn {
namespace scenario4 {

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
        Ptr<MobilityModel> uavMobility = CreateObject<ConstantPositionMobilityModel>();
        uavMobility->SetPosition(Vector(
            m_config.bsPositionX,
            m_config.bsPositionY,
            m_config.uavAltitude));
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
    (void)groundDevices;
    cc2420.Install(m_bsNode);
    if (m_uavNodes.GetN() > 0)
    {
        cc2420.Install(m_uavNodes);
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
