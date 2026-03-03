/*
 * Copyright (c) 2026 WSN Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: WSN Project <support@wsn-project.org>
 */

#ifndef SCENARIO3_PACKET_HEADER_H
#define SCENARIO3_PACKET_HEADER_H

#include "ns3/header.h"
#include "ns3/mac16-address.h"

#include <stdint.h>

namespace ns3
{
namespace wsn
{
namespace scenario3
{

/**
 * @defgroup Scenario3PacketHeaders Scenario3 Packet Headers
 * @ingroup Wsn
 *
 * This module defines packet header structures for communication between
 * ground nodes and UAV nodes in the Scenario3 network.
 */

/**
 * @brief Message type enumeration
 *
 * Defines all possible message types that can be transmitted
 * between ground nodes and UAVs.
 */
enum MessageType : uint8_t
{
    MSG_TYPE_DISCOVERY = 0,          ///< Node discovery message
    MSG_TYPE_FRAGMENT = 1,           ///< Fragment transmission
    MSG_TYPE_ACKNOWLEDGEMENT = 2,    ///< Acknowledgement message
    MSG_TYPE_HEARTBEAT = 3,          ///< Heartbeat/keep-alive message
    MSG_TYPE_SYNC = 4,               ///< Synchronization message
    MSG_TYPE_QUERY = 5,              ///< Data query message
    MSG_TYPE_RESPONSE = 6,           ///< Query response message
    MSG_TYPE_ALERT = 7,              ///< Alert/warning message
    MSG_TYPE_LOCATION = 8,           ///< Location/position update
    MSG_TYPE_GLOBAL_SETUP_PHASE = 9, ///< Global setup phase coordination message
    MSG_TYPE_RESERVED_10 = 10,       ///< Reserved for future use
    MSG_TYPE_RESERVED_11 = 11,       ///< Reserved for future use
    MSG_TYPE_RESERVED_12 = 12,       ///< Reserved for future use
    MSG_TYPE_RESERVED_13 = 13,       ///< Reserved for future use
    MSG_TYPE_RESERVED_14 = 14,       ///< Reserved for future use
    MSG_TYPE_CUSTOM = 15             ///< Custom message type
};

/**
 * @brief Node type enumeration
 *
 * Identifies the type of node sending the message.
 */
enum NodeType : uint8_t
{
    NODE_TYPE_GROUND = 0,            ///< Ground sensor node
    NODE_TYPE_UAV = 1,               ///< Unmanned Aerial Vehicle
    NODE_TYPE_SINK = 2,              ///< Sink/base station
    NODE_TYPE_UNKNOWN = 3            ///< Unknown node type
};

/**
 * @brief Common packet header for Scenario3 communication
 *
 * This header is used for all messages transmitted between nodes in the
 * Scenario3 network. It provides basic information for routing, packet
 * identification, and network management.
 *
 * **Header Structure (24 bytes):**
 * ```
 * Offset  Size  Field                  Description
 * ------  ----  -----                  -----------
 *  0      1     Version                Protocol version (0x01)
 *  1      1     MessageType            Type of message (see MessageType enum)
 *  2      1     NodeType               Type of sender (see NodeType enum)
 *  3      1     Reserved               Reserved for future use
 *  4      2     SequenceNumber         Message sequence number
 *  6      2     SourceNodeId           ID of sender
 *  8      2     DestinationNodeId      ID of receiver (0xFFFF = broadcast)
 * 10      2     PayloadLength          Length of payload in bytes
 * 12      2     ChecksumVersion        Checksum and version info
 * 14      2     HopLimit               Maximum remaining hops
 * 16      2     TimestampHigh          Timestamp (high 16 bits)
 * 18      2     TimestampLow           Timestamp (low 16 bits)
 * 20      2     Confidence             Confidence level (0-1000)
 * 22      2     Reserved2              Reserved for future extensions
 * ```
 *
 * **Usage Example:**
 * ```cpp
 * Scenario3PacketHeader header;
 * header.SetMessageType(MSG_TYPE_FRAGMENT);
 * header.SetNodeType(NODE_TYPE_GROUND);
 * header.SetSourceNodeId(5);
 * header.SetDestinationNodeId(15);
 * header.SetPayloadLength(64);
 * header.SetSequenceNumber(1);
 * 
 * Ptr<Packet> packet = Create<Packet>(payload, 64);
 * packet->AddHeader(header);
 * ```
 */
class Scenario3PacketHeader : public Header
{
  public:
    Scenario3PacketHeader();
    ~Scenario3PacketHeader() override;

    /**
     * @brief Register this type with the TypeId system
     *
     * @return TypeId for this class
     */
    static TypeId GetTypeId();

    /**
     * @brief Get the type ID of an instance
     *
     * @return TypeId for this header
     */
    TypeId GetInstanceTypeId() const override;

    /**
     * @brief Get the serialized size of the header
     *
     * @return Size in bytes (24)
     */
    uint32_t GetSerializedSize() const override;

