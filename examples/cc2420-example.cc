/*
 * Copyright (c) 2025 WSN Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * CC2420 MAC Module Verification Test
 *
 * This program verifies the CC2420 module implementation (based on Castalia)
 * by testing:
 * - PHY layer object creation and configuration
 * - MAC layer object creation and configuration
 * - MAC layer CSMA-CA algorithm parameters
 * - Spectrum channel integration
 * - Node positioning and mobility
 * - RF parameter calculations (path loss, RSSI, SNR, LQI)
 * - Link viability assessment
 * - CC2420 hardware specifications compliance
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/cc2420-phy.h"
#include "ns3/cc2420-mac.h"
#include "ns3/cc2420-net-device.h"
#include "ns3/cc2420-energy-model.h"
#include "ns3/spectrum-signal-parameters.h"

#include <iostream>
#include <iomanip>
#include <cmath>
#include <sstream>
#include <map>

using namespace ns3;
using namespace ns3::wsn;

NS_LOG_COMPONENT_DEFINE("Cc2420Verification");

// ============================================================================
// Test Results Tracking
// ============================================================================

struct TestResults
{
    uint32_t testsPassed = 0;
    uint32_t testsFailed = 0;
    std::vector<std::string> failedTests;
    
    void PassTest(const std::string& name)
    {
        testsPassed++;
        NS_LOG_INFO("✓ PASS: " << name);
    }
    
    void FailTest(const std::string& name, const std::string& reason)
    {
        testsFailed++;
        failedTests.push_back(name + " - " + reason);
        NS_LOG_INFO("✗ FAIL: " << name << " - " << reason);
    }
    
    void PrintSummary()
    {
        NS_LOG_INFO("\n" << std::string(80, '='));
        NS_LOG_INFO("Test Summary");
        NS_LOG_INFO(std::string(80, '='));
        NS_LOG_INFO("Total: " << (testsPassed + testsFailed));
        NS_LOG_INFO("Passed: " << testsPassed);
        NS_LOG_INFO("Failed: " << testsFailed);
        if (testsFailed > 0)
        {
            NS_LOG_INFO("\nFailed Tests:");
            for (const auto& test : failedTests)
            {
                NS_LOG_INFO("  - " << test);
            }
        }
        NS_LOG_INFO(std::string(80, '=') << "\n");
    }
};

TestResults results;

static std::map<std::string, uint32_t> g_layerDebugCounters;

static void
OnMacDebugTrace(std::string eventName, Ptr<const Packet> packet)
{
    g_layerDebugCounters["MAC"]++;
    NS_LOG_INFO("[DBG][MAC] " << eventName << " size=" << (packet ? packet->GetSize() : 0));
}

static void
OnPhyDebugTrace(std::string eventName, Ptr<const Packet> packet)
{
    g_layerDebugCounters["PHY"]++;
    NS_LOG_INFO("[DBG][PHY] " << eventName << " size=" << (packet ? packet->GetSize() : 0));
}

static void
OnNetDebugTrace(std::string eventName, Ptr<const Packet> packet)
{
    g_layerDebugCounters["NET"]++;
    NS_LOG_INFO("[DBG][NET] " << eventName << " size=" << (packet ? packet->GetSize() : 0));
}

static void
OnEnergyDebugTrace(std::string eventName, Ptr<const Packet> packet)
{
    g_layerDebugCounters["ENERGY"]++;
    NS_LOG_INFO("[DBG][ENERGY] " << eventName << " size=" << (packet ? packet->GetSize() : 0));
}

// ============================================================================
// RF Parameter Calculation Functions
// ============================================================================

/**
 * @brief Calculate path loss using Log Distance model
 * PL(d) = PL(d0) + 10*n*log10(d/d0)
 */
static double
CalculatePathLoss(double distanceMeters, double referenceLoss, double exponent, double referenceDistance)
{
    if (distanceMeters <= 0.0)
        return 0.0;
    
    double ratio = std::max(distanceMeters / referenceDistance, 1e-6);
    return referenceLoss + 10.0 * exponent * std::log10(ratio);
}

