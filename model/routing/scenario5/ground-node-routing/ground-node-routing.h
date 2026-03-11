/*
 * Scenario 4 - Ground Node Routing
 */

#ifndef SCENARIO5_GROUND_NODE_ROUTING_H
#define SCENARIO5_GROUND_NODE_ROUTING_H

#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/packet.h"
#include "ns3/net-device.h"
#include "ns3/vector.h"
#include "../fragment.h"
#include "../base-station-node/base-station-node.h"
#include <map>
#include <set>

namespace ns3 {
namespace wsn {
namespace scenario5 {
namespace routing {

/**
 * Ground node network state.
 * Thiết kế đầy đủ cho vòng đời: Startup → Cell Formation → Fragment Collection → Cooperation
 */
enum class GroundNodeLifecyclePhase
{
    BOOTSTRAP = 0,
    DISCOVERY,
    ACTIVE,
    DEGRADED,
    DEAD
};

struct GroundNetworkState
{
    // === Node Identity ===
    uint32_t nodeId;                          // Node ID trong hệ thống
    int32_t cellId;                           // Cell ID dựa trên vị trí địa lý
    bool isCellLeader;                        // Node này có phải cell leader không
    Vector position;                          // Vị trí hiện tại của node
    uint32_t cellColor;                       // Màu cell (phục vụ visualize)
    bool isTimeSynchronized;                  // Đã đồng bộ thời gian chưa
    double clockOffsetSec;                    // Sai lệch clock với global time (giây)
    double lastSyncTime;                      // Timestamp lần sync gần nhất
    
    // === Neighbor Discovery (Startup Phase) ===
    std::set<uint32_t> neighbors;             // Danh sách neighbor nodes (in range)
    std::set<uint32_t> twoHopNeighbors;       // Danh sách neighbor 2-hop
    std::map<uint32_t, double> neighborRssi;  // RSSI từ mỗi neighbor (dBm)
    std::map<uint32_t, double> neighborDistance; // Khoảng cách đến neighbor (m)
    bool startupComplete;                     // Đã hoàn thành startup discovery chưa
    
    // === Fragment Management ===
    FragmentCollection fragments;             // Collection của các fragment node đang giữ
    double confidence;                        // Tổng confidence = fragments.totalConfidence
    uint32_t expectedFragmentCount;           // Tổng số fragment kỳ vọng trong phiên
    double fragmentCoverageRatio;             // Tỉ lệ fragment hiện có / kỳ vọng
    std::map<uint32_t, double> fragmentLastUpdateTime; // Lần cập nhật cuối của từng fragment
    uint32_t fragmentsReceivedFromUav;        // Số fragment nhận từ UAV
    uint32_t fragmentsReceivedFromPeers;      // Số fragment nhận từ cell peers (đánh giá độ hiệu quả khi có cell cooperation)
    uint32_t duplicateFragmentsDiscarded;     // Fragment trùng/không tốt hơn bị loại
    
    // === Cell Cooperation ===
    std::set<uint32_t> cellPeers;             // Danh sách nodes cùng cell
    std::map<uint32_t, double> peerConfidence; // Confidence của từng peer (shared info)
    uint32_t cooperationRequestsSent;         // Số lần request sharing fragment
    uint32_t cooperationRequestsReceived;     // Số lần nhận request từ peer
    bool cooperationEnabled;                  // Node có tham gia cooperation không
    double lastCooperationTime;              // Timestamp cooperation gần nhất
    bool cooperationTimeoutScheduled;         // Đã schedule cooperation timeout chưa
    double cooperationTimeoutTime;            // Thời điểm timeout sẽ trigger
    
    // === Communication Statistics ===
    uint32_t packetCount;                     // Tổng số packet nhận được
    uint32_t startupPacketsReceived;          // Packet nhận trong startup phase
    uint32_t fragmentPacketsReceived;         // Packet fragment từ UAV
    uint32_t cooperationPacketsReceived;      // Packet cooperation từ peers
    uint32_t packetsSent;                     // Tổng số packet đã gửi
    double totalBytesReceived;                // Tổng bytes nhận
    double totalBytesSent;                    // Tổng bytes gửi
    double lastPacketRssiDbm;                 // RSSI gói gần nhất
    double avgPacketRssiDbm;                  // RSSI trung bình lũy tiến
    uint32_t rssiSampleCount;                 // Số mẫu RSSI đã ghi nhận
    
    // === UAV Interaction ===
    std::set<uint32_t> uavsInRange;           // Danh sách UAV đang trong tầm
    std::map<uint32_t, double> uavRssi;       // RSSI từ mỗi UAV
    double lastUavContactTime;                // Timestamp lần cuối liên lạc UAV
    uint32_t uavEncounters;                   // Số lần gặp UAV
    
    // === Topology Reporting ===
    double lastTopologyReportTime;            // Timestamp lần cuối báo cáo topology
    uint32_t topologyReportCount;             // Số lần đã gửi topology lên BS
    
    // === Energy & Resource (dự phòng cho mở rộng) ===
    double remainingEnergy;                   // Năng lượng còn lại (joules)
    double energyConsumedTx;                  // Năng lượng tiêu thụ truyền
    double energyConsumedRx;                  // Năng lượng tiêu thụ nhận

    // === Lifecycle ===
    GroundNodeLifecyclePhase lifecyclePhase;  // Trạng thái vòng đời node
    bool isIsolated;                          // Node cô lập (không có neighbor)
    double initializationTime;                // Thời điểm khởi tạo
    double lastActivityTime;                  // Hoạt động gần nhất
};

// Global storage for ground node states
extern std::map<uint32_t, GroundNetworkState> g_groundNetworkPerNode;

// Shared topology cache (pull-based access by BS)
extern GlobalTopology g_latestTopologySnapshot;
extern bool g_hasLatestTopologySnapshot;

/**
 * Initialize ground node routing for all nodes.
 * 
 * \param nodes Ground node container
 * \param numFragments Number of fragments to generate
 */
void InitializeGroundNodeRouting(NodeContainer nodes, uint32_t numFragments);

/**
 * Handle packet reception on ground node.
 * 
 * \param nodeId Receiving node ID
 * \param packet Received packet
 * \param rssiDbm Signal strength
 */
void OnGroundNodeReceivePacket(uint32_t nodeId, Ptr<const Packet> packet, double rssiDbm);

/**
 * Build topology snapshot from ground network.
 * 
 * \return Global topology
 */
GlobalTopology BuildTopologySnapshot();

/**
 * Send topology snapshot to base station.
 */
void SendTopologyToBS();

/**
 * Get pointer to latest cached topology snapshot.
 * Returns nullptr if no snapshot has been cached yet.
 */
const GlobalTopology* GetLatestTopologySnapshotPtr();

} // namespace routing
} // namespace scenario5
} // namespace wsn
} // namespace ns3

#endif // SCENARIO5_GROUND_NODE_ROUTING_H
