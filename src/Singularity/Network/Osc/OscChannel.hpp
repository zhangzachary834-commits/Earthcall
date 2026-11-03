#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

namespace Singularity {
namespace Network {
namespace Osc {

class OscChannel : public Law {
public:
    OscChannel();
    ~OscChannel() override = default;

    bool isFirstMover() const override { return true; }

    static void syncRegister(LawManager& laws);

private:
    void buildProperties() override;

    std::string propHost() const { return _host; }
    void propSetHost(const std::string& v) { _host = v; }

    double propPort() const { return _port; }
    void propSetPort(const double& v) { _port = v; }

    std::string propAddress() const { return _address; }
    void propSetAddress(const std::string& v) { _address = v; }

    double propFloatArg() const { return _floatArg; }
    void propSetFloatArg(const double& v) { _floatArg = v; }

    double propSendRequests() const { return _sendRequests; }
    void propSetSendRequests(const double& v);

    std::string _host = "127.0.0.1";
    double _port = 8000.0;
    std::string _address = "/earthcall";
    double _floatArg = 0.0;
    double _sendRequests = 0.0;
};

} // namespace Osc
} // namespace Network
} // namespace Singularity
