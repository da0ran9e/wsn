#include "wireless-channel.h"
namespace ns3 {
namespace wsn {

bool WirelessChannel::SetProperty(const std::string &key, const std::string &value)
{
    if (key == "collectTraceInfo") {
        collectTraceInfo = (value == "true" || value == "1");
    }
    else if (key == "onlyStaticNodes") {
        onlyStaticNodes = (value == "true" || value == "1");
    }
    else if (key == "xCellSize") {
        xCellSize = std::stoi(value);
    }
    else if (key == "yCellSize") {
        yCellSize = std::stoi(value);
    }
    else if (key == "zCellSize") {
        zCellSize = std::stoi(value);
    }
    else if (key == "pathLossExponent") {
        pathLossExponent = std::stod(value);
    }
    else if (key == "PLd0") {
        PLd0 = std::stod(value);
    }
    else if (key == "d0") {
        d0 = std::stod(value);
    }
    else if (key == "sigma") {
        sigma = std::stod(value);
    }
    else if (key == "bidirectionalSigma") {
        bidirectionalSigma = std::stod(value);
    }
    else if (key == "pathLossMapFile") {
        pathLossMapFile = value;
    }
    else if (key == "temporalModelParametersFile") {
        temporalModelParametersFile = value;
    }
    else if (key == "signalDeliveryThreshold") {
        signalDeliveryThreshold = std::stod(value);
    }
    else {
        return false;
    }

    NotifyAttributeChanged(key, value);
    return true;
}

void WirelessChannel::Build(BuildContext& ctx)
{
    if(m_built) {
        return;
    }
    m_built = true;

    std::cout << "Building Wireless Channel: " << GetInstanceName() << std::endl;
    // Implementation of the Build method
    // WsnObject::Build(ctx);
    ctx.spectrumChannel = ns3::CreateObject<ns3::SingleModelSpectrumChannel>();
    ctx.lossModel = ns3::CreateObject<ns3::LogDistancePropagationLossModel>();
    ctx.delayModel = ns3::CreateObject<ns3::ConstantSpeedPropagationDelayModel>();

    ctx.spectrumChannel->AddPropagationLossModel(ctx.lossModel);
    ctx.spectrumChannel->SetPropagationDelayModel(ctx.delayModel);
}

} // namespace wsn
} // namespace ns3