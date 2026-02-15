#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <mutex>
#include <optional>
#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/log.h"
#include "ns3/mac16-address.h"
#include "ns3/simulator.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/wsn-forwarder.h"
#include "ns3/wsn-routing-protocol.h"

namespace ns3 {
namespace wsn {

class WsnObject;
using WsnObjectPtr = std::shared_ptr<WsnObject>;
using WeakWsnObjectPtr = std::weak_ptr<WsnObject>;
using FactoryFunc = std::function<WsnObjectPtr(void)>;

static void PrintIndent(std::ostream& os, int indent)
{
    for (int i = 0; i < indent; ++i)
        os << "  ";
}

struct BuildContext {
    // ----- global topology -----
    ns3::NodeContainer nodes;
    ns3::MobilityHelper mobility;

    ns3::Ptr<ns3::SingleModelSpectrumChannel> spectrumChannel;
    ns3::Ptr<ns3::LogDistancePropagationLossModel> lossModel;
    ns3::Ptr<ns3::ConstantSpeedPropagationDelayModel> delayModel;

    ns3::LrWpanHelper lrwpan;
    ns3::NetDeviceContainer netDevices;

    // ----- mapping / metadata -----
    std::map<uint16_t, uint16_t> nodeAddr;
};

// Base configurable object for WSN configuration tree
class WsnObject : public std::enable_shared_from_this<WsnObject>
{
public:
    WsnObject(const std::string &typeName = "WsnObject", const std::string &instanceName = "");
    virtual ~WsnObject();

    template <typename T>
    std::shared_ptr<T> FindAncestor()
    {
        auto cur = m_parent.lock();
        while (cur) {
            if (auto casted = std::dynamic_pointer_cast<T>(cur))
                return casted;
            cur = cur->m_parent.lock();
        }
        return nullptr;
    }

    // Identification
    const std::string & GetTypeName() const { return m_typeName; }
    const std::string & GetInstanceName() const { return m_instanceName; }
    void SetInstanceName(const std::string &name) { m_instanceName = name; }

    // Parent / path helpers
    void SetParent(WeakWsnObjectPtr parent);
    WeakWsnObjectPtr GetParent() const;
    std::string GetPath() const; // e.g. "SN.node[3].Communication.MAC"
    virtual void DebugPrint(std::ostream& os, int indent = 0) const;
    WsnObjectPtr GetChild(const std::string &name, bool createIfMissing = false);
    WsnObjectPtr GetChildIndexed(const std::string &name, size_t idx, bool createIfMissing = false);
    void AddChild(const std::string &name, WsnObjectPtr child);
    std::vector<std::string> GetChildNames() const;
    std::vector<WsnObjectPtr> GetChildren(const std::string &name) const;
   
    // Returns true if property handled (consumed).
    virtual bool SetProperty(const std::string &key, const std::string &value);
    virtual std::optional<std::string> GetProperty(const std::string &key) const;

    // Lifecycle hooks
    virtual void Initialize(); // called after parsing full tree
    virtual void Validate();   // validate parameters, throw or log errors
    virtual void Build(BuildContext& ctx); // Build() may convert config -> runtime (ns-3) objects; default: no-op      

    // Factory registry: register per-type factory
    static void RegisterFactory(const std::string &typeName, FactoryFunc f);
    static WsnObjectPtr CreateByType(const std::string &typeName);
    
    // Listener interface
    class Listener {
    public:
        virtual ~Listener() = default;

        // Khi một thuộc tính được parse: name="node[0].xCoor", value="1"
        virtual void OnAttributeChanged(
            WsnObject* obj,
            const std::string& attrName,
            const std::string& attrValue
        ) = 0;

        // Khi một object con được thêm (ví dụ Node thuộc về Network)
        virtual void OnChildAdded(
            WsnObject* parent,
            WsnObject* child
        ) = 0;
    };

    void AddListener(std::shared_ptr<Listener> listener) {
        m_listeners.push_back(listener);
    }

    void RemoveListener(std::shared_ptr<Listener> listener) {
        m_listeners.erase(
            std::remove(m_listeners.begin(), m_listeners.end(), listener),
            m_listeners.end()
        );
    }

protected:
    // Helper: ensure vector for child name exists
    std::vector<WsnObjectPtr> & EnsureChildVector(const std::string &name);

    // Raw property store, default handling
    std::map<std::string, std::string> m_properties;

    // children: name -> vector<instances>
    std::map<std::string, std::vector<WsnObjectPtr>> m_children;

    // Backlink to parent (weak to avoid cycles)
    WeakWsnObjectPtr m_parent;

    // Identification
    std::string m_typeName;     // e.g. "node", "MAC", "SensorNetwork"
    std::string m_instanceName; // instance label (optional) e.g. "3" or user label

    // Synchronization (optional)
    mutable std::mutex m_mutex;

    // Static factory registry
    static std::map<std::string, FactoryFunc> s_factoryRegistry;

    bool m_built = false;
private:
    std::vector<std::shared_ptr<Listener>> m_listeners;

protected:
    // Notify listeners about attribute change
    void NotifyAttributeChanged(const std::string& attrName, const std::string& attrValue) {
        for (const auto& listener : m_listeners) {
            listener->OnAttributeChanged(this, attrName, attrValue);
        }
    }

    // Notify listeners about child addition
    void NotifyChildAdded(WsnObjectPtr child) {
        for (const auto& listener : m_listeners) {
            listener->OnChildAdded(this, child.get());
        }
    }
};

} // namespace wsn
} // namespace ns3
