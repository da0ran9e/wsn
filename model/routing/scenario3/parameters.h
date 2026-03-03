/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Scenario3 Parameters: Central configuration for multi-UAV scenario simulation
 */

#ifndef SCENARIO3_PARAMETERS_H
#define SCENARIO3_PARAMETERS_H

namespace ns3
{
namespace wsn
{
namespace scenario3
{

/**
 * @brief Ground Network Parameters
 */
struct GroundNetworkParams
{
    // Grid layout
    static constexpr uint32_t DEFAULT_GRID_SIZE = 10;
    static constexpr double DEFAULT_GRID_SPACING = 20.0;              // meters

    // Radio parameters
    static constexpr double DEFAULT_TX_POWER_DBM = 0.0;              // dBm
    static constexpr double DEFAULT_RX_SENSITIVITY_DBM = -95.0;      // dBm

    // Packet parameters
    static constexpr uint32_t DEFAULT_PACKET_SIZE = 64;              // bytes

    // Timing parameters
    static constexpr double DEFAULT_SIM_TIME = 60.0;                 // seconds (longer for multi-UAV)
    static constexpr double DEFAULT_FIRST_TX_TIME = 1.0;             // seconds
};

/**
 * @brief UAV Parameters
 */
struct UAVParams
{
    // Altitude
    static constexpr double DEFAULT_ALTITUDE = 20.0;                 // meters (higher for better coverage)
    static constexpr double MIN_ALTITUDE = 1.0;                      // meters
    static constexpr double MAX_ALTITUDE = 200.0;                    // meters

    // Speed
    static constexpr double DEFAULT_SPEED = 10.0;                    // m/s
    static constexpr double MIN_SPEED = 1.0;                         // m/s
    static constexpr double MAX_SPEED = 50.0;                        // m/s

    // Communication parameters
    static constexpr double DEFAULT_BROADCAST_INTERVAL = 1.0;        // seconds
    static constexpr double MIN_BROADCAST_INTERVAL = 0.1;            // seconds

    // Trajectory parameters
    static constexpr double APPROACH_DISTANCE = 50.0;                // meters before/after grid
};

/**
 * @brief Propagation and Channel Parameters
 */
struct ChannelParams
{
    // Propagation model parameters (log-distance)
    static constexpr double PATH_LOSS_EXPONENT = 3.0;                // for indoor/urban
    static constexpr double REFERENCE_DISTANCE = 1.0;                // meters
    static constexpr double REFERENCE_LOSS = 46.6776;                // dBm @ 2.4GHz, 1m (FSPL)
};

/**
 * @brief Traffic Pattern Parameters
 */
struct TrafficParams
{
    // Center node broadcast
    static constexpr double CENTER_BROADCAST_TIME_OFFSET = 0.0;      // seconds

    // Corner node broadcasts (relative to first TX time)
    static constexpr double TOP_LEFT_BROADCAST_OFFSET = 1.0;         // seconds
    static constexpr double TOP_RIGHT_BROADCAST_OFFSET = 1.2;        // seconds
    static constexpr double BOTTOM_LEFT_BROADCAST_OFFSET = 1.4;      // seconds
    static constexpr double BOTTOM_RIGHT_BROADCAST_OFFSET = 1.6;     // seconds
};

/**
 * @brief Helper functions for parameter calculations
 */
class ParameterCalculators
{
  public:
    /**
     * @brief Calculate grid dimensions
     * @param gridSize Grid size (N x N)
     * @param spacing Spacing between nodes
     * @return Width and height of the grid
     */
    static double CalculateGridDimension(uint32_t gridSize, double spacing)
    {
        return (gridSize - 1) * spacing;
    }

    /**
     * @brief Calculate UAV flight time
     * @param distance Distance to fly
     * @param speed Flight speed
     * @return Time in seconds
     */
    static double CalculateFlightTime(double distance, double speed)
    {
        return distance / speed;
    }

    /**
     * @brief Calculate number of broadcasts during flight
     * @param flightTime Duration of flight
     * @param broadcastInterval Interval between broadcasts
     * @return Number of broadcasts
     */
    static uint32_t CalculateNumberOfBroadcasts(double flightTime, double broadcastInterval)
    {
        return static_cast<uint32_t>(flightTime / broadcastInterval) + 1;
    }

    /**
     * @brief Validate UAV altitude
     * @param altitude Altitude in meters
     * @return True if valid, false otherwise
     */
    static bool ValidateAltitude(double altitude)
    {
        return altitude >= UAVParams::MIN_ALTITUDE && altitude <= UAVParams::MAX_ALTITUDE;
    }

    /**
     * @brief Validate UAV speed
     * @param speed Speed in m/s
     * @return True if valid, false otherwise
     */
    static bool ValidateSpeed(double speed)
    {
        return speed >= UAVParams::MIN_SPEED && speed <= UAVParams::MAX_SPEED;
    }

    /**
     * @brief Validate broadcast interval
     * @param interval Broadcast interval in seconds
     * @return True if valid, false otherwise
     */
    static bool ValidateBroadcastInterval(double interval)
    {
        return interval >= UAVParams::MIN_BROADCAST_INTERVAL;
    }
};

} // namespace scenario3
} // namespace wsn
} // namespace ns3

#endif // SCENARIO3_PARAMETERS_H
