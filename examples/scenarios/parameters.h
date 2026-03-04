/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Scenario-level parameters for example scenario implementations.
 */

#ifndef WSN_EXAMPLES_SCENARIOS_PARAMETERS_H
#define WSN_EXAMPLES_SCENARIOS_PARAMETERS_H

#include <cstdint>
#include <cmath>
#include <random>

namespace ns3
{
namespace wsn
{
namespace scenario3
{

/**
 * @brief UAV waypoint trajectory tuning parameters for Scenario3 example.
 */
struct WaypointTrajectoryParams
{
	static constexpr uint32_t HORIZONTAL_UAV_INDEX = 0;              ///< UAV#0: West -> East
	static constexpr uint32_t VERTICAL_UAV_INDEX = 1;                ///< UAV#1: North -> South
	static constexpr double DIAGONAL_ANGLE_STEP_RAD = 0.7853981633974483; // pi/4
	static constexpr double START_TIME_SECONDS = 0.0;                ///< First waypoint time
};

/**
 * @brief Start/end points for one UAV waypoint leg.
 */
struct WaypointEndpoints
{
	double startX;
	double startY;
	double endX;
	double endY;
};

/**
 * @brief Compute Scenario3 UAV trajectory endpoints based on UAV index.
 */
class WaypointTrajectoryCalculator
{
  public:
	static WaypointEndpoints Calculate(uint32_t uavIndex,
									   double gridWidth,
									   double gridHeight,
									   double approachDistance)
	{
		const double centerX = gridWidth / 2.0;
		const double centerY = gridHeight / 2.0;

		WaypointEndpoints points{0.0, 0.0, 0.0, 0.0};

		if (uavIndex == WaypointTrajectoryParams::HORIZONTAL_UAV_INDEX)
		{
			// Horizontal: West -> East through grid center
			points.startX = -approachDistance;
			points.startY = centerY;
			points.endX = gridWidth + approachDistance;
			points.endY = centerY;
			return points;
		}

		if (uavIndex == WaypointTrajectoryParams::VERTICAL_UAV_INDEX)
		{
			// Vertical: North -> South through grid center
			points.startX = centerX;
			points.startY = gridHeight + approachDistance;
			points.endX = centerX;
			points.endY = -approachDistance;
			return points;
		}

		// Diagonal pattern for additional UAVs
		const double angleOffset =
			static_cast<double>(uavIndex - 2) * WaypointTrajectoryParams::DIAGONAL_ANGLE_STEP_RAD;
		const double radius = std::sqrt(gridWidth * gridWidth + gridHeight * gridHeight) / 2.0 +
							  approachDistance;

		points.startX = centerX + radius * std::cos(angleOffset);
		points.startY = centerY + radius * std::sin(angleOffset);
		points.endX = centerX - radius * std::cos(angleOffset);
		points.endY = centerY - radius * std::sin(angleOffset);
		return points;
	}
};

/**
 * @brief 2D point representing a location in the grid.
 */
struct Point2D
{
	double x;
	double y;

	Point2D() : x(0.0), y(0.0) {}
	Point2D(double px, double py) : x(px), y(py) {}
};

/**
 * @brief Signal source location with grid cell information.
 */
struct SignalSourceLocationData
{
	Point2D location;		///< (x, y) coordinates in meters
	int32_t cellId;			///< Hex cell ID containing this location
	int32_t cellQ;			///< Axial Q coordinate of the cell
	int32_t cellR;			///< Axial R coordinate of the cell

	SignalSourceLocationData() : location(0.0, 0.0), cellId(0), cellQ(0), cellR(0) {}
};

/**
 * @brief Centralized storage for signal source location and containing cell after global setup phase completes.
 */
class SignalSourceLocation
{
  private:
	static SignalSourceLocationData g_selectedData;
	static bool g_locationSelected;

  public:
	/**
	 * @brief Select and store a random signal source location within the grid and determine its cell.
	 * @param gridWidth Width of the grid in meters
	 * @param gridHeight Height of the grid in meters
	 * @param hexCellRadius Hex cell radius used in topology (for coordinate conversion)
	 * @param hexGridOffset Grid offset for cell ID calculation
	 * @return The randomly selected SignalSourceLocationData
	 */
	static SignalSourceLocationData SelectRandomLocation(double gridWidth,
														  double gridHeight,
														  double hexCellRadius,
														  int32_t hexGridOffset)
	{
		// Use uniform distribution to select random point within grid
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<double> distX(0.0, gridWidth);
		std::uniform_real_distribution<double> distY(0.0, gridHeight);

		g_selectedData.location.x = distX(gen);
		g_selectedData.location.y = distY(gen);

		// Convert (x, y) to hex cell coordinates
		const double fracQ = (std::sqrt(3.0) / 3.0 * g_selectedData.location.x -
							  1.0 / 3.0 * g_selectedData.location.y) /
							 hexCellRadius;
		const double fracR = (2.0 / 3.0 * g_selectedData.location.y) / hexCellRadius;

		// Round to nearest hex cell
		HexRound(fracQ, fracR, g_selectedData.cellQ, g_selectedData.cellR);

		// Compute cell ID
		g_selectedData.cellId = g_selectedData.cellQ +
								g_selectedData.cellR * hexGridOffset;

		g_locationSelected = true;

		return g_selectedData;
	}

	/**
	 * @brief Get the currently stored signal source location data.
	 * @return The stored SignalSourceLocationData
	 */
	static SignalSourceLocationData GetLocationData()
	{
		return g_selectedData;
	}

	/**
	 * @brief Get the currently stored signal source location point.
	 * @return The (x, y) coordinates
	 */
	static Point2D GetLocation()
	{
		return g_selectedData.location;
	}

	/**
	 * @brief Get the cell ID containing the signal source location.
	 * @return The hex cell ID, or 0 if not selected
	 */
	static int32_t GetCellId()
	{
		return g_selectedData.cellId;
	}

	/**
	 * @brief Check if a location has been selected.
	 * @return True if location has been selected, false otherwise
	 */
	static bool IsLocationSelected()
	{
		return g_locationSelected;
	}

	/**
	 * @brief Reset the location selection.
	 */
	static void Reset()
	{
		g_selectedData = SignalSourceLocationData();
		g_locationSelected = false;
	}

  private:
	/**
	 * @brief Hex rounding function to snap fractional coordinates to nearest hex cell.
	 */
	static void HexRound(double fracQ, double fracR, int32_t& q, int32_t& r)
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
};

// Static member initialization (implementation in scenario3.cc)
inline SignalSourceLocationData SignalSourceLocation::g_selectedData = SignalSourceLocationData();
inline bool SignalSourceLocation::g_locationSelected = false;

} // namespace scenario3
} // namespace wsn
} // namespace ns3

#endif // WSN_EXAMPLES_SCENARIOS_PARAMETERS_H
