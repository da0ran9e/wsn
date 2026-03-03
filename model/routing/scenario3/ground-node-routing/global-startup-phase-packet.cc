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

#include "global-startup-phase-packet.h"

#include "ns3/buffer.h"
#include "ns3/log.h"
#include "ns3/type-id.h"

namespace ns3
{
namespace wsn
{
namespace scenario3
{

NS_LOG_COMPONENT_DEFINE("GlobalStartupPhasePacket");

// ============================================================================
// GlobalStartupPhasePacketHeader Implementation
// ============================================================================

GlobalStartupPhasePacketHeader::GlobalStartupPhasePacketHeader()
    : m_startupPhaseId(0),
      m_nodeGridRow(0),
      m_nodeGridCol(0),
      m_startupPhaseState(STARTUP_STATE_IDLE),
      m_activationStatus(ACTIVATION_PENDING),
      m_totalNodesInPhase(0),
      m_activatedNodesCount(0),
      m_activationTimestamp(0),
      m_nodeCapability(0),
      m_batteryLevel(1000)
{
}

GlobalStartupPhasePacketHeader::~GlobalStartupPhasePacketHeader()
{
}

TypeId
GlobalStartupPhasePacketHeader::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::wsn::scenario3::GlobalStartupPhasePacketHeader")
            .SetParent<Header>()
            .SetGroupName("Wsn")
            .AddConstructor<GlobalStartupPhasePacketHeader>();
    return tid;
}

TypeId
GlobalStartupPhasePacketHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
GlobalStartupPhasePacketHeader::GetSerializedSize() const
{
    return 20; // Total header size in bytes
}

void
GlobalStartupPhasePacketHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;

    // Serialize all fields in network byte order (big-endian)
    i.WriteHtonU16(m_startupPhaseId);
    i.WriteHtonU16(m_nodeGridRow);
    i.WriteHtonU16(m_nodeGridCol);
    i.WriteU8(m_startupPhaseState);
    i.WriteU8(m_activationStatus);
    i.WriteHtonU16(m_totalNodesInPhase);
    i.WriteHtonU16(m_activatedNodesCount);
    i.WriteHtonU16(m_activationTimestamp);
    i.WriteHtonU16(m_nodeCapability);
    i.WriteHtonU16(m_batteryLevel);
}

uint32_t
GlobalStartupPhasePacketHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    // Deserialize all fields from network byte order
    m_startupPhaseId = i.ReadNtohU16();
    m_nodeGridRow = i.ReadNtohU16();
    m_nodeGridCol = i.ReadNtohU16();
    m_startupPhaseState = i.ReadU8();
    m_activationStatus = i.ReadU8();
    m_totalNodesInPhase = i.ReadNtohU16();
    m_activatedNodesCount = i.ReadNtohU16();
    m_activationTimestamp = i.ReadNtohU16();
    m_nodeCapability = i.ReadNtohU16();
    m_batteryLevel = i.ReadNtohU16();

    return GetSerializedSize();
}

void
GlobalStartupPhasePacketHeader::Print(std::ostream& os) const
{
    os << "GlobalStartupPhasePacketHeader [" << std::endl
       << "  PhaseId=" << m_startupPhaseId << " GridPos=(" << m_nodeGridRow << ","
       << m_nodeGridCol << ")" << std::endl
       << "  State=" << GetStateString() << " Status=" << GetStatusString() << std::endl
       << "  ActivatedNodes=" << m_activatedNodesCount << "/" << m_totalNodesInPhase
       << " (Progress=" << (int)GetActivationProgress() << "%)" << std::endl
       << "  Timestamp=" << m_activationTimestamp << "ms BatteryLevel=" << m_batteryLevel
       << "/1000" << std::endl
       << "  Capability=0x" << std::hex << m_nodeCapability << std::dec << std::endl
       << "]" << std::endl;
}

std::string
GlobalStartupPhasePacketHeader::GetStateString() const
{
    switch (m_startupPhaseState)
    {
    case STARTUP_STATE_IDLE:
        return "IDLE";
    case STARTUP_STATE_ACTIVATED:
        return "ACTIVATED";
    case STARTUP_STATE_DISCOVERING:
        return "DISCOVERING";
    case STARTUP_STATE_SYNCED:
        return "SYNCED";
    case STARTUP_STATE_READY:
        return "READY";
    case STARTUP_STATE_COMPLETED:
        return "COMPLETED";
    case STARTUP_STATE_ERROR:
        return "ERROR";
    case STARTUP_STATE_SUSPENDED:
        return "SUSPENDED";
    default:
        return "UNKNOWN";
    }
}

std::string
GlobalStartupPhasePacketHeader::GetStatusString() const
{
    switch (m_activationStatus)
    {
    case ACTIVATION_PENDING:
        return "PENDING";
    case ACTIVATION_IN_PROGRESS:
        return "IN_PROGRESS";
    case ACTIVATION_SUCCESS:
        return "SUCCESS";
    case ACTIVATION_FAILED:
        return "FAILED";
    case ACTIVATION_TIMEOUT:
        return "TIMEOUT";
    default:
        return "UNKNOWN";
    }
}

} // namespace scenario3
} // namespace wsn
} // namespace ns3
