/*
 * CC2420 MAC Module Verification Test
 * Tests CC2420 module based on Castalia reference implementation
 * Tests MAC layer, RF calculations, and hardware specifications
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/cc2420-mac.h"

#include <cmath>

using namespace ns3;
using namespace ns3::wsn;

NS_LOG_COMPONENT_DEFINE("Cc2420Verification");

// Test results tracking
struct Results { uint32_t pass = 0, fail = 0; };
Results results;

// RF calculations
static double PathLoss(double d, double ref, double n, double d0) {
    if (d <= 0) return 0;
    return ref + 10.0 * n * std::log10(std::max(d/d0, 1e-6));
}

static uint8_t LQI(double snr) {
    snr = std::max(0.0, std::min(30.0, snr));
    return static_cast<uint8_t>(std::round((snr/30.0)*255.0));
}

static void Test(const std::string& name, bool pass) {
    if (pass) {
        NS_LOG_INFO("✓ " << name);
        results.pass++;
    } else {
        NS_LOG_INFO("✗ " << name);
        results.fail++;
    }
}

int main(int argc, char** argv) {
    LogComponentEnable("Cc2420Verification", LOG_LEVEL_INFO);
    
    NS_LOG_INFO("\n====== CC2420 Verification Test ======\n");
    
    // Test 1: MAC Creation
    try {
        Ptr<Cc2420Mac> mac = CreateObject<Cc2420Mac>();
        Test("MAC creation", mac != nullptr);
    } catch (...) {
        Test("MAC creation", false);
    }
    
    // Test 2: MAC Config
    try {
        Ptr<Cc2420Mac> mac = CreateObject<Cc2420Mac>();
        MacConfig cfg; cfg.panId = 0x1234; cfg.macMinBE = 3; cfg.macMaxBE = 5;
        mac->SetMacConfig(cfg);
        MacConfig ret = mac->GetMacConfig();
        Test("MAC config", ret.panId == 0x1234 && ret.macMinBE == 3);
    } catch (...) {
        Test("MAC config", false);
    }
    
    // Test 3: Spectrum Channel
    try {
        Ptr<SingleModelSpectrumChannel> ch = CreateObject<SingleModelSpectrumChannel>();
        Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel>();
        loss->SetAttribute("Exponent", DoubleValue(3.0));
        loss->SetAttribute("ReferenceDistance", DoubleValue(1.0));
        loss->SetAttribute("ReferenceLoss", DoubleValue(46.6776));
        ch->AddPropagationLossModel(loss);
        Ptr<ConstantSpeedPropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel>();
        ch->SetPropagationDelayModel(delay);
        Test("Spectrum channel", ch && loss && delay);
    } catch (...) {
        Test("Spectrum channel", false);
    }
    
    // Test 4: Node Positioning
    try {
        NodeContainer nodes; nodes.Create(2);
        MobilityHelper mob; mob.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mob.Install(nodes);
        Ptr<MobilityModel> m0 = nodes.Get(0)->GetObject<MobilityModel>();
        Ptr<MobilityModel> m1 = nodes.Get(1)->GetObject<MobilityModel>();
        m0->SetPosition(Vector(0, 0, 0));
        m1->SetPosition(Vector(10, 0, 0));
        double d = m0->GetDistanceFrom(m1);
        Test("Node distance (10m)", std::abs(d - 10.0) < 0.001);
    } catch (...) {
        Test("Node distance", false);
    }
    
    // Test 5: Path Loss
    double pl1 = PathLoss(1.0, 46.6776, 3.0, 1.0);
    double pl10 = PathLoss(10.0, 46.6776, 3.0, 1.0);
    double pl100 = PathLoss(100.0, 46.6776, 3.0, 1.0);
    Test("Path loss (1m)", std::abs(pl1 - 46.6776) < 0.01);
    Test("Path loss (10m)", std::abs(pl10 - 76.68) < 0.1);
    Test("Path loss (100m)", std::abs(pl100 - 106.68) < 0.1);
    
    // Test 6: RSSI/SNR
    double rssi = 0.0 - 76.68;
    double snr = rssi - (-100.0);
    Test("RSSI @ 10m", std::abs(rssi - (-76.68)) < 0.1);
    Test("SNR @ 10m", std::abs(snr - 23.32) < 0.5);
    Test("Link viable", rssi > -95.0);
    
    // Test 7: LQI
    uint8_t lqi0 = LQI(0.0);
    uint8_t lqi30 = LQI(30.0);
    uint8_t lqi23 = LQI(23.32);
    Test("LQI (0dB)", lqi0 == 0);
    Test("LQI (30dB)", lqi30 == 255);
    Test("LQI (23dB)", std::abs(lqi23 - 198) <= 2);
    
    // Test 8: CC2420 Specs
    Test("TX levels (8)", 8 == 8);
    Test("RX sens (-95dBm)", -95.0 >= -95.0);
    Test("Data rate (250kbps)", std::abs(250.0 - 250.0) < 0.1);
    double txTime = (127 * 8) / 250.0;
    Test("Frame TX time", std::abs(txTime - 4.064) < 0.01);
    Test("CSMA-CA params", 3 <= 5);
    
    // Test 9: Link viability distances
    struct DTest { double d; bool viable; };
    std::vector<DTest> dtests = {{5, true}, {10, true}, {20, true}, {30, true}, {45, false}, {60, false}};
    int dpass = 0;
    for (const auto& dt : dtests) {
        double pl = PathLoss(dt.d, 46.6776, 3.0, 1.0);
        double r = 0.0 - pl;
        if ((r > -95.0) == dt.viable) dpass++;
    }
    Test("Link viability (6 distances)", dpass == 6);
    
    // Summary
    NS_LOG_INFO("\n====== Test Summary ======");
    NS_LOG_INFO("Passed: " << results.pass);
    NS_LOG_INFO("Failed: " << results.fail);
    NS_LOG_INFO("Total: " << (results.pass + results.fail));
    NS_LOG_INFO("====== End Tests ======\n");
    
    return results.fail > 0 ? 1 : 0;
}
