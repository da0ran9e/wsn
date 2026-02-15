#pragma once
#ifndef WSN_APP_H
#define WSN_APP_H

#include "wsn-object.h"

#include <string>

namespace ns3 {
namespace wsn {
class WsnApp : public ns3::wsn::WsnObject
{
public:
    explicit WsnApp(const std::string& name) : WsnObject("WsnApp", name),
        applicationID("defaultApp"),
        priority(0),
        packetHeaderOverhead(0),
        constantDataPayload(0)
        {}

    ~WsnApp() override = default;

    bool SetProperty(const std::string &key, const std::string &value) override;
    void Build(BuildContext& ctx) override;
    
private:
    std::string applicationID;
	bool collectTraceInfo;
	int priority;
	int packetHeaderOverhead;	// in bytes
	int constantDataPayload;
};
} // namespace wsn
} // namespace ns3

#endif // WSN_APP_H