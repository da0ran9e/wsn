#include "scenario4-scheduler.h"
#include "scenario4-visualizer.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "../../../model/routing/scenario4/node-routing.h"
#include "../../../model/routing/scenario4/base-station-node/base-station-node.h"
#include "../../../model/routing/scenario4/ground-node-routing/startup-phase.h"
#include "../../../model/routing/scenario4/ground-node-routing/ground-node-routing.h"
#include <iomanip>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Scenario4Scheduler");

namespace wsn {
namespace scenario4 {

namespace
{
void
SchedulePeriodicTopologyTick(double intervalSec, double endSec)
{
    if (Simulator::Now().GetSeconds() + intervalSec >= endSec)
    {
        return;
    }

    Simulator::Schedule(Seconds(intervalSec), [] {
        routing::SendTopologyToBS();
        routing::TickBaseStationControl();
    });

    Simulator::Schedule(Seconds(intervalSec), &SchedulePeriodicTopologyTick, intervalSec, endSec);
}

void
LogSuspiciousConfidenceSnapshot()
{
    if (!params::g_resultFileStream)
    {
        return;
    }

    const auto& suspiciousNodes = routing::GetSuspiciousNodes();
    auto& os = *params::g_resultFileStream;

    os << "\n[EVENT] " << std::fixed << std::setprecision(3) << Simulator::Now().GetSeconds()
       << " | event=SuspiciousConfidenceSnapshot | ";

    for (uint32_t nodeId : suspiciousNodes)
    {
        auto it = routing::g_groundNetworkPerNode.find(nodeId);
        const double confidence = (it != routing::g_groundNetworkPerNode.end()) ? it->second.confidence : 0.0;
        os << nodeId << "(" << std::fixed << std::setprecision(3) << confidence << ") ";
    }
    os << std::endl;
}

void
SchedulePeriodicSuspiciousConfidenceLog(double intervalSec, double endSec)
{
    if (intervalSec <= 0.0)
    {
        return;
    }

    if (Simulator::Now().GetSeconds() + intervalSec >= endSec)
    {
        return;
    }

    Simulator::Schedule(Seconds(intervalSec), &LogSuspiciousConfidenceSnapshot);
    Simulator::Schedule(
        Seconds(intervalSec),
        &SchedulePeriodicSuspiciousConfidenceLog,
        intervalSec,
        endSec);
}
} // namespace

void
ScheduleScenario4Events(const Scenario4RunConfig& config)
{
    routing::SetScenarioStopTime(config.simTime);

    // Step 1: Ground nodes startup discovery phase
    Simulator::Schedule(Seconds(config.startupPhaseDuration), &routing::RunStartupPhase);

    // Step 2: Run deferred BS post-init tasks (fragment generation + UAV path planning).
    Simulator::Schedule(Seconds(config.startupPhaseDuration + 0.01),
                        &routing::RunBaseStationPostInitTasks);
    
    // Step 3: BS control tick for latest shared topology.
    Simulator::Schedule(Seconds(config.startupPhaseDuration + 0.02), &routing::TickBaseStationControl);
    
    // Step 4: Initialize UAV flight - UAVs start flying to waypoints
    Simulator::Schedule(Seconds(config.startupPhaseDuration + 0.1), &routing::InitializeUavFlight);
    
    // Step 5: Initialize UAV fragment broadcast
    Simulator::Schedule(Seconds(config.startupPhaseDuration + 0.2), &routing::InitializeUavBroadcast);
    
    // Step 6: Initialize cell cooperation timeout (force cooperation after delay)
    Simulator::Schedule(Seconds(config.startupPhaseDuration + 0.3), &routing::InitializeCellCooperationTimeout);
    
    // Step 7: Periodic topology updates and BS control ticks
    Simulator::Schedule(Seconds(config.startupPhaseDuration + 0.5),
                        &SchedulePeriodicTopologyTick,
                        1.0,
                        config.simTime);

    // Step 8: Global periodic suspicious-node confidence snapshots.
    Simulator::Schedule(Seconds(config.startupPhaseDuration + 0.5),
                        &SchedulePeriodicSuspiciousConfidenceLog,
                        config.suspiciousConfidenceLogInterval,
                        config.simTime);
    
    Simulator::Stop(Seconds(config.simTime));
    NS_LOG_INFO("Scenario4 events scheduled");
}

void
ScheduleSingleScenario4Event(const Scenario4RunConfig& config)
{
    routing::SetScenarioStopTime(config.simTime);

    Simulator::Schedule(Seconds(config.startupPhaseDuration + 0.01), &routing::RunBaseStationPostInitTasks);
    Simulator::Schedule(Seconds(config.startupPhaseDuration + 0.1), &routing::InitializeUavFlight);
    Simulator::Schedule(Seconds(config.startupPhaseDuration + 0.2), &routing::InitializeUavBroadcast);
    Simulator::Schedule(Seconds(config.startupPhaseDuration + 0.3), &routing::InitializeCellCooperationTimeout);
    Simulator::Schedule(Seconds(config.startupPhaseDuration + 0.5),
                        &SchedulePeriodicSuspiciousConfidenceLog,
                        config.suspiciousConfidenceLogInterval,
                        config.simTime);
    Simulator::Stop(Seconds(config.simTime));
    NS_LOG_INFO("Single Scenario4 event scheduled");
}

} // namespace scenario4
} // namespace wsn
} // namespace ns3
