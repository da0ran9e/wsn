/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * WSN Radio MAC Layer Implementation
 */

#include "wsn-radio-mac.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/mac16-address.h"

#include <cstdio>

NS_LOG_COMPONENT_DEFINE("WsnRadioMac");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(WsnRadioMac);

TypeId
WsnRadioMac::GetTypeId()
{
    static TypeId tid = TypeId("ns3::WsnRadioMac")
        .SetParent<Object>()
        .SetGroupName("Wsn")
        .AddConstructor<WsnRadioMac>();
    return tid;
}

WsnRadioMac::WsnRadioMac()
    : m_packetsSent(0),
      m_packetsReceived(0),
      m_txFailures(0),
      m_seqNum(0)
{
    NS_LOG_FUNCTION(this);
}

WsnRadioMac::~WsnRadioMac()
{
    NS_LOG_FUNCTION(this);
}

void
WsnRadioMac::AttachToNode(Ptr<Node> node, Ptr<NetDevice> radioDevice)
{
    NS_LOG_FUNCTION(this << node << radioDevice);
    m_node = node;
    m_radioDevice = radioDevice;

    if (radioDevice)
    {
        // Build a CC2420 MAC shim per node so packets traverse cc2420-mac.cc path
        m_cc2420Mac = CreateObject<wsn::Cc2420Mac>();
        wsn::MacConfig cfg = m_cc2420Mac->GetMacConfig();
        char srcAddrStr[6];
        std::snprintf(srcAddrStr, sizeof(srcAddrStr), "%02x:%02x",
                      static_cast<unsigned>((node->GetId() >> 8) & 0xff),
                      static_cast<unsigned>(node->GetId() & 0xff));
        cfg.shortAddress = Mac16Address(srcAddrStr);
        m_cc2420Mac->SetMacConfig(cfg);
        m_cc2420Mac->SetMcpsDataIndicationCallback(
            MakeCallback(&WsnRadioMac::HandleCc2420DataIndication, this));

        // Register packet reception callback with the radio device
        // The NetDevice callback signature: bool(Ptr<NetDevice>, Ptr<const Packet>, uint16_t, const Address&)
        Ptr<WsnRadioMac> self = this;
        radioDevice->SetReceiveCallback(
            [self](Ptr<NetDevice> device, Ptr<const Packet> packet,
                   uint16_t protocol, const Address& from) -> bool {
                // Call ReceivePacket with default values for missing parameters
                return self->ReceivePacket(device, packet, protocol, from,
                                          Address(), NetDevice::PACKET_HOST);
            }
        );
        
        NS_LOG_INFO("WsnRadioMac attached to node " << node->GetId()
                   << " with LR-WPAN device, reception callback registered");
    }
}

void
WsnRadioMac::HandleCc2420DataIndication(Ptr<Packet> packet, Mac16Address source, double rssi)
{
    NS_LOG_FUNCTION(this << packet << source << rssi);
    ReceivePacket(m_radioDevice,
                  packet,
                  0,
                  source,
                  Address(),
                  NetDevice::PACKET_HOST);
}

bool
WsnRadioMac::SendHelloPacket(const HelloPacket& hello)
{
    NS_LOG_FUNCTION(this);

    Ptr<Packet> payload = SerializeHelloPacket(hello);
    return TransmitPacket(payload, WsnMacHeader::HELLO_PACKET,
                         hello.senderId, 0xFFFF); // Broadcast
}

bool
WsnRadioMac::SendCLAnnouncement(const CLAnnouncementPacket& announcement)
{
    NS_LOG_FUNCTION(this);

    Ptr<Packet> payload = SerializeCLAnnouncement(announcement);
    return TransmitPacket(payload, WsnMacHeader::CL_ANNOUNCEMENT,
                         announcement.senderId, 0xFFFF); // Broadcast to cell
}

bool
WsnRadioMac::SendMemberFeedback(const CLMemberFeedbackPacket& feedback)
{
    NS_LOG_FUNCTION(this);

    Ptr<Packet> payload = SerializeMemberFeedback(feedback);
    return TransmitPacket(payload, WsnMacHeader::MEMBER_FEEDBACK,
                         feedback.senderId, 0xFFFF); // Broadcast to CL
}

bool
WsnRadioMac::SendFragment(const Fragment& fragment)
{
    NS_LOG_FUNCTION(this);

    Ptr<Packet> payload = SerializeFragment(fragment);
    return TransmitPacket(payload, WsnMacHeader::UAV_FRAGMENT,
                         m_node->GetId(), 0xFFFF); // Broadcast
}

void
WsnRadioMac::SetHelloReceivedCallback(HelloReceivedCallback cb)
{
    m_helloRxCb = cb;
}

