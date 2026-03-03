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

#ifndef GLOBAL_STARTUP_PHASE_PACKET_H
#define GLOBAL_STARTUP_PHASE_PACKET_H

#include "ns3/header.h"

#include <stdint.h>
#include <vector>

namespace ns3
{
namespace wsn
{
namespace scenario3
{

/**
 * @defgroup GlobalStartupPhasePacket Global Startup Phase Packet Headers
 * @ingroup Wsn
 *
 * This module defines packet header structures specific to the global
 * startup phase, used for node initialization and activation coordination.
 */

/**
 * @brief Startup phase state enumeration
 *
 * Defines the different states a node can be in during the global startup phase.
 */
enum StartupPhaseState : uint8_t
{
    STARTUP_STATE_IDLE = 0,              ///< Idle, waiting for startup
    STARTUP_STATE_ACTIVATED = 1,         ///< Node activated
    STARTUP_STATE_DISCOVERING = 2,       ///< Discovering neighbors
    STARTUP_STATE_SYNCED = 3,            ///< Time synchronized
    STARTUP_STATE_READY = 4,             ///< Ready for operation
    STARTUP_STATE_COMPLETED = 5,         ///< Startup phase completed
    STARTUP_STATE_ERROR = 6,             ///< Error during startup
    STARTUP_STATE_SUSPENDED = 7          ///< Suspended/paused
};

/**
 * @brief Node activation status enumeration
 *
 * Indicates the activation status of a node during startup.
 */
enum ActivationStatus : uint8_t
{
    ACTIVATION_PENDING = 0,              ///< Waiting for activation
    ACTIVATION_IN_PROGRESS = 1,          ///< Currently activating
    ACTIVATION_SUCCESS = 2,              ///< Successfully activated
    ACTIVATION_FAILED = 3,               ///< Activation failed
    ACTIVATION_TIMEOUT = 4               ///< Activation timeout
};

/**
 * @brief Global startup phase packet header
 *
 * Extended header for messages sent during the global startup phase.
 * Used in conjunction with Scenario3PacketHeader for node initialization,
 * activation notification, and synchronization.
 *
 * **Header Structure (20 bytes):**
 * ```
 * Offset  Size  Field                     Description
 * ------  ----  -----                     -----------
 *  0      2     StartupPhaseId            Unique startup phase identifier
 *  2      2     NodeGridRow               Node row position in grid
 *  4      2     NodeGridCol               Node column position in grid
 *  6      1     StartupPhaseState         Current node state
 *  7      1     ActivationStatus          Node activation status
 *  8      2     TotalNodesInPhase         Total nodes in this startup phase
 *  10     2     ActivatedNodesCount       Number of activated nodes so far
 *  12     2     ActivationTimestamp      Time since phase start (milliseconds)
 *  14     2     NodeCapability           Node capability flags
 *  16     2     BatteryLevel             Battery level (0-1000)
 *  18     2     Reserved                 Reserved for future extensions
 * ```
 *
 * **Usage Example:**
 * ```cpp
 * GlobalStartupPhasePacketHeader startupHeader;
 * startupHeader.SetStartupPhaseId(1);
 * startupHeader.SetNodeGridRow(5);
 * startupHeader.SetNodeGridCol(3);
 * startupHeader.SetStartupPhaseState(STARTUP_STATE_ACTIVATED);
 * startupHeader.SetActivationStatus(ACTIVATION_SUCCESS);
 * startupHeader.SetTotalNodesInPhase(100);
 * startupHeader.SetActivatedNodesCount(45);
 * startupHeader.SetActivationTimestamp(250);
 * ```
 */
class GlobalStartupPhasePacketHeader : public Header
{
  public:
    GlobalStartupPhasePacketHeader();
    ~GlobalStartupPhasePacketHeader() override;

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
     * @return Size in bytes (20)
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
     * @brief Get the startup phase identifier
     *
     * @return Startup phase ID (0-65535)
     */
    uint16_t GetStartupPhaseId() const { return m_startupPhaseId; }

