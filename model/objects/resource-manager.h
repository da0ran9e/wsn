#pragma once
#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H
#include <string>
#include <map>
#include <memory>
#include "wsn-object.h"

namespace ns3 {
namespace wsn {
class ResourceManager : public ns3::wsn::WsnObject
{   
public:
    explicit ResourceManager(const std::string& name) : WsnObject("ResourceManager", name),
        collectTraceInfo(false),
        ramSize(0.0),
        flashSize(0.0),
        flashWriteCost(0.0),
        flashReadCost(0.0),
        imageSize(0.0),
        cpuPowerSpeedLevelNames(""),
        cpuPowerPerLevel(""),
        cpuSpeedPerLevel(""),
        cpuInitialPowerLevel(-1),
        sigmaCPUClockDrift(0.00003),
        initialEnergy(18720),
        baselineNodePower(6),
        periodicEnergyCalculationInterval(1000) 
    {
    }
    ~ResourceManager() override = default;

    bool SetProperty(const std::string &key, const std::string &value) override;
    void Build(BuildContext& ctx) override;

private:
    bool collectTraceInfo; // = default (false);
	double ramSize; // = default (0.0);			//in kB
	double flashSize; // = default (0.0);		//in kB
	double flashWriteCost; // = default (0.0);	//per kB
	double flashReadCost; // = default (0.0);	//per kB
	double imageSize; // = default (0.0);		//the space that the OS (e.g. Contiki or TinyOS) occupies in the flash

	std::string cpuPowerSpeedLevelNames; // = default ("");
	std::string cpuPowerPerLevel; // = default ("");	//spent energy per time unit
	std::string cpuSpeedPerLevel; // = default ("");
	int cpuInitialPowerLevel; // = default (-1);	// index for the cpuPowerLevels array
	double sigmaCPUClockDrift; // = default (0.00003);	// the standard deviation of the Drift of the CPU

	double initialEnergy; // = default (18720);
	// energy of the node in Joules, default value corresponds to two AA batteries
	// source http://www.allaboutbatteries.com/Energy-tables.html

	double baselineNodePower; // = default (6);	// the baseline/minimum power consumption in mWatts
	double periodicEnergyCalculationInterval; // = default (1000);	// maximum interval for periodic energy calculation, in msec  
};
} // namespace wsn
} // namespace ns3  
#endif // RESOURCE_MANAGER_H