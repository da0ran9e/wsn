#ifndef SCENARIO4_SCHEDULER_H
#define SCENARIO4_SCHEDULER_H

#include "scenario4-config.h"

namespace ns3 {
namespace wsn {
namespace scenario4 {

void ScheduleScenario4Events(const Scenario4RunConfig& config);

void ScheduleSingleScenario4Event(const Scenario4RunConfig& config);


} // namespace scenario4
} // namespace wsn
} // namespace ns3

#endif