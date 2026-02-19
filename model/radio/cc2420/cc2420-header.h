/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Wireless Sensor Network Team
 *
 * CC2420 MAC Frame Header
 * Simplified IEEE 802.15.4 MAC frame header for v1.0
 */

#ifndef CC2420_HEADER_H
#define CC2420_HEADER_H

#include "ns3/header.h"
#include "ns3/mac16-address.h"

namespace ns3
{
namespace cc2420
{

/**
 * @ingroup cc2420
 *
 * Represents a CC2420 MAC Frame Header.
 *
 * This is a simplified version supporting:
 * - Frame Control Field (FCF)
 * - Sequence Number (DSN)
 * - Source PAN ID
 * - Source Address (16-bit)
 * - Destination PAN ID
 * - Destination Address (16-bit)
 *
 * Frame Format (v1.0 - unslotted, no beacon):
 * ============================================
 * | FCF (2B) | DSN (1B) | DestPAN (2B) | DestAddr (2B) |
 * | SrcPAN (2B) | SrcAddr (2B) | Payload |
 */
class Cc2420Header : public Header
{
  public:
    /**
     * Frame Type enumeration (FCF bits 0-2)
     */
    enum FrameType
    {
        FRAME_TYPE_BEACON = 0,      //!< Beacon (not used in v1.0)
        FRAME_TYPE_DATA = 1,        //!< Data frame
        FRAME_TYPE_ACK = 2,         //!< Acknowledgment frame
        FRAME_TYPE_MAC_CMD = 3      //!< MAC command (not used in v1.0)
    };

    Cc2420Header();
    ~Cc2420Header() override;

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
     * @brief Get the size of serialized header
     * @return the serialized size
     */
    uint32_t GetSerializedSize() const override;

    /**
     * @brief Serialize the header
     * @param start iterator to serialize at
     */
    void Serialize(Buffer::Iterator start) const override;

    /**
     * @brief Deserialize the header
     * @param start iterator to deserialize from
     * @return bytes read
     */
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * @brief Print the header
     * @param os output stream
     */
    void Print(std::ostream& os) const override;

    // Frame Control Field accessors
    void SetFrameType(FrameType type);
    FrameType GetFrameType() const;

    void SetSecurityEnabled(bool enabled);
    bool GetSecurityEnabled() const;

    void SetFramePending(bool pending);
    bool GetFramePending() const;

    void SetAckRequest(bool ackReq);
    bool GetAckRequest() const;

    void SetPanIdCompression(bool compression);
    bool GetPanIdCompression() const;

    void SetDestinationAddressingMode(uint8_t mode);
    uint8_t GetDestinationAddressingMode() const;

    void SetSourceAddressingMode(uint8_t mode);
    uint8_t GetSourceAddressingMode() const;

    // Sequence number (DSN)
    void SetSequenceNumber(uint8_t dsn);
    uint8_t GetSequenceNumber() const;

    // PAN IDs
    void SetDestinationPanId(uint16_t panId);
    uint16_t GetDestinationPanId() const;

    void SetSourcePanId(uint16_t panId);
    uint16_t GetSourcePanId() const;

    // Addresses
    void SetDestinationAddress(Mac16Address addr);
    Mac16Address GetDestinationAddress() const;

    void SetSourceAddress(Mac16Address addr);
    Mac16Address GetSourceAddress() const;

  private:
    // Frame Control Field (2 bytes)
    uint16_t m_frameControl;

    // Sequence Number (1 byte)
    uint8_t m_sequenceNumber;

    // PAN IDs (2 bytes each)
    uint16_t m_destinationPanId;
    uint16_t m_sourcePanId;

    // Addresses (2 bytes each for short addressing)
    Mac16Address m_destinationAddress;
    Mac16Address m_sourceAddress;
};

} // namespace cc2420
} // namespace ns3

#endif // CC2420_HEADER_H
