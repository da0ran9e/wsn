/*
 * Scenario 4 - API Runner Implementation
 */

#include "scenario4-api.h"
#include "scenario4-params.h"
#include "scenario4-scheduler.h"
#include "scenario4-metrics.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/mobility-helper.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/internet-stack-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Scenario4Api");

namespace wsn {
namespace scenario4 {

// External functions from routing layer (implemented in wsn module)
namespace routing {
    extern void InitializeGroundNodeRouting(NodeContainer nodes, uint32_t numFragments);
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

    // Create UAV node (position and routing initialized later)
    m_uavNode = CreateObject<Node>();
    {
        Ptr<MobilityModel> uavMobility = CreateObject<ConstantPositionMobilityModel>();
        uavMobility->SetPosition(Vector(0.0, 0.0, params::DEFAULT_UAV_ALTITUDE));
        m_uavNode->AggregateObject(uavMobility);
    }

    InstallProtocolStack();
    
    // Initialize routing layers
    routing::InitializeGroundNodeRouting(m_groundNodes, m_config.numFragments);
    routing::InitializeBaseStation(m_bsNode->GetId());

    // Initialize UAV routing callback endpoint
    routing::InitializeUavRouting(m_uavNode);
    
    NS_LOG_INFO("Network built: " << m_groundNodes.GetN() << " ground nodes + 1 BS");
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
        params::BS_POSITION_X,
        params::BS_POSITION_Y,
        params::BS_POSITION_Z
    ));
    m_bsNode->AggregateObject(mobility);
    
    NS_LOG_INFO("Base station positioned at (" 
                << params::BS_POSITION_X << ", "
                << params::BS_POSITION_Y << ", "
                << params::BS_POSITION_Z << ")");
}

void
Scenario4Runner::InstallProtocolStack()
{
    NS_LOG_FUNCTION(this);
    
    // WiFi configuration (placeholder - will integrate with routing layer)
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(wifiChannel.Create());
    
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    
    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");
    
    // Install on ground nodes
    NetDeviceContainer groundDevices = wifi.Install(wifiPhy, wifiMac, m_groundNodes);
    
    // Install on BS (for control plane, though it uses callbacks)
    NetDeviceContainer bsDevice = wifi.Install(wifiPhy, wifiMac, m_bsNode);

    // Install on UAV if already created
    NetDeviceContainer uavDevice;
    if (m_uavNode)
    {
        uavDevice = wifi.Install(wifiPhy, wifiMac, m_uavNode);
    }
    
    // Internet stack
    InternetStackHelper internet;
    internet.Install(m_groundNodes);
    internet.Install(m_bsNode);
    if (m_uavNode)
    {
        internet.Install(m_uavNode);
    }
    
    NS_LOG_INFO("Protocol stack installed");
}

void
Scenario4Runner::Schedule()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Scheduling simulation events...");
    ScheduleScenario4Events(m_config);
    
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
