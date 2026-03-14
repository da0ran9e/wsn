/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "cc2420-contact-window-model.h"

#include "cc2420-phy.h"
#include "../../propagation/cc2420-spectrum-propagation-loss-model.h"

#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"

#include <algorithm>
#include <cmath>

namespace ns3
{
namespace wsn
{

NS_LOG_COMPONENT_DEFINE("Cc2420ContactWindowModel");
NS_OBJECT_ENSURE_REGISTERED(Cc2420ContactWindowModel);

TypeId
Cc2420ContactWindowModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::wsn::Cc2420ContactWindowModel")
        .SetParent<Object>()
        .SetGroupName("Cc2420")
        .AddConstructor<Cc2420ContactWindowModel>()
        .AddAttribute("Enabled",
                      "Enable contact-time gating before PHY reception evaluation.",
                      BooleanValue(true),
                      MakeBooleanAccessor(&Cc2420ContactWindowModel::m_enabled),
                      MakeBooleanChecker())
        .AddAttribute("DataRateBps",
                      "Effective PHY data rate used to estimate packet airtime.",
                      DoubleValue(250000.0),
                      MakeDoubleAccessor(&Cc2420ContactWindowModel::m_dataRateBps),
                      MakeDoubleChecker<double>(1.0))
        .AddAttribute("GuardTimeSeconds",
                      "Extra margin required beyond pure packet airtime.",
                      DoubleValue(0.002),
                      MakeDoubleAccessor(&Cc2420ContactWindowModel::m_guardTimeSeconds),
                      MakeDoubleChecker<double>(0.0))
        .AddAttribute("SampleStepSeconds",
                      "Sampling step for projected link validation during one packet airtime.",
                      DoubleValue(0.001),
                      MakeDoubleAccessor(&Cc2420ContactWindowModel::m_sampleStepSeconds),
                      MakeDoubleChecker<double>(1e-5))
        .AddAttribute("RequiredMarginDb",
                      "Extra RSSI margin above sensitivity required for contact validation.",
                      DoubleValue(0.0),
                      MakeDoubleAccessor(&Cc2420ContactWindowModel::m_requiredMarginDb),
                      MakeDoubleChecker<double>())
        .AddAttribute("EnableVelocityAwareMargin",
                      "Enable extra contact margin when coherence time is small versus packet airtime.",
                      BooleanValue(false),
                      MakeBooleanAccessor(&Cc2420ContactWindowModel::m_enableVelocityAwareMargin),
                      MakeBooleanChecker())
        .AddAttribute("CarrierFrequencyHz",
                      "Carrier frequency used for Doppler/coherence-time based margin (Hz).",
                      DoubleValue(2.4e9),
                      MakeDoubleAccessor(&Cc2420ContactWindowModel::m_carrierFrequencyHz),
                      MakeDoubleChecker<double>(1e6))
        .AddAttribute("VelocityPenaltySlopeDb",
                      "Slope of additional margin versus airtime/Tc ratio above 1.",
                      DoubleValue(2.0),
                      MakeDoubleAccessor(&Cc2420ContactWindowModel::m_velocityPenaltySlopeDb),
                      MakeDoubleChecker<double>(0.0))
        .AddAttribute("VelocityPenaltyCapDb",
                      "Maximum additional margin from velocity-aware penalty (dB).",
                      DoubleValue(8.0),
                      MakeDoubleAccessor(&Cc2420ContactWindowModel::m_velocityPenaltyCapDb),
                      MakeDoubleChecker<double>());
    return tid;
}

Cc2420ContactWindowModel::Cc2420ContactWindowModel()
    : m_enabled(true),
      m_dataRateBps(250000.0),
      m_guardTimeSeconds(0.002),
      m_sampleStepSeconds(0.001),
    m_requiredMarginDb(0.0),
    m_enableVelocityAwareMargin(false),
    m_carrierFrequencyHz(2.4e9),
    m_velocityPenaltySlopeDb(2.0),
    m_velocityPenaltyCapDb(8.0)
{
    NS_LOG_FUNCTION(this);
}

bool
Cc2420ContactWindowModel::IsEnabled() const
{
    return m_enabled;
}

double
Cc2420ContactWindowModel::GetPacketAirtimeSeconds(uint32_t packetSizeBytes) const
{
    if (packetSizeBytes == 0)
    {
        return 0.0;
    }
    return (static_cast<double>(packetSizeBytes) * 8.0) / m_dataRateBps;
}

bool
Cc2420ContactWindowModel::HasContactForPacket(Ptr<const Cc2420Phy> txPhy,
                                              Ptr<const Cc2420Phy> rxPhy,
                                              uint32_t packetSizeBytes) const
{
    if (!m_enabled || packetSizeBytes == 0)
    {
        return true;
    }

    if (!txPhy || !rxPhy)
    {
        return false;
    }

    Ptr<const MobilityModel> txMob = txPhy->GetMobility();
    Ptr<const MobilityModel> rxMob = rxPhy->GetMobility();
    Ptr<propagation::Cc2420SpectrumPropagationLossModel> propagation = rxPhy->GetPropagationLossModel();
    if (!txMob || !rxMob || !propagation)
    {
        return true;
    }

    const double airtime = GetPacketAirtimeSeconds(packetSizeBytes);
    const double requiredTime = airtime + m_guardTimeSeconds;
    const double sampleStep = std::min(std::max(m_sampleStepSeconds, 1e-5), std::max(requiredTime, 1e-5));

    const Vector txStart = txMob->GetPosition();
    const Vector rxStart = rxMob->GetPosition();
    const Vector txVel = txMob->GetVelocity();
    const Vector rxVel = rxMob->GetVelocity();

    // Velocity-aware margin from coherence-time approximation:
    // fD,max ~= (v_rel / c) * fc ; Tc ~= 0.423 / fD,max.
    // If airtime/Tc > 1, add a bounded extra margin.
    double velocityPenaltyDb = 0.0;
    if (m_enableVelocityAwareMargin && airtime > 0.0)
    {
        const Vector relVel(txVel.x - rxVel.x, txVel.y - rxVel.y, txVel.z - rxVel.z);
        const double vRel = std::sqrt(relVel.x * relVel.x + relVel.y * relVel.y + relVel.z * relVel.z);
        const double c = 3.0e8;
        const double fD = (vRel / c) * m_carrierFrequencyHz;
        if (fD > 1e-9)
        {
            const double tc = 0.423 / fD;
            const double ratio = airtime / tc;
            if (ratio > 1.0)
            {
                velocityPenaltyDb = std::min(m_velocityPenaltyCapDb,
                                             (ratio - 1.0) * m_velocityPenaltySlopeDb);
            }
        }
    }

    const double minRxDbm = rxPhy->GetRxSensitivity() + m_requiredMarginDb + velocityPenaltyDb;

    for (double dt = 0.0; dt <= requiredTime + 1e-9; dt += sampleStep)
    {
        const Vector txProjected(txStart.x + txVel.x * dt,
                                 txStart.y + txVel.y * dt,
                                 txStart.z + txVel.z * dt);
        const Vector rxProjected(rxStart.x + rxVel.x * dt,
                                 rxStart.y + rxVel.y * dt,
                                 rxStart.z + rxVel.z * dt);

        const double rxPowerDbm = propagation->CalcRxPowerDbmFromPositions(
            txPhy->GetTxPower(), txProjected, rxProjected, false);
        if (rxPowerDbm < minRxDbm)
        {
            NS_LOG_DEBUG("[ContactWindow] insufficient contact: dt=" << dt
                         << "s required=" << requiredTime
                         << "s rx=" << rxPowerDbm
                         << "dBm threshold=" << minRxDbm
                         << "dBm (baseMargin=" << m_requiredMarginDb
                         << "dB velocityPenalty=" << velocityPenaltyDb << "dB)");
            return false;
        }
    }

    return true;
}

} // namespace wsn
} // namespace ns3
