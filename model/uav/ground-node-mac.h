/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Ground Node MAC Layer - Packet Reception
 */

#ifndef GROUND_NODE_MAC_H
#define GROUND_NODE_MAC_H

#include "ns3/object.h"
#include "ns3/vector.h"
#include "ns3/callback.h"
#include "fragment.h"

#include <vector>
#include <set>

namespace ns3
{

/**
 * @brief Ground Node MAC layer for packet reception
 *
 * Handles:
 * - Packet reception from UAV
 * - RSSI tracking
 * - Reception statistics
@@ * - Fragment-based confidence accumulation
@@ * - Alert generation
 */
class GroundNodeMac : public Object
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
    GroundNodeMac();

    /**
     * @brief Destructor
     */
    ~GroundNodeMac() override;

    /**
     * @brief Receive packet from UAV
     * @param seqNum Sequence number
     * @param uavPos UAV position
     * @param distance Distance from UAV
     * @param rssiDbm Received signal strength
     */
    void ReceivePacket(uint32_t seqNum, Vector uavPos, double distance, double rssiDbm);

    /**
     * @brief Receive fragment from UAV
     * @param fragment Fragment data
     * @param rssiDbm Received signal strength
     */
    void ReceiveFragment(const Fragment& fragment, double rssiDbm);

    /**
     * @brief Get total packets received
     * @return Packet count
     */
    uint32_t GetPacketsReceived() const;

    /**
     * @brief Get average RSSI
     * @return Average RSSI in dBm
     */
    double GetAverageRssi() const;

    /**
     * @brief Get minimum distance to UAV seen
     * @return Minimum distance in meters
     */
    double GetMinDistance() const;

    /**
     * @brief Reset statistics
     */
    void ResetStatistics();

    /**
     * @brief Set reception callback
     * @param cb Callback function
     */
    typedef Callback<void, uint32_t, double, double> ReceptionCallback;
    void SetReceptionCallback(ReceptionCallback cb);

    /**
     * @brief Get confidence level
     * @return Current confidence [0.0, 1.0]
     */
    double GetConfidence() const;

    /**
     * @brief Get fragments received
     * @return Fragment count
     */
    uint32_t GetFragmentsReceived() const;

    /**
     * @brief Check if alert has been triggered
     * @return True if confidence >= threshold
     */
    bool HasAlerted() const;

private:
    uint32_t m_packetsReceived;     //!< Packets received count
    double m_rssiSum;               //!< Sum of RSSI values
    double m_minDistance;           //!< Minimum distance seen
    
    ReceptionCallback m_receptionCallback; //!< Reception callback
    
    // Fragment-based detection
    std::vector<Fragment> m_receivedFragments;  //!< Received fragments
    std::set<uint32_t> m_receivedFragmentIds;   //!< Fragment IDs already received (deduplication)
    double m_confidence;                        //!< Confidence accumulation [0, 1]
    uint32_t m_fragmentsProcessed;              //!< Fragment count
    bool m_alerted;                             //!< Alert triggered flag
    double m_confidenceThreshold;               //!< Alert threshold (0.75)
    std::set<uint32_t> m_sensorTypeSeen;        //!< Unique sensor types seen

    /**
     * @brief Evaluate confidence contribution from fragment
     * @param frag Fragment to evaluate
     * @param rssi Received signal strength
     * @return Confidence delta [0, 0.25]
     */
    double EvaluateConfidenceFromFragment(const Fragment& frag, double rssi);
};

} // namespace ns3

#endif /* GROUND_NODE_MAC_H */
