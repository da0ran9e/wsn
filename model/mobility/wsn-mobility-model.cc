#include "wsn-mobility-model.h"

namespace ns3 {
namespace wsn {

NS_OBJECT_ENSURE_REGISTERED (WsnMobilityModel);

TypeId
WsnMobilityModel::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::wsn::WsnMobilityModel")
        .SetParent<ConstantPositionMobilityModel> ()
        .SetGroupName ("Wsn")
        .AddConstructor<WsnMobilityModel> ();

    return tid;
}

WsnMobilityModel::WsnMobilityModel ()
{
    // currently identical to ConstantPositionMobilityModel
}

WsnMobilityModel::~WsnMobilityModel () = default;

} // namespace wsn
} // namespace ns3
