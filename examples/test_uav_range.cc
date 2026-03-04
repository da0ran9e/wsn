/*
 * Test program for UAV-to-Ground communication range calculation functions
 */

#include "scenarios/scenario3.h"
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"

#include <iostream>
#include <iomanip>

using namespace ns3;
using namespace ns3::wsn;

int main(int argc, char* argv[])
{
    // Enable logging
    LogComponentEnable("Scenario3", LOG_LEVEL_INFO);
    
    std::cout << "\n=== UAV-to-Ground Communication Range Test ===\n" << std::endl;
    
    // Test parameters (matching CC2420 radio)
    double txPowerDbm = 0.0;           // 0 dBm transmit power
    double rxSensitivityDbm = -95.0;   // -95 dBm receiver sensitivity
    
    // Test Case 1: Calculate range for various UAV positions
    std::cout << "TEST 1: UAV Communication Range at Different Positions\n";
    std::cout << std::string(80, '-') << std::endl;
    std::cout << std::setw(15) << "UAV Position" 
              << std::setw(15) << "Ground Pos"
              << std::setw(12) << "Distance(m)"
              << std::setw(12) << "PathLoss(dB)"
              << std::setw(12) << "RxPower(dBm)"
              << std::setw(10) << "InRange?"
              << std::setw(12) << "Margin(dB)"
              << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    // Test different UAV altitudes and horizontal distances
    struct TestPoint {
        double uavX, uavY, uavZ;
        double groundX, groundY;
        std::string description;
    };
    
    std::vector<TestPoint> testPoints = {
        {50, 50, 50, 50, 50, "UAV directly above node"},
        {50, 50, 50, 100, 50, "50m horizontal, 50m altitude"},
        {50, 50, 100, 50, 50, "UAV at 100m altitude"},
        {50, 50, 100, 150, 50, "100m horizontal, 100m altitude"},
        {50, 50, 200, 50, 50, "UAV at 200m altitude"},
        {50, 50, 50, 200, 50, "150m horizontal distance"},
        {50, 50, 50, 500, 50, "450m horizontal (far)"},
    };
    
    for (const auto& test : testPoints)
    {
        UavGroundCommRange range = CalculateUavGroundCommRange(
            test.uavX, test.uavY, test.uavZ,
            test.groundX, test.groundY,
            txPowerDbm, rxSensitivityDbm);
        
        std::cout << std::setw(15) << ("(" + std::to_string((int)test.uavX) + "," + 
                                       std::to_string((int)test.uavY) + "," + 
                                       std::to_string((int)test.uavZ) + ")")
                  << std::setw(15) << ("(" + std::to_string((int)test.groundX) + "," + 
                                       std::to_string((int)test.groundY) + ",0)")
                  << std::setw(12) << std::fixed << std::setprecision(1) << range.distance3D
                  << std::setw(12) << std::setprecision(1) << range.pathLossDb
                  << std::setw(12) << std::setprecision(1) << range.receivedPowerDbm
                  << std::setw(10) << (range.isInRange ? "YES" : "NO")
                  << std::setw(12) << std::setprecision(1) << range.linkMarginDb
                  << "  // " << test.description
                  << std::endl;
    }
    
    // Test Case 2: Calculate maximum communication range for different altitudes
    std::cout << "\n\nTEST 2: Maximum Horizontal Communication Range vs Altitude\n";
    std::cout << std::string(80, '-') << std::endl;
    std::cout << std::setw(20) << "UAV Altitude (m)"
              << std::setw(30) << "Max Horizontal Range (m)"
              << std::setw(25) << "Coverage Area (km²)"
              << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    std::vector<double> testAltitudes = {10, 25, 50, 75, 100, 150, 200, 250, 300};
    
    for (double altitude : testAltitudes)
    {
        double maxRange = CalculateMaxUavCommRange(altitude, txPowerDbm, rxSensitivityDbm);
        double coverageArea = 3.14159 * maxRange * maxRange / 1000000.0; // km²
        
        std::cout << std::setw(20) << std::fixed << std::setprecision(0) << altitude
                  << std::setw(30) << std::setprecision(1) << maxRange
                  << std::setw(25) << std::setprecision(3) << coverageArea
                  << std::endl;
    }
    
    // Test Case 3: Find ground nodes in UAV range (requires global topology)
    std::cout << "\n\nTEST 3: Ground Nodes in UAV Range (Simulation Required)\n";
    std::cout << std::string(80, '-') << std::endl;
    std::cout << "Note: This test requires running a full scenario3 simulation with network setup.\n";
    std::cout << "After global setup phase completes, the GetGroundNodesInUavRange() function\n";
    std::cout << "can be called to find all nodes within communication range.\n";
    std::cout << std::endl;
    std::cout << "Example usage:\n";
    std::cout << "  double uavX = 100.0, uavY = 100.0, uavZ = 50.0;\n";
    std::cout << "  std::vector<uint32_t> nodesInRange = GetGroundNodesInUavRange(\n";
    std::cout << "      uavX, uavY, uavZ, txPowerDbm, rxSensitivityDbm);\n";
    std::cout << "  NS_LOG_INFO(\"UAV at (\" << uavX << \",\" << uavY << \",\" << uavZ << \"): \"\n";
    std::cout << "              << nodesInRange.size() << \" nodes in range\");\n";
    
    // Test Case 4: Path loss model validation
    std::cout << "\n\nTEST 4: Free Space Path Loss Model Validation (2.4 GHz)\n";
    std::cout << std::string(80, '-') << std::endl;
    std::cout << "FSPL formula: PathLoss(dB) = 40.2 + 20*log10(distance_m)\n";
    std::cout << "This is the theoretical path loss for line-of-sight 2.4 GHz transmission\n";
    std::cout << "(CC2420 radio frequency).\n";
    std::cout << std::endl;
    std::cout << std::setw(20) << "Distance (m)"
              << std::setw(20) << "Path Loss (dB)"
              << std::setw(25) << "Signal Strength @ 0dBm"
              << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    std::vector<double> testDistances = {1, 10, 50, 100, 200, 500, 1000, 2000};
    
    for (double dist : testDistances)
    {
        UavGroundCommRange range = CalculateUavGroundCommRange(
            0, 0, 0, dist, 0, txPowerDbm, rxSensitivityDbm);
        
        std::cout << std::setw(20) << std::fixed << std::setprecision(0) << dist
                  << std::setw(20) << std::setprecision(1) << range.pathLossDb
                  << std::setw(25) << std::setprecision(1) << range.receivedPowerDbm << " dBm"
                  << std::endl;
    }
    
    std::cout << "\n=== Test Complete ===\n" << std::endl;
    
    return 0;
}
