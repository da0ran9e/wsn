/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Scenario1: Grid network setup with corner and center broadcasts
 */

#include "scenario1.h"
#include "../../model/routing/scenario1/ground-node-routing.h"
#include "../../model/routing/scenario1/uav-node-routing.h"

#include "ns3/mobility-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/cc2420-helper.h"
#include "ns3/cc2420-net-device.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

#include <vector>
#include <algorithm>

namespace ns3
{
namespace wsn
{

NS_LOG_COMPONENT_DEFINE("Scenario1");

Ptr<SpectrumChannel>
SetupScenario1Network(const Scenario1Config& config,
                      NodeContainer& nodes,
                      NetDeviceContainer& devices)
{
    NS_LOG_FUNCTION_NOARGS();

    // Create nodes
    const uint32_t numNodes = config.gridSize * config.gridSize;
    nodes.Create(numNodes);

    // Setup mobility (grid layout)
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(0.0),
                                  "MinY", DoubleValue(0.0),
                                  "DeltaX", DoubleValue(config.spacing),
                                  "DeltaY", DoubleValue(config.spacing),
                                  "GridWidth", UintegerValue(config.gridSize),
                                  "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // Setup CC2420 devices
    Cc2420Helper cc2420;
    Ptr<SingleModelSpectrumChannel> channel = cc2420.CreateChannel();
    cc2420.SetChannel(channel);
    cc2420.SetPhyAttribute("TxPower", DoubleValue(config.txPowerDbm));
    cc2420.SetPhyAttribute("RxSensitivity", DoubleValue(config.rxSensitivityDbm));

    devices = cc2420.Install(nodes);

    // Initialize ground node network layer routing
    scenario1::InitializeGroundNodeRouting(devices, config.packetSize);

    NS_LOG_INFO("Scenario1 network setup complete: " << numNodes << " nodes in "
                << config.gridSize << "x" << config.gridSize << " grid");

    return channel;
}

void
ScheduleScenario1Traffic(const Scenario1Config& config, NodeContainer nodes)
{
    NS_LOG_FUNCTION_NOARGS();

    // Delegate to ground node network layer routing module
    scenario1::ScheduleGroundTrafficPattern(nodes,
                                           config.gridSize,
                                           config.packetSize,
                                           config.firstTxTimeSeconds);
}

uint32_t
GetScenario1TotalTx()
{
    return scenario1::GetGroundTotalTransmissions();
}

uint32_t
GetScenario1TotalRx()
{
    return scenario1::GetGroundTotalReceptions();
}

uint32_t
GetScenario1UavTotalTx()
{
    return scenario1::GetUavTotalTransmissions();
}

uint32_t
GetScenario1UavTotalRx()
{
    return scenario1::GetUavTotalReceptions();
}

uint32_t
ScheduleScenario1UavTraffic(const Scenario1Config& groundConfig,
                            const Scenario1UavConfig& uavConfig,
                            NodeContainer& nodes,
                            Ptr<SpectrumChannel> channel)
{
    NS_LOG_FUNCTION_NOARGS();

    // Validate UAV parameters
    if (!scenario1::ParameterCalculators::ValidateAltitude(uavConfig.uavAltitude))
    {
        NS_FATAL_ERROR("Invalid UAV altitude: " << uavConfig.uavAltitude 
                       << " (min: " << scenario1::UAVParams::MIN_ALTITUDE 
                       << ", max: " << scenario1::UAVParams::MAX_ALTITUDE << ")");
    }
    if (!scenario1::ParameterCalculators::ValidateSpeed(uavConfig.uavSpeed))
    {
        NS_FATAL_ERROR("Invalid UAV speed: " << uavConfig.uavSpeed 
                       << " (min: " << scenario1::UAVParams::MIN_SPEED 
                       << ", max: " << scenario1::UAVParams::MAX_SPEED << ")");
    }
    if (!scenario1::ParameterCalculators::ValidateBroadcastInterval(uavConfig.uavBroadcastInterval))
    {
        NS_FATAL_ERROR("Invalid broadcast interval: " << uavConfig.uavBroadcastInterval 
                       << " (min: " << scenario1::UAVParams::MIN_BROADCAST_INTERVAL << ")");
    }

    // Create UAV node
    Ptr<Node> uavNode = CreateObject<Node>();
    
    // Setup UAV mobility - waypoint trajectory
    Ptr<WaypointMobilityModel> uavMobility = CreateObject<WaypointMobilityModel>();
    uavNode->AggregateObject(uavMobility);

    // Calculate grid boundaries
    double gridWidth = scenario1::ParameterCalculators::CalculateGridDimension(
        groundConfig.gridSize, groundConfig.spacing);
    double gridHeight = scenario1::ParameterCalculators::CalculateGridDimension(
        groundConfig.gridSize, groundConfig.spacing);

    // UAV trajectory: fly from far left to far right, then return
    double startX = -scenario1::UAVParams::APPROACH_DISTANCE;
    double endX = gridWidth + scenario1::UAVParams::APPROACH_DISTANCE;
    double centerY = gridHeight / 2.0;

    // Initial position
    uavMobility->AddWaypoint(Waypoint(Seconds(0.0), 
                                     Vector(startX, centerY, uavConfig.uavAltitude)));

    // Fly across the grid
    double flyTime = scenario1::ParameterCalculators::CalculateFlightTime(
        endX - startX, uavConfig.uavSpeed);
    uavMobility->AddWaypoint(Waypoint(
        Seconds(groundConfig.firstTxTimeSeconds + flyTime), 
        Vector(endX, centerY, uavConfig.uavAltitude)));

    // Return journey
    double returnTime = groundConfig.firstTxTimeSeconds + flyTime + 
                       scenario1::ParameterCalculators::CalculateFlightTime(
                           endX - startX, uavConfig.uavSpeed);
    uavMobility->AddWaypoint(Waypoint(Seconds(returnTime), 
                                     Vector(startX, centerY, uavConfig.uavAltitude)));

    // Install CC2420 device on UAV
    Cc2420Helper cc2420Uav;
    cc2420Uav.SetChannel(channel);
    cc2420Uav.SetPhyAttribute("TxPower", DoubleValue(groundConfig.txPowerDbm));
    cc2420Uav.SetPhyAttribute("RxSensitivity", DoubleValue(groundConfig.rxSensitivityDbm));
    
    NetDeviceContainer uavDevices = cc2420Uav.Install(uavNode);
    
    // Initialize UAV routing
    scenario1::InitializeUavNodeRouting(uavDevices, groundConfig.packetSize);

    // Add UAV to node container
    uint32_t uavNodeId = nodes.GetN();
    nodes.Add(uavNode);
    
    // Schedule periodic UAV broadcasts
    scenario1::ScheduleUavPeriodicBroadcasts(nodes, uavNodeId, groundConfig.packetSize,
                                            groundConfig.firstTxTimeSeconds,
                                            returnTime,
                                            uavConfig.uavBroadcastInterval);

    NS_LOG_INFO("UAV scheduler: altitude=" << uavConfig.uavAltitude << "m, speed=" 
                << uavConfig.uavSpeed << "m/s, interval=" << uavConfig.uavBroadcastInterval << "s");
    NS_LOG_INFO("UAV flight time: " << flyTime << "s, total time: " << returnTime << "s");

    return uavNodeId;
}

void
GenerateScenario1LogicFragments(uint32_t numFragments, double totalConfidence)
{
    NS_LOG_FUNCTION(numFragments << totalConfidence);
    scenario1::GenerateUavLogicFragments(numFragments, totalConfidence);
}

void
SimulateScenario1LogicBroadcasts(const Scenario1Config& groundConfig,
                                 const Scenario1UavConfig& uavConfig,
                                 uint32_t numFragments)
{
    NS_LOG_FUNCTION_NOARGS();

    // Reset logic state
    scenario1::ResetGroundLogicState();
    scenario1::ResetUavLogicState();
    GenerateScenario1LogicFragments(numFragments, 1.0);

    const uint32_t numNodes = groundConfig.gridSize * groundConfig.gridSize;
    const double gridWidth = scenario1::ParameterCalculators::CalculateGridDimension(
        groundConfig.gridSize, groundConfig.spacing);
    const double gridHeight = scenario1::ParameterCalculators::CalculateGridDimension(
        groundConfig.gridSize, groundConfig.spacing);
    const double centerY = gridHeight / 2.0;

    // Generate ground node positions
    std::vector<Vector> groundPositions;
    groundPositions.reserve(numNodes);
    for (uint32_t r = 0; r < groundConfig.gridSize; ++r)
    {
        for (uint32_t c = 0; c < groundConfig.gridSize; ++c)
        {
            groundPositions.emplace_back(c * groundConfig.spacing, r * groundConfig.spacing, 0.0);
        }
    }

    // Calculate UAV trajectory
    const double startX = -scenario1::UAVParams::APPROACH_DISTANCE;
    const double endX = gridWidth + scenario1::UAVParams::APPROACH_DISTANCE;
    const double pathLength = std::max(1.0, endX - startX);
    const uint32_t numBroadcasts = scenario1::ParameterCalculators::CalculateNumberOfBroadcasts(
        groundConfig.simTimeSeconds,
        uavConfig.uavBroadcastInterval);

    // Simulate UAV broadcasts along trajectory
    for (uint32_t i = 0; i < numBroadcasts; ++i)
    {
        const double alpha = (numBroadcasts > 1) ? static_cast<double>(i) / (numBroadcasts - 1) : 0.0;
        const double x = startX + alpha * pathLength;
        const Vector uavPos(x, centerY, uavConfig.uavAltitude);

        scenario1::SimulateUavLogicBroadcast(i + 1,
                                             uavPos,
                                             groundConfig.txPowerDbm,
                                             groundConfig.rxSensitivityDbm,
                                             groundPositions);
    }

    NS_LOG_INFO("Logic simulation completed: " << numBroadcasts << " broadcasts, "
                << scenario1::GetUavLogicTotalReceptions() << " receptions");
}

uint32_t
GetScenario1LogicBroadcasts()
{
    return scenario1::GetUavLogicTotalBroadcasts();
}

uint32_t
GetScenario1LogicReceptions()
{
    return scenario1::GetUavLogicTotalReceptions();
}

double
GetScenario1LogicNodeConfidence(uint32_t nodeId)
{
    return scenario1::GetGroundNodeLogicConfidence(nodeId);
}

uint32_t
GetScenario1LogicNodeFragments(uint32_t nodeId)
{
    return scenario1::GetGroundNodeLogicFragments(nodeId);
}

bool
GetScenario1LogicNodeAlerted(uint32_t nodeId)
{
    return scenario1::HasGroundNodeLogicAlerted(nodeId);
}

uint32_t
ScheduleScenario1UavFragmentTraffic(const Scenario1Config& groundConfig,
                                    const Scenario1UavConfig& uavConfig,
                                    NodeContainer& nodes,
                                    Ptr<SpectrumChannel> channel,
                                    uint32_t numFragments,
                                    double totalConfidence)
{
    NS_LOG_FUNCTION(numFragments << totalConfidence);

    // Generate network fragments first
    scenario1::GenerateUavNetworkFragments(numFragments, totalConfidence);

    // Call standard UAV traffic setup (which will now use generated fragments)
    uint32_t uavNodeId = ScheduleScenario1UavTraffic(groundConfig, uavConfig, nodes, channel);

    NS_LOG_INFO("UAV fragment scheduler: " << numFragments << " fragments generated, "
                << "total confidence=" << totalConfidence);

    return uavNodeId;
}

double
GetScenario1NetworkNodeConfidence(uint32_t nodeId)
{
    return scenario1::GetGroundNodeNetworkConfidence(nodeId);
}

uint32_t
GetScenario1NetworkNodeFragments(uint32_t nodeId)
{
    return scenario1::GetGroundNodeNetworkFragments(nodeId);
}

bool
GetScenario1NetworkNodeAlerted(uint32_t nodeId)
{
    return scenario1::HasGroundNodeNetworkAlerted(nodeId);
}

} // namespace wsn
} // namespace ns3