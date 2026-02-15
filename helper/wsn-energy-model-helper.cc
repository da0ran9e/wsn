#include "wsn-energy-model-helper.h"
#include "../model/resource/wsn-energy-model.h"

#include "ns3/log.h"

namespace ns3 {
namespace wsn {

NS_LOG_COMPONENT_DEFINE("WsnEnergyModelHelper");

WsnEnergyModelHelper::WsnEnergyModelHelper()
{
    // IMPORTANT: bind factory to your model
    m_factory.SetTypeId("ns3::wsn::WsnEnergyModel");
}

void
WsnEnergyModelHelper::Set(const std::string &name,
                          const AttributeValue &value)
{
    m_factory.Set(name, value);
}

ns3::Ptr<ns3::energy::DeviceEnergyModel>
WsnEnergyModelHelper::DoInstall(ns3::Ptr<ns3::NetDevice> device,
                                ns3::Ptr<ns3::energy::EnergySource> source) const
{
    NS_LOG_FUNCTION(this << device << source);

    ns3::Ptr<ns3::wsn::WsnEnergyModel> model =
        m_factory.Create<ns3::wsn::WsnEnergyModel>();

    model->SetEnergySource(source);

    // Attach to device
    device->AggregateObject(model);

    return model;
}

} // namespace wsn
} // namespace ns3