void
WsnRadioMac::SetCLAnnouncementReceivedCallback(CLAnnouncementReceivedCallback cb)
{
    m_clAnnRxCb = cb;
}

void
WsnRadioMac::SetMemberFeedbackReceivedCallback(MemberFeedbackReceivedCallback cb)
{
    m_feedbackRxCb = cb;
}

void
WsnRadioMac::SetFragmentReceivedCallback(FragmentReceivedCallback cb)
{
    m_fragmentRxCb = cb;
}

void
WsnRadioMac::ResetStatistics()
{
    m_packetsSent = 0;
    m_packetsReceived = 0;
    m_txFailures = 0;
}

bool
WsnRadioMac::ReceivePacket(Ptr<NetDevice> device, Ptr<const Packet> packet,
                          uint16_t protocol, const Address& sender,
                          const Address& receiver, NetDevice::PacketType packetType)
{
    NS_LOG_FUNCTION(this << device << packet->GetSize());

    if (!packet || packet->GetSize() < WsnMacHeader().GetSerializedSize())
    {
        NS_LOG_WARN("Received packet too small");
        return false;
    }

    // Deserialize MAC header
    Ptr<Packet> pktCopy = packet->Copy();
    WsnMacHeader macHeader;
    pktCopy->RemoveHeader(macHeader);

    m_packetsReceived++;

    NS_LOG_DEBUG("Received " << macHeader.GetPacketType() << " from node "
                            << macHeader.GetSource());

    // Process based on packet type
    switch (macHeader.GetPacketType())
    {
        case WsnMacHeader::HELLO_PACKET: {
            HelloPacket hello;
            if (DeserializeHelloPacket(pktCopy, hello))
            {
                if (!m_helloRxCb.IsNull())
                {
                    m_helloRxCb(hello);
                }
            }
            break;
        }

        case WsnMacHeader::CL_ANNOUNCEMENT: {
            CLAnnouncementPacket announcement;
            if (DeserializeCLAnnouncement(pktCopy, announcement))
            {
                if (!m_clAnnRxCb.IsNull())
                {
                    m_clAnnRxCb(announcement);
                }
            }
            break;
        }

        case WsnMacHeader::MEMBER_FEEDBACK: {
            CLMemberFeedbackPacket feedback;
            if (DeserializeMemberFeedback(pktCopy, feedback))
            {
                if (!m_feedbackRxCb.IsNull())
                {
                    m_feedbackRxCb(feedback);
                }
            }
            break;
        }

        case WsnMacHeader::UAV_FRAGMENT: {
            Fragment fragment;
            if (DeserializeFragment(pktCopy, fragment))
            {
                if (!m_fragmentRxCb.IsNull())
                {
                    m_fragmentRxCb(fragment);
                }
            }
            break;
        }

        default:
            NS_LOG_WARN("Unknown packet type: " << (int)macHeader.GetPacketType());
            return false;
    }

    return true;
}

bool
WsnRadioMac::TransmitPacket(Ptr<Packet> frame, WsnMacHeader::PacketType type,
                           uint16_t srcId, uint16_t dstId)
{
    NS_LOG_FUNCTION(this << frame->GetSize() << (int)type);

    if (!m_radioDevice || !m_cc2420Mac)
    {
        NS_LOG_WARN("No radio device/cc2420 MAC attached");
        m_txFailures++;
        return false;
    }

    // Create and add MAC header
    WsnMacHeader macHeader;
    macHeader.SetPacketType(type);
    macHeader.SetSeqNum(m_seqNum++);
    macHeader.SetSource(srcId);
    macHeader.SetDestination(dstId);

    Ptr<Packet> pkt = frame->Copy();
    pkt->AddHeader(macHeader);

    // Transmit via cc2420 MAC path (required by architecture)
    Mac16Address dstAddr("ff:ff");
    if (dstId != 0xFFFF)
    {
        char dstAddrStr[6];
        std::snprintf(dstAddrStr, sizeof(dstAddrStr), "%02x:%02x",
                      static_cast<unsigned>((dstId >> 8) & 0xff),
                      static_cast<unsigned>(dstId & 0xff));
        dstAddr = Mac16Address(dstAddrStr);
    }
    bool success = m_cc2420Mac->McpsDataRequest(pkt, dstAddr, true);

    if (success)
    {
        m_packetsSent++;
        NS_LOG_INFO("Transmitted packet " << m_seqNum - 1 << " via radio (size: "
                                          << pkt->GetSize() << " bytes)");
    }
    else
    {
        m_txFailures++;
        NS_LOG_WARN("Failed to transmit packet");
    }

    return success;
}

} // namespace ns3
