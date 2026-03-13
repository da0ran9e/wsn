/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "cc2420-error-model.h"

#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/boolean.h"

#include <cmath>
#include <algorithm>

namespace ns3
{
namespace wsn
{

NS_LOG_COMPONENT_DEFINE("Cc2420ErrorModel");
NS_OBJECT_ENSURE_REGISTERED(Cc2420ErrorModel);

TypeId
Cc2420ErrorModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::wsn::Cc2420ErrorModel")
            .SetParent<Object>()
            .SetGroupName("Cc2420")
            .AddConstructor<Cc2420ErrorModel>()
            // ── Enable / disable ─────────────────────────────────────────────
            .AddAttribute("Enabled",
                          "Enable BER-based packet dropping.  "
                          "Set to false for an ideal (error-free) channel.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&Cc2420ErrorModel::m_enabled),
                          MakeBooleanChecker())
            // ── DSSS processing gain ──────────────────────────────────────────
            //
            //  CC2420 at 2.4 GHz:
            //    chip_rate = 2 Mcps,  bit_rate = 250 kbps
            //    Gp = chip_rate / bit_rate = 8  →  10·log10(8) ≈ 9.03 dB
            //
            //  Increasing this value shifts the BER curve to the left
            //  (better sensitivity); decreasing it shifts it right (worse).
            .AddAttribute("ProcessingGainDb",
                          "DSSS spreading processing gain [dB].  "
                          "Default: 10·log10(2e6/250e3) = 9.03 dB (CC2420).",
                          DoubleValue(9.03),
                          MakeDoubleAccessor(&Cc2420ErrorModel::m_processingGainDb),
                          MakeDoubleChecker<double>(0.0, 30.0));
    return tid;
}

Cc2420ErrorModel::Cc2420ErrorModel()
    : m_enabled(true),
      m_processingGainDb(9.03),
      m_rng(CreateObject<UniformRandomVariable>())
{
    NS_LOG_FUNCTION(this);
}

// =============================================================================
// Enable / Disable
// =============================================================================

void
Cc2420ErrorModel::SetEnabled(bool enable)
{
    m_enabled = enable;
}

bool
Cc2420ErrorModel::IsEnabled() const
{
    return m_enabled;
}

// =============================================================================
// BER  —  Bit Error Rate from received SNR
// =============================================================================
//
//  CC2420 uses O-QPSK with DSSS spreading.  The DSSS correlator provides a
//  processing gain Gp that effectively boosts Eb/N0:
//
//    Eb/N0 = SNR_linear × Gp
//
//  The erfc formula below is the standard matched-filter bound for O-QPSK:
//
//    BER = (1/2) × erfc( sqrt(Eb/N0) )
//
//  Intuition:
//   • std::erfc(x) = 1 − erf(x)  →  rapidly decays for x > 1.
//   • At Eb/N0 = 0 dB (SNR = −9 dB after gain) → erfc(1) ≈ 0.157 → BER ≈ 0.08
//   • At Eb/N0 = 10 dB             → erfc(3.16) ≈ 2.2e-5 → BER ≈ 1e-5
//   • At Eb/N0 = 20 dB             → BER ≈ 8.2e-24  (perfect link)
//
//  Example for scenario4 (CC2420 at 0 dBm TX):
//   SNR = 5 dB  →  Eb/N0 = 5 + 9.03 ≈ 14 dB  →  BER ≈ 3.4e-6
//   SNR = 0 dB  →  Eb/N0 ≈ 9 dB               →  BER ≈ 4.6e-4
//   SNR = −3 dB →  Eb/N0 ≈ 6 dB               →  BER ≈ 5.1e-3
// =============================================================================

double
Cc2420ErrorModel::GetBer(double snrDb) const
{
    // 1. Linear SNR (received power / noise power)
    const double snrLinear = std::pow(10.0, snrDb / 10.0);

    // 2. Apply DSSS processing gain to get Eb/N0 (per bit, linear)
    const double gpLinear = std::pow(10.0, m_processingGainDb / 10.0);
    const double ebN0     = snrLinear * gpLinear;

    // 3. O-QPSK erfc formula
    //    std::erfc is defined in <cmath> for all C++11 conforming compilers.
    const double ber = 0.5 * std::erfc(std::sqrt(ebN0));

    // Clamp to physically valid range [0, 0.5]
    // (BER > 0.5 would mean the receiver is inverting bits — not physical)
    return std::max(0.0, std::min(0.5, ber));
}

// =============================================================================
// PER  —  Packet Error Rate from BER and packet length
// =============================================================================
//
//  A packet of L bytes contains 8·L independent bits.  Assuming each bit
//  experiences the same BER (short packet, quasi-static channel), the
//  probability that ALL bits are received correctly is:
//
//    P_correct = (1 − BER)^(8·L)
//
//  Therefore:
//    PER = 1 − (1 − BER)^(8·L)
//
//  Why this matters for fragmentation:
//    If a file is split into N fragments of L bytes each, the probability
//    that ALL N fragments arrive without error is:
//
//    P_all = (1 − PER)^N  =  [(1 − BER)^(8·L)]^N  =  (1 − BER)^(8·L·N)
//
//  i.e. the total bit count of the entire transfer determines reliability.
//  At BER = 1e-3, a 128-byte fragment already has PER ≈ 64%.
// =============================================================================

double
Cc2420ErrorModel::GetPer(double ber, uint32_t packetSizeBytes) const
{
    if (packetSizeBytes == 0 || ber <= 0.0)
    {
        return 0.0; // no bits → no error
    }
    if (ber >= 0.5)
    {
        return 1.0; // link is completely broken
    }

    const uint32_t totalBits = packetSizeBytes * 8u;
    const double   per       = 1.0 - std::pow(1.0 - ber, static_cast<double>(totalBits));
    return std::max(0.0, std::min(1.0, per));
}

// =============================================================================
// PacketIsLost  —  stochastic drop decision
// =============================================================================
//
//  Draws one uniform variate u ∈ [0, 1).  Packet is "lost" when u < PER.
//  This models the go/no-go channel outcome as a Bernoulli trial.
//
//  For reproducible runs use AssignStreams() before the simulation starts.
// =============================================================================

bool
Cc2420ErrorModel::PacketIsLost(double per)
{
    if (!m_enabled || per <= 0.0)
    {
        return false;
    }
    if (per >= 1.0)
    {
        return true;
    }
    return (m_rng->GetValue() < per);
}

// =============================================================================
// Stream assignment
// =============================================================================

int64_t
Cc2420ErrorModel::AssignStreams(int64_t stream)
{
    m_rng->SetStream(stream);
    return 1;
}

} // namespace wsn
} // namespace ns3