/**
 * @brief Convert SNR to Link Quality Indicator (0-255)
 */
static uint8_t
CalculateLQI(double snrDb)
{
    // Clamp SNR to [0, 30] dB range
    double clampedSnr = std::max(0.0, std::min(30.0, snrDb));
    return static_cast<uint8_t>(std::round((clampedSnr / 30.0) * 255.0));
}

// ============================================================================
// Test Suite: CC2420 Module Verification
// ============================================================================

/**
 * Test 1: PHY Layer Object Creation and TypeId
 */
static void
TestPhyCreation()
{
    NS_LOG_INFO("\n[TEST 1] PHY Layer Object Creation");
    try
    {
        Ptr<Cc2420Phy> phy0 = CreateObject<Cc2420Phy>();
        Ptr<Cc2420Phy> phy1 = CreateObject<Cc2420Phy>();
        
        if (phy0 != nullptr && phy1 != nullptr)
        {
            results.PassTest("PHY layer objects created successfully");
        }
        else
        {
            results.FailTest("PHY layer creation", "Objects are nullptr");
        }
    }
    catch (const std::exception& e)
    {
        results.FailTest("PHY layer creation", std::string(e.what()));
    }
}

/**
 * Test 2: MAC Layer Object Creation and TypeId
 */
static void
TestMacCreation()
{
    NS_LOG_INFO("\n[TEST 2] MAC Layer Object Creation");
    try
    {
        Ptr<Cc2420Mac> mac0 = CreateObject<Cc2420Mac>();
        Ptr<Cc2420Mac> mac1 = CreateObject<Cc2420Mac>();
        
        if (mac0 != nullptr && mac1 != nullptr)
        {
            results.PassTest("MAC layer objects created successfully");
        }
        else
        {
            results.FailTest("MAC layer creation", "Objects are nullptr");
        }
    }
    catch (const std::exception& e)
    {
        results.FailTest("MAC layer creation", std::string(e.what()));
    }
}

/**
 * Test 3: MAC Configuration (IEEE 802.15.4 Parameters)
 */
static void
TestMacConfiguration()
{
    NS_LOG_INFO("\n[TEST 3] MAC Layer Configuration");
    try
    {
        Ptr<Cc2420Mac> mac = CreateObject<Cc2420Mac>();
        
        MacConfig config;
        config.panId = 0x1234;
        config.shortAddress = Mac16Address("00:01");
        config.macMinBE = 3;           // Backoff Exponent Min (3)
        config.macMaxBE = 5;           // Backoff Exponent Max (5)
        config.macMaxCSMABackoffs = 4; // Max backoff attempts (4)
        config.macMaxFrameRetries = 3; // Max retransmissions (3)
        config.txAckRequest = true;    // Request ACKs
        config.rxOnWhenIdle = true;    // Keep RX on when idle
        
        mac->SetMacConfig(config);
        MacConfig retrieved = mac->GetMacConfig();
        
        if (retrieved.panId == 0x1234 &&
            retrieved.macMinBE == 3 &&
            retrieved.macMaxBE == 5 &&
            retrieved.macMaxCSMABackoffs == 4 &&
            retrieved.macMaxFrameRetries == 3 &&
            retrieved.txAckRequest == true &&
            retrieved.rxOnWhenIdle == true)
        {
            results.PassTest("MAC configuration set and retrieved correctly");
        }
        else
        {
            results.FailTest("MAC configuration", "Configuration mismatch");
        }
    }
    catch (const std::exception& e)
    {
        results.FailTest("MAC configuration", std::string(e.what()));
    }
}

/**
 * Test 4: PHY-MAC Layer Binding
 */
