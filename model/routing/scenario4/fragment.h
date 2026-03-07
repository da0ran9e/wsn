/*
 * Scenario 4 - Fragment Data Structure
 */

#ifndef SCENARIO4_FRAGMENT_H
#define SCENARIO4_FRAGMENT_H

#include <cstdint>
#include <map>
#include <vector>

namespace ns3 {
namespace wsn {
namespace scenario4 {
namespace routing {

/**
 * File fragment with metadata.
 */
struct Fragment
{
    uint32_t fragmentId;      ///< Unique fragment ID
    double confidence;        ///< Confidence level [0, 1]
    uint32_t size;           ///< Fragment size in bytes
    std::vector<uint8_t> data; ///< Fragment data (placeholder)
};

/**
 * Fragment collection for a node.
 */
struct FragmentCollection
{
    std::map<uint32_t, Fragment> fragments;
    double totalConfidence;
    
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
 * Generate fragments with distributed confidence.
 * 
 * \param numFragments Number of fragments to generate
 * \return Fragment collection
 */
FragmentCollection GenerateFragments(uint32_t numFragments);

} // namespace routing
} // namespace scenario4
} // namespace wsn
} // namespace ns3

#endif // SCENARIO4_FRAGMENT_H
