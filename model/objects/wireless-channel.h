#pragma once
#ifndef WIRELESS_CHANNEL_H
#define WIRELESS_CHANNEL_H

#include <string>

#include "wsn-object.h"

namespace ns3 {
namespace wsn {
    
class WirelessChannel : public ns3::wsn::WsnObject
{
public:
    explicit WirelessChannel(const std::string& name) : WsnObject("wirelessChannel", name),
        collectTraceInfo(false),
        onlyStaticNodes(true),
        xCellSize(5),
        yCellSize(5),
        zCellSize(1),
        pathLossExponent(2.4),
        PLd0(55),
        d0(1.0),
        sigma(4.0),
        bidirectionalSigma(1.0),
        pathLossMapFile(""),
        temporalModelParametersFile(""),
        signalDeliveryThreshold(-100)
    {
    };
    ~WirelessChannel() override = default;
    
    bool SetProperty(const std::string &key, const std::string &value) override;
    void Build(BuildContext& ctx) override;
    
private:
    bool collectTraceInfo; // default (false);
	bool onlyStaticNodes; // default (true);		// if NO mobility, set it to true for greater efficiency 

	int xCellSize; // default (5);		// if we define cells (to handle mobility)
	int yCellSize; // default (5);		// how big are the cells in each dimension
	int zCellSize; // default (1);

	double pathLossExponent; // default (2.4);	// how fast is the signal strength fading
	double PLd0; // default (55);					// path loss at reference distance d0 (in dBm)
	double d0; // default (1.0);					// reference distance d0 (in meters)

	double sigma; // default (4.0);				// how variable is the average fade for nodes at the same distance
												// from eachother. std of a gaussian random variable.

	double bidirectionalSigma; // default (1.0);	// how variable is the average fade for link B->A if we know
												// the fade of link A->B. std of a gaussian random variable

	std::string pathLossMapFile; // default ("");		// describes a map of the connectivity based on pathloss
												// if defined, then the parameters above become irrelevant

	std::string temporalModelParametersFile; // default ("");	
												// the filename that contains all parameters for 
												// the temporal channel variation model

	double signalDeliveryThreshold; // default (-100);
};
} // namespace wsn
} // namespace ns3

#endif // WIRELESS_CHANNEL_H