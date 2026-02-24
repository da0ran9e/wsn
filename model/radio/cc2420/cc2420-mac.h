/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Wireless Sensor Network Team
 *
 * CC2420 MAC Layer
 * Implements unslotted CSMA-CA and simplified frame handling
 */

#ifndef CC2420_MAC_H
#define CC2420_MAC_H

#include "cc2420-phy.h"

#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/mac16-address.h"
#include "ns3/event-id.h"
#include "ns3/traced-callback.h"

#include <queue>
#include <cstdint>

namespace ns3
{
namespace wsn
{

/**
 * @ingroup cc2420
 *
 * MAC State Enumeration
 */
enum MacState
{
    MAC_IDLE = 0,              //!< Ready for transmission
    MAC_CSMA_BACKOFF = 1,      //!< Backoff timer running
    MAC_CCA = 2,               //!< Performing Clear Channel Assessment
    MAC_SENDING = 3,           //!< Transmitting frame
    MAC_ACK_PENDING = 4,       //!< Waiting for ACK
    MAC_FRAME_RECEPTION = 5    //!< Receiving frame
};

/**
 * @ingroup cc2420
 *
 * MAC Parameters (IEEE 802.15.4)
 */
struct MacConfig
{
    uint16_t panId;                 //!< PAN ID
    Mac16Address shortAddress;      //!< 16-bit short address
    uint8_t macMinBE;              //!< Min Backoff Exponent (default 3)
    uint8_t macMaxBE;              //!< Max Backoff Exponent (default 5)
    uint8_t macMaxCSMABackoffs;    //!< Max CSMA backoff attempts (default 4)
    uint8_t macMaxFrameRetries;    //!< Max frame retries (default 3)
    bool txAckRequest;              //!< Request ACK on TX
    bool rxOnWhenIdle;              //!< Keep RX on during idle
};

/**
 * @ingroup cc2420
 *
 * CC2420 MAC Layer
 *
 * Implements:
 * - Unslotted CSMA-CA (no beacon, no superframes)
 * - Simplified frame transmission/reception
 * - ACK handling (basic)
 * - TX queue management
 */
class Cc2420Mac : public Object
{
  public:
    /**
     * @brief Get the TypeId
     * @return the TypeId
     */
    static TypeId GetTypeId();

    Cc2420Mac();
    ~Cc2420Mac() override;

    // =============================================================================
    // Initialization
    // =============================================================================

    /**
     * @brief Set the PHY layer
     * @param phy the physical layer
     */
    void SetPhy(Ptr<Cc2420Phy> phy);

    /**
     * @brief Get the PHY layer
     * @return the PHY layer
     */
    Ptr<Cc2420Phy> GetPhy() const;

    /**
     * @brief Set MAC configuration
     * @param config the MAC configuration
     */
    void SetMacConfig(const MacConfig& config);

    /**
     * @brief Get MAC configuration
     * @return the MAC configuration
     */
    MacConfig GetMacConfig() const;

    /**
     * @brief Start the MAC layer
     */
    void Start();

    // =============================================================================
    // Data Transmission Interface
    // =============================================================================

    /**
     * @brief Request data transmission (MCPS-DATA.request primitive)
     * @param packet the packet to transmit
     * @param destAddr destination short address
     * @param requestAck whether to request ACK
     * @return true if request accepted
     */
    bool McpsDataRequest(Ptr<Packet> packet, Mac16Address destAddr, bool requestAck);

    // =============================================================================
    // Frame Reception (from PHY)
    // =============================================================================

    /**
     * Handle received packet from PHY
     * Called by PHY when frame is successfully received
     */
    void FrameReceptionCallback(Ptr<Packet> packet, double rssi, uint8_t lqi);

    /**
     * Handle CCA result from PHY
     * Called by PHY after CCA operation
     */
    void CcaConfirmCallback(int result);

    /**
     * Handle TX completion from PHY
     * Called by PHY when transmission is complete
     */
    void TxConfirmCallback(int status);

    // =============================================================================
    // CSMA-CA Algorithm (Unslotted)
    // =============================================================================

    /**
     * @brief Start CSMA-CA backoff for current packet
     */
    void StartCSMACA();

    /**
     * @brief Handle backoff timer expiration
     */
    void BackoffExpired();

    /**
     * @brief Perform CCA
     */
    void DoCCA();

    /**
     * @brief Handle CCA result and decide next action
     */
    void HandleCCAResult(int result);

    /**
     * @brief Attempt transmission of current packet
     */
    void AttemptTransmission();

    // =============================================================================
    // Callback Types
    // =============================================================================

    /**
     * Callback for data indication (received packet)
     * Arguments: packet, source address, RSSI
     */
    typedef Callback<void, Ptr<Packet>, Mac16Address, double> McpsDataIndicationCallback;

    /**
     * Callback for data confirm (TX completion)
     * Arguments: status (0=success, 1=failure)
     */
    typedef Callback<void, int> McpsDataConfirmCallback;

    /**
     * Set RX indication callback (for upper layer)
     */
    void SetMcpsDataIndicationCallback(McpsDataIndicationCallback callback);

    /**
     * Set TX confirm callback (for upper layer)
     */
    void SetMcpsDataConfirmCallback(McpsDataConfirmCallback callback);

  private:
    // =============================================================================
    // Helper Methods
    // =============================================================================

    /**
     * Calculate random backoff delay
     */
    Time CalculateBackoffDelay();

    /**
     * Handle ACK reception
     */
    void HandleAckPacket(Ptr<Packet> packet);

    /**
     * Clear current packet and update state
     */
    void ClearCurrentPacket();

    // =============================================================================
    // Member Variables
    // =============================================================================

    // Layer references
    Ptr<Cc2420Phy> m_phy;

    // MAC configuration
    MacConfig m_config;

    // MAC state
    MacState m_macState;

    // TX queue and current packet
    std::queue<Ptr<Packet>> m_txQueue;
    Ptr<Packet> m_currentPacket;
    Mac16Address m_currentDestAddr;
    bool m_currentAckRequest;

    // CSMA-CA parameters
    uint8_t m_NB;   // Number of backoffs (0 to macMaxCSMABackoffs)
    uint8_t m_BE;   // Backoff exponent (macMinBE to macMaxBE)
    uint8_t m_CW;   // Contention window (always 1 for unslotted)
    uint8_t m_retries; // Retry count for current packet

    // Sequence number
    uint8_t m_sequenceNumber;

    // Event IDs for scheduling
    EventId m_backoffEvent;
    EventId m_ccaEvent;
    EventId m_txEvent;
    EventId m_ackWaitEvent;

    // Callbacks
    McpsDataIndicationCallback m_mcpsDataIndicationCallback;
    McpsDataConfirmCallback m_mcpsDataConfirmCallback;

    // Statistics / Energy tracking
    uint32_t m_txCount;
    uint32_t m_rxCount;
    uint32_t m_txFailureCount;
};

} // namespace cc2420
} // namespace ns3

#endif // CC2420_MAC_H
