#include "wsn-trace.h"
#include <iostream>

namespace ns3 {
namespace wsn {

WsnTrace::~WsnTrace()
{
    if (m_ofs.is_open())
        m_ofs.close();
}

bool WsnTrace::Open(const std::string &path)
{
    m_path = path;
    m_ofs.open(path, std::ios::out | std::ios::app);   // append mode

    if (!m_ofs.is_open())
    {
        std::cerr << "[WsnTrace] Could not open file: " << path << std::endl;
        return false;
    }
    m_ofs << "==== WSN Trace Start ====\n";
    return true;
}

void WsnTrace::Trace(const std::string &msg)
{
    if (!m_ofs.is_open())
    {
        std::cerr << "[WsnTrace] Trace called but file not opened!\n";
        return;
    }

    m_ofs << msg << "\n";
}

} // namespace wsn
} // namespace ns3
