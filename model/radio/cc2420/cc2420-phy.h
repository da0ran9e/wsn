/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Wireless Sensor Network Team
 *
 * CC2420 PHY Layer
 * Implements physical layer with 6-state machine (SLEEP/IDLE/RX/TX/CCA/SWITCHING)
 * Supports SpectrumPhy integration and SIMPLE_COLLISION_MODEL
 */

#ifndef CC2420_PHY_H
#define CC2420_PHY_H

#include "ns3/spectrum-phy.h"
#include "ns3/spectrum-value.h"
#include "ns3/spectrum-signal-parameters.h"
#include "ns3/traced-callback.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"

#include <vector>
#include <map>

namespace ns3
{

class MobilityModel;
class SpectrumChannel;
class AntennaModel;
class NetDevice;

namespace wsn
{

/**
 * @ingroup cc2420
 *
 * CC2420 Radio State Enumeration
 */
enum PhyState
{
    PHY_SLEEP = 0,      //!< Sleep mode (1.4 mW)
    PHY_IDLE = 1,       //!< Idle/RX listening (62 mW)
    PHY_RX = 2,         //!< Receiving packet (62 mW)
    PHY_TX = 3,         //!< Transmitting packet (57.42-29.04 mW per level)
    PHY_CCA = 4,        //!< Clear Channel Assessment (62 mW)
    PHY_SWITCHING = 5   //!< State transition (variable)
};

/**
 * @ingroup cc2420
 *
 * Signal reception tracking structure (from Castalia)
 */
struct ReceivedSignal
{
    int sourceNodeId;           //!< Source node ID
    double powerDbm;            //!< Received power in dBm
    double currentInterference; //!< Current interference level
    double maxInterference;     //!< Peak interference
    int bitErrors;              //!< Accumulated bit errors
    Time startTime;             //!< When signal started
};

/**
 * @ingroup cc2420
 *
 * CC2420 Physical Layer (PHY)
 *
 * Implements:
 * - SpectrumPhy interface for channel integration
 * - 6-state state machine (SLEEP/IDLE/RX/TX/CCA/SWITCHING)
 * - Castalia-style SIMPLE_COLLISION_MODEL
 * - Signal reception tracking and interference calculation
 * - Energy state reporting
 */
class Cc2420Phy : public SpectrumPhy
{
  public:
    /**
     * @brief Get the TypeId
     * @return the TypeId
     */
    static TypeId GetTypeId();

    Cc2420Phy();
    ~Cc2420Phy() override;

    // =============================================================================
    // SpectrumPhy Interface Implementation
    // =============================================================================

    /**
     * Set the mobility model
     */
    void SetMobility(Ptr<MobilityModel> m) override;

    /**
     * Get the mobility model
     */
    Ptr<MobilityModel> GetMobility() const override;

    /**
     * Set the antenna model
     */
    void SetAntenna(Ptr<AntennaModel> a) override;

    /**
     * Set the channel
     */
    void SetChannel(Ptr<SpectrumChannel> c) override;

    /**
     * Start reception of a spectrum signal
     */
    int StartRx(Ptr<SpectrumSignalParameters> params) override;

    /**
     * Get the device
     */
    Ptr<NetDevice> GetDevice() const override;

    /**
     * Set the device
     */
    void SetDevice(Ptr<NetDevice> d) override;

    /**
     * Get RX spectrum model
     */
    Ptr<const SpectrumModel> GetRxSpectrumModel() const override;

    /**
     * Add RX antenna model
     */
    void AddRxAntenna(Ptr<AntennaModel> a) override;

    // =============================================================================
    // CC2420-Specific Interface
    // =============================================================================

    /**
     * @brief Transmit a packet
     * @param packet the packet to transmit
     * @param duration transmission duration
     */
    void TransmitPacket(Ptr<Packet> packet, Time duration);

    /**
     * @brief Request state change
     * @param newState the desired state
     * @return true if state change is valid
     */
    bool SetState(PhyState newState);

    /**
     * @brief Get current state
     * @return the current PHY state
     */
    PhyState GetState() const;

    /**
     * @brief Get state name for logging
     * @param state the state
     * @return string representation
     */
    static std::string GetStateName(PhyState state);

    /**
     * @brief Perform CCA (Clear Channel Assessment)
     * @return true if channel is clear
     */
    bool PerformCCA();

