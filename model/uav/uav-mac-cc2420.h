/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * UAV MAC Layer extending CC2420 MAC
 * Implements Phase 1 UAV broadcasting on top of CC2420 radio
 */

#ifndef UAV_MAC_CC2420_H
#define UAV_MAC_CC2420_H

#include "../radio/cc2420/cc2420-mac.h"
#include "uav-mac.h"

#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/timer.h"
#include "ns3/callback.h"

namespace ns3
{
namespace wsn
{

/**
 * @brief UAV MAC Layer extending CC2420 MAC
 *
 * Implements Phase 1 UAV broadcast controller:
 * - Periodic file fragment broadcasting
 * - Transmission power control
 * - Fragment scheduling
 * - RX statistics tracking
 */
class UavMacCc2420 : public Cc2420Mac
{
  public:
    /**
     * @brief Get the TypeId
     * @return the TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Constructor
     */
    UavMacCc2420();

    /**
     * @brief Destructor
     */
    ~UavMacCc2420() override;

    /**
     * @brief Initialize UAV MAC
     * @param uavNode The UAV node
     * @param groundNodes Container of ground nodes
     */
    void Initialize(Ptr<Node> uavNode, NodeContainer groundNodes);

    /**
     * @brief Start broadcasting
     * @param interval Broadcast interval
     * @param stopTime Stop time for broadcasting
     */
    void StartBroadcast(Time interval, Time stopTime);

    /**
     * @brief Stop broadcasting
     */
    void StopBroadcast();

    /**
     * @brief Set TX power
     * @param txPowerDbm TX power in dBm
     */
    void SetTxPower(double txPowerDbm);

    /**
     * @brief Get TX power
     * @return TX power in dBm
     */
    double GetTxPower() const;

    /**
     * @brief Set RX sensitivity threshold
     * @param rxSensitivityDbm RX sensitivity in dBm
     */
    void SetRxSensitivity(double rxSensitivityDbm);

    /**
     * @brief Get total broadcasts count
     * @return Total number of broadcasts
     */
    uint32_t GetTotalBroadcasts() const;

    /**
     * @brief Get total successful receptions across all ground nodes
     * @return Total reception count
     */
    uint32_t GetTotalReceptions() const;

    /**
     * @brief Set broadcast callback
     * @param cb Callback function
     */
    typedef Callback<void, uint32_t, Vector, double> BroadcastCallback;
    void SetBroadcastCallback(BroadcastCallback cb);

    /**
     * @brief Get fragment count
     * @return Total fragments sent
     */
    uint32_t GetFragmentsSent() const;

    /**
     * @brief Set number of fragments in file (for partitioning)
     * @param numFragments Number of fragments to partition file into
     */
    void SetNumFragments(uint32_t numFragments);

    /**
     * @brief Get number of fragments in file
     * @return Number of fragments
     */
    uint32_t GetNumFragments() const;

    /**
     * @brief Generate fragment set for file partitioning
     * @param numFragments Number of fragments to create
     * @param totalConfidence Total confidence (default 1.0)
     */
    void GenerateFragmentSet(uint32_t numFragments, double totalConfidence = 1.0);

  private:
    /**
     * @brief Perform broadcast
     */
    void DoBroadcast();

    /**
     * @brief Calculate received power at distance
     * @param distanceMeters Distance in meters
     * @return Received power in dBm
     */
    double CalculateRxPower(double distanceMeters);

    /**
     * @brief Handle RX indication from CC2420 MAC
     */
    void ReceiveFromCc2420(Ptr<Packet> packet, Mac16Address source, double rssi);

    Ptr<Node> m_uavNode;              //!< UAV node
    NodeContainer m_groundNodes;      //!< Ground nodes
    
    Timer m_broadcastTimer;           //!< Broadcast timer
    Time m_broadcastInterval;         //!< Broadcast interval
    Time m_stopTime;                  //!< Stop time
    
    uint32_t m_seqNum;                //!< Sequence number
    double m_txPowerDbm;              //!< TX power (dBm)
    double m_rxSensitivityDbm;        //!< RX sensitivity (dBm)
    
    // Path loss model parameters
    double m_referenceLoss;           //!< Reference loss at 1m (dB)
    double m_pathLossExponent;        //!< Path loss exponent
    double m_referenceDistance;       //!< Reference distance (m)
    
    // Statistics
    uint32_t m_totalBroadcasts;       //!< Total broadcasts
    uint32_t m_totalReceptions;       //!< Total receptions
    
    BroadcastCallback m_broadcastCallback; //!< Broadcast callback

    // Fragment management - delegated to original UavMac
    Ptr<UavMac> m_originalUavMac;     //!< Original UavMac for fragment management
};

} // namespace wsn
} // namespace ns3

#endif // UAV_MAC_CC2420_H
