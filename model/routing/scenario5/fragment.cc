/*
 * Scenario 4 - Fragment Implementation
 */

#include "fragment.h"
#include "../../../examples/scenarios/scenario5/scenario5-params.h"
#include "ns3/log.h"
#include <cmath>
#include <vector>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Scenario5Fragment");

namespace wsn {
namespace scenario5 {
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

    // Union probability: p_union = 1 - prod(1 - p_i)
    // Fuses independent per-fragment detection probabilities.
    // When all N fragments are present this recovers the full-image base confidence.
    double product = 1.0;
    for (const auto& pair : fragments) {
        product *= (1.0 - pair.second.confidence);
    }
    totalConfidence = 1.0 - product;
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

    // Image dimensions and encoding parameters.
    const uint32_t imageWidth    = ::ns3::wsn::scenario5::params::DEFAULT_IMAGE_WIDTH;
    const uint32_t imageHeight   = ::ns3::wsn::scenario5::params::DEFAULT_IMAGE_HEIGHT;
    const uint32_t bytesPerPixel = ::ns3::wsn::scenario5::params::DEFAULT_BYTES_PER_PIXEL;
    const double   baseConf      = ::ns3::wsn::scenario5::params::DEFAULT_MASTER_FILE_CONFIDENCE;
    const uint32_t totalPixels   = imageWidth * imageHeight;

    // Pixel-stride interleaving (stride = numFragments):
    // Fragment i owns pixels at row-major positions i, i+N, i+2N, ...
    // giving each fragment a spatially-uniform sample of the full image.
    const uint32_t basePixels  = totalPixels / numFragments;
    const uint32_t extraPixels = totalPixels % numFragments; // first extraPixels fragments get +1

    for (uint32_t i = 0; i < numFragments; ++i)
    {
        Fragment frag;
        frag.fragmentId   = i;
        frag.strideOffset = i;  // stride slot = fragment index for N-way uniform split
        frag.pixelCount   = basePixels + (i < extraPixels ? 1u : 0u);

        // Per-fragment detection probability derived from the inverse union-probability formula:
        //   p_i = 1 - (1 - baseConf)^(pixelCount_i / totalPixels)
        // This guarantees: 1 - prod(1 - p_i) = baseConf when all N fragments are present.
        const double pixelFraction = static_cast<double>(frag.pixelCount)
                                     / static_cast<double>(totalPixels);
        frag.confidence = 1.0 - std::pow(1.0 - baseConf, pixelFraction);

        // Simulated pixel data: RGB bytes, byte pattern distinguishes fragments.
        frag.data.resize(frag.pixelCount * bytesPerPixel, static_cast<uint8_t>(i % 256));

        collection.AddFragment(frag);
    }

    // Logging summary.
    uint64_t totalDataBytes = 0;
    for (const auto& [id, frag] : collection.fragments)
    {
        (void)id;
        totalDataBytes += static_cast<uint64_t>(frag.pixelCount) * bytesPerPixel;
    }

    NS_LOG_INFO("Generated " << numFragments
                << " pixel-region fragments | image=" << imageWidth << "x" << imageHeight
                << " (" << totalPixels << " px)"
                << " | totalDataBytes=" << totalDataBytes
                << " | baseConf=" << baseConf
                << " | p_union=" << collection.totalConfidence);

    return collection;
}

} // namespace routing
} // namespace scenario5
} // namespace wsn
} // namespace ns3
