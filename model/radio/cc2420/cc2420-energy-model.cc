/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "cc2420-energy-model.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/energy-source.h"

namespace ns3
{
namespace wsn
{

NS_LOG_COMPONENT_DEFINE("Cc2420EnergyModel");
NS_OBJECT_ENSURE_REGISTERED(Cc2420EnergyModel);

// =============================================================================
// PowerConfig Default Constructor
// =============================================================================

PowerConfig::PowerConfig()
    : sleepPowerMw(1.4),
      idlePowerMw(62.0),
      rxPowerMw(62.0),
      ccaPowerMw(62.0),
      sleepToRxDelayMs(0.05),
      sleepToTxDelayMs(0.05),
      rxToTxDelayMs(0.01),
      txToRxDelayMs(0.01),
      rxToSleepDelayMs(0.194),
      txToSleepDelayMs(0.194)
{
    // CC2420 TX Power Levels (8 levels in dBm → mW)
    // Based on Castalia specifications
    txPowerLevels[0] = 57.42;  // 0 dBm
    txPowerLevels[1] = 55.18;  // -1 dBm
    txPowerLevels[2] = 50.69;  // -3 dBm
    txPowerLevels[3] = 46.20;  // -5 dBm
    txPowerLevels[4] = 42.24;  // -7 dBm
    txPowerLevels[5] = 36.30;  // -10 dBm
    txPowerLevels[6] = 32.67;  // -15 dBm
    txPowerLevels[7] = 29.04;  // -25 dBm
}

// =============================================================================
// Cc2420EnergyModel Implementation
// =============================================================================

TypeId
Cc2420EnergyModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::cc2420::Cc2420EnergyModel")
        .SetParent<DeviceEnergyModel>()
        .SetGroupName("Energy")
        .AddConstructor<Cc2420EnergyModel>()
        .AddAttribute("SleepPower",
                      "Sleep mode power consumption (mW)",
                      DoubleValue(1.4),
                      MakeDoubleAccessor(&Cc2420EnergyModel::m_powerConfig.sleepPowerMw),
                      MakeDoubleChecker<double>())
        .AddAttribute("IdlePower",
                      "Idle/RX listening power consumption (mW)",
                      DoubleValue(62.0),
                      MakeDoubleAccessor(&Cc2420EnergyModel::m_powerConfig.idlePowerMw),
                      MakeDoubleChecker<double>())
        .AddAttribute("RxPower",
                      "RX power consumption (mW)",
                      DoubleValue(62.0),
                      MakeDoubleAccessor(&Cc2420EnergyModel::m_powerConfig.rxPowerMw),
                      MakeDoubleChecker<double>())
        .AddAttribute("CcaPower",
                      "CCA power consumption (mW)",
                      DoubleValue(62.0),
                      MakeDoubleAccessor(&Cc2420EnergyModel::m_powerConfig.ccaPowerMw),
                      MakeDoubleChecker<double>());
    return tid;
}

Cc2420EnergyModel::Cc2420EnergyModel()
    : m_totalEnergyJ(0.0),
      m_currentState(PHY_SLEEP),
      m_stateEntryTime(Seconds(0)),
      m_currentTxPowerMw(57.42),
      m_energyDepleted(false)
{
    NS_LOG_FUNCTION(this);
}

Cc2420EnergyModel::~Cc2420EnergyModel()
{
    NS_LOG_FUNCTION(this);
}

// =============================================================================
// Setup & Configuration
// =============================================================================

void
Cc2420EnergyModel::SetEnergySource(Ptr<EnergySource> source)
{
    m_energySource = source;
}

Ptr<EnergySource>
Cc2420EnergyModel::GetEnergySource() const
{
    return m_energySource;
}

void
Cc2420EnergyModel::SetPhy(Ptr<Cc2420Phy> phy)
{
    m_phy = phy;
    if (m_phy)
    {
        // Connect to PHY state change callback
        m_phy->SetStateChangeCallback(
            MakeCallback(&Cc2420EnergyModel::HandlePhyStateChange, this));
    }
}

void
Cc2420EnergyModel::SetPowerConfig(const PowerConfig& config)
{
    m_powerConfig = config;
}

PowerConfig
Cc2420EnergyModel::GetPowerConfig() const
{
    return m_powerConfig;
}

// =============================================================================
// Energy Tracking
// =============================================================================

double
Cc2420EnergyModel::GetTotalEnergyConsumption() const
{
    return m_totalEnergyJ;
}

void
Cc2420EnergyModel::HandlePhyStateChange(PhyState oldState, PhyState newState)
{
    NS_LOG_FUNCTION(this << GetStateName(oldState) << GetStateName(newState));
    
    // Update energy consumption for the previous state
    UpdateEnergyConsumption();
    
    // Update state information
    m_currentState = newState;
    m_stateEntryTime = Simulator::Now();
}

// =============================================================================
// DeviceEnergyModel Pure Virtual Implementations
// =============================================================================

void
Cc2420EnergyModel::ChangeState(int newState)
{
    NS_LOG_FUNCTION(this << newState);
    // TODO: Implement device state change handling
}

void
Cc2420EnergyModel::HandleEnergyDepletion()
{
    NS_LOG_WARN("Energy depleted at " << Simulator::Now().GetSeconds() << "s");
    m_energyDepleted = true;
}

void
Cc2420EnergyModel::HandleEnergyRecharged()
{
    NS_LOG_INFO("Energy recharged at " << Simulator::Now().GetSeconds() << "s");
    m_energyDepleted = false;
}

void
Cc2420EnergyModel::HandleEnergyChanged()
{
    // Optional: Called when energy source is updated
}

// =============================================================================
// Private Helper Methods
// =============================================================================

void
Cc2420EnergyModel::UpdateEnergyConsumption()
{
    NS_LOG_FUNCTION(this);

    Time duration = Simulator::Now() - m_stateEntryTime;
    double durationSeconds = duration.GetSeconds();

    double powerW = GetStatePowerW(m_currentState);
    double energyJ = powerW * durationSeconds;

    m_totalEnergyJ += energyJ;

    NS_LOG_DEBUG("Energy update: state=" << GetStateName(m_currentState)
                                        << " duration=" << durationSeconds
                                        << "s power=" << powerW
                                        << "W energy=" << energyJ
                                        << "J total=" << m_totalEnergyJ << "J");

    if (m_energySource)
    {
        m_energySource->UpdateEnergySource();
    }
}

double
Cc2420EnergyModel::GetStatePowerW(PhyState state) const
{
    // Convert mW to W
    switch (state)
    {
    case PHY_SLEEP:
        return m_powerConfig.sleepPowerMw / 1000.0;
    case PHY_IDLE:
        return m_powerConfig.idlePowerMw / 1000.0;
    case PHY_RX:
        return m_powerConfig.rxPowerMw / 1000.0;
    case PHY_CCA:
        return m_powerConfig.ccaPowerMw / 1000.0;
    case PHY_TX:
        return m_currentTxPowerMw / 1000.0;
    case PHY_SWITCHING:
        // Average power during switching (use RX power as estimate)
        return m_powerConfig.idlePowerMw / 1000.0;
    default:
        return 0.0;
    }
}

std::string
Cc2420EnergyModel::GetStateName(PhyState state) const
{
    return Cc2420Phy::GetStateName(state);
}

} // namespace cc2420
} // namespace ns3
