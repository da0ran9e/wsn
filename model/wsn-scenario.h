#ifndef WSN_SCENARIO_H
#define WSN_SCENARIO_H

#pragma once
#include "ini-parser.h"
#include "objects/wsn-object-registry.h"
#include "wsn-trace.h"

#include "ns3/wsn-object.h"
// #include "sensor-network.h"
// #include "wsn-routing.h"      
// #include "wsn-app.h"          // app non-IP

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <unordered_map>

namespace ns3 {
namespace wsn {

struct ParsedKey {
    std::string objectPath;
    std::string property;
};


class WsnScenario : public IniParser::Listener 
{
public:
    void configure(std::string iniFile, BuildContext &ctx);

private:
    // callbacks
    void onSection(const std::string &section) override;
    void onKeyValue(const std::string &key,
                    const std::string &value,
                    const std::string &comment,
                    const std::string &baseDir) override;

    ParsedKey ParseIniKey(const std::string& key);

private:
    std::string m_currentSection;
    WsnObjectRegistry m_registry;
    std::unordered_map<std::string, std::string> m_rawProperties;

    bool m_traceEnabled = false;
    std::string m_traceFile = "wsn-trace.txt";
    WsnTrace m_trace;


};

} // namespace wsn
} // namespace ns3

#endif // WSN_SCENARIO_H