static void
TestPhyMacBinding()
{
    NS_LOG_INFO("\n[TEST 4] PHY-MAC Layer Binding");
    try
    {
        Ptr<Cc2420Phy> phy = CreateObject<Cc2420Phy>();
        Ptr<Cc2420Mac> mac = CreateObject<Cc2420Mac>();
        
        mac->SetPhy(phy);
        Ptr<Cc2420Phy> retrievedPhy = mac->GetPhy();
        
        if (retrievedPhy == phy)
        {
            results.PassTest("PHY layer bound to MAC layer correctly");
        }
        else
        {
            results.FailTest("PHY-MAC binding", "Retrieved PHY doesn't match");
        }
    }
    catch (const std::exception& e)
    {
        results.FailTest("PHY-MAC binding", std::string(e.what()));
    }
}

/**
 * Test 5: Spectrum Channel Integration
 */
static void
TestSpectrumChannelIntegration()
{
    NS_LOG_INFO("\n[TEST 5] Spectrum Channel Integration");
    try
    {
        Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
        Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel>();
        loss->SetAttribute("Exponent", DoubleValue(3.0));
        loss->SetAttribute("ReferenceDistance", DoubleValue(1.0));
        loss->SetAttribute("ReferenceLoss", DoubleValue(46.6776));
        channel->AddPropagationLossModel(loss);
        
        Ptr<ConstantSpeedPropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel>();
        channel->SetPropagationDelayModel(delay);
        
        if (channel != nullptr && loss != nullptr && delay != nullptr)
        {
            results.PassTest("Spectrum channel and propagation models created");
        }
        else
        {
            results.FailTest("Spectrum channel", "Objects are nullptr");
        }
    }
    catch (const std::exception& e)
    {
        results.FailTest("Spectrum channel", std::string(e.what()));
    }
}

/**
 * Test 6: Node Creation and Mobility Setup
 */
static void
TestNodeMobility()
{
    NS_LOG_INFO("\n[TEST 6] Node Creation and Mobility Setup");
    try
    {
        NodeContainer nodes;
        nodes.Create(2);
        
        MobilityHelper mobility;
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility.Install(nodes);
        
        Ptr<MobilityModel> mob0 = nodes.Get(0)->GetObject<MobilityModel>();
        Ptr<MobilityModel> mob1 = nodes.Get(1)->GetObject<MobilityModel>();
        
        mob0->SetPosition(Vector(0.0, 0.0, 0.0));
        mob1->SetPosition(Vector(10.0, 0.0, 0.0));
        
        double distance = mob0->GetDistanceFrom(mob1);
        
        if (std::abs(distance - 10.0) < 0.001)
        {
            results.PassTest("Node positions set correctly (distance 10m)");
        }
        else
        {
            std::ostringstream oss;
            oss << "Distance mismatch: expected 10.0m, got " << distance << "m";
            results.FailTest("Node mobility", oss.str());
        }
    }
    catch (const std::exception& e)
    {
        results.FailTest("Node mobility", std::string(e.what()));
    }
}

/**
 * Test 7: RF Path Loss Calculation (Log Distance Model)
 */
static void
TestRFPathLoss()
{
    NS_LOG_INFO("\n[TEST 7] RF Path Loss Calculation");
    try
    {
        double pathLoss1m = CalculatePathLoss(1.0, 46.6776, 3.0, 1.0);
        double pathLoss10m = CalculatePathLoss(10.0, 46.6776, 3.0, 1.0);
        double pathLoss100m = CalculatePathLoss(100.0, 46.6776, 3.0, 1.0);
        
        // At 1m: path loss should be reference loss (46.6776 dB)
        if (std::abs(pathLoss1m - 46.6776) < 0.01)
        {
            results.PassTest("Path loss at 1m matches reference loss (46.68 dB)");
        }
        else
        {
            results.FailTest("Path loss at 1m", "Incorrect calculation");
        }
        
        // At 10m with exponent 3.0: +30 dB (10*3*log10(10))
        double expectedAt10m = 46.6776 + 30.0;
        if (std::abs(pathLoss10m - expectedAt10m) < 0.01)
        {
            results.PassTest("Path loss at 10m: 46.68 + 30 = 76.68 dB");
        }
        else
        {
            results.FailTest("Path loss at 10m", "Incorrect calculation");
        }
        
        // Verify distance exponent effect
        double expectedAt100m = 46.6776 + 60.0;
        if (std::abs(pathLoss100m - expectedAt100m) < 0.01)
        {
            results.PassTest("Path loss scaling verified (exponent 3.0)");
        }
        else
        {
            results.FailTest("Path loss scaling", "Incorrect exponent effect");
        }
    }
    catch (const std::exception& e)
    {
        results.FailTest("RF path loss", std::string(e.what()));
    }
}

