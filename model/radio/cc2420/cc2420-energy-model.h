/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Wireless Sensor Network Team
 *
 * CC2420 Energy Model
 * Extends ns-3 DeviceEnergyModel with CC2420-specific power states
 */

#ifndef CC2420_ENERGY_MODEL_H
#define CC2420_ENERGY_MODEL_H

#include "cc2420-phy.h"

#include "ns3/energy-model-helper.h"
#include "ns3/device-energy-model.h"
#include "ns3/energy-source.h"
#include "ns3/nstime.h"
#include "ns3/traced-value.h"

#include <map>

namespace ns3
{
namespace cc2420
{

/**
 * @ingroup cc2420
 *
 * CC2420 Power State Configuration
 * Based on Castalia CC2420 specifications
 */
struct PowerConfig
{
    // State-based power consumption (mW)
    double sleepPowerMw;      //!< Sleep state (1.4 mW)
    double idlePowerMw;       //!< Idle/RX listening (62 mW)
    double rxPowerMw;         //!< RX packet reception (62 mW)
    double ccaPowerMw;        //!< CCA operation (62 mW)

    // TX power levels (dBm → mW mapping)
    // CC2420 supports 8 TX power levels
    std::map<int, double> txPowerLevels;  // [level] = mW

    // State transition delays (ms)
    double sleepToRxDelayMs;
    double sleepToTxDelayMs;
    double rxToTxDelayMs;
    double txToRxDelayMs;
    double rxToSleepDelayMs;
    double txToSleepDelayMs;

    // Default constructor with CC2420 defaults
    PowerConfig();
};

/**
 * @ingroup cc2420
 *
 * CC2420 Energy Model
 *
 * Implements:
 * - State-based energy consumption tracking
 * - 6 PHY states: SLEEP, IDLE, RX, TX, CCA, SWITCHING
 * - Power consumption per state and TX level
 * - Energy depletion detection
 */
class Cc2420EnergyModel : public DeviceEnergyModel
{
  public:
    /**
     * @brief Get the TypeId
     * @return the TypeId
     */
    static TypeId GetTypeId();

    Cc2420EnergyModel();
    ~Cc2420EnergyModel() override;

    // =============================================================================
    // Setup & Configuration
    // =============================================================================

    /**
     * @brief Set the energy source
     * @param source the EnergySource
     */
    void SetEnergySource(Ptr<EnergySource> source) override;

    /**
     * @brief Get the energy source
     * @return the EnergySource
     */
    Ptr<EnergySource> GetEnergySource() const;

    /**
     * @brief Set the PHY layer
     * @param phy the CC2420Phy
     */
    void SetPhy(Ptr<Cc2420Phy> phy);

    /**
     * @brief Set power configuration
     * @param config the power configuration
     */
    void SetPowerConfig(const PowerConfig& config);

    /**
     * @brief Get power configuration
     * @return the power configuration
     */
    PowerConfig GetPowerConfig() const;

    // =============================================================================
    // Energy Tracking
    // =============================================================================

    /**
     * @brief Get total energy consumption since start
     * @return energy in Joules
     */
    double GetTotalEnergyConsumption() const override;

    /**
     * @brief Handle PHY state change
     * Called by PHY layer on state transition
     */
    void HandlePhyStateChange(PhyState oldState, PhyState newState);

    // =============================================================================
    // DeviceEnergyModel Pure Virtual Implementations
    // =============================================================================

    /**
     * @brief Notification on device state change
     * @param newState the new device state
     */
    void ChangeState(int newState) override;

    /**
     * @brief Handle energy depletion
     */
    void HandleEnergyDepletion() override;

    /**
     * @brief Handle energy recharged
     */
    void HandleEnergyRecharged() override;

    /**
     * @brief Handle energy changed
     */
    void HandleEnergyChanged() override;

  private:
    // =============================================================================
    // Helper Methods
    // =============================================================================

    /**
     * Update energy consumed based on current state duration
     */
    void UpdateEnergyConsumption();

    /**
     * Get power consumption for a given state (in Watts)
     */
    double GetStatePowerW(PhyState state) const;

    /**
     * Get state name for logging
     */
    std::string GetStateName(PhyState state) const;

    // =============================================================================
    // Member Variables
    // =============================================================================

    // Energy source and tracking
    Ptr<EnergySource> m_energySource;
    double m_totalEnergyJ;          // Total energy consumed (Joules)
    TracedValue<double> m_totalEnergyTrace;

    // PHY reference
    Ptr<Cc2420Phy> m_phy;

    // Power configuration
    PowerConfig m_powerConfig;

    // State tracking
    PhyState m_currentState;
    Time m_stateEntryTime;
    double m_currentTxPowerMw;   // Current TX power for energy calc

    // Monitoring
    bool m_energyDepleted;
};

} // namespace cc2420
} // namespace ns3

#endif // CC2420_ENERGY_MODEL_H
