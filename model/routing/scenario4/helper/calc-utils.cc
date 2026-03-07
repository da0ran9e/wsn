/*
 * Scenario 4 - Calculation Utilities Implementation
 */

#include "calc-utils.h"
#include <cmath>

namespace ns3 {
namespace wsn {
namespace scenario4 {
namespace helper {

int32_t
ComputeCellId(double x, double y, double cellRadius)
{
    // Simplified hexagonal grid mapping
    // Using axial coordinates for hexagonal tiling
    
    const double sqrt3 = std::sqrt(3.0);
    const double cellWidth = cellRadius * sqrt3;
    const double cellHeight = cellRadius * 2.0;
    
    // Convert to axial coordinates
    int32_t q = static_cast<int32_t>(std::round(x / cellWidth));
    int32_t r = static_cast<int32_t>(std::round(y / cellHeight));
    
    // Combine into single cell ID
    // Using cantor pairing function for unique mapping
    int32_t cellId = ((q + r) * (q + r + 1)) / 2 + r;
    
    return cellId;
}

double
ComputeSuspiciousScore(double confidence, uint32_t packetCount, double rssiDbm)
{
    // Lower confidence = higher suspicion
    double confidenceScore = 1.0 - confidence;
    
    // Low packet count = more suspicious (less data coverage)
    double packetScore = (packetCount == 0) ? 1.0 : 1.0 / std::sqrt(packetCount);
    
    // Weak signal = more suspicious
    double rssiScore = (rssiDbm < -80.0) ? 1.0 : 0.5;
    
    // Weighted combination
    return 0.6 * confidenceScore + 0.3 * packetScore + 0.1 * rssiScore;
}

} // namespace helper
} // namespace scenario4
} // namespace wsn
} // namespace ns3
