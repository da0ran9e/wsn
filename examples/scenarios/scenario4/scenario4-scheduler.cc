#include "scenario4-scheduler.h"
#include "scenario4-visualizer.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "../../../model/routing/scenario4/node-routing.h"
#include "../../../model/routing/scenario4/ground-node-routing/startup-phase.h"
#include "../../../model/routing/scenario4/ground-node-routing/ground-node-routing.h"

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
} // namespace

void
ScheduleScenario4Events(const Scenario4RunConfig& config)
{
    Simulator::Schedule(Seconds(config.startupPhaseDuration), &routing::RunStartupPhase);
    Simulator::Schedule(Seconds(config.startupPhaseDuration + 0.005), &DumpScenario4CellFormationSnapshot);
    Simulator::Schedule(Seconds(config.startupPhaseDuration + 0.01), &routing::TickBaseStationControl);
    Simulator::Schedule(Seconds(config.startupPhaseDuration + 0.5),
                        &SchedulePeriodicTopologyTick,
                        1.0,
                        config.simTime);
    Simulator::Stop(Seconds(config.simTime));
    NS_LOG_INFO("Scenario4 events scheduled");
}

} // namespace scenario4
} // namespace wsn
} // namespace ns3
