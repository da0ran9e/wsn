/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "cc2420-helper.h"

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/mobility-model.h"
#include "ns3/antenna-model.h"

namespace ns3
{
namespace cc2420
{

NS_LOG_COMPONENT_DEFINE("Cc2420Helper");

// =============================================================================
// Cc2420Helper Implementation
// =============================================================================

Cc2420Helper::Cc2420Helper()
{
    NS_LOG_FUNCTION(this);

    // Set default factories
    m_macFactory.SetTypeId("ns3::cc2420::Cc2420Mac");
    m_phyFactory.SetTypeId("ns3::cc2420::Cc2420Phy");
    m_energyFactory.SetTypeId("ns3::cc2420::Cc2420EnergyModel");
}

Cc2420Helper::~Cc2420Helper()
{
    NS_LOG_FUNCTION(this);
}

void
Cc2420Helper::SetChannel(Ptr<SpectrumChannel> channel)
{
    m_channel = channel;
}

void
Cc2420Helper::SetMacAttribute(const std::string& name, const AttributeValue& value)
{
    m_macFactory.Set(name, value);
}

void
Cc2420Helper::SetPhyAttribute(const std::string& name, const AttributeValue& value)
{
    m_phyFactory.Set(name, value);
}

void
Cc2420Helper::SetEnergyAttribute(const std::string& name, const AttributeValue& value)
{
    m_energyFactory.Set(name, value);
}

NetDeviceContainer
Cc2420Helper::Install(NodeContainer c) const
{
    NetDeviceContainer devices;

    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        devices.Add(Install(*i));
    }

    return devices;
}

Ptr<NetDevice>
Cc2420Helper::Install(Ptr<Node> node) const
{
    return CreateDevice(node);
}

Ptr<NetDevice>
Cc2420Helper::CreateDevice(Ptr<Node> node) const
{
    NS_LOG_FUNCTION(this << node);

    // Create components
    Ptr<Cc2420NetDevice> dev = CreateObject<Cc2420NetDevice>();
    Ptr<Cc2420Mac> mac = m_macFactory.Create<Cc2420Mac>();
    Ptr<Cc2420Phy> phy = m_phyFactory.Create<Cc2420Phy>();
    Ptr<Cc2420EnergyModel> energyModel = m_energyFactory.Create<Cc2420EnergyModel>();

    // Setup device
    dev->SetMac(mac);
    dev->SetPhy(phy);
    dev->SetChannel(m_channel);

    // Setup MAC-PHY connection
    mac->SetPhy(phy);

    // Setup energy model
    energyModel->SetPhy(phy);

    // Setup PHY with node's mobility
    if (node->GetObject<MobilityModel>())
    {
        phy->SetMobility(node->GetObject<MobilityModel>());
    }

    // Attach device to node
    node->AddDevice(dev);

    // Setup address
    Mac16Address addr = Mac16Address::Allocate();
    dev->SetAddress(addr);
    mac->GetMacConfig().shortAddress = addr;

    return dev;
}

} // namespace cc2420
} // namespace ns3
