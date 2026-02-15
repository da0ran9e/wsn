#pragma once
#ifndef WSN_OBJECT_FACTORY_H
#define WSN_OBJECT_FACTORY_H

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <sstream>

#include "wsn-object.h"

namespace ns3 {
namespace wsn {

class WsnObjectFactory
{
public:

    using CreatorFunc = std::function<std::shared_ptr<WsnObject>(const std::string& name)>;

    static WsnObjectFactory& Instance();

    void RegisterType(const std::string& key, CreatorFunc fn);

    std::shared_ptr<WsnObject> Create(const std::string& key,
                                      const std::string& name) const;

    bool HasType(const std::string& key) const;

private:
    WsnObjectFactory() = default;
    ~WsnObjectFactory() = default;

    WsnObjectFactory(const WsnObjectFactory&) = delete;
    WsnObjectFactory& operator=(const WsnObjectFactory&) = delete;

private:
    std::unordered_map<std::string, CreatorFunc> m_creators;
    mutable std::mutex m_mutex;
};

} // namespace wsn
} // namespace ns3

#endif // WSN_OBJECT_FACTORY_H