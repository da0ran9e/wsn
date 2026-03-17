/*
 * Scenario 4 - Common Calculation Utilities
 * 
 * Pure utility functions for geometry, scoring, and threshold checks.
 * No simulation state or event scheduling.
 */

#ifndef SCENARIO4_CALC_UTILS_H
#define SCENARIO4_CALC_UTILS_H

#include <cstdint>
#include <cmath>

namespace ns3 {
namespace wsn {
namespace scenario4 {
namespace helper {

struct HexCellCoord
{
    int32_t q;
    int32_t r;
};

/**
 * Calculate Euclidean distance between two 2D points.
 */
inline double
CalculateDistance(double x1, double y1, double x2, double y2)
{
    double dx = x2 - x1;
    double dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

/**
 * Compute hexagonal cell ID from position.
 * 
 * \param x X coordinate
 * \param y Y coordinate
 * \param cellRadius Hexagonal cell radius
 * \return Cell ID
 */
int32_t ComputeCellId(double x, double y, double cellRadius);

/**
 * Compute axial (q,r) coordinate from world position on hex grid.
 * Logic is consistent with scenario3 global startup phase.
 */
HexCellCoord ComputeHexCellCoord(double x, double y, double cellRadius);

/**
 * Make cell id from axial coordinate.
 */
int32_t MakeCellId(int32_t q, int32_t r, int32_t gridOffset);

/**
 * Compute 3-color index for axial hex cell.
 */
uint32_t ComputeHexColor(int32_t q, int32_t r);

/**
 * Compute stable color index/value for a cell id.
 *
 * \/param cellId Cell ID
 * \/return Color code (RGB packed in uint32_t)
 */
uint32_t ComputeCellColor(int32_t cellId);

/**
 * Compute hexagonal cell center position from axial coordinates.
 * 
 * \param q Axial q coordinate
 * \param r Axial r coordinate
 * \param cellRadius Hexagonal cell radius
 * \param outCenterX Output: X coordinate of cell center
 * \param outCenterY Output: Y coordinate of cell center
 */
void ComputeHexCellCenter(int32_t q, int32_t r, double cellRadius,
                          double& outCenterX, double& outCenterY);

/**
 * Compute suspicious score for a node.
 * 
 * Lower confidence = higher suspicion.
 * 
 * \param confidence Current confidence level [0, 1]
 * \param packetCount Number of packets received
 * \param rssiDbm Average RSSI in dBm
 * \return Suspicious score (higher = more suspicious)
 */
double ComputeSuspiciousScore(double confidence, uint32_t packetCount, double rssiDbm);

/**
 * Check if cooperation should be triggered.
 * 
 * \param confidence Current confidence level
 * \param threshold Cooperation threshold
 * \return true if cooperation should trigger
 */
inline bool
IsCooperationTriggered(double confidence, double threshold)
{
    return confidence >= threshold;
}

/**
 * Check if alert state should be triggered.
 * 
 * \param confidence Current confidence level
 * \param threshold Alert threshold
 * \return true if alert should trigger
 */
inline bool
IsAlertTriggered(double confidence, double threshold)
{
    return confidence >= threshold;
}

// ===== RADIO TRANSMISSION TIME HELPERS =====

/**
 * Calculate radio transmission contact time for a packet.
 * 
 * Formula: transmission_time = (packet_size_bits / bitrate_bps)
 * 
 * For CC2420 (802.15.4 ZigBee radio):
 * - Physical bitrate: 250 kbps (250,000 bits per second)
 * - Frame overhead: MAC header (up to 25 bytes)
 * - Max frame: 127 bytes (11 bytes header + 116 bytes payload)
 * 
 * Example:
 * - Packet size: 100 bytes
 * - Total bits: 100 * 8 = 800 bits
 * - Contact time: 800 / 250,000 = 3.2 milliseconds
 * 
 * \param packetSizeBytes Packet size in bytes
 * \param bitrateBps Physical layer bitrate in bits per second (default: 250,000 for CC2420)
 * \return Contact/transmission time in seconds
 */
inline double
CalculateTransmissionTime(uint32_t packetSizeBytes, uint32_t bitrateBps = 250000)
{
    if (bitrateBps == 0)
    {
        return 0.0;
    }
    
    // Total bits = packet size in bytes * 8 bits per byte
    uint64_t totalBits = static_cast<uint64_t>(packetSizeBytes) * 8;
    
    // Contact time = total bits / bitrate
    return static_cast<double>(totalBits) / static_cast<double>(bitrateBps);
}

/**
 * Calculate total transmission time for sending entire master file.
 * 
 * Used for UAV1 hover time calculation.
 * Master file size = masterFileConfidence + fragment metadata
 * 
 * \param masterFileSizeBytes Total master file size in bytes
 * \param bitrateBps Physical layer bitrate (default: 250,000 for CC2420)
 * \return Total transmission time in seconds
 */
inline double
CalculateMasterFileTransmissionTime(uint32_t masterFileSizeBytes, uint32_t bitrateBps = 250000)
{
    return CalculateTransmissionTime(masterFileSizeBytes, bitrateBps);
}

/**
 * Calculate transmission time for a single fragment packet.
 * 
 * Fragment packet size = fragment data + header
 * 
 * \param fragmentPixelCount Number of pixels in fragment
 * \param bytesPerPixel Bytes per pixel (typically 3 for RGB)
 * \param bitrateBps Physical layer bitrate (default: 250,000 for CC2420)
 * \return Transmission time for one fragment in seconds
 */
inline double
CalculateFragmentTransmissionTime(uint32_t fragmentPixelCount, 
                                  uint32_t bytesPerPixel,
                                  uint32_t bitrateBps = 250000)
{
    uint32_t fragmentSizeBytes = fragmentPixelCount * bytesPerPixel;
    return CalculateTransmissionTime(fragmentSizeBytes, bitrateBps);
}

/**
 * Calculate CC2420-specific transmission time.
 * 
 * CC2420 is IEEE 802.15.4 compliant with 250 kbps bitrate.
 * Reference: cc2420-example.cc shows frame TX time: (127 * 8) / 250.0 = 4.064 ms
 * 
 * \param packetSizeBytes Packet size in bytes
 * \return Contact time in seconds
 */
inline double
CalculateCc2420TransmissionTime(uint32_t packetSizeBytes)
{
    // CC2420 bitrate: 250 kbps
    return CalculateTransmissionTime(packetSizeBytes, 250000);
}

} // namespace helper
} // namespace scenario4
} // namespace wsn
} // namespace ns3

#endif // SCENARIO4_CALC_UTILS_H
