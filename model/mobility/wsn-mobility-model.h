#ifndef WSN_MOBILITY_MODEL_H
#define WSN_MOBILITY_MODEL_H

#include "ns3/constant-position-mobility-model.h"
#include "ns3/type-id.h"
#include "ns3/vector.h"

namespace ns3 {
namespace wsn {

class WsnMobilityModel : public ConstantPositionMobilityModel
{
public:
    static TypeId GetTypeId (void);

    WsnMobilityModel ();
    ~WsnMobilityModel () override;

    // Future extension:
    // virtual Vector DoGetPosition () const override;
    // virtual void DoSetPosition (const Vector &position) override;
    // virtual Vector DoGetVelocity () const override;
};

} // namespace wsn
} // namespace ns3

#endif // WSN_MOBILITY_MODEL_H