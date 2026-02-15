#pragma once
#ifndef SENSOR_NETWORK_H
#define SENSOR_NETWORK_H

#include "wsn-object.h"
#include "wsn-node.h"

#include <string>
#include <vector>
#include <memory>

namespace ns3 {
namespace wsn {

class SensorNetwork : public ns3::wsn::WsnObject
{
public:
    explicit SensorNetwork(const std::string& name) : WsnObject("SN", name),
        field_x(30),
        field_y(30),
        field_z(0),
        numNodes(0),
        numPhysicalProcesses(1),
        deployment(""),
        wirelessChannelName("WirelessChannel"),
        debugInfoFileName("Trace.txt")
    {
    }
    ~SensorNetwork() override;

    bool SetProperty(const std::string &key, const std::string &value) override;
    void Build(BuildContext& ctx) override;

private:
    int field_x; // default (30);			// the length of the deployment field
	int field_y; // default (30);			// the width of the deployment field
	int field_z; // default (0);			// the height of the deployment field (2-D field by default)

	int numNodes;						// the number of nodes

	int numPhysicalProcesses; // default (1);

    std::string deployment; // default ("");
	std::string wirelessChannelName; // default ("WirelessChannel");
	std::string debugInfoFileName; // default ("Castalia-Trace.txt");
};


} // namespace wsn
} // namespace ns3

#endif // SENSOR_NETWORK_H