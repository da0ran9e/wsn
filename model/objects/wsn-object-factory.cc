#include "wsn-object-factory.h"

namespace ns3 {
namespace wsn {

WsnObjectFactory& WsnObjectFactory::Instance()
{
    static WsnObjectFactory instance;
    return instance;
}

void WsnObjectFactory::RegisterType(const std::string& key, CreatorFunc fn)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!fn) {
        throw std::runtime_error("WsnObjectFactory: creator function is null for key '" + key + "'");
    }

    auto it = m_creators.find(key);
    if (it != m_creators.end()) {
        throw std::runtime_error("WsnObjectFactory: type '" + key + "' already registered");
    }

    m_creators[key] = fn;
}

std::shared_ptr<WsnObject>
WsnObjectFactory::Create(const std::string& key,
                         const std::string& name) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_creators.find(key);
    if (it == m_creators.end()) {
        std::ostringstream oss;
        oss << "WsnObjectFactory: unknown object type '" << key << "'";
        throw std::runtime_error(oss.str());
    }

    auto obj = it->second(name);
    if (!obj) {
        throw std::runtime_error("WsnObjectFactory: creator returned null for key '" + key + "'");
    }

    return obj;
}

bool WsnObjectFactory::HasType(const std::string& key) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_creators.find(key) != m_creators.end();
}

} // namespace wsn
} // namespace ns3
