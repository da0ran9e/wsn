/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * WSN Radio MAC Layer - Integrates Phase 0 and Phase 1 with CC2420/LR-WPAN
 */

#ifndef WSN_RADIO_MAC_H
#define WSN_RADIO_MAC_H

#include "ns3/object.h"
#include "ns3/node.h"
#include "ns3/net-device.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/timer.h"

#include "mac-frame.h"
#include "fragment.h"
#include "cell-forming-packet.h"
#include "../radio/cc2420/cc2420-mac.h"

namespace ns3
{

/**
 * @brief WSN Radio MAC Layer
 * 
 * Provides unified MAC layer for:
 * - Phase 0: Cell Forming packets (HELLO, CL announcement, member feedback)
 * - Phase 1: UAV Fragment broadcasts
 * - Uses LR-WPAN (802.15.4) radio for actual transmission
 * 
 * Features:
 * - Packet serialization to MAC frames
 * - Radio device integration
 * - Packet reception callbacks
 * - Statistics tracking
 */
class WsnRadioMac : public Object
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
    WsnRadioMac();

    /**
     * @brief Destructor
     */
    ~WsnRadioMac() override;

    // === Configuration ===

    /**
     * @brief Attach to node and radio device
     * @param node The node this MAC belongs to
     * @param radioDevice LR-WPAN radio device
     */
    void AttachToNode(Ptr<Node> node, Ptr<NetDevice> radioDevice);

    /**
     * @brief Get the attached node
     * @return Node pointer
     */
    Ptr<Node> GetNode() const { return m_node; }

    /**
     * @brief Get the radio device
     * @return NetDevice pointer
     */
    Ptr<NetDevice> GetRadioDevice() const { return m_radioDevice; }

    // === Phase 0 Packet Transmission ===

    /**
     * @brief Send HELLO packet via radio
     * @param hello HELLO packet to send
     * @return True if scheduled successfully
     */
    bool SendHelloPacket(const HelloPacket& hello);

    /**
     * @brief Send CL announcement via radio
     * @param announcement CL announcement packet
     * @return True if scheduled successfully
     */
    bool SendCLAnnouncement(const CLAnnouncementPacket& announcement);

    /**
     * @brief Send member feedback via radio
     * @param feedback Member feedback packet
     * @return True if scheduled successfully
     */
    bool SendMemberFeedback(const CLMemberFeedbackPacket& feedback);

    // === Phase 1 Packet Transmission ===

    /**
     * @brief Send fragment via radio (Phase 1 UAV broadcast)
     * @param fragment Fragment to broadcast
     * @return True if scheduled successfully
     */
    bool SendFragment(const Fragment& fragment);

    // === Packet Reception Callbacks ===

    /**
     * @brief Set callback for received HELLO packets
     */
    typedef Callback<void, const HelloPacket&> HelloReceivedCallback;
    void SetHelloReceivedCallback(HelloReceivedCallback cb);

    /**
     * @brief Set callback for received CL announcements
     */
    typedef Callback<void, const CLAnnouncementPacket&> CLAnnouncementReceivedCallback;
    void SetCLAnnouncementReceivedCallback(CLAnnouncementReceivedCallback cb);

    /**
     * @brief Set callback for received member feedback
     */
    typedef Callback<void, const CLMemberFeedbackPacket&> MemberFeedbackReceivedCallback;
    void SetMemberFeedbackReceivedCallback(MemberFeedbackReceivedCallback cb);

    /**
     * @brief Set callback for received fragments
     */
    typedef Callback<void, const Fragment&> FragmentReceivedCallback;
    void SetFragmentReceivedCallback(FragmentReceivedCallback cb);

    // === Statistics ===

    /**
     * @brief Get number of packets sent
     * @return Packet count
     */
    uint32_t GetPacketsSent() const { return m_packetsSent; }

    /**
     * @brief Get number of packets received
     * @return Packet count
     */
    uint32_t GetPacketsReceived() const { return m_packetsReceived; }

    /**
     * @brief Get number of transmission failures
     * @return Failure count
     */
    uint32_t GetTransmissionFailures() const { return m_txFailures; }

    /**
     * @brief Reset statistics
     */
    void ResetStatistics();

private:
    // === Internal Methods ===

    /**
     * @brief Handle packet reception from radio
     * @param device The radio device
     * @param packet The received packet
     * @param protocol Protocol number
     * @param sender Sender address
     * @param receiver Receiver address
     * @param packetType Packet type (unused)
     * @return Always true (packet handled)
     */
    bool ReceivePacket(Ptr<NetDevice> device, Ptr<const Packet> packet,
                      uint16_t protocol, const Address& sender,
                      const Address& receiver, NetDevice::PacketType packetType);

    /**
     * @brief Transmit a MAC frame via radio
     * @param frame The packet to transmit
     * @param type Packet type
     * @param srcId Source node ID
     * @param dstId Destination node ID
     * @return True if transmission queued
     */
    bool TransmitPacket(Ptr<Packet> frame, WsnMacHeader::PacketType type,
                       uint16_t srcId, uint16_t dstId);

    /**
     * @brief Handle frame indication coming from cc2420 MAC shim
     */
    void HandleCc2420DataIndication(Ptr<Packet> packet, Mac16Address source, double rssi);

    // === Member Variables ===

    Ptr<Node> m_node;                      // Associated node
    Ptr<NetDevice> m_radioDevice;          // LR-WPAN radio device
    Ptr<wsn::Cc2420Mac> m_cc2420Mac;       // CC2420 MAC shim path

    uint32_t m_packetsSent;               // Statistics: packets sent
    uint32_t m_packetsReceived;           // Statistics: packets received
    uint32_t m_txFailures;                // Statistics: transmission failures
    uint16_t m_seqNum;                    // Sequence number

    // Callbacks
    HelloReceivedCallback m_helloRxCb;
    CLAnnouncementReceivedCallback m_clAnnRxCb;
    MemberFeedbackReceivedCallback m_feedbackRxCb;
    FragmentReceivedCallback m_fragmentRxCb;
};

} // namespace ns3

#endif /* WSN_RADIO_MAC_H */