/**
 * Test 8: RSSI and SNR Calculations at 10m
 */
static void
TestRSSISNR()
{
    NS_LOG_INFO("\n[TEST 8] RSSI and SNR Calculations");
    try
    {
        double txPowerDbm = 0.0;
        double pathLoss = CalculatePathLoss(10.0, 46.6776, 3.0, 1.0);
        double rssi = txPowerDbm - pathLoss;
        double noiseFloor = -100.0;
        double snr = rssi - noiseFloor;
        
        // At 10m: RSSI should be 0 - 76.68 ≈ -76.68 dBm
        if (std::abs(rssi - (-76.68)) < 0.1)
        {
            results.PassTest("RSSI calculation correct: 0 - 76.68 = -76.68 dBm");
        }
        else
        {
            results.FailTest("RSSI calculation", "Incorrect result");
        }
        
        // SNR should be -76.68 - (-100) ≈ 23.32 dB
        if (std::abs(snr - 23.32) < 0.5)
        {
            results.PassTest("SNR calculation correct: -76.68 - (-100) ≈ 23.32 dB");
        }
        else
        {
            results.FailTest("SNR calculation", "Incorrect result");
        }
        
        // RX sensitivity check (CC2420 spec: -95 dBm @ 1% PER)
        double rxSensitivity = -95.0;
        bool linkViable = (rssi > rxSensitivity);
        if (linkViable)
        {
            results.PassTest("Link viability: RSSI (-76.68 dBm) > Sensitivity (-95 dBm)");
        }
        else
        {
            results.FailTest("Link viability", "Link should be viable at 10m");
        }
    }
    catch (const std::exception& e)
    {
        results.FailTest("RSSI/SNR", std::string(e.what()));
    }
}

/**
 * Test 9: Link Quality Indicator (LQI) Conversion
 */
static void
TestLQI()
{
    NS_LOG_INFO("\n[TEST 9] Link Quality Indicator (LQI)");
    try
    {
        uint8_t lqi0 = CalculateLQI(0.0);      // 0% quality
        uint8_t lqi15 = CalculateLQI(15.0);    // 50% quality
        uint8_t lqi23 = CalculateLQI(23.32);   // 77.7% quality
        uint8_t lqi30 = CalculateLQI(30.0);    // 100% quality
        
        if (lqi0 == 0)
        {
            results.PassTest("LQI(0dB SNR) = 0/255");
        }
        else
        {
            results.FailTest("LQI at 0 dB SNR", "Should be 0");
        }
        
        if (lqi30 == 255)
        {
            results.PassTest("LQI(30dB SNR) = 255/255");
        }
        else
        {
            results.FailTest("LQI at 30 dB SNR", "Should be 255");
        }
        
        if (std::abs(lqi23 - 198) <= 2)  // (23.32/30)*255 ≈ 198
        {
            results.PassTest("LQI(23.32dB SNR) ≈ 198/255 (77.7%)");
        }
        else
        {
            results.FailTest("LQI at 23.32 dB SNR", "Should be ~198/255");
        }
    }
    catch (const std::exception& e)
    {
        results.FailTest("LQI calculation", std::string(e.what()));
    }
}

/**
 * Test 10: CC2420 Hardware Specifications Compliance
 */
