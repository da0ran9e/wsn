#include "wsn-object.h"
#include <sstream>
#include <stdexcept>

namespace ns3 {
namespace wsn {

// static factory registry init
std::map<std::string, FactoryFunc> WsnObject::s_factoryRegistry;

WsnObject::WsnObject(const std::string &typeName, const std::string &instanceName)
    : m_typeName(typeName), m_instanceName(instanceName)
{
}

WsnObject::~WsnObject() = default;

void WsnObject::SetParent(WeakWsnObjectPtr parent) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_parent = parent;
}

WeakWsnObjectPtr WsnObject::GetParent() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_parent;
}

std::string WsnObject::GetPath() const {
    // build path from root; avoid recursion heavy locks
    std::vector<std::string> parts;
    auto p = m_parent.lock();
    if (p) {
        // recursively collect parent's path
        std::string parentPath = p->GetPath();
        if (!parentPath.empty()) parts.push_back(parentPath);
    }
    std::string me = m_typeName;
    if (!m_instanceName.empty()) me += "[" + m_instanceName + "]";
    // join
    std::ostringstream oss;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i) oss << ".";
        oss << parts[i];
    }
    if (!parts.empty()) oss << ".";
    oss << me;
    return oss.str();
}

void WsnObject::DebugPrint(std::ostream& os, int indent) const
{
    PrintIndent(os, indent);

    os << GetPath()
       << "  [" << GetTypeName() << "]"
       << std::endl;

    // Print properties
    for (const auto& [key, value] : m_properties)
    {
        PrintIndent(os, indent + 1);
        os << key << " = " << value << std::endl;
    }

    // Recurse to children
    for (const auto& [key, vec] : m_children)
    {
        for (const auto& child : vec)
        {
            child->DebugPrint(os, indent + 1);
        }
    }

}


WsnObjectPtr WsnObject::GetChild(const std::string &name, bool createIfMissing) {
    //std::cout << "[obj] " << name << std::endl;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_children.find(name);
    if (it != m_children.end() && !it->second.empty())
        return it->second.front(); 
    if (!createIfMissing) return nullptr;
    // create via factory if available
    WsnObjectPtr child;
    child = CreateByType(name);
    if (!child) {
        // fallback to generic object
        child = std::make_shared<WsnObject>(name, "");
    }
    std::cout << "[obj] Create obj " << name << std::endl;
    child->SetParent(this->shared_from_this());
    m_children[name].push_back(child);
    NotifyChildAdded(child);
    return child;
}

WsnObjectPtr WsnObject::GetChildIndexed(const std::string &name, size_t idx, bool createIfMissing) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto &vec = EnsureChildVector(name);
    if (idx < vec.size()) return vec[idx];
    if (!createIfMissing) return nullptr;
    // create up to idx
    for (size_t i = vec.size(); i <= idx; ++i) {
        WsnObjectPtr child = CreateByType(name);
        if (!child) child = std::make_shared<WsnObject>(name, "");
        child->SetParent(this->shared_from_this());
        child->SetInstanceName(std::to_string(i));
        vec.push_back(child);
        NotifyChildAdded(child);
    }
    return vec[idx];
}

void WsnObject::AddChild(const std::string &name, WsnObjectPtr child) {
    std::lock_guard<std::mutex> lock(m_mutex);
    child->SetParent(this->shared_from_this());
    m_children[name].push_back(child);
    NotifyChildAdded(child);
}

std::vector<std::string> WsnObject::GetChildNames() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> names;
    for (auto &p : m_children) names.push_back(p.first);
    return names;
}

std::vector<WsnObjectPtr> WsnObject::GetChildren(const std::string &name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_children.find(name);
    if (it == m_children.end()) return {};
    return it->second;
}

bool WsnObject::SetProperty(const std::string &key, const std::string &value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_properties[key] = value;
    NotifyAttributeChanged(key, value);
    return true;
}


std::optional<std::string> WsnObject::GetProperty(const std::string &key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_properties.find(key);
    if (it == m_properties.end()) return std::nullopt;
    return it->second;
}

void WsnObject::Initialize() {
    // default: call Initialize on children
    for (auto &pair : m_children) {
        for (auto &child : pair.second) {
            child->Initialize();
        }
    }
}

void WsnObject::Validate() {
    // default: validate children
    for (auto &pair : m_children) {
        for (auto &child : pair.second) {
            child->Validate();
        }
    }
}

void WsnObject::Build(BuildContext& ctx) {
    //std::cout << "Building WsnObject: " << GetPath() << std::endl;
    // default: no-op; subclasses override to create ns-3 runtime objects
    std::cout << "++BaseBuilder++" << std::endl;
    for (auto &pair : m_children) {
        for (auto &child : pair.second) {
            // std::cout << "Building child: " << child->GetPath() << std::endl;
            child->Build(ctx);
        }
    }
}

std::vector<WsnObjectPtr> & WsnObject::EnsureChildVector(const std::string &name) {
    auto it = m_children.find(name);
    if (it == m_children.end()) {
        auto r = m_children.emplace(name, std::vector<WsnObjectPtr>{});
        return r.first->second;
    }
    return it->second;
}

// Factory registry
void WsnObject::RegisterFactory(const std::string &typeName, FactoryFunc f) {
    s_factoryRegistry[typeName] = f;
}

WsnObjectPtr WsnObject::CreateByType(const std::string &typeName) {
    auto it = s_factoryRegistry.find(typeName);
    if (it != s_factoryRegistry.end()) {
        return (it->second)();
    }
    return nullptr;
}

} // namespace wsn
} // namespace ns3
