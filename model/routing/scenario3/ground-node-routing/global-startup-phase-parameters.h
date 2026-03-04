#ifndef GLOBAL_STARTUP_PHASE_PARAMETERS_H
#define GLOBAL_STARTUP_PHASE_PARAMETERS_H

#include <cstdint>

namespace ns3
{

/**
 * \brief Global Setup Phase Configuration Parameters
 *
 * This file contains all tunable parameters for the hex-cell topology
 * precalculation and logical neighbor discovery.
 */
namespace GlobalSetupParams
{

/**
 * \brief Hex Cell Radius Multiplier
 *
 * Multiplier applied to minimum node distance to determine hex cell radius.
 * Larger values create bigger cells with more members.
 * Default: 1.60
 */
constexpr double HEX_CELL_RADIUS_MULTIPLIER = 1.60;

/**
 * \brief Logical Neighbor Radius Multiplier
 *
 * Multiplier applied to minimum node distance to determine logical neighbor radius.
 * Larger values create denser neighbor graphs, enabling more routing options.
 * Must be >= HEX_CELL_RADIUS_MULTIPLIER for proper intra-cell connectivity.
 * Default: 1.90
 */
constexpr double LOGICAL_NEIGHBOR_RADIUS_MULTIPLIER = 1.90;

/**
 * \brief Minimum Hex Cell Radius (meters)
 *
 * Absolute minimum value for hex cell radius regardless of node density.
 * Prevents numerical issues in sparse or single-node scenarios.
 * Default: 1.0
 */
constexpr double MIN_HEX_CELL_RADIUS = 1.0;

/**
 * \brief Minimum Logical Neighbor Radius (meters)
 *
 * Absolute minimum value for logical neighbor radius.
 * Prevents numerical issues in sparse or single-node scenarios.
 * Default: 1.0
 */
constexpr double MIN_LOGICAL_NEIGHBOR_RADIUS = 1.0;

/**
 * \brief Fallback Minimum Distance (meters)
 *
 * Used when no positive inter-node distances are found (e.g., single node).
 * Prevents division by zero or invalid radius calculations.
 * Default: 50.0
 */
constexpr double FALLBACK_MIN_DISTANCE = 50.0;

/**
 * \brief Grid Offset Base
 *
 * Minimum grid offset for hex coordinate to cell ID mapping.
 * Ensures unique cell IDs for typical grid sizes.
 * Default: 32
 */
constexpr uint32_t GRID_OFFSET_BASE = 32;

/**
 * \brief Grid Offset Multiplier
 *
 * Multiplier for gridSize when computing hex grid offset.
 * Formula: max(GRID_OFFSET_BASE, gridSize * GRID_OFFSET_MULTIPLIER + GRID_OFFSET_EXTRA)
 * Default: 8
 */
constexpr uint32_t GRID_OFFSET_MULTIPLIER = 8;

/**
 * \brief Grid Offset Extra
 *
 * Extra offset added to computed grid offset for safety margin.
 * Formula: max(GRID_OFFSET_BASE, gridSize * GRID_OFFSET_MULTIPLIER + GRID_OFFSET_EXTRA)
 * Default: 8
 */
constexpr uint32_t GRID_OFFSET_EXTRA = 8;

/**
 * \brief Hex Direction Vectors
 *
 * The 6 axial coordinate directions for hex neighbor cells.
 * Order: East, Southeast, Southwest, West, Northwest, Northeast
 */
constexpr int HEX_DIRECTIONS[6][2] = {
    {1, 0},   // East
    {1, -1},  // Southeast
    {0, -1},  // Southwest
    {-1, 0},  // West
    {-1, 1},  // Northwest
    {0, 1}    // Northeast
};

} // namespace GlobalSetupParams

} // namespace ns3

#endif // GLOBAL_STARTUP_PHASE_PARAMETERS_H
