/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * UAV MAC Layer - Broadcast Controller
 */

#ifndef UAV_MAC_H
#define UAV_MAC_H

#include "ns3/object.h"
#include "ns3/node.h"
#include "ns3/mobility-model.h"
#include "ns3/node-container.h"
#include "ns3/nstime.h"
#include "ns3/timer.h"
#include "ns3/callback.h"

#include "fragment.h"

#include <random>

namespace ns3
{

/**
 * @brief UAV MAC layer for periodic broadcast
 *
 * Handles:
 * - Periodic packet broadcasting
 * - Transmission power control
 * - Broadcast scheduling
@@ * - Fragment generation and distribution
 * - Statistics tracking
 */
class UavMac : public Object
{
public:
    /**
     * @brief Get the TypeId
     * @return TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Constructor
     */
    UavMac();

    /**
     * @brief Destructor
     */
    ~UavMac() override;

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
     * 
     * Creates n fragments with random baseConfidence values that sum to totalConfidence
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
    
    // Fragment set management
    std::vector<Fragment> m_fragmentSet;  //!< Pre-generated fragment set
    uint32_t m_currentFragmentIndex;      //!< Current fragment index (loops)
    uint32_t m_numFragments;              //!< Number of fragments in file
    std::mt19937 m_rng;                   //!< Random number generator
};

} // namespace ns3

#endif /* UAV_MAC_H */
