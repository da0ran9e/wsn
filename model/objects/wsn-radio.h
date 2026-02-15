#ifndef WSN_RADIO_H
#define WSN_RADIO_H

#include"wsn-object.h"
#include"ns3/ptr.h"

namespace ns3 {
namespace wsn {

class Radio : public ns3::wsn::WsnObject
{
public:
    explicit Radio(const std::string& name) : WsnObject("Radio", name),
        radioTxPowerDbm(0.0),
        radioRxSensitivityDbm(0.0),
        radioChannelBandwidthKbps(250.0),
        radioHeaderOverhead(11)
    {
    }
    ~Radio() override = default; 

    bool SetProperty(const std::string &key, const std::string &value) override;
    void Build(BuildContext& ctx) override;
private:
    double radioTxPowerDbm; // default (0.0);
    double radioRxSensitivityDbm; // default (0.0);
    double radioChannelBandwidthKbps; // default (250.0);
    int radioHeaderOverhead; // default (11);

    ns3::Ptr<ns3::NetDevice> m_selfDetDevice;
};

} // namespace wsn
} // namespace ns3

#endif