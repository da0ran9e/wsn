#include "wsn-app.h"

namespace ns3 {
namespace wsn {
    
bool WsnApp::SetProperty(const std::string &key, const std::string &value)
{
    if (key == "applicationID") {
        applicationID = value;
    }
    else if (key == "collectTraceInfo") {
        collectTraceInfo = (value == "true" || value == "1");
    }
    else if (key == "priority") {
        priority = std::stoi(value);
    }
    else if (key == "packetHeaderOverhead") {
        packetHeaderOverhead = std::stoi(value);
    }
    else if (key == "constantDataPayload") {
        constantDataPayload = std::stoi(value);
    }
    else {
        return false;
    }

    NotifyAttributeChanged(key, value);
    return true;
}  

void WsnApp::Build(BuildContext& ctx)
{
    if(m_built) {
        return;
    }
    m_built = true;
    std::cout << "Building App: " << GetInstanceName() << std::endl;
    // Implementation of the Build method
    //WsnObject::Build(ctx);
}

} // namespace wsn
} // namespace ns3