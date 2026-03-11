#include "scenario5-scheduler.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "../../../model/routing/scenario5/ground-node-routing/ground-node-routing.h"
#include "../../../model/routing/scenario5/ground-node-routing/startup-phase.h"
#include "../../../model/routing/scenario5/node-routing.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Scenario5Scheduler");

namespace wsn {
namespace scenario5 {

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
} // namespace

void
ScheduleScenario5Events(const Scenario5RunConfig& config)
{
    Simulator::Schedule(Seconds(config.startupPhaseDuration), &routing::RunStartupPhase);
    Simulator::Schedule(Seconds(config.startupPhaseDuration + 0.01), &routing::TickBaseStationControl);
    Simulator::Schedule(Seconds(config.startupPhaseDuration + 0.1), &routing::InitializeUavFlight);
    Simulator::Schedule(Seconds(config.startupPhaseDuration + 0.2), &routing::InitializeUavBroadcast);
    Simulator::Schedule(Seconds(config.startupPhaseDuration + 0.3), &routing::InitializeCellCooperationTimeout);

    Simulator::Schedule(Seconds(config.startupPhaseDuration + 0.5),
                        &SchedulePeriodicTopologyTick,
                        1.0,
                        config.simTime);

    Simulator::Stop(Seconds(config.simTime));
    NS_LOG_INFO("Scenario5 events scheduled");
}

void
ScheduleSingleScenario5Event(const Scenario5RunConfig& config)
{
    Simulator::Schedule(Seconds(config.startupPhaseDuration + 0.1), &routing::InitializeUavFlight);
    Simulator::Schedule(Seconds(config.startupPhaseDuration + 0.2), &routing::InitializeUavBroadcast);
    Simulator::Schedule(Seconds(config.startupPhaseDuration + 0.3), &routing::InitializeCellCooperationTimeout);
    Simulator::Stop(Seconds(config.simTime));
    NS_LOG_INFO("Single Scenario5 event scheduled");
}

} // namespace scenario5
} // namespace wsn
} // namespace ns3
