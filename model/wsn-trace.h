#ifndef WSN_TRACE_H
#define WSN_TRACE_H

#pragma once
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include "ns3/simulator.h"

namespace ns3 {
namespace wsn {

class WsnTrace
{
public:
    WsnTrace() = default;
    ~WsnTrace();
    
    bool Open(const std::string &path);
    void Trace(const std::string &msg);
    bool IsOpen() const { return m_ofs.is_open(); }

private:
    std::ofstream m_ofs;
    std::string   m_path;
};

} // namespace wsn
} // namespace ns3

#endif // WSN_TRACE_H