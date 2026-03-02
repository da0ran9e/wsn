/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Wireless Sensor Network Team
 *
 * CC2420 Network Device
 */

#ifndef CC2420_NET_DEVICE_H
#define CC2420_NET_DEVICE_H

#include "cc2420-mac.h"
#include "cc2420-phy.h"

#include "ns3/net-device.h"
#include "ns3/channel.h"
#include "ns3/traced-callback.h"

#include <string>

namespace ns3
{
namespace wsn
{

/**
 * @ingroup cc2420
 *
 * CC2420 Network Device
 *
 * Wraps the MAC and PHY layers as an ns-3 NetDevice.
 * Provides the standard NetDevice interface for packet transmission
 * and reception integration with ns-3 core.
 */
class Cc2420NetDevice : public NetDevice
{
  public:
    /**
     * @brief Get the TypeId
     * @return the TypeId
     */
    static TypeId GetTypeId();

    Cc2420NetDevice();
    ~Cc2420NetDevice() override;

    // =============================================================================
    // Component Setup
    // =============================================================================

    /**
     * @brief Set the MAC layer
     * @param mac the MAC layer
     */
    void SetMac(Ptr<Cc2420Mac> mac);

    /**
     * @brief Get the MAC layer
     * @return the MAC layer
     */
    Ptr<Cc2420Mac> GetMac() const;

    /**
     * @brief Set the PHY layer
     * @param phy the PHY layer
     */
    void SetPhy(Ptr<Cc2420Phy> phy);

    /**
     * @brief Get the PHY layer
     * @return the PHY layer
     */
    Ptr<Cc2420Phy> GetPhy() const;

    /**
     * @brief Set the channel
     * @param channel the SpectrumChannel
     */
    void SetChannel(Ptr<SpectrumChannel> channel);

    // =============================================================================
    // NetDevice Interface Implementation
    // =============================================================================

    /**
     * Set the interface index
     */
    void SetIfIndex(const uint32_t index) override;

    /**
     * Get the interface index
     */
    uint32_t GetIfIndex() const override;

    /**
     * Get the channel
     */
    Ptr<Channel> GetChannel() const override;

    /**
     * Set the address (short address in this case)
     */
    void SetAddress(Address address) override;

    /**
     * Get the address
     */
    Address GetAddress() const override;

    /**
     * Get the MTU
     */
    uint16_t GetMtu() const override;

    /**
     * Set the MTU
     */
    bool SetMtu(const uint16_t mtu) override;

    /**
     * Is link up?
     */
    bool IsLinkUp() const override;

    /**
     * Set link change callback
     */
    void AddLinkChangeCallback(Callback<void> callback) override;

    /**
     * Is broadcast?
     */
    bool IsBroadcast() const override;

    /**
     * Get broadcast address
     */
    Address GetBroadcast() const override;

    /**
     * Is multicast?
     */
    bool IsMulticast() const override;

    /**
     * Get multicast address
     */
    Address GetMulticast(Ipv4Address multicastGroup) const override;

    /**
     * Get multicast address (IPv6)
     */
    Address GetMulticast(Ipv6Address addr) const override;

    /**
     * Is point-to-point?
     */
    bool IsPointToPoint() const override;

    /**
     * Is bridge?
     */
    bool IsBridge() const override;

    /**
     * Get attached node
     */
    Ptr<Node> GetNode() const override;

    /**
     * Set attached node
     */
    void SetNode(Ptr<Node> node) override;

    /**
     * Whether this device needs ARP
     */
    bool NeedsArp() const override;

    /**
     * Send packet
     */
    bool Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) override;

    /**
     * Send packet to (not implemented in v1.0)
     */
    bool SendFrom(Ptr<Packet> packet,
                  const Address& source,
                  const Address& dest,
                  uint16_t protocolNumber) override;

    /**
     * Set receive callback
     */
    void SetReceiveCallback(NetDevice::ReceiveCallback cb) override;

    /**
     * Set promiscuous receive callback
     */
    void SetPromiscReceiveCallback(NetDevice::PromiscReceiveCallback cb) override;

    /**
     * Supports sending and receiving with addresses
     */
    bool SupportsSendFrom() const override;

    /**
     * Get device name
     */
    std::string GetName() const;

    /**
     * Set device name
     */
    void SetName(const std::string& name);

    /**
     * Callback for packet path debugging in NetDevice layer
     * Arguments: event name, packet
     */
    typedef Callback<void, std::string, Ptr<const Packet>> DebugPacketTraceCallback;

    /**
     * Set debug callback for NetDevice packet path tracing
     */
    void SetDebugPacketTraceCallback(DebugPacketTraceCallback callback);

  private:
    // =============================================================================
    // Helper Methods
    // =============================================================================

    /**
     * Callback from MAC layer when frame is received
     */
    void ReceiveFrameFromMac(Ptr<Packet> packet, Mac16Address source, double rssi);

    /**
     * Callback from MAC layer when TX is complete
     */
    void TxCompleteFromMac(int status);

    void OnMacDebugTrace(std::string eventName, Ptr<const Packet> packet);
    void OnPhyDebugTrace(std::string eventName, Ptr<const Packet> packet);
    void EmitDebugTrace(const std::string& eventName, Ptr<const Packet> packet) const;

    // =============================================================================
    // Member Variables
    // =============================================================================

    Ptr<Cc2420Mac> m_mac;
    Ptr<Cc2420Phy> m_phy;
    Ptr<SpectrumChannel> m_channel;
    Ptr<Node> m_node;

    uint32_t m_ifIndex;
    std::string m_name;
    Mac16Address m_address;
    uint16_t m_mtu;
    bool m_linkUp;

    NetDevice::ReceiveCallback m_receiveCallback;
    NetDevice::PromiscReceiveCallback m_promiscuousReceiveCallback;
    Callback<void> m_linkChangeCallback;
    DebugPacketTraceCallback m_debugPacketTraceCallback;
};

} // namespace wsn
} // namespace ns3

#endif // CC2420_NET_DEVICE_H
