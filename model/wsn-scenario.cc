#include "wsn-scenario.h"

namespace ns3 {
namespace wsn {

void WsnScenario::onSection(const std::string &section)
{
    m_currentSection = section;
    std::cout << "Callback Section: " << section << std::endl;
    m_trace.Trace("Section: " + section);
    // if (m_config.find(section) == m_config.end())
    //     m_config[section] = {};
}

bool HasWildcard(const std::string& path)
{
    return path.find("[*]") != std::string::npos;
}

void ParseWildcard(const std::string& path,
                   std::string& outPrefix,
                   std::string& outSuffix)
{
    auto starPos = path.find("[*]");
    if (starPos == std::string::npos) {
        outPrefix = path;
        outSuffix = "";
        return;
    }

    outPrefix = path.substr(0, starPos);
    outSuffix = path.substr(starPos + 4);
}

void WsnScenario::onKeyValue(const std::string &key,
                             const std::string &value,
                             const std::string &comment,
                             const std::string &baseDir)
{
    if (m_currentSection != "General")
        return;

    ParsedKey parsed = ParseIniKey(key);

    std::string outPrefix, outSuffix;
    ParseWildcard(key, outPrefix, outSuffix);

    if (HasWildcard(parsed.objectPath)) {
        m_registry.AddWildcardRule(outPrefix,
                                outSuffix,
                                value);
        return;
    }

    auto obj = m_registry.ResolveOrCreate(parsed.objectPath);

    if (!obj) {
        //NS_FATAL_ERROR("Cannot resolve object path: " << parsed.objectPath);
        std::cout << "Cannot resolve object path: " << parsed.objectPath << std::endl;
        return;
    }

    // set property
    if (!obj->SetProperty(parsed.property, value)) {
        std::cout << "Warning: Unknown property '" << parsed.property
                  << "' for object at path '" << parsed.objectPath << "'" << std::endl;
    }

    if (parsed.property == "numNodes" && parsed.objectPath == "SN") {
        //m_numNodes = std::stoi(value);
        for (int i = 0; i < std::stoi(value); ++i) {
            auto nodeObj = m_registry.ResolveOrCreate("SN.node[" + std::to_string(i) + "]");
            auto mobilityObj = m_registry.ResolveOrCreate("SN.node[" + std::to_string(i) + "].Mobility");
            auto macObj = m_registry.ResolveOrCreate("SN.node[" + std::to_string(i) + "].MAC");
            auto radioObj = m_registry.ResolveOrCreate("SN.node[" + std::to_string(i) + "].Radio");
            auto appObj = m_registry.ResolveOrCreate("SN.node[" + std::to_string(i) + "].App");
            auto routingObj = m_registry.ResolveOrCreate("SN.node[" + std::to_string(i) + "].Routing");
            
            //obj->AddChild("node", nodeObj);
            //obj->GetChildIndexed("node", i, true);
        }
    } 
}

ParsedKey WsnScenario::ParseIniKey(const std::string& key)
{
    auto pos = key.rfind('.');
    if (pos == std::string::npos) {
        // no dot → property belongs to root object
        return { "", key };
    }

    ParsedKey result;
    result.objectPath = key.substr(0, pos);
    result.property   = key.substr(pos + 1);
    return result;
}


void WsnScenario::configure(std::string iniFile, BuildContext &ctx)
{
    RegisterWsnObjects();
    m_trace.Open(m_traceFile);
    IniParser iniParser;
    iniParser.setListener(this);
    
    iniParser.read(iniFile);

    auto root = m_registry.GetRoot("SN");
    //BuildContext ctx{};
    if (root){
        std::ostringstream os;
        root->DebugPrint(os);
        std::cout << os.str();
        root->Build(ctx);
    }
}


} // namespace wsn
} // namespace ns3