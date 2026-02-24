/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "cc2420-phy.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-value.h"
#include "ns3/mobility-model.h"
#include "ns3/antenna-model.h"

namespace ns3
{
namespace wsn
{

NS_LOG_COMPONENT_DEFINE("Cc2420Phy");
NS_OBJECT_ENSURE_REGISTERED(Cc2420Phy);

// =============================================================================
// Cc2420Phy Implementation
// =============================================================================

TypeId
Cc2420Phy::GetTypeId()
{
    static TypeId tid = TypeId("ns3::cc2420::Cc2420Phy")
        .SetParent<SpectrumPhy>()
        .SetGroupName("Cc2420")
        .AddConstructor<Cc2420Phy>()
        .AddAttribute("TxPower",
                      "Transmission power in dBm",
                      DoubleValue(0.0),
                      MakeDoubleAccessor(&Cc2420Phy::m_txPowerDbm),
                      MakeDoubleChecker<double>())
        .AddAttribute("RxSensitivity",
                      "Reception sensitivity in dBm",
                      DoubleValue(-95.0),
                      MakeDoubleAccessor(&Cc2420Phy::m_rxSensitivityDbm),
                      MakeDoubleChecker<double>())
        .AddAttribute("NoiseFloor",
                      "Noise floor in dBm",
                      DoubleValue(-100.0),
                      MakeDoubleAccessor(&Cc2420Phy::m_noiseFloorDbm),
                      MakeDoubleChecker<double>())
        .AddAttribute("CCAThreshold",
                      "CCA threshold in dBm",
                      DoubleValue(-77.0),
                      MakeDoubleAccessor(&Cc2420Phy::m_ccaThresholdDbm),
                      MakeDoubleChecker<double>());
    return tid;
}

Cc2420Phy::Cc2420Phy()
    : m_txPowerDbm(0.0),
      m_rxSensitivityDbm(-95.0),
      m_noiseFloorDbm(-100.0),
      m_ccaThresholdDbm(-77.0),
      m_currentState(PHY_SLEEP),
      m_pendingState(PHY_SLEEP),
      m_totalPowerDbm(-100.0),
      m_lastSignalChange(Seconds(0)),
      m_stateStartTime(Seconds(0)),
      m_previousState(PHY_SLEEP)
{
    NS_LOG_FUNCTION(this);
}

Cc2420Phy::~Cc2420Phy()
{
    NS_LOG_FUNCTION(this);
}

// =============================================================================
// SpectrumPhy Interface
// =============================================================================

void
Cc2420Phy::SetMobility(Ptr<MobilityModel> m)
{
    m_mobility = m;
}

Ptr<MobilityModel>
Cc2420Phy::GetMobility() const
{
    return m_mobility;
}

void
Cc2420Phy::SetAntenna(Ptr<AntennaModel> a)
{
    m_antenna = a;
}

void
Cc2420Phy::SetChannel(Ptr<SpectrumChannel> c)
{
    m_channel = c;
}

int
Cc2420Phy::StartRx(Ptr<SpectrumSignalParameters> params)
{
    NS_LOG_FUNCTION(this << params);
    // TODO: Implement RX signal processing
    return 0;
}

Ptr<NetDevice>
Cc2420Phy::GetDevice() const
{
    return m_netDevice;
}

void
Cc2420Phy::SetDevice(Ptr<NetDevice> d)
{
    m_netDevice = d;
}

Ptr<const SpectrumModel>
Cc2420Phy::GetRxSpectrumModel() const
{
    return m_rxSpectrumModel;
}

void
Cc2420Phy::AddRxAntenna(Ptr<AntennaModel> a)
{
    // TODO: Handle multiple RX antennas if needed
}

// =============================================================================
// CC2420-Specific Interface
// =============================================================================

void
Cc2420Phy::TransmitPacket(Ptr<Packet> packet, Time duration)
{
    NS_LOG_FUNCTION(this << packet << duration);
    // TODO: Implement TX
}

bool
Cc2420Phy::SetState(PhyState newState)
{
    NS_LOG_FUNCTION(this << GetStateName(newState));
    // TODO: Implement state validation and transition
    return true;
}

PhyState
Cc2420Phy::GetState() const
{
    return m_currentState;
}

std::string
Cc2420Phy::GetStateName(PhyState state)
{
    switch (state)
    {
    case PHY_SLEEP:
        return "SLEEP";
    case PHY_IDLE:
        return "IDLE";
    case PHY_RX:
        return "RX";
    case PHY_TX:
        return "TX";
    case PHY_CCA:
        return "CCA";
    case PHY_SWITCHING:
        return "SWITCHING";
    default:
        return "UNKNOWN";
    }
}

bool
Cc2420Phy::PerformCCA()
{
    NS_LOG_FUNCTION(this);
    // TODO: Implement CCA logic
    return true;
}

double
Cc2420Phy::GetRSSI() const
{
    // TODO: Calculate RSSI from received signals
    return m_totalPowerDbm;
}

void
Cc2420Phy::SetTxPower(double powerDbm)
{
    m_txPowerDbm = powerDbm;
}

double
Cc2420Phy::GetTxPower() const
{
    return m_txPowerDbm;
}

void
Cc2420Phy::SetRxSensitivity(double sensitivityDbm)
{
    m_rxSensitivityDbm = sensitivityDbm;
}

double
Cc2420Phy::GetRxSensitivity() const
{
    return m_rxSensitivityDbm;
}

// =============================================================================
// Callback Setup
// =============================================================================

void
Cc2420Phy::SetPdDataIndicationCallback(PdDataIndicationCallback callback)
{
    m_pdDataIndicationCallback = callback;
}

void
Cc2420Phy::SetPdDataConfirmCallback(PdDataConfirmCallback callback)
{
    m_pdDataConfirmCallback = callback;
}

void
Cc2420Phy::SetPlmeCcaConfirmCallback(PlmeCcaConfirmCallback callback)
{
    m_plmeCcaConfirmCallback = callback;
}

void
Cc2420Phy::SetStateChangeCallback(StateChangeCallback callback)
{
    m_stateChangeCallback = callback;
}

// =============================================================================
// Private Helper Methods
// =============================================================================

void
Cc2420Phy::DoStateChange(PhyState newState)
{
    NS_LOG_FUNCTION(this << GetStateName(newState));
    // TODO: Implement state change logic
}

void
Cc2420Phy::TxComplete()
{
    NS_LOG_FUNCTION(this);
    // TODO: Handle TX completion
}

void
Cc2420Phy::RxComplete()
{
    NS_LOG_FUNCTION(this);
    // TODO: Handle RX completion
}

void
Cc2420Phy::ProcessSignalStart(Ptr<SpectrumSignalParameters> params)
{
    NS_LOG_FUNCTION(this);
    // TODO: Implement signal start processing
}

void
Cc2420Phy::ProcessSignalEnd()
{
    NS_LOG_FUNCTION(this);
    // TODO: Implement signal end processing
}

void
Cc2420Phy::UpdateInterference()
{
    NS_LOG_FUNCTION(this);
    // TODO: Update interference levels
}

double
Cc2420Phy::CalculateSNR(const ReceivedSignal& signal) const
{
    // SNR = signal power - interference
    return signal.powerDbm - signal.currentInterference;
}

bool
Cc2420Phy::IsPacketDestroyed(const ReceivedSignal& signal) const
{
    // SIMPLE_COLLISION_MODEL:
    // If any other signal is within 6dB of sensitivity, packet is destroyed
    if (signal.maxInterference > (m_rxSensitivityDbm - 6.0))
        return true;
    return false;
}

} // namespace cc2420
} // namespace ns3
