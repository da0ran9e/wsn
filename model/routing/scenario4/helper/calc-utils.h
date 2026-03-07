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

} // namespace helper
} // namespace scenario4
} // namespace wsn
} // namespace ns3

#endif // SCENARIO4_CALC_UTILS_H
