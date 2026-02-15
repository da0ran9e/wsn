#pragma once
#ifndef WSN_OBJECT_REGISTRY_H
#define WSN_OBJECT_REGISTRY_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#include "wsn-object-factory.h"
#include "wsn-object.h"
// Network-level
#include "sensor-network.h"
#include "wireless-channel.h"

// Node-level
#include "wsn-node.h"
#include "wsn-mobility.h"
#include "wsn-radio.h"

// Communication submodules
#include "wsn-mac.h"
#include "wsn-routing.h"

// Upper layer
#include "wsn-app.h"

// Resources
#include "resource-manager.h"

namespace ns3 {
namespace wsn {

void RegisterWsnObjects();

class WsnObjectRegistry 
{
public:

    struct WildcardRule
    {
        std::string pathPattern;   // "SN.node[*]"
        std::string property;      // "xCoor"
        std::string value;         // "10"
    };

    WsnObjectRegistry();
    ~WsnObjectRegistry() = default;

    /**
     * Resolve or create object by hierarchical path
     * Example: "SN.node[0].mac"
     */
    std::shared_ptr<ns3::wsn::WsnObject> ResolveOrCreate(const std::string& path);

    /**
     * Get root object by name
     */
    std::shared_ptr<ns3::wsn::WsnObject> GetRoot(const std::string& name) const;

    /**
     * Clear all objects
     */
    void Clear();
    void AddWildcardRule(const std::string& pathPattern,
                        const std::string& property,
                        const std::string& value);
    void ApplyWildcardRules(std::shared_ptr<ns3::wsn::WsnObject> obj);
    struct PathSegment {
        std::string type;   // e.g., "node"
        std::string name;   // e.g., "0"
    };

    // Helpers
    static std::vector<PathSegment> ParsePath(const std::string& path);
    static PathSegment ParseSegment(const std::string& segment);

    std::shared_ptr<ns3::wsn::WsnObject> GetOrCreateRoot(const PathSegment& seg);
    std::shared_ptr<ns3::wsn::WsnObject> GetOrCreateChild(const std::string& path);

    std::unordered_map<std::string, std::shared_ptr<ns3::wsn::WsnObject>> m_roots;
    std::vector<WildcardRule> m_wildcardRules;

    private:
    WsnObjectFactory& m_factory;
};

} // namespace wsn
} // namespace ns3

#endif // WSN_OBJECT_REGISTRY_H