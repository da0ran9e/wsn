/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "cc2420-net-device.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-channel.h"

namespace ns3
{
namespace wsn
{

NS_LOG_COMPONENT_DEFINE("Cc2420NetDevice");
NS_OBJECT_ENSURE_REGISTERED(Cc2420NetDevice);

// =============================================================================
// Cc2420NetDevice Implementation
// =============================================================================

TypeId
Cc2420NetDevice::GetTypeId()
{
    static TypeId tid = TypeId("ns3::cc2420::Cc2420NetDevice")
        .SetParent<NetDevice>()
        .SetGroupName("Cc2420")
        .AddConstructor<Cc2420NetDevice>();
    return tid;
}

Cc2420NetDevice::Cc2420NetDevice()
    : m_ifIndex(0),
      m_name("cc2420"),
      m_address(),
      m_mtu(127),
      m_linkUp(false)
{
    NS_LOG_FUNCTION(this);
}

Cc2420NetDevice::~Cc2420NetDevice()
{
    NS_LOG_FUNCTION(this);
}

// =============================================================================
// Component Setup
// =============================================================================

void
Cc2420NetDevice::SetMac(Ptr<Cc2420Mac> mac)
{
    m_mac = mac;
    if (m_mac)
    {
        m_mac->SetMcpsDataIndicationCallback(
            MakeCallback(&Cc2420NetDevice::ReceiveFrameFromMac, this));
        m_mac->SetMcpsDataConfirmCallback(
            MakeCallback(&Cc2420NetDevice::TxCompleteFromMac, this));
    }
}

Ptr<Cc2420Mac>
Cc2420NetDevice::GetMac() const
{
    return m_mac;
}

void
Cc2420NetDevice::SetPhy(Ptr<Cc2420Phy> phy)
{
    m_phy = phy;
    if (m_phy)
    {
        m_phy->SetDevice(this);
    }
}

Ptr<Cc2420Phy>
Cc2420NetDevice::GetPhy() const
{
    return m_phy;
}

void
Cc2420NetDevice::SetChannel(Ptr<SpectrumChannel> channel)
{
    m_channel = channel;
    if (m_phy)
    {
        m_phy->SetChannel(channel);
    }
}

// =============================================================================
// NetDevice Interface
// =============================================================================

void
Cc2420NetDevice::SetIfIndex(const uint32_t index)
{
    m_ifIndex = index;
}

uint32_t
Cc2420NetDevice::GetIfIndex() const
{
    return m_ifIndex;
}

Ptr<Channel>
Cc2420NetDevice::GetChannel() const
{
    return DynamicCast<Channel>(m_channel);
}

void
Cc2420NetDevice::SetAddress(Address address)
{
    m_address = Mac16Address::ConvertFrom(address);
}

Address
Cc2420NetDevice::GetAddress() const
{
    return m_address;
}

uint16_t
Cc2420NetDevice::GetMtu() const
{
    return m_mtu;
}

void
Cc2420NetDevice::SetMtu(const uint16_t mtu)
{
    m_mtu = mtu;
}

bool
Cc2420NetDevice::IsLinkUp() const
{
    return m_linkUp;
}

void
Cc2420NetDevice::AddLinkChangeCallback(Callback<void> callback)
{
    m_linkChangeCallback = callback;
}

bool
Cc2420NetDevice::IsBroadcast() const
{
    return true;
}

Address
Cc2420NetDevice::GetBroadcast() const
{
    return Mac16Address("FF:FF");
}

bool
Cc2420NetDevice::IsMulticast() const
{
    return false;
}

Address
Cc2420NetDevice::GetMulticast(Ipv4Address multicastGroup) const
{
    return Mac16Address("FF:FF");
}

Address
Cc2420NetDevice::GetMulticast(Ipv6Address addr) const
{
    return Mac16Address("FF:FF");
}

bool
Cc2420NetDevice::IsPointToPoint() const
{
    return false;
}

bool
Cc2420NetDevice::Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(this << packet << dest << protocolNumber);

    if (!m_mac)
    {
        NS_LOG_WARN("MAC not set");
        return false;
    }

    Mac16Address destAddr = Mac16Address::ConvertFrom(dest);
    return m_mac->McpsDataRequest(packet, destAddr, true);
}

bool
Cc2420NetDevice::SendFrom(Ptr<Packet> packet,
                          const Address& source,
                          const Address& dest,
                          uint16_t protocolNumber)
{
    NS_LOG_WARN("SendFrom not implemented in v1.0");
    return false;
}

void
Cc2420NetDevice::SetReceiveCallback(NetDevice::ReceiveCallback cb)
{
    m_receiveCallback = cb;
}

void
Cc2420NetDevice::SetPromiscuousReceiveCallback(NetDevice::PromiscuousReceiveCallback cb)
{
    m_promiscuousReceiveCallback = cb;
}

bool
Cc2420NetDevice::SupportsSendFrom() const
{
    return false;
}

std::string
Cc2420NetDevice::GetName() const
{
    return m_name;
}

void
Cc2420NetDevice::SetName(const std::string& name)
{
    m_name = name;
}

// =============================================================================
// Private Helper Methods
// =============================================================================

void
Cc2420NetDevice::ReceiveFrameFromMac(Ptr<Packet> packet, Mac16Address source, double rssi)
{
    NS_LOG_FUNCTION(this << packet << source << rssi);

    if (!m_receiveCallback.IsNull())
    {
        m_receiveCallback(this, packet, 0, source);
    }
}

void
Cc2420NetDevice::TxCompleteFromMac(int status)
{
    NS_LOG_FUNCTION(this << status);
    // TODO: Handle TX completion if needed
}

} // namespace cc2420
} // namespace ns3
