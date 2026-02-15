#include "resource-manager.h"

namespace ns3 {
namespace wsn {
bool ResourceManager::SetProperty(const std::string &key, const std::string &value)
{
    if (key == "collectTraceInfo") {
        collectTraceInfo = (value == "true" || value == "1");
    }
    else if (key == "ramSize") {
        ramSize = std::stod(value);
    }
    else if (key == "flashSize") {
        flashSize = std::stod(value);
    }
    else if (key == "flashWriteCost") {
        flashWriteCost = std::stod(value);
    }
    else if (key == "flashReadCost") {
        flashReadCost = std::stod(value);
    }
    else if (key == "imageSize") {
        imageSize = std::stod(value);
    }
    else if (key == "cpuPowerSpeedLevelNames") {
        cpuPowerSpeedLevelNames = value;
    }
    else if (key == "cpuPowerPerLevel") {
        cpuPowerPerLevel = value;
    }
    else if (key == "cpuSpeedPerLevel") {
        cpuSpeedPerLevel = value;
    }
    else if (key == "cpuInitialPowerLevel") {
        cpuInitialPowerLevel = std::stoi(value);
    }
    else if (key == "sigmaCPUClockDrift") {
        sigmaCPUClockDrift = std::stod(value);
    }
    else if (key == "initialEnergy") {
        initialEnergy = std::stod(value);
    }
    else if (key == "baselineNodePower") {
        baselineNodePower = std::stod(value);
    }
    else if (key == "periodicEnergyCalculationInterval") {
        periodicEnergyCalculationInterval = std::stod(value);
    }
    else {
        return false;
    }

    NotifyAttributeChanged(key, value);
    return true;
}

void ResourceManager::Build(BuildContext& ctx)
{
    std::cout << "Building Resource Manager: " << GetInstanceName() << std::endl;

    //WsnObject::Build(ctx);
}

} // namespace wsn
} // namespace ns3