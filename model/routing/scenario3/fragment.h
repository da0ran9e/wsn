/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Fragment data structure for Scenario3 multi-UAV person detection with network integration
 */

#ifndef SCENARIO3_FRAGMENT_H
#define SCENARIO3_FRAGMENT_H

#include "ns3/vector.h"
#include "ns3/header.h"
#include "ns3/buffer.h"

#include <cstdint>

namespace ns3
{
namespace wsn
{
namespace scenario3
{

/**
 * @brief Fragment data structure for multi-UAV person detection
 * 
 * Contains sensor data fragment with confidence value and UAV source ID
 * Total size: 32 bytes over the network
 */
struct FragmentData
{
    uint32_t fragmentId;           // Unique fragment identifier
    uint32_t sensorType;           // 0=thermal, 1=visual, 2=acoustic, 3=motion
    double baseConfidence;         // Initial confidence from sensor (0.0-1.0)
    uint32_t sequenceNumber;       // Broadcast sequence number
    float txPowerDbm;              // Transmit power in dBm
    uint32_t uavSourceId;          // Which UAV sent this (0, 1, 2, ...)

    FragmentData()
        : fragmentId(0),
          sensorType(0),
          baseConfidence(0.5),
          sequenceNumber(0),
          txPowerDbm(0.0f),
          uavSourceId(0)
    {
    }

    FragmentData(uint32_t id, uint32_t sensor, double conf, uint32_t seqNum, float txPow, uint32_t uavId)
        : fragmentId(id),
          sensorType(sensor),
          baseConfidence(conf),
          sequenceNumber(seqNum),
          txPowerDbm(txPow),
          uavSourceId(uavId)
    {
    }
};

/**
 * @brief Network header for fragment transmission
 */
class FragmentHeader : public Header
{
public:
    FragmentHeader();
    FragmentHeader(const FragmentData& data);
    virtual ~FragmentHeader();

    /**
     * @brief Set fragment data
     * @param data Fragment data structure
     */
    void SetFragmentData(const FragmentData& data);

    /**
     * @brief Get fragment data
     * @return Fragment data structure
     */
    FragmentData GetFragmentData() const;

    // Header implementation
    static TypeId GetTypeId();
    virtual TypeId GetInstanceTypeId() const override;
    virtual void Print(std::ostream& os) const override;
    virtual uint32_t GetSerializedSize() const override;
    virtual void Serialize(Buffer::Iterator start) const override;
    virtual uint32_t Deserialize(Buffer::Iterator start) override;

private:
    FragmentData m_data;
};

} // namespace scenario3
} // namespace wsn
} // namespace ns3

#endif // SCENARIO3_FRAGMENT_H