    /**
     * @brief Get the node's grid row position
     *
     * @return Row index in ground node grid
     */
    uint16_t GetNodeGridRow() const { return m_nodeGridRow; }

    /**
     * @brief Get the node's grid column position
     *
     * @return Column index in ground node grid
     */
    uint16_t GetNodeGridCol() const { return m_nodeGridCol; }

    /**
     * @brief Get the current startup phase state
     *
     * @return StartupPhaseState value
     */
    uint8_t GetStartupPhaseState() const { return m_startupPhaseState; }

    /**
     * @brief Get the node's activation status
     *
     * @return ActivationStatus value
     */
    uint8_t GetActivationStatus() const { return m_activationStatus; }

    /**
     * @brief Get total number of nodes in this startup phase
     *
     * @return Total nodes count
     */
    uint16_t GetTotalNodesInPhase() const { return m_totalNodesInPhase; }

    /**
     * @brief Get the number of nodes that have been activated
     *
     * @return Activated nodes count
     */
    uint16_t GetActivatedNodesCount() const { return m_activatedNodesCount; }

    /**
     * @brief Get the time elapsed since startup phase start
     *
     * @return Time in milliseconds
     */
    uint16_t GetActivationTimestamp() const { return m_activationTimestamp; }

    /**
     * @brief Get node capability flags
     *
     * Bit flags indicating node capabilities:
     * - Bit 0: Supports time sync
     * - Bit 1: Supports aggregation
     * - Bit 2: Supports routing
     * - Bit 3: Supports compression
     * - Bits 4-15: Reserved
     *
     * @return Capability flags (16-bit)
     */
    uint16_t GetNodeCapability() const { return m_nodeCapability; }

    /**
     * @brief Get the node's battery level
     *
     * @return Battery level (0-1000, where 1000 = 100%)
     */
    uint16_t GetBatteryLevel() const { return m_batteryLevel; }

    // ========== Setter Methods ==========

    /**
     * @brief Set the startup phase identifier
     *
     * @param phaseId Startup phase ID (0-65535)
     */
    void SetStartupPhaseId(uint16_t phaseId) { m_startupPhaseId = phaseId; }

    /**
     * @brief Set the node's grid row position
     *
     * @param row Row index
     */
    void SetNodeGridRow(uint16_t row) { m_nodeGridRow = row; }

    /**
     * @brief Set the node's grid column position
     *
     * @param col Column index
     */
    void SetNodeGridCol(uint16_t col) { m_nodeGridCol = col; }

    /**
     * @brief Set the startup phase state
     *
     * @param state StartupPhaseState value
     */
    void SetStartupPhaseState(uint8_t state) { m_startupPhaseState = state & 0x07; }

    /**
     * @brief Set the activation status
     *
     * @param status ActivationStatus value
     */
    void SetActivationStatus(uint8_t status) { m_activationStatus = status & 0x07; }

    /**
     * @brief Set total number of nodes in this startup phase
     *
     * @param total Total nodes count
     */
    void SetTotalNodesInPhase(uint16_t total) { m_totalNodesInPhase = total; }

    /**
     * @brief Set the number of activated nodes
     *
     * @param count Activated nodes count
     */
    void SetActivatedNodesCount(uint16_t count) { m_activatedNodesCount = count; }

    /**
     * @brief Set the time elapsed since startup phase start
     *
     * @param timestamp Time in milliseconds
     */
    void SetActivationTimestamp(uint16_t timestamp) { m_activationTimestamp = timestamp; }

    /**
     * @brief Set node capability flags
     *
     * @param capability Capability flags (16-bit)
     */
    void SetNodeCapability(uint16_t capability) { m_nodeCapability = capability; }

    /**
     * @brief Set the node's battery level
     *
     * @param level Battery level (0-1000)
     */
    void SetBatteryLevel(uint16_t level) { m_batteryLevel = (level > 1000) ? 1000 : level; }

