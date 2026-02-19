/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Wireless Sensor Network Team
 *
 * CC2420 MAC Frame Trailer (FCS)
 */

#ifndef CC2420_TRAILER_H
#define CC2420_TRAILER_H

#include "ns3/trailer.h"

namespace ns3
{
namespace cc2420
{

/**
 * @ingroup cc2420
 *
 * Represents a CC2420 MAC Frame Trailer (FCS - Frame Check Sequence).
 *
 * Contains:
 * - CRC/FCS (2 bytes) - Frame Check Sequence
 * - LQI (1 byte) - Link Quality Indicator (optional, added on reception)
 */
class Cc2420Trailer : public Trailer
{
  public:
    Cc2420Trailer();
    ~Cc2420Trailer() override;

    /**
     * @brief Get the TypeId
     * @return the TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Get instance type
     * @return the instance TypeId
     */
    TypeId GetInstanceTypeId() const override;

    /**
     * @brief Get the size of serialized trailer
     * @return the serialized size
     */
    uint32_t GetSerializedSize() const override;

    /**
     * @brief Serialize the trailer
     * @param start iterator to serialize at
     */
    void Serialize(Buffer::Iterator start) const override;

    /**
     * @brief Deserialize the trailer
     * @param start iterator to deserialize from
     * @return bytes read
     */
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * @brief Print the trailer
     * @param os output stream
     */
    void Print(std::ostream& os) const override;

    /**
     * @brief Set the FCS value
     * @param fcs the Frame Check Sequence
     */
    void SetFrameCheckSequence(uint16_t fcs);

    /**
     * @brief Get the FCS value
     * @return the Frame Check Sequence
     */
    uint16_t GetFrameCheckSequence() const;

    /**
     * @brief Set the LQI value (added on reception)
     * @param lqi the Link Quality Indicator (0-255)
     */
    void SetLinkQualityIndicator(uint8_t lqi);

    /**
     * @brief Get the LQI value
     * @return the Link Quality Indicator
     */
    uint8_t GetLinkQualityIndicator() const;

  private:
    // Frame Check Sequence (2 bytes)
    uint16_t m_fcs;

    // Link Quality Indicator (1 byte, added on reception)
    uint8_t m_lqi;
};

} // namespace cc2420
} // namespace ns3

#endif // CC2420_TRAILER_H
