/*
 * Scenario 4 - Calculation Utilities Implementation
 */

#include "calc-utils.h"
#include <cmath>

namespace ns3 {
namespace wsn {
namespace scenario4 {
namespace helper {

namespace {

void
HexRound(double fracQ, double fracR, int32_t& q, int32_t& r)
{
    const double fracS = -fracQ - fracR;

    int32_t rq = static_cast<int32_t>(std::round(fracQ));
    int32_t rr = static_cast<int32_t>(std::round(fracR));
    int32_t rs = static_cast<int32_t>(std::round(fracS));

    const double qDiff = std::fabs(rq - fracQ);
    const double rDiff = std::fabs(rr - fracR);
    const double sDiff = std::fabs(rs - fracS);

    if (qDiff > rDiff && qDiff > sDiff)
    {
        rq = -rr - rs;
    }
    else if (rDiff > sDiff)
    {
        rr = -rq - rs;
    }

    q = rq;
    r = rr;
}

} // namespace

HexCellCoord
ComputeHexCellCoord(double x, double y, double cellRadius)
{
    const double fracQ = (std::sqrt(3.0) / 3.0 * x - 1.0 / 3.0 * y) / cellRadius;
    const double fracR = (2.0 / 3.0 * y) / cellRadius;

    HexCellCoord coord{0, 0};
    HexRound(fracQ, fracR, coord.q, coord.r);
    return coord;
}

int32_t
MakeCellId(int32_t q, int32_t r, int32_t gridOffset)
{
    return q + r * gridOffset;
}

uint32_t
ComputeHexColor(int32_t q, int32_t r)
{
    return static_cast<uint32_t>(((q - r) % 3 + 3) % 3);
}

void
ComputeHexCellCenter(int32_t q, int32_t r, double cellRadius,
                     double& outCenterX, double& outCenterY)
{
    outCenterX = cellRadius * (std::sqrt(3.0) * q + std::sqrt(3.0) / 2.0 * r);
    outCenterY = cellRadius * (3.0 / 2.0 * r);
}

int32_t
ComputeCellId(double x, double y, double cellRadius)
{
    // Backward-compatible wrapper: use scenario3-compatible hex mapping
    // with default grid offset.
    static constexpr int32_t kDefaultGridOffset = 10000;
    const HexCellCoord coord = ComputeHexCellCoord(x, y, cellRadius);
    return MakeCellId(coord.q, coord.r, kDefaultGridOffset);
}

uint32_t
ComputeCellColor(int32_t cellId)
{
    // Legacy API kept for compatibility. Prefer ComputeHexColor(q, r).
    if (cellId < 0)
    {
        return 0;
    }
    return static_cast<uint32_t>(cellId % 3);
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