    /**
     * @brief Get RSSI (Received Signal Strength Indicator)
     * @return RSSI value in dBm
     */
    double GetRSSI() const;

    /**
     * @brief Set TX power
     * @param powerDbm power in dBm
     */
    void SetTxPower(double powerDbm);

    /**
     * @brief Get TX power
     * @return power in dBm
     */
    double GetTxPower() const;

    /**
     * @brief Set RX sensitivity
     * @param sensitivityDbm sensitivity in dBm
     */
    void SetRxSensitivity(double sensitivityDbm);

    /**
     * @brief Get RX sensitivity
     * @return sensitivity in dBm
     */
    double GetRxSensitivity() const;

    // =============================================================================
    // Callback Types
    // =============================================================================

    /**
     * Callback signature for successful packet reception
     * Arguments: packet, RSSI, LQI
     */
    typedef Callback<void, Ptr<Packet>, double, uint8_t> PdDataIndicationCallback;

    /**
     * Callback signature for packet transmission completion
     * Arguments: status (0=success, 1=failure)
     */
    typedef Callback<void, int> PdDataConfirmCallback;

    /**
     * Callback signature for CCA result
     * Arguments: result (0=clear, 1=busy, 2=not valid)
     */
    typedef Callback<void, int> PlmeCcaConfirmCallback;

    /**
     * Callback signature for state change
     * Arguments: old state, new state
     */
    typedef Callback<void, PhyState, PhyState> StateChangeCallback;

    /**
     * Set RX indication callback
     */
    void SetPdDataIndicationCallback(PdDataIndicationCallback callback);

    /**
     * Set TX completion callback
     */
    void SetPdDataConfirmCallback(PdDataConfirmCallback callback);

    /**
     * Set CCA result callback
     */
    void SetPlmeCcaConfirmCallback(PlmeCcaConfirmCallback callback);

    /**
     * Set state change callback (for energy model notification)
     */
    void SetStateChangeCallback(StateChangeCallback callback);

  private:
    // =============================================================================
    // State Machine
    // =============================================================================

    /**
     * Handle state change with energy tracking
     */
    void DoStateChange(PhyState newState);

    /**
     * Handle TX completion
     */
    void TxComplete();

    /**
     * Handle RX completion
     */
    void RxComplete();

    // =============================================================================
    // Signal Reception (Castalia-style)
    // =============================================================================

    /**
     * Process signal start event
     */
    void ProcessSignalStart(Ptr<SpectrumSignalParameters> params);

    /**
     * Process signal end event
     */
    void ProcessSignalEnd();

    /**
     * Update bit errors for overlapping signals
     */
    void UpdateInterference();

    /**
     * Calculate SNR and BER
     */
    double CalculateSNR(const ReceivedSignal& signal) const;

    /**
     * SIMPLE_COLLISION_MODEL: Check if signal is destructed by interference
     */
    bool IsPacketDestroyed(const ReceivedSignal& signal) const;

    // =============================================================================
    // Member Variables
    // =============================================================================

    // SpectrumPhy components
    Ptr<MobilityModel> m_mobility;
    Ptr<AntennaModel> m_antenna;
    Ptr<SpectrumChannel> m_channel;
    Ptr<NetDevice> m_netDevice;
    Ptr<const SpectrumModel> m_rxSpectrumModel;

    // Radio parameters
    double m_txPowerDbm;
    double m_rxSensitivityDbm;
    double m_noiseFloorDbm;
    double m_ccaThresholdDbm;

    // State machine
    PhyState m_currentState;
    PhyState m_pendingState;
    EventId m_stateChangeEvent;
    EventId m_txCompleteEvent;

    // Signal reception
    std::vector<ReceivedSignal> m_receivedSignals;
    double m_totalPowerDbm; // Total received power (interference + signal)
    Time m_lastSignalChange;

    // Callbacks
    PdDataIndicationCallback m_pdDataIndicationCallback;
    PdDataConfirmCallback m_pdDataConfirmCallback;
    PlmeCcaConfirmCallback m_plmeCcaConfirmCallback;
    StateChangeCallback m_stateChangeCallback;

    // Energy tracking
    Time m_stateStartTime;
    PhyState m_previousState;
};

} // namespace cc2420
} // namespace ns3

#endif // CC2420_PHY_H
