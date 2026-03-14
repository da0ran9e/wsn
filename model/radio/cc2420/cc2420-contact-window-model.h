/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef CC2420_CONTACT_WINDOW_MODEL_H
#define CC2420_CONTACT_WINDOW_MODEL_H

#include "ns3/object.h"

namespace ns3
{
namespace wsn
{

class Cc2420Phy;

/**
 * Predicts whether a link remains receivable long enough to finish a packet.
 *
 * This first implementation uses short-horizon projection over one packet
 * airtime, assuming constant velocity during that short interval.
 */
class Cc2420ContactWindowModel : public Object
{
  public:
    static TypeId GetTypeId();

    Cc2420ContactWindowModel();
    ~Cc2420ContactWindowModel() override = default;

    bool IsEnabled() const;

    double GetPacketAirtimeSeconds(uint32_t packetSizeBytes) const;

    bool HasContactForPacket(Ptr<const Cc2420Phy> txPhy,
                             Ptr<const Cc2420Phy> rxPhy,
                             uint32_t packetSizeBytes) const;

  private:
    bool m_enabled;
    double m_dataRateBps;
    double m_guardTimeSeconds;
    double m_sampleStepSeconds;
    double m_requiredMarginDb;

    // Optional velocity-aware margin based on coherence-time intuition:
    //   fD,max ~= (v_rel / c) * fc,   Tc ~= 0.423 / fD,max.
    // If packet airtime approaches/exceeds Tc, require extra link margin.
    bool m_enableVelocityAwareMargin;
    double m_carrierFrequencyHz;
    double m_velocityPenaltySlopeDb;
    double m_velocityPenaltyCapDb;
};

} // namespace wsn
} // namespace ns3

#endif // CC2420_CONTACT_WINDOW_MODEL_H
