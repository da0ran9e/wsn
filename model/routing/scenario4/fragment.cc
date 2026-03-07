/*
 * Scenario 4 - Fragment Implementation
 */

#include "fragment.h"
#include "ns3/random-variable-stream.h"
#include "ns3/double.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Scenario4Fragment");

namespace wsn {
namespace scenario4 {
namespace routing {

void
FragmentCollection::AddFragment(const Fragment& frag)
{
    fragments[frag.fragmentId] = frag;
    UpdateTotalConfidence();
}

bool
FragmentCollection::HasFragment(uint32_t fragmentId) const
{
    return fragments.find(fragmentId) != fragments.end();
}

const Fragment*
FragmentCollection::GetFragment(uint32_t fragmentId) const
{
    auto it = fragments.find(fragmentId);
    if (it != fragments.end()) {
        return &(it->second);
    }
    return nullptr;
}

void
FragmentCollection::UpdateTotalConfidence()
{
    if (fragments.empty()) {
        totalConfidence = 0.0;
        return;
    }
    
    double sum = 0.0;
    for (const auto& pair : fragments) {
        sum += pair.second.confidence;
    }
    totalConfidence = sum / fragments.size();
}

FragmentCollection
GenerateFragments(uint32_t numFragments)
{
    NS_LOG_FUNCTION(numFragments);
    
    FragmentCollection collection;
    
    // Use uniform random for confidence distribution
    Ptr<UniformRandomVariable> uniformRv = CreateObject<UniformRandomVariable>();
    uniformRv->SetAttribute("Min", DoubleValue(0.1));
    uniformRv->SetAttribute("Max", DoubleValue(0.9));
    
    for (uint32_t i = 0; i < numFragments; ++i) {
        Fragment frag;
        frag.fragmentId = i;
        frag.confidence = uniformRv->GetValue();
        frag.size = 1024;  // 1KB placeholder
        // frag.data left empty for now
        
        collection.AddFragment(frag);
    }
    
    NS_LOG_INFO("Generated " << numFragments << " fragments with avg confidence: " 
                << collection.totalConfidence);
    
    return collection;
}

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3
