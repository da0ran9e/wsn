#ifndef SCENARIO5_SCHEDULER_H
#define SCENARIO5_SCHEDULER_H

#include "scenario5-config.h"

namespace ns3 {
namespace wsn {
namespace scenario5 {

void ScheduleScenario5Events(const Scenario5RunConfig& config);

void ScheduleSingleScenario5Event(const Scenario5RunConfig& config);

} // namespace scenario5
} // namespace wsn
} // namespace ns3

#endif
