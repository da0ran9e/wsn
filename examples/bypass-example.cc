#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/spectrum-module.h"

#include "ns3/wsn-module.h"
#include "ns3/wsn-forwarder.h"
#include "ns3/bypass-routing-protocol.h"

using namespace ns3;
using namespace ns3::wsn;

void TraceTxSpectrum(
    Ptr<const SpectrumSignalParameters> params)
{
    std::cout << "[SensorNetwork] Transmission started at time "
              << Simulator::Now().GetSeconds() << "s"
              << std::endl;
}

void TraceMacTx(
    Ptr<const Packet> packet)
{
    std::cout << "[SensorNetwork] MAC transmitted packet of size "
              << packet->GetSize() << " bytes at time "
              << Simulator::Now().GetSeconds() << "s"
              << std::endl;
}

void TracePhyTx(
    Ptr<const Packet> packet)
{
    std::cout << "[SensorNetwork] PHY transmitted packet of size "
              << packet->GetSize() << " bytes at time "
              << Simulator::Now().GetSeconds() << "s"
              << std::endl;
}

int
main(int argc, char *argv[])
{
  // -----------------------------
  // 1. Create nodes
  // -----------------------------
  NodeContainer nodes;
  nodes.Create(2);

  // -----------------------------
  // 2. Mobility (fixed)
  // -----------------------------
  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(nodes);

  nodes.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(0, 0, 0));
  nodes.Get(1)->GetObject<MobilityModel>()->SetPosition(Vector(10, 0, 0));

  // -----------------------------
  // 3. Channel
  // -----------------------------
  Ptr<SingleModelSpectrumChannel> channel =
      CreateObject<SingleModelSpectrumChannel>();

  Ptr<LogDistancePropagationLossModel> loss =
      CreateObject<LogDistancePropagationLossModel>();
  Ptr<ConstantSpeedPropagationDelayModel> delay =
      CreateObject<ConstantSpeedPropagationDelayModel>();

  channel->AddPropagationLossModel(loss);
  channel->SetPropagationDelayModel(delay);
channel->TraceConnectWithoutContext(
    "TxStart",
    MakeCallback(&TraceTxSpectrum));
  // -----------------------------
  // 4. LR-WPAN
  // -----------------------------
  LrWpanHelper lrwpan;
  lrwpan.SetChannel(channel);

  NetDeviceContainer devices = lrwpan.Install(nodes);

  for (uint32_t i = 0; i < devices.GetN(); ++i)
  {
    Ptr<lrwpan::LrWpanNetDevice> dev =
        DynamicCast<lrwpan::LrWpanNetDevice>(devices.Get(i));
    dev->GetMac()->SetShortAddress(Mac16Address::Allocate());
    dev->GetMac()->TraceConnectWithoutContext(
    "McpsDataRequest",
    MakeCallback(&TraceMacTx));
    dev->GetPhy()->TraceConnectWithoutContext(
    "TxBegin",
    MakeCallback(&TracePhyTx));

  }

  // -----------------------------
  // 5. Install WSN stack
  // -----------------------------
  for (uint32_t i = 0; i < nodes.GetN(); ++i)
  {
    Ptr<ns3::Node> node = nodes.Get(i);
    Ptr<NetDevice> dev = devices.Get(i);

    Ptr<WsnForwarder> forwarder =
        CreateObject<WsnForwarder>();
    Ptr<ns3::wsn::BypassRoutingProtocol> routing =
        CreateObject<ns3::wsn::BypassRoutingProtocol>();

    routing->SetSelfNodeProperties({
        static_cast<uint16_t>(i),
        node->GetObject<MobilityModel>()->GetPosition().x,
        node->GetObject<MobilityModel>()->GetPosition().y,
        node->GetObject<MobilityModel>()->GetPosition().z
    });

    //routing->SetNode(node);
    routing->SetForwarder(forwarder);

    forwarder->SetNetDevice(dev);
    //forwarder->SetRouting(routing);

    node->AggregateObject(forwarder);
    node->AggregateObject(routing);

    // -----------------------------
  // 6. Tic → Toc
  // -----------------------------
  routing->Start();
  }

  

  Simulator::Stop(Seconds(5.0));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}