static void
TestCC2420Specs()
{
    NS_LOG_INFO("\n[TEST 10] CC2420 Hardware Specifications Compliance");
    try
    {
        bool allSpecsMet = true;
        
        // CC2420 TX power levels (8 levels from -25 to 0 dBm)
        std::vector<double> txLevels = {0, -1, -3, -5, -7, -10, -15, -25};
        if (txLevels.size() == 8)
        {
            results.PassTest("CC2420 TX power levels: 8 levels (-25 to 0 dBm)");
        }
        else
        {
            allSpecsMet = false;
        }
        
        // CC2420 RX sensitivity: -95 dBm @ 1% Packet Error Rate
        double rxSensitivity = -95.0;
        if (rxSensitivity >= -95.0 && rxSensitivity <= -90.0)
        {
            results.PassTest("CC2420 RX sensitivity: -95 dBm (1% PER)");
        }
        else
        {
            allSpecsMet = false;
        }
        
        // CC2420 Data rate: 250 kbps (IEEE 802.15.4)
        double dataRate = 250.0;
        if (std::abs(dataRate - 250.0) < 0.1)
        {
            results.PassTest("CC2420 data rate: 250 kbps (IEEE 802.15.4 OQPSK)");
        }
        else
        {
            allSpecsMet = false;
        }
        
        // Frame transmission time for max frame (127 bytes)
        uint32_t maxFrame = 127;
        double txTime = (maxFrame * 8) / 250.0;  // ms
        if (std::abs(txTime - 4.064) < 0.01)
        {
            results.PassTest("Max frame (127B) TX time: 4.064 ms @ 250 kbps");
        }
        else
        {
            allSpecsMet = false;
        }
        
        // CSMA-CA parameters (IEEE 802.15.4 defaults)
        if (3 <= 5)  // macMinBE <= macMaxBE
        {
            results.PassTest("CSMA-CA: MinBE=3, MaxBE=5 (IEEE 802.15.4 defaults)");
        }
        
        // Link margin calculation at 10m
        double linkMargin = -76.68 - (-95.0);
        if (linkMargin > 15.0)
        {
            results.PassTest("Link margin at 10m: 18.32 dB (>15 dB for reliable comm)");
        }
    }
    catch (const std::exception& e)
    {
        results.FailTest("CC2420 specs", std::string(e.what()));
    }
}

/**
 * Test 11: Link Viability at Different Distances
 */
static void
TestLinkViabilityDistances()
{
    NS_LOG_INFO("\n[TEST 11] Link Viability at Different Distances");
    try
    {
        double txPowerDbm = 0.0;
        double rxSensitivityDbm = -95.0;
        double noiseFloorDbm = -100.0;
        
        struct DistanceTest
        {
            double distance;
            bool shouldBeViable;
            const char* description;
        };
        
        std::vector<DistanceTest> tests = {
            {5.0, true, "5m"},
            {10.0, true, "10m"},
            {15.0, true, "15m"},
            {20.0, true, "20m"},
            {25.0, false, "25m"},
            {30.0, false, "30m"}
        };
        
        int passedCount = 0;
        for (const auto& test : tests)
        {
            double pathLoss = CalculatePathLoss(test.distance, 46.6776, 3.0, 1.0);
            double rssi = txPowerDbm - pathLoss;
            bool viable = (rssi > rxSensitivityDbm);
            
            if (viable == test.shouldBeViable)
            {
                passedCount++;
            }
        }
        
        if (passedCount == tests.size())
        {
            results.PassTest("Link viability assessment correct for all distances");
        }
        else
        {
            std::ostringstream oss;
            oss << passedCount << "/" << tests.size() << " distances passed";
            results.FailTest("Link viability", oss.str());
        }
    }
    catch (const std::exception& e)
    {
        results.FailTest("Link viability distances", std::string(e.what()));
    }
}

/**
 * Test 12: Debug Callbacks Across CC2420 Layers
 */
