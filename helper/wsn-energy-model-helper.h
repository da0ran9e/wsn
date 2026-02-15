#ifndef WSN_ENERGY_MODEL_HELPER_H
#define WSN_ENERGY_MODEL_HELPER_H

#include "ns3/energy-model-helper.h"
#include "ns3/object-factory.h"

namespace ns3 {
namespace wsn {

class WsnEnergyModelHelper : public ns3::DeviceEnergyModelHelper
{
public:
    WsnEnergyModelHelper();
    void Set(const std::string &name, const AttributeValue &value);

private:
    ns3::Ptr<ns3::energy::DeviceEnergyModel>
    DoInstall(ns3::Ptr<ns3::NetDevice> device,
              ns3::Ptr<ns3::energy::EnergySource> source) const override;

    ObjectFactory m_factory;
};

} // namespace wsn
} // namespace ns3

#endif // WSN_ENERGY_MODEL_HELPER_H