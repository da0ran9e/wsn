/*
 * Scenario 4 - Fragment Data Structure
 */

#ifndef SCENARIO5_FRAGMENT_H
#define SCENARIO5_FRAGMENT_H

#include <cstdint>
#include <map>
#include <vector>

namespace ns3 {
namespace wsn {
namespace scenario5 {
namespace routing {

/**
 * Image pixel-region fragment.
 *
 * The source image (DEFAULT_IMAGE_WIDTH x DEFAULT_IMAGE_HEIGHT pixels) is
 * partitioned into N fragments using pixel-stride interleaving with stride N:
 * fragment i holds pixels at row-major positions i, i+N, i+2N, …
 * giving each fragment a spatially-uniform sample of the full image.
 *
 * Per-fragment detection probability:
 *   p_i = 1 - (1 - p_base)^(pixelCount_i / totalPixels)
 *
 * Combined detection confidence via union probability:
 *   p_union = 1 - prod(1 - p_i)  for all held fragments
 */
struct Fragment
{
    uint32_t fragmentId;       ///< Fragment ID — stride slot index (0-based)
    uint32_t strideOffset;     ///< Stride slot (= fragmentId for uniform N-way split)
    uint32_t pixelCount;       ///< Number of pixels in this region
    double   confidence;       ///< Per-fragment detection probability p_i in [0,1]
    std::vector<uint8_t> data; ///< Simulated pixel data (RGB, pixelCount * 3 bytes)
};

/**
 * Fragment collection for a node.
 */
struct FragmentCollection
{
    std::map<uint32_t, Fragment> fragments;
    double totalConfidence; ///< Combined detection confidence: p_union = 1 - prod(1-p_i)
    
    /**
     * Add or update fragment.
     */
    void AddFragment(const Fragment& frag);
    
    /**
     * Check if fragment exists.
     */
    bool HasFragment(uint32_t fragmentId) const;
    
    /**
     * Get fragment.
     */
    const Fragment* GetFragment(uint32_t fragmentId) const;
    
    /**
     * Update total confidence.
     */
    void UpdateTotalConfidence();
};

/**
 * Partition a detection image into pixel-region fragments via stride interleaving.
 *
 * The image (DEFAULT_IMAGE_WIDTH x DEFAULT_IMAGE_HEIGHT) is split into N fragments
 * using pixel-stride: fragment i holds pixels at row-major positions i, i+N, i+2N, …
 * Individual confidence: p_i = 1 - (1-p_base)^(pixels_i / totalPixels).
 * This ensures 1 - prod(1-p_i) = p_base when all N fragments are held.
 *
 * \param numFragments  Number of stride-interleaved pixel-region fragments
 * \return FragmentCollection with per-region pixel data and detection confidence
 */
FragmentCollection GenerateFragments(uint32_t numFragments);

} // namespace routing
} // namespace scenario5
} // namespace wsn
} // namespace ns3

#endif // SCENARIO5_FRAGMENT_H
