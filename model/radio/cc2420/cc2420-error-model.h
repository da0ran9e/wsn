/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * CC2420 BER/PER Error Model
 *
 * Models bit and packet error rates for the IEEE 802.15.4 O-QPSK DSSS physical
 * layer as implemented by the CC2420 chip at 2.4 GHz.
 *
 * ┌─────────────────────────────────────────────────────────────────┐
 * │  CC2420 PHY parameters (2.4 GHz, IEEE 802.15.4-2006 §6.5)      │
 * │  Modulation  : O-QPSK with half-sine pulse shaping             │
 * │  Spreading   : 16-ary DSSS  (4-bit symbol → 32-chip sequence)  │
 * │  Chip rate   : 2 Mcps                                          │
 * │  Bit rate    : 250 kbps                                        │
 * │  Proc. gain  : Gp = 2e6 / 250e3 = 8  (+9.03 dB)               │
 * └─────────────────────────────────────────────────────────────────┘
 *
 * BER formula (erfc model with DSSS processing gain):
 *
 *   Eb/N0 = SNR_linear × Gp
 *   BER   = (1/2) × erfc( sqrt(Eb/N0) )
 *
 * The erfc formula is the standard matched-filter bound for O-QPSK and gives
 * results consistent with measured CC2420 link data (Zuniga & Krishnamachari
 * 2004; Srinivasan & Levis 2006).
 *
 * PER formula for a packet of L bytes:
 *
 *   PER = 1 − (1 − BER)^(8·L)
 *
 * Each fragment / packet is an independent Bernoulli trial; the product over
 * all N fragments gives the end-to-end reliability used by the application.
 *
 * References
 * ----------
 * [1] Zuniga & Krishnamachari, "Analyzing the Transitional Region in Low Power
 *     Wireless Links", SECON 2004.
 * [2] Srinivasan & Levis, "RSSI is Under Appreciated", EMNETS 2006.
 * [3] IEEE Std 802.15.4-2006, §6.5 and Annex D.
 */

#ifndef CC2420_ERROR_MODEL_H
#define CC2420_ERROR_MODEL_H

#include "ns3/object.h"
#include "ns3/random-variable-stream.h"

#include <cstdint>

namespace ns3
{
namespace wsn
{

/**
 * @ingroup cc2420
 *
 * @brief IEEE 802.15.4 O-QPSK DSSS Bit/Packet Error Rate model for CC2420.
 *
 * Typical usage inside Cc2420Phy::EvaluateReceptionFrom():
 * @code
 *   double ber = m_errorModel->GetBer(snrDb);          // snrDb = rssi - noise
 *   double per = m_errorModel->GetPer(ber, pktBytes);
 *   if (m_errorModel->PacketIsLost(per)) { return false; }
 * @endcode
 *
 * The model is enabled by default.  Set attribute "Enabled = false" to
 * disable BER-based drops (useful for debugging / ideal-channel scenarios).
 */
class Cc2420ErrorModel : public Object
{
  public:
    static TypeId GetTypeId();

    Cc2420ErrorModel();
    ~Cc2420ErrorModel() override = default;

    // -------------------------------------------------------------------------
    // Enable / Disable
    // -------------------------------------------------------------------------

    /**
     * @brief Enable or disable BER-based packet dropping.
     *
     * When disabled, PacketIsLost() always returns false (perfect channel).
     */
    void SetEnabled(bool enable);

    /** @return true when BER-based dropping is active. */
    bool IsEnabled() const;

    // -------------------------------------------------------------------------
    // Core computations
    // -------------------------------------------------------------------------

    /**
     * @brief Compute Bit Error Rate (BER) from received SNR.
     *
     * Converts signal SNR (received power / noise floor, in dB) to BER using
     * the O-QPSK erfc model with CC2420 DSSS processing gain:
     *
     *   Eb/N0 [linear] = 10^(snrDb/10) × 10^(processingGainDb/10)
     *   BER            = (1/2) × erfc( sqrt(Eb/N0) )
     *
     * @param snrDb  SNR at the receiver in dB  (= rssiDbm − noiseFloorDbm).
     * @return       BER in [0, 0.5].
     */
    double GetBer(double snrDb) const;

    /**
     * @brief Compute Packet Error Rate (PER) from BER and packet size.
     *
     *   PER = 1 − (1 − BER)^(8 × packetSizeBytes)
     *
     * All bytes in the packet (headers + payload) are included because a
     * corrupted header byte is just as fatal as a corrupted payload byte.
     *
     * @param ber              BER from GetBer().
     * @param packetSizeBytes  Total packet size in bytes.
     * @return                 PER in [0, 1].
     */
    double GetPer(double ber, uint32_t packetSizeBytes) const;

    /**
     * @brief Stochastic packet-drop decision.
     *
     * Draws one uniform sample u ∈ [0,1).  Returns true (packet lost) when
     * u < per.  Must only be called after IsEnabled() returns true.
     *
     * @param per  Packet Error Rate from GetPer().
     * @return     true if the packet should be discarded.
     */
    bool PacketIsLost(double per);

    /**
     * Assign deterministic stream(s) for reproducible simulations.
     * @return Number of streams consumed (always 1).
     */
    int64_t AssignStreams(int64_t stream);

  private:
    bool   m_enabled;           ///< Gate: false → never drop
    double m_processingGainDb;  ///< DSSS processing gain [dB] — default 9.03 dB

    Ptr<UniformRandomVariable> m_rng; ///< Uniform draw for PacketIsLost()
};

} // namespace wsn
} // namespace ns3

#endif // CC2420_ERROR_MODEL_H
