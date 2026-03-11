/*
 * Scenario 5 - API Runner Implementation
 */

#include "scenario5-api.h"
#include "scenario5-params.h"
#include "scenario5-scheduler.h"
#include "scenario5-metrics.h"
#include "scenario5-visualizer.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/mobility-helper.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/internet-stack-helper.h"
#include "../../../model/routing/scenario5/ground-node-routing/ground-node-routing.h"
#include "../../../model/routing/scenario5/node-routing.h"
#include "../../../model/routing/scenario5/uav-node-routing/uav-node-routing.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Scenario5Api");

namespace wsn {
namespace scenario5 {

Scenario5Runner::Scenario5Runner(const Scenario5RunConfig& config)
    : m_config(config)
{
    NS_LOG_FUNCTION(this);

    // Set random seed for reproducibility
    RngSeedManager::SetSeed(m_config.seed);
    RngSeedManager::SetRun(m_config.runId);
}

Scenario5Runner::~Scenario5Runner()
{
    NS_LOG_FUNCTION(this);
}

void
Scenario5Runner::Build()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Building Scenario 5 network...");

    BuildGroundNetwork();
    BuildBaseStation();
    BuildUavNodes();

    InstallProtocolStack();

    // Use scenario5 routing layers
    routing::InitializeGroundNodeRouting(m_groundNodes, m_config.numFragments);
    routing::InitializeBaseStation(m_bsNode->GetId());

    for (uint32_t i = 0; i < m_uavNodes.GetN(); ++i)
    {
        routing::InitializeUavRouting(m_uavNodes.Get(i));
    }
}

void
Scenario5Runner::BuildGroundNetwork()
{
    NS_LOG_FUNCTION(this);

    uint32_t numNodes = m_config.gridSize * m_config.gridSize;
    m_groundNodes.Create(numNodes);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(m_config.gridSpacing),
                                  "DeltaY",
                                  DoubleValue(m_config.gridSpacing),
                                  "GridWidth",
                                  UintegerValue(m_config.gridSize),
                                  "LayoutType",
                                  StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(m_groundNodes);

    NS_LOG_INFO("Ground network: " << numNodes << " nodes in " << m_config.gridSize << "x" << m_config.gridSize
                                   << " grid");
}

void
Scenario5Runner::BuildBaseStation()
{
    NS_LOG_FUNCTION(this);

    m_bsNode = CreateObject<Node>();

    Ptr<MobilityModel> mobility = CreateObject<ConstantPositionMobilityModel>();
    mobility->SetPosition(Vector(m_config.bsPositionX, m_config.bsPositionY, m_config.bsPositionZ));
    m_bsNode->AggregateObject(mobility);

    NS_LOG_INFO("Base station positioned at (" << m_config.bsPositionX << ", " << m_config.bsPositionY << ", "
                                               << m_config.bsPositionZ << ")");
}

void
Scenario5Runner::BuildUavNodes()
{
    NS_LOG_FUNCTION(this);

    m_uavNodes.Create(m_config.numUavs);
    for (uint32_t i = 0; i < m_uavNodes.GetN(); ++i)
    {
        Ptr<Node> uav = m_uavNodes.Get(i);
        Ptr<MobilityModel> uavMobility = CreateObject<ConstantPositionMobilityModel>();
        uavMobility->SetPosition(Vector(m_config.bsPositionX, m_config.bsPositionY, m_config.uavAltitude));
        uav->AggregateObject(uavMobility);
    }

    NS_LOG_INFO("UAV nodes created: " << m_uavNodes.GetN() << " (initial position: " << m_config.bsPositionX << ", "
                                      << m_config.bsPositionY << ", " << m_config.uavAltitude << ")");
}

void
Scenario5Runner::InstallProtocolStack()
{
    NS_LOG_FUNCTION(this);

    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(wifiChannel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);

    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer groundDevices = wifi.Install(wifiPhy, wifiMac, m_groundNodes);
    NetDeviceContainer bsDevice = wifi.Install(wifiPhy, wifiMac, m_bsNode);

    NetDeviceContainer uavDevice;
    if (m_uavNodes.GetN() > 0)
    {
        uavDevice = wifi.Install(wifiPhy, wifiMac, m_uavNodes);
    }

    (void)groundDevices;
    (void)bsDevice;
    (void)uavDevice;

    InternetStackHelper internet;
    internet.Install(m_groundNodes);
    internet.Install(m_bsNode);
    if (m_uavNodes.GetN() > 0)
    {
        internet.Install(m_uavNodes);
    }

    NS_LOG_INFO("Protocol stack installed");
}

void
Scenario5Runner::Schedule()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Scheduling simulation events...");
    ScheduleSingleScenario5Event(m_config);

    NS_LOG_INFO("Events scheduled for " << m_config.simTime << " seconds");
}

void
Scenario5Runner::Run()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Running simulation...");

    Simulator::Run();
    ReportScenario5Metrics();

    NS_LOG_INFO("Simulation complete");
}

} // namespace scenario5
} // namespace wsn
} // namespace ns3
