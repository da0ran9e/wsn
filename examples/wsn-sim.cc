#include "ns3/core-module.h"

#include "ns3/wsn-module.h"

using namespace ns3;

int main(int argc, char *argv[])
{
    ns3::wsn::BuildContext ctx{};
    wsn::WsnScenario scenario;
    scenario.configure("input-pecee.ini", ctx);

    Simulator::Stop(Seconds(100));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}