    // ========== Helper Methods ==========

    /**
     * @brief Check if node is in activated state
     *
     * @return True if state is ACTIVATED, SYNCED, READY, or COMPLETED
     */
    bool IsNodeActivated() const
    {
        return m_startupPhaseState >= STARTUP_STATE_ACTIVATED &&
               m_startupPhaseState != STARTUP_STATE_ERROR;
    }

    /**
     * @brief Check if node activation was successful
     *
     * @return True if activation status is SUCCESS
     */
    bool IsActivationSuccessful() const { return m_activationStatus == ACTIVATION_SUCCESS; }

    /**
     * @brief Check if node is ready for operation
     *
     * @return True if state is READY or COMPLETED
     */
    bool IsNodeReady() const
    {
        return m_startupPhaseState == STARTUP_STATE_READY ||
               m_startupPhaseState == STARTUP_STATE_COMPLETED;
    }

    /**
     * @brief Check if startup phase is complete
     *
     * @return True if state is COMPLETED
     */
    bool IsStartupPhaseComplete() const { return m_startupPhaseState == STARTUP_STATE_COMPLETED; }

    /**
     * @brief Check if node experienced an error
     *
     * @return True if state is ERROR or status is FAILED/TIMEOUT
     */
    bool HasError() const
    {
        return m_startupPhaseState == STARTUP_STATE_ERROR ||
               m_activationStatus == ACTIVATION_FAILED ||
               m_activationStatus == ACTIVATION_TIMEOUT;
    }

    /**
     * @brief Get activation progress percentage
     *
     * @return Percentage of nodes activated (0-100)
     */
    uint8_t GetActivationProgress() const
    {
        if (m_totalNodesInPhase == 0)
            return 0;
        return (uint8_t)((m_activatedNodesCount * 100) / m_totalNodesInPhase);
    }

    /**
     * @brief Check if node supports a specific capability
     *
     * @param capabilityBit The capability bit to check (0-15)
     * @return True if capability is supported
     */
    bool SupportsCapability(uint8_t capabilityBit) const
    {
        if (capabilityBit > 15)
            return false;
        return (m_nodeCapability & (1 << capabilityBit)) != 0;
    }

    /**
     * @brief Set a specific capability flag
     *
     * @param capabilityBit The capability bit to set (0-15)
     * @param enabled True to enable, false to disable
     */
    void SetCapability(uint8_t capabilityBit, bool enabled)
    {
        if (capabilityBit <= 15)
        {
            if (enabled)
                m_nodeCapability |= (1 << capabilityBit);
            else
                m_nodeCapability &= ~(1 << capabilityBit);
        }
    }

    /**
     * @brief Get string representation of current state
     *
     * @return State name as string
     */
    std::string GetStateString() const;

    /**
     * @brief Get string representation of activation status
     *
     * @return Status name as string
     */
    std::string GetStatusString() const;

  private:
    uint16_t m_startupPhaseId;       ///< Startup phase identifier (2 bytes)
    uint16_t m_nodeGridRow;          ///< Node grid row (2 bytes)
    uint16_t m_nodeGridCol;          ///< Node grid column (2 bytes)
    uint8_t m_startupPhaseState;     ///< Startup phase state (1 byte)
    uint8_t m_activationStatus;      ///< Activation status (1 byte)
    uint16_t m_totalNodesInPhase;    ///< Total nodes in phase (2 bytes)
    uint16_t m_activatedNodesCount;  ///< Activated nodes count (2 bytes)
    uint16_t m_activationTimestamp;  ///< Time since phase start (2 bytes)
    uint16_t m_nodeCapability;       ///< Node capabilities (2 bytes)
    uint16_t m_batteryLevel;         ///< Battery level (2 bytes)
};

} // namespace scenario3
} // namespace wsn
} // namespace ns3

#endif /* GLOBAL_STARTUP_PHASE_PACKET_H */
