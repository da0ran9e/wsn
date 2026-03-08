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

} // namespace helper
} // namespace scenario4
} // namespace wsn
} // namespace ns3

#endif // SCENARIO4_CALC_UTILS_H