    /**
     * @brief Serialize the header into a buffer
     *
     * @param start Start of buffer to write to
     */
    void Serialize(Buffer::Iterator start) const override;

    /**
     * @brief Deserialize the header from a buffer
     *
     * @param start Start of buffer to read from
     * @return Number of bytes read
     */
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * @brief Print header contents to a stream
     *
     * @param os Output stream
     */
    void Print(std::ostream& os) const override;

    // ========== Getter Methods ==========

    /**
     * @brief Get the protocol version
     *
     * @return Protocol version (typically 0x01)
     */
    uint8_t GetVersion() const { return m_version; }

    /**
     * @brief Get the message type
     *
     * @return MessageType value (0-15)
     */
    uint8_t GetMessageType() const { return m_messageType; }

    /**
     * @brief Get the sender node type
     *
     * @return NodeType value (0-3)
     */
    uint8_t GetNodeType() const { return m_nodeType; }

    /**
     * @brief Get the sequence number
     *
     * @return Sequence number (0-65535)
     */
    uint16_t GetSequenceNumber() const { return m_sequenceNumber; }

    /**
     * @brief Get the source node ID
     *
     * @return Source node ID (0-65535)
     */
    uint16_t GetSourceNodeId() const { return m_sourceNodeId; }

    /**
     * @brief Get the destination node ID
     *
     * @return Destination node ID (0xFFFF = broadcast)
     */
    uint16_t GetDestinationNodeId() const { return m_destinationNodeId; }

    /**
     * @brief Get the payload length
     *
     * @return Payload length in bytes
     */
    uint16_t GetPayloadLength() const { return m_payloadLength; }

    /**
     * @brief Get the hop limit
     *
     * @return Maximum remaining hops
     */
    uint16_t GetHopLimit() const { return m_hopLimit; }

    /**
     * @brief Get the timestamp
     *
     * @return 32-bit timestamp
     */
    uint32_t GetTimestamp() const { return (m_timestampHigh << 16) | m_timestampLow; }

    /**
     * @brief Get the confidence level
     *
     * @return Confidence (0-1000)
     */
    uint16_t GetConfidence() const { return m_confidence; }

    // ========== Setter Methods ==========

    /**
     * @brief Set the protocol version
     *
     * @param version Protocol version (typically 0x01)
     */
    void SetVersion(uint8_t version) { m_version = version; }

    /**
     * @brief Set the message type
     *
     * @param messageType MessageType value (0-15)
     */
    void SetMessageType(uint8_t messageType) { m_messageType = messageType & 0x0F; }

    /**
     * @brief Set the sender node type
     *
     * @param nodeType NodeType value (0-3)
     */
    void SetNodeType(uint8_t nodeType) { m_nodeType = nodeType & 0x03; }

    /**
     * @brief Set the sequence number
     *
     * @param seqNum Sequence number (0-65535)
     */
    void SetSequenceNumber(uint16_t seqNum) { m_sequenceNumber = seqNum; }

    /**
     * @brief Set the source node ID
     *
     * @param sourceId Source node ID
     */
    void SetSourceNodeId(uint16_t sourceId) { m_sourceNodeId = sourceId; }

    /**
     * @brief Set the destination node ID
     *
     * @param destId Destination node ID (0xFFFF for broadcast)
     */
    void SetDestinationNodeId(uint16_t destId) { m_destinationNodeId = destId; }

    /**
     * @brief Set the payload length
     *
     * @param length Payload length in bytes
     */
    void SetPayloadLength(uint16_t length) { m_payloadLength = length; }

    /**
     * @brief Set the hop limit
     *
     * @param hopLimit Maximum remaining hops
     */
    void SetHopLimit(uint16_t hopLimit) { m_hopLimit = hopLimit; }

    /**
     * @brief Set the timestamp
     *
     * @param timestamp 32-bit timestamp value
     */
    void SetTimestamp(uint32_t timestamp)
    {
        m_timestampHigh = (timestamp >> 16) & 0xFFFF;
        m_timestampLow = timestamp & 0xFFFF;
    }

    /**
     * @brief Set the confidence level
     *
     * @param confidence Confidence value (0-1000)
     */
    void SetConfidence(uint16_t confidence) { m_confidence = confidence; }

    /**
     * @brief Check if this is a broadcast message
     *
     * @return True if destination is 0xFFFF (broadcast)
     */
    bool IsBroadcast() const { return m_destinationNodeId == 0xFFFF; }

    /**
     * @brief Check if sender is a ground node
     *
     * @return True if sender node type is GROUND
     */
    bool IsFromGroundNode() const { return m_nodeType == NODE_TYPE_GROUND; }

    /**
     * @brief Check if sender is a UAV
     *
     * @return True if sender node type is UAV
     */
    bool IsFromUAV() const { return m_nodeType == NODE_TYPE_UAV; }

