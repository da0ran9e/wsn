/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Cell Forming MAC Layer extending CC2420 MAC
 * Implements Phase 0 protocol on top of CC2420 radio
 */

#ifndef CELL_FORMING_MAC_H
#define CELL_FORMING_MAC_H

#include "../radio/cc2420/cc2420-mac.h"
#include "cell-forming.h"
#include "cell-forming-packet.h"

#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/event-id.h"

#include <vector>
#include <set>
#include <map>

namespace ns3
{
namespace wsn
{

/**
 * @brief Cell Forming MAC Layer
 *
 * Extends CC2420 MAC with Phase 0 functionality:
 * - HELLO packet transmission and reception
 * - CL announcement handling
 * - Member feedback processing
 * - Cell topology management
 */
class CellFormingMac : public Cc2420Mac
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
    CellFormingMac();

    /**
     * @brief Destructor
     */
    ~CellFormingMac() override;

    /**
     * @brief Set the CellForming protocol handler
     * @param cellForming pointer to CellForming module
     */
    void SetCellFormingModule(Ptr<CellForming> cellForming);

    /**
     * @brief Get the CellForming protocol handler
     * @return pointer to CellForming module
     */
    Ptr<CellForming> GetCellFormingModule() const;

    /**
     * @brief Send HELLO packet for neighbor discovery
     * @param nodeId Local node ID
     * @param position Local node position
     */
    void SendHelloPacket(uint32_t nodeId, Vector position);

    /**
     * @brief Send CL announcement packet
     * @param sourceId Source node ID (the CL)
     * @param cellId Cell ID
     * @param fitness Fitness score
     */
    void SendCLAnnouncement(uint32_t sourceId, int32_t cellId, double fitness);

    /**
     * @brief Send member feedback packet
     * @param memberId Member node ID
     * @param cellLeaderId CL node ID
     * @param cellId Cell ID
     * @param feedbackValue Feedback value
     */
    void SendMemberFeedback(uint32_t memberId, uint32_t cellLeaderId, int32_t cellId, double feedbackValue);

  private:
    /**
     * @brief Handle incoming HELLO packet
     */
    void HandleReceivedHelloPacket(Ptr<Packet> packet, Mac16Address source, double rssi);

    /**
     * @brief Handle incoming CL announcement
     */
    void HandleReceivedCLAnnouncement(Ptr<Packet> packet, Mac16Address source, double rssi);

    /**
     * @brief Handle incoming member feedback
     */
    void HandleReceivedMemberFeedback(Ptr<Packet> packet, Mac16Address source, double rssi);

    /**
     * @brief Internal method to process received packet from CC2420 MAC layer
     */
    void ReceiveFromCc2420(Ptr<Packet> packet, Mac16Address source, double rssi);

    Ptr<CellForming> m_cellForming;  //!< CellForming protocol instance

    // Internal packet type detection
    static const uint16_t PACKET_TYPE_HELLO = 0x0001;
    static const uint16_t PACKET_TYPE_CL_ANNOUNCEMENT = 0x0002;
    static const uint16_t PACKET_TYPE_MEMBER_FEEDBACK = 0x0003;
};

} // namespace wsn
} // namespace ns3

#endif // CELL_FORMING_MAC_H
