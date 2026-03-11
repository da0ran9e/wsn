#include "scenario5-visualizer.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/simulator.h"
#include "../../../model/routing/scenario5/ground-node-routing/ground-node-routing.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <vector>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Scenario5Visualizer");

namespace wsn {
namespace scenario5 {

namespace
{
std::string
JoinUintList(const std::vector<uint32_t>& values)
{
    if (values.empty())
    {
        return "-";
    }
    std::ostringstream oss;
    for (size_t i = 0; i < values.size(); ++i)
    {
        if (i > 0)
        {
            oss << ",";
        }
        oss << values[i];
    }
    return oss.str();
}

std::string
JoinIntList(const std::vector<int32_t>& values)
{
    if (values.empty())
    {
        return "-";
    }
    std::ostringstream oss;
    for (size_t i = 0; i < values.size(); ++i)
    {
        if (i > 0)
        {
            oss << ",";
        }
        oss << values[i];
    }
    return oss.str();
}
} // namespace

void
EnableScenario5Visualization()
{
    NS_LOG_INFO("Scenario5 visualization enabled (placeholder)");
}

void
DumpScenario5InitialNodesToFile(const NodeContainer& groundNodes,
                                const Ptr<Node>& bsNode,
                                const NodeContainer& uavNodes,
                                const Scenario5RunConfig& config)
{
    const std::string outPath =
        "/Users/mophan/Github/ns-3-dev-git-ns-3.46/src/wsn/examples/visualize/results/scenario5_nodes_init.txt";

    std::filesystem::create_directories(
        "/Users/mophan/Github/ns-3-dev-git-ns-3.46/src/wsn/examples/visualize/results");

    std::ofstream out(outPath, std::ios::out | std::ios::trunc);
    if (!out.is_open())
    {
        NS_LOG_ERROR("Cannot open visualize output file: " << outPath);
        return;
    }

    out << std::fixed << std::setprecision(3);

    out << "META scenario=scenario5\n";
    out << "META phase=post-initialize\n";
    out << "META gridSize=" << config.gridSize << "\n";
    out << "META gridSpacing=" << config.gridSpacing << "\n";
    out << "META numGroundNodes=" << groundNodes.GetN() << "\n";
    out << "META numUavs=" << uavNodes.GetN() << "\n";

    for (uint32_t i = 0; i < groundNodes.GetN(); ++i)
    {
        Ptr<Node> node = groundNodes.Get(i);
        Ptr<MobilityModel> mm = node->GetObject<MobilityModel>();
        if (!mm)
        {
            continue;
        }

        Vector p = mm->GetPosition();
        const int32_t rowCell = static_cast<int32_t>(i / config.gridSize);
        out << "NODE id=" << node->GetId() << " x=" << p.x << " y=" << p.y << " cell=" << rowCell
            << " leader=" << node->GetId() << " color=0\n";
    }

    if (bsNode)
    {
        Ptr<MobilityModel> bsMobility = bsNode->GetObject<MobilityModel>();
        if (bsMobility)
        {
            Vector p = bsMobility->GetPosition();
            out << "NODE id=" << bsNode->GetId() << " x=" << p.x << " y=" << p.y
                << " cell=-1 leader=" << bsNode->GetId() << " color=1\n";
        }
    }

    for (uint32_t i = 0; i < uavNodes.GetN(); ++i)
    {
        Ptr<Node> uav = uavNodes.Get(i);
        Ptr<MobilityModel> uavMobility = uav->GetObject<MobilityModel>();
        if (!uavMobility)
        {
            continue;
        }

        Vector p = uavMobility->GetPosition();
        out << "NODE id=" << uav->GetId() << " x=" << p.x << " y=" << p.y
            << " cell=-2 leader=" << uav->GetId() << " color=2\n";
    }

    out.flush();
    out.close();

    NS_LOG_INFO("Scenario5 initial node info saved: " << outPath);
}

void
DumpScenario5CellFormationSnapshot()
{
    const std::string outPath =
        "/Users/mophan/Github/ns-3-dev-git-ns-3.46/src/wsn/examples/visualize/results/scenario5_cell_formation.txt";

    std::filesystem::create_directories(
        "/Users/mophan/Github/ns-3-dev-git-ns-3.46/src/wsn/examples/visualize/results");

    std::ofstream out(outPath, std::ios::out | std::ios::trunc);
    if (!out.is_open())
    {
        NS_LOG_ERROR("Cannot open visualize output file: " << outPath);
        return;
    }

    std::map<int32_t, std::vector<uint32_t>> cellMembers;
    for (const auto& [nodeId, state] : routing::g_groundNetworkPerNode)
    {
        cellMembers[state.cellId].push_back(nodeId);
    }

    for (auto& [cellId, members] : cellMembers)
    {
        std::sort(members.begin(), members.end());
    }

    std::vector<int32_t> formedCells;
    formedCells.reserve(cellMembers.size());
    for (const auto& [cellId, _members] : cellMembers)
    {
        formedCells.push_back(cellId);
    }

    out << "META scenario=scenario5\n";
    out << "META phase=cell-formation\n";
    out << "META source=ground-node-routing\n";
    out << "META totalNodes=" << routing::g_groundNetworkPerNode.size() << "\n";

    out << "FORMEVENT t=" << Simulator::Now().GetSeconds() << " stage=startup-discovery"
        << " formedCells=" << JoinIntList(formedCells) << " totalCells=" << cellMembers.size()
        << " totalNodes=" << routing::g_groundNetworkPerNode.size() << "\n";

    out << "FORMSNAPSHOT t=" << Simulator::Now().GetSeconds() << " stage=stable\n";

    for (const auto& [cellId, members] : cellMembers)
    {
        uint32_t leader = members.empty() ? 0u : members.front();
        uint32_t color = static_cast<uint32_t>((cellId % 3 + 3) % 3);

        out << "CELL id=" << cellId << " leader=" << leader << " size=" << members.size()
            << " members=" << JoinUintList(members) << " color=" << color << "\n";
    }

    out << "ENDFORMSNAPSHOT\n";
    out.flush();
    out.close();

    NS_LOG_INFO("Scenario5 cell formation snapshot saved: " << outPath);
}

} // namespace scenario5
} // namespace wsn
} // namespace ns3