  private:
    uint8_t m_version;              ///< Protocol version (1 byte)
    uint8_t m_messageType;          ///< Message type (1 byte, 4-bit field)
    uint8_t m_nodeType;             ///< Sender node type (1 byte, 2-bit field)
    uint8_t m_reserved1;            ///< Reserved (1 byte)
    uint16_t m_sequenceNumber;      ///< Sequence number (2 bytes)
    uint16_t m_sourceNodeId;        ///< Source node ID (2 bytes)
    uint16_t m_destinationNodeId;   ///< Destination node ID (2 bytes)
    uint16_t m_payloadLength;       ///< Payload length (2 bytes)
    uint16_t m_checksumVersion;     ///< Checksum and version (2 bytes)
    uint16_t m_hopLimit;            ///< Hop limit (2 bytes)
    uint16_t m_timestampHigh;       ///< Timestamp high bits (2 bytes)
    uint16_t m_timestampLow;        ///< Timestamp low bits (2 bytes)
    uint16_t m_confidence;          ///< Confidence level (2 bytes)
    uint16_t m_reserved2;           ///< Reserved (2 bytes)
};

/**
 * @brief Fragment-specific packet header extension
 *
 * Extended header used for fragment transmission messages (MSG_TYPE_FRAGMENT).
 * Contains information specific to data fragments.
 *
 * **Header Structure (12 bytes):**
 * ```
 * Offset  Size  Field                  Description
 * ------  ----  -----                  -----------
 *  0      2     FragmentId             Unique fragment identifier
 *  2      2     FragmentSequence       Fragment sequence number in set
 *  4      2     TotalFragments         Total fragments in this message
 *  6      1     SensorType             Type of sensor that produced data
 *  7      1     Flags                  Flags (last fragment, compressed, etc.)
 *  8      2     DataLength             Length of actual sensor data
 * 10      2     CRC16                  CRC16 checksum of data
 * ```
 */
class Scenario3FragmentHeader : public Header
{
  public:
    Scenario3FragmentHeader();
    ~Scenario3FragmentHeader() override;

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    // Getters
    uint16_t GetFragmentId() const { return m_fragmentId; }
    uint16_t GetFragmentSequence() const { return m_fragmentSequence; }
    uint16_t GetTotalFragments() const { return m_totalFragments; }
    uint8_t GetSensorType() const { return m_sensorType; }
    uint8_t GetFlags() const { return m_flags; }
    uint16_t GetDataLength() const { return m_dataLength; }
    uint16_t GetCRC16() const { return m_crc16; }

    // Setters
    void SetFragmentId(uint16_t id) { m_fragmentId = id; }
    void SetFragmentSequence(uint16_t seq) { m_fragmentSequence = seq; }
    void SetTotalFragments(uint16_t total) { m_totalFragments = total; }
    void SetSensorType(uint8_t type) { m_sensorType = type; }
    void SetFlags(uint8_t flags) { m_flags = flags; }
    void SetDataLength(uint16_t length) { m_dataLength = length; }
    void SetCRC16(uint16_t crc) { m_crc16 = crc; }

    // Helper flags
    bool IsLastFragment() const { return (m_flags & 0x01) != 0; }
    bool IsCompressed() const { return (m_flags & 0x02) != 0; }
    void SetLastFragment(bool last) { m_flags = last ? (m_flags | 0x01) : (m_flags & ~0x01); }
    void SetCompressed(bool compressed)
    {
        m_flags = compressed ? (m_flags | 0x02) : (m_flags & ~0x02);
    }

  private:
    uint16_t m_fragmentId;       ///< Fragment identifier
    uint16_t m_fragmentSequence; ///< Fragment sequence in set
    uint16_t m_totalFragments;   ///< Total fragments in message
    uint8_t m_sensorType;        ///< Type of sensor
    uint8_t m_flags;             ///< Flags (last, compressed, etc.)
    uint16_t m_dataLength;       ///< Length of actual data
    uint16_t m_crc16;            ///< CRC16 checksum
};

/**
 * @brief Acknowledgement packet header
 *
 * Used for acknowledgement messages (MSG_TYPE_ACKNOWLEDGEMENT).
 * Confirms receipt and processing of previous messages.
 */
class Scenario3AcknowledgementHeader : public Header
{
  public:
    Scenario3AcknowledgementHeader();
    ~Scenario3AcknowledgementHeader() override;

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    uint16_t GetAcknowledgedSequence() const { return m_acknowledgedSequence; }
    uint8_t GetAcknowledgementType() const { return m_acknowledgementType; }
    uint8_t GetStatus() const { return m_status; }

    void SetAcknowledgedSequence(uint16_t seq) { m_acknowledgedSequence = seq; }
    void SetAcknowledgementType(uint8_t type) { m_acknowledgementType = type; }
    void SetStatus(uint8_t status) { m_status = status; }

  private:
    uint16_t m_acknowledgedSequence; ///< Sequence number being acknowledged
    uint8_t m_acknowledgementType;   ///< Type of acknowledgement
    uint8_t m_status;                ///< Status code
};

} // namespace scenario3
} // namespace wsn
} // namespace ns3

#endif /* SCENARIO3_PACKET_HEADER_H */
