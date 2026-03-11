/*
 * Scenario 4 - Fragment Implementation
 */

#include "fragment.h"
#include "../../../examples/scenarios/scenario4/scenario4-params.h"
#include "ns3/random-variable-stream.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

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
    totalConfidence = sum;
}

FragmentCollection
GenerateFragments(uint32_t numFragments)
{
    NS_LOG_FUNCTION(numFragments);
    
    FragmentCollection collection;

    if (numFragments == 0)
    {
        NS_LOG_WARN("GenerateFragments called with numFragments=0");
        return collection;
    }

    const uint32_t masterFileSize = ::ns3::wsn::scenario4::params::DEFAULT_MASTER_FILE_SIZE_BYTES;
    const double masterFileConfidence = ::ns3::wsn::scenario4::params::DEFAULT_MASTER_FILE_CONFIDENCE;
    const double weightMin = ::ns3::wsn::scenario4::params::FRAGMENT_WEIGHT_MIN;
    const double weightMax = ::ns3::wsn::scenario4::params::FRAGMENT_WEIGHT_MAX;

    // 1) Randomly split a large master file into different fragment sizes.
    Ptr<UniformRandomVariable> uniformRv = CreateObject<UniformRandomVariable>();
    uniformRv->SetAttribute("Min", DoubleValue(weightMin));
    uniformRv->SetAttribute("Max", DoubleValue(weightMax));

    std::vector<double> weights(numFragments, 1.0);
    for (uint32_t i = 0; i < numFragments; ++i)
    {
        weights[i] = uniformRv->GetValue();
    }

    const double sumWeights = std::accumulate(weights.begin(), weights.end(), 0.0);

    std::vector<uint32_t> fragmentSizes(numFragments, 0);
    std::vector<double> fractionalParts(numFragments, 0.0);
    uint64_t allocatedSize = 0;

    for (uint32_t i = 0; i < numFragments; ++i)
    {
        const double exactSize = (weights[i] / sumWeights) * static_cast<double>(masterFileSize);
        const uint32_t baseSize = static_cast<uint32_t>(std::floor(exactSize));
        fragmentSizes[i] = baseSize;
        fractionalParts[i] = exactSize - static_cast<double>(baseSize);
        allocatedSize += baseSize;
    }

    // Distribute remainder bytes by largest fractional parts to match exactly masterFileSize.
    uint32_t remainder = (allocatedSize < masterFileSize)
                             ? static_cast<uint32_t>(masterFileSize - allocatedSize)
                             : 0;

    std::vector<uint32_t> order(numFragments);
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&fractionalParts](uint32_t a, uint32_t b) {
        return fractionalParts[a] > fractionalParts[b];
    });

    for (uint32_t i = 0; i < remainder; ++i)
    {
        fragmentSizes[order[i % numFragments]] += 1;
    }
    
    // 2) Confidence depends on fragment size and is split from master-file confidence budget.
    for (uint32_t i = 0; i < numFragments; ++i) {
        Fragment frag;
        frag.fragmentId = i;
        frag.size = fragmentSizes[i];
        frag.confidence = (static_cast<double>(frag.size) / static_cast<double>(masterFileSize))
                          * masterFileConfidence;

        // Placeholder payload chunk (simulating file split).
        frag.data.resize(frag.size, static_cast<uint8_t>(i % 256));
        
        collection.AddFragment(frag);
    }

    uint64_t totalSize = 0;
    double totalConfidence = 0.0;
    for (const auto& [id, frag] : collection.fragments)
    {
        (void)id;
        totalSize += frag.size;
        totalConfidence += frag.confidence;
    }
    
    NS_LOG_INFO("Generated " << numFragments
                << " fragments from master file=" << masterFileSize << " bytes"
                << " | totalFragmentSize=" << totalSize << " bytes"
                << " | masterConfidence=" << masterFileConfidence
                << " | allocatedConfidence=" << totalConfidence
                << " | collectionConfidence=" << collection.totalConfidence);
    
    return collection;
}

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3
