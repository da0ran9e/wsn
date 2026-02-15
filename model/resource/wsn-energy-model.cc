#include "wsn-energy-model.h"
#include "ns3/log.h"
#include "ns3/double.h"

namespace ns3 {
namespace wsn {

NS_LOG_COMPONENT_DEFINE("WsnEnergyModel");

NS_OBJECT_ENSURE_REGISTERED(WsnEnergyModel);

TypeId
WsnEnergyModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::wsn::WsnEnergyModel")
        .SetParent<ns3::energy::DeviceEnergyModel>()
        .SetGroupName("Energy")
        .AddConstructor<WsnEnergyModel>()

        .AddAttribute("TxPower",
            "Transmission power (W)",
            DoubleValue(0.06),
            MakeDoubleAccessor(&WsnEnergyModel::m_txPowerW),
            MakeDoubleChecker<double>())

        .AddAttribute("RxPower",
            "Reception power (W)",
            DoubleValue(0.03),
            MakeDoubleAccessor(&WsnEnergyModel::m_rxPowerW),
            MakeDoubleChecker<double>())

        .AddAttribute("IdlePower",
            "Idle power (W)",
            DoubleValue(0.001),
            MakeDoubleAccessor(&WsnEnergyModel::m_idlePowerW),
            MakeDoubleChecker<double>());
    return tid;
}

WsnEnergyModel::WsnEnergyModel() = default;
WsnEnergyModel::~WsnEnergyModel() = default;

ns3::Ptr<ns3::energy::EnergySource>
WsnEnergyModel::GetEnergySource() const
{
    return m_source;
}

void
WsnEnergyModel::SetEnergySource(ns3::Ptr<ns3::energy::EnergySource> source)
{
    m_source = source;
}
void
WsnEnergyModel::NotifyTx(Time duration)
{
    double energy = m_txPowerW * duration.GetSeconds();
    m_totalEnergyConsumed += energy;
    m_source->UpdateEnergySource();
}

void
WsnEnergyModel::NotifyRx(Time duration)
{
    double energy = m_rxPowerW * duration.GetSeconds();
    m_totalEnergyConsumed += energy;
    m_source->UpdateEnergySource();
}

void
WsnEnergyModel::NotifyIdle(Time duration)
{
    double energy = m_idlePowerW * duration.GetSeconds();
    m_totalEnergyConsumed += energy;
    m_source->UpdateEnergySource();
}

double
WsnEnergyModel::GetTotalEnergyConsumption() const
{
    return m_totalEnergyConsumed;
}


/* ===== PURE VIRTUAL IMPLEMENTATIONS ===== */

void
WsnEnergyModel::ChangeState(int newState)
{
    NS_LOG_DEBUG("ChangeState -> " << newState);
}

void
WsnEnergyModel::HandleEnergyDepletion()
{
    NS_LOG_WARN("Energy depleted");
}

void
WsnEnergyModel::HandleEnergyRecharged()
{
    NS_LOG_INFO("Energy recharged");
}

void
WsnEnergyModel::HandleEnergyChanged()
{
    // optional
}

} // namespace wsn
} // namespace ns3