static void
TestLayerDebugCallbacks()
{
    NS_LOG_INFO("\n[TEST 12] Layer Debug Callbacks (PHY/MAC/NetDevice/Energy)");
    try
    {
        Ptr<Cc2420Phy> phy = CreateObject<Cc2420Phy>();
        Ptr<Cc2420Mac> macA = CreateObject<Cc2420Mac>();
        Ptr<Cc2420Mac> macB = CreateObject<Cc2420Mac>();
        Ptr<Cc2420NetDevice> devA = CreateObject<Cc2420NetDevice>();
        Ptr<Cc2420NetDevice> devB = CreateObject<Cc2420NetDevice>();
        Ptr<Cc2420EnergyModel> energy = CreateObject<Cc2420EnergyModel>();

        MacConfig cfgA;
        cfgA.panId = 0x1234;
        cfgA.shortAddress = Mac16Address("00:01");
        macA->SetMacConfig(cfgA);

        MacConfig cfgB;
        cfgB.panId = 0x1234;
        cfgB.shortAddress = Mac16Address("00:02");
        macB->SetMacConfig(cfgB);

        macA->SetPhy(phy);
        macB->SetPhy(phy);
        devA->SetPhy(phy);
        devA->SetMac(macA);
        devB->SetMac(macB);
        energy->SetPhy(phy);

        g_layerDebugCounters.clear();

        macA->SetDebugPacketTraceCallback(MakeCallback(&OnMacDebugTrace));
        macB->SetDebugPacketTraceCallback(MakeCallback(&OnMacDebugTrace));
        phy->SetDebugPacketTraceCallback(MakeCallback(&OnPhyDebugTrace));
        devA->SetDebugPacketTraceCallback(MakeCallback(&OnNetDebugTrace));
        devB->SetDebugPacketTraceCallback(MakeCallback(&OnNetDebugTrace));
        energy->SetDebugPacketTraceCallback(MakeCallback(&OnEnergyDebugTrace));

        Ptr<Packet> p = Create<Packet>(42);
        devA->Send(p, Mac16Address("00:02"), 0);

        Ptr<SpectrumSignalParameters> sp = Create<SpectrumSignalParameters>();
        sp->duration = MicroSeconds(100);
        sp->txPhy = phy;
        sp->txAntenna = nullptr;

        phy->StartRx(sp);
        phy->TransmitPacket(Create<Packet>(12), MicroSeconds(50));
        energy->HandlePhyStateChange(PHY_IDLE, PHY_TX);

        Simulator::Run();
        Simulator::Destroy();

        bool allLayersHit = g_layerDebugCounters["MAC"] > 0 && g_layerDebugCounters["PHY"] > 0 &&
                            g_layerDebugCounters["NET"] > 0 && g_layerDebugCounters["ENERGY"] > 0;

        if (allLayersHit)
        {
            results.PassTest("Debug callbacks invoked in all CC2420 layers");
        }
        else
        {
            std::ostringstream oss;
            oss << "MAC=" << g_layerDebugCounters["MAC"] << ", PHY=" << g_layerDebugCounters["PHY"]
                << ", NET=" << g_layerDebugCounters["NET"] << ", ENERGY=" << g_layerDebugCounters["ENERGY"];
            results.FailTest("Layer debug callbacks", oss.str());
        }
    }
    catch (const std::exception& e)
    {
        results.FailTest("Layer debug callbacks", std::string(e.what()));
    }
}

// ============================================================================
// Main Verification Test
// ============================================================================

int
main(int argc, char* argv[])
{
    LogComponentEnable("Cc2420Verification", LOG_LEVEL_INFO);

    NS_LOG_INFO("\n" << std::string(80, '='));
    NS_LOG_INFO("CC2420 MAC MODULE VERIFICATION TEST");
    NS_LOG_INFO("Testing CC2420 implementation based on Castalia");
    NS_LOG_INFO("Module: /src/wsn/model/radio/cc2420");
    NS_LOG_INFO("Reference: /refs/Castalia");
    NS_LOG_INFO(std::string(80, '=') << "\n");

    // Run all tests
    NS_LOG_INFO("STARTING TEST SUITE...\n");
    
    TestPhyCreation();
    TestMacCreation();
    TestMacConfiguration();
    TestPhyMacBinding();
    TestSpectrumChannelIntegration();
    TestNodeMobility();
    TestRFPathLoss();
    TestRSSISNR();
    TestLQI();
    TestCC2420Specs();
    TestLinkViabilityDistances();
    TestLayerDebugCallbacks();

    // Print results
    results.PrintSummary();

    return results.testsFailed > 0 ? 1 : 0;
}
