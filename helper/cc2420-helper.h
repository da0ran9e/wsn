/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Wireless Sensor Network Team
 *
 * CC2420 Helper
 * Helper class for easy setup of CC2420 devices
 */

#ifndef CC2420_HELPER_H
#define CC2420_HELPER_H

#include "../model/radio/cc2420/cc2420-net-device.h"
#include "../model/radio/cc2420/cc2420-mac.h"
#include "../model/radio/cc2420/cc2420-phy.h"
#include "../model/radio/cc2420/cc2420-energy-model.h"

#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/spectrum-channel.h"
#include "ns3/single-model-spectrum-channel.h"
#include "ns3/object-factory.h"

namespace ns3
{
namespace wsn
{

/**
 * @ingroup cc2420
 *
 * Helper class for CC2420 device installation
 *
 * Simplifies the setup of CC2420 network devices on nodes,
 * including PHY, MAC, and energy model configuration.
 */
class Cc2420Helper
{
  public:
    /**
     * Constructor
     */
    Cc2420Helper();

    /**
     * Destructor
     */
    virtual ~Cc2420Helper();

    /**
     * @brief Set the spectrum channel with default propagation model
     * @param channel the SpectrumChannel
     */
    void SetChannel(Ptr<SpectrumChannel> channel);

    /**
     * @brief Create and configure a spectrum channel with propagation models
     * @return configured SpectrumChannel
     */
    Ptr<SingleModelSpectrumChannel> CreateChannel() const;

    /**
     * @brief Set MAC attribute
     * @param name attribute name
     * @param value attribute value
     */
    void SetMacAttribute(const std::string& name, const AttributeValue& value);

    /**
     * @brief Set PHY attribute
     * @param name attribute name
     * @param value attribute value
     */
    void SetPhyAttribute(const std::string& name, const AttributeValue& value);

    /**
     * @brief Set energy model attribute
     * @param name attribute name
     * @param value attribute value
     */
    void SetEnergyAttribute(const std::string& name, const AttributeValue& value);

    /**
     * @brief Install CC2420 devices on nodes
     * @param c the node container
     * @return the net device container
     */
    NetDeviceContainer Install(NodeContainer c) const;

    /**
     * @brief Install a CC2420 device on a single node
     * @param node the node
     * @return the net device
     */
    Ptr<NetDevice> Install(Ptr<Node> node) const;

  private:
    /**
     * Create a CC2420 device (internal helper)
     */
    Ptr<NetDevice> CreateDevice(Ptr<Node> node) const;

    Ptr<SpectrumChannel> m_channel;
    ObjectFactory m_macFactory;
    ObjectFactory m_phyFactory;
    ObjectFactory m_energyFactory;
};

} // namespace wsn
} // namespace ns3

#endif // CC2420_HELPER_H
