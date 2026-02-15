#ifndef WSN_ENERGY_MODEL_H
#define WSN_ENERGY_MODEL_H

#include "ns3/device-energy-model.h"
#include "ns3/energy-source.h"
#include "ns3/traced-value.h"
#include "ns3/nstime.h"

namespace ns3 {
namespace wsn {

class WsnEnergyModel : public ns3::energy::DeviceEnergyModel
{
public:
    static TypeId GetTypeId();
    WsnEnergyModel();
    ~WsnEnergyModel() override;

    // Required overrides
    double GetTotalEnergyConsumption() const override;
    void ChangeState(int newState) override;
    void HandleEnergyDepletion() override;
    void HandleEnergyRecharged() override;
    void HandleEnergyChanged() override;

    ns3::Ptr<ns3::energy::EnergySource> GetEnergySource() const;
    void SetEnergySource(ns3::Ptr<ns3::energy::EnergySource> source) override;

    // ===== API =====
    void NotifyTx(Time duration);
    void NotifyRx(Time duration);
    void NotifyIdle(Time duration);

private:
    ns3::Ptr<ns3::energy::EnergySource> m_source;

    double m_txPowerW;
    double m_rxPowerW;
    double m_idlePowerW;
    double m_totalEnergyConsumed = 0.0;
};

} // namespace wsn
} // namespace ns3

#endif // WSN_ENERGY_MODEL_H
