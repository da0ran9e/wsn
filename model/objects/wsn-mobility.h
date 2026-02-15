#pragma once
#ifndef MOBILITY_H
#define MOBILITY_H

#include "wsn-object.h"

namespace ns3 {
namespace wsn {

class Mobility : public ns3::wsn::WsnObject
{
public:
    Mobility(const std::string& name) : WsnObject("mobility", name),
        updateInterval(1000),
        xCoorDestination(0.0),
        yCoorDestination(0.0),
        zCoorDestination(0.0),
        speed(1.0)
    {
    };
    ~Mobility() override = default;

    bool SetProperty(const std::string &key, const std::string &value) override;
    void Build(BuildContext& ctx) override;
    
private:
    double updateInterval; // = default (1000);
	double xCoorDestination; // = default (0);
	double yCoorDestination; // = default (0);
	double zCoorDestination; // = default (0);
	double speed; // = default (1);
};
} // namespace wsn
} // namespace ns3
#endif // MOBILITY_H