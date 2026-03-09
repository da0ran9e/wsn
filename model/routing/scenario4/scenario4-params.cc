/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/wsn-module.h"

namespace ns3
{
namespace wsn
{
namespace scenario4
{
namespace params
{

// Global result file stream definition
// This is managed by example4.cc - it opens and closes the stream
// Other files can write directly: if (g_resultFileStream) *g_resultFileStream << "content";
std::ofstream* g_resultFileStream = nullptr;

} // namespace params
} // namespace scenario4
} // namespace wsn
} // namespace ns3
