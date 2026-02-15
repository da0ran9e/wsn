#include "wsn-object-registry.h"

namespace ns3 {
namespace wsn {

void RegisterWsnObjects()
{
    auto& factory = WsnObjectFactory::Instance();

    factory.RegisterType("SN",
        [](const std::string& name) {
            //std::cout << "Creating SensorNetwork with name: " << name << std::endl;
            return std::make_shared<ns3::wsn::SensorNetwork>(name);
        });

    factory.RegisterType("wirelessChannel",
        [](const std::string& name) {
            return std::make_shared<ns3::wsn::WirelessChannel>(name);
        });

    factory.RegisterType("node",
        [](const std::string& name) {
            //std::cout << "Creating node with name: " << name << std::endl;
            std::shared_ptr<ns3::wsn::Node> node = std::make_shared<ns3::wsn::Node>(name);
            node->SetProperty("nodeAddr", name);
            return node;
        });

    factory.RegisterType("Mobility",
        [](const std::string& name) {
            return std::make_shared<ns3::wsn::Mobility>(name);
        });

    factory.RegisterType("MAC",
        [](const std::string& name) {
            return std::make_shared<ns3::wsn::WsnMac>(name);
        });

    factory.RegisterType("Radio",
        [](const std::string& name) {
            return std::make_shared<ns3::wsn::Radio>(name);
        });

    factory.RegisterType("Routing",
        [](const std::string& name) {
            return std::make_shared<ns3::wsn::WsnRouting>(name);
        });

    factory.RegisterType("Application",
        [](const std::string& name) {
            return std::make_shared<ns3::wsn::WsnApp>(name);
        });

    factory.RegisterType("ResourceManager",
        [](const std::string& name) {
            return std::make_shared<ns3::wsn::ResourceManager>(name);
        });
}

bool MatchWildcard(const std::string& pattern,
                   const std::string& path)
{
    std::string tempPath = path;
    if (path[path.size() - 1] == ']'){
        for (size_t i = path.size() - 1; i > 0; --i) {
        if (path[i - 1] == '[') {
            tempPath = path.substr(0, i - 1);
            break;
        }
    }
    }
    auto starPos = pattern.find("[*]");
    if (starPos == std::string::npos)
        return pattern == tempPath;
    std::string prefix = pattern.substr(0, starPos);
    std::string suffix = pattern.substr(starPos + 3);

    return tempPath.compare(0, prefix.size(), prefix) == 0 &&
           tempPath.size() >= prefix.size() + suffix.size() &&
           tempPath.compare(tempPath.size() - suffix.size(),
                         suffix.size(),
                         suffix) == 0;
}

WsnObjectRegistry::WsnObjectRegistry() : m_factory(WsnObjectFactory::Instance())
{
}

std::shared_ptr<ns3::wsn::WsnObject>
WsnObjectRegistry::ResolveOrCreate(const std::string& path)
{
     
    if (path.empty())
        return nullptr;

    auto segments = ParsePath(path);
    if (segments.empty())
        return nullptr;
    
    //std::cout << "obj: " << segments[segments.size()-1].type << "-" << segments[segments.size()-1].name << std::endl;
    std::shared_ptr<ns3::wsn::WsnObject> current;

    for (size_t i = 0; i < segments.size(); ++i) {
        const auto& seg = segments[i];

        if (i == 0) {
            // Root object (e.g., SN)
            current = GetOrCreateRoot(seg);
            continue;
        }

        // Child object
        auto child = GetOrCreateChild(path);
        if (!child) {
            //std::cout << "Cannot create child object at path: " << path << std::endl;
            // NS_LOG_WARN("Cannot create child object at path: " << path);
            return nullptr;
        }
        if (seg.name != "") {
            child = current->GetChildIndexed(seg.type, std::stoul(seg.name), true);
            //child->SetProperty("nodeAddr", seg.name);
        }
        // if (!child) {
        //     child = m_factory.Create(seg.type, seg.name);
        //     current->AddChild(child);
        // }
        ApplyWildcardRules(child);
        current = child;
    }

    return current;
}

void WsnObjectRegistry::AddWildcardRule(
        const std::string& pathPattern,
        const std::string& property,
        const std::string& value)
{
    // std::cout << "Adding wildcard rule: "
    //           << pathPattern << " -> "
    //           << property << " = "
    //           << value << std::endl;
              
    m_wildcardRules.push_back({ pathPattern, property, value });
}

bool SplitSuffix(const std::string& suffix,
                 std::vector<std::string>& objectPath,
                 std::string& property)
{
    //std::cout << "Suffix to split: " << suffix << std::endl;
    objectPath.clear();
    property.clear();

    if (suffix.empty())
        return false;

    size_t start = 0;
    size_t pos;

    std::vector<std::string> tokens;

    while ((pos = suffix.find('.', start)) != std::string::npos) {
        tokens.push_back(suffix.substr(start, pos - start));
        //std::cout << "Token : " << suffix.substr(start, pos - start) << std::endl;
        start = pos + 1;
    }
    tokens.push_back(suffix.substr(start));
    

    if (tokens.size() < 1)
        return false;

    // last token is property
    property = tokens.back();
    tokens.pop_back();

    // remaining tokens are object path
    objectPath = std::move(tokens);

    return true;
}

void WsnObjectRegistry::ApplyWildcardRules(
        std::shared_ptr<ns3::wsn::WsnObject> obj)
{
    std::string objPath = obj->GetPath();

    for (const auto& rule : m_wildcardRules) {
        //std::cout << "wildcard size: " << m_wildcardRules.size() << std::endl;
        // std::cout << "Checking wildcard rule: "
        //           << rule.pathPattern << " against object path: "
        //           << objPath << std::endl;
        if (MatchWildcard(rule.pathPattern, objPath)) {
            // std::cout << "Applying wildcard rule: "
            //           << rule.pathPattern << " -> "
            //           << rule.property << " = "
            //           << rule.value << " to object at path: "
            //           << objPath << std::endl;
            std::vector<std::string> objName;
            std::string property;
            bool isObject = SplitSuffix(rule.property, objName, property);
            std::shared_ptr<ns3::wsn::WsnObject> tempObj = obj;
            if (isObject) {
                for (const auto& name : objName) {
                    //std::cout << "Creating object for wildcard rule: " << name << std::endl;
                    tempObj = GetOrCreateChild(tempObj->GetPath() + "." + name);
                    if (!tempObj) {
                        // NS_LOG_WARN("Cannot create object for wildcard rule at path: "
                        //              << objPath);
                        break;
                    }
                }
                if (tempObj){
                    tempObj->SetProperty(property, rule.value);
                }
                
            } else {
                obj->SetProperty(rule.property, rule.value);
            }
        }
    }
}


std::shared_ptr<WsnObject>
WsnObjectRegistry::GetRoot(const std::string& name) const
{
    auto it = m_roots.find(name);
    return (it != m_roots.end()) ? it->second : nullptr;
}

void
WsnObjectRegistry::Clear()
{
    m_roots.clear();
}

/* =========================
 * Path parsing helpers
 * ========================= */

WsnObjectRegistry::PathSegment
WsnObjectRegistry::ParseSegment(const std::string& segment)
{
    PathSegment seg;

    auto lb = segment.find('[');
    auto rb = segment.find(']');

    if (lb != std::string::npos && rb != std::string::npos && rb > lb) {
        seg.type = segment.substr(0, lb);
        seg.name = segment.substr(lb + 1, rb - lb - 1);
    } else {
        seg.type = segment;
        seg.name = "";
    }

    return seg;
}

std::vector<WsnObjectRegistry::PathSegment>
WsnObjectRegistry::ParsePath(const std::string& path)
{
    std::vector<PathSegment> segments;
    std::stringstream ss(path);
    std::string token;

    while (std::getline(ss, token, '.')) {
        if (!token.empty())
            segments.push_back(ParseSegment(token));
    }

    return segments;
}

/* =========================
 * Root handling
 * ========================= */

std::shared_ptr<WsnObject>
WsnObjectRegistry::GetOrCreateRoot(const PathSegment& seg)
{
    auto it = m_roots.find(seg.type);
    if (it != m_roots.end())
        return it->second;

    auto root = m_factory.Create(seg.type, seg.name);
    m_roots[seg.type] = root;
    return root;
}

std::shared_ptr<ns3::wsn::WsnObject>
WsnObjectRegistry::GetOrCreateChild(const std::string& path)
{
    //std::cout << "***** " << path << std::endl;
    auto segments = ParsePath(path);
    if (segments.empty())
        return nullptr;

    // 1. Root
    const PathSegment& rootSeg = segments[0];

    if (!m_factory.HasType(rootSeg.type)) {
        std::cout << "Dropped unknown object type: " << rootSeg.type << std::endl;
        return nullptr;
    }

    auto current = GetOrCreateRoot(rootSeg);

    // 2. Walk down the tree
    for (size_t i = 1; i < segments.size(); ++i) {
        const PathSegment& seg = segments[i];

        // Nếu type chưa được register → bỏ qua toàn bộ subtree
        if (!m_factory.HasType(seg.type)) {
            // NS_LOG_WARN("Unknown object type: " << seg.type
            //              << " at path " << current->GetPath()
            //              << " (ignored)");
            return nullptr;
        }
        ns3::wsn::WsnObjectPtr child;
        if (seg.type == "node"){
            int nodeId = atoi(strcpy(new char[seg.name.size() + 1], seg.name.c_str()));
            child = current->GetChildIndexed(seg.type, nodeId, false);
        } else {
            child = current->GetChild(seg.type, false);
        }
        //std::cout << "Creating/getting child: " << seg.name << std::endl;
        
        if (!child) {
            child = m_factory.Create(seg.type, seg.name);
            current->AddChild(seg.type, child);
        }

        current = child;
    }

    return current;
}

} // namespace wsn
} // namespace ns3
