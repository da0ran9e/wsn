#ifndef FRAGMENT_H
#define FRAGMENT_H

#include "ns3/vector.h"
#include <cstdint>

namespace ns3 {

/**
 * \brief Fragment data structure for UAV-IoT person detection
 * 
 * Total size: 56 bytes (on typical 64-bit platforms)
 * - fragmentId (4B)
 * - sensorType (4B)
 * - baseConfidence (8B)
 * - broadcastPosition (24B: 3 doubles)
 * - timestamp (8B)
 * - txPowerDbm (4B)
 */
struct Fragment {
    uint32_t fragmentId;           // Unique fragment identifier
    uint32_t sensorType;           // 0=thermal, 1=visual, 2=acoustic, 3=motion
    double baseConfidence;         // Initial confidence from sensor (0.0-1.0)
    Vector broadcastPosition;      // UAV position when broadcast
    int64_t timestamp;             // Nanoseconds since simulation start
    float txPowerDbm;              // Transmit power in dBm

    Fragment()
        : fragmentId(0),
          sensorType(0),
          baseConfidence(0.5),
          broadcastPosition(0, 0, 0),
          timestamp(0),
          txPowerDbm(0.0f) {}

    Fragment(uint32_t id, uint32_t sensor, double conf,
         const Vector& pos, int64_t ts, float txPow)
        : fragmentId(id),
          sensorType(sensor),
          baseConfidence(conf),
          broadcastPosition(pos),
          timestamp(ts),
          txPowerDbm(txPow) {}
};

}  // namespace ns3

#endif /* FRAGMENT_H */
