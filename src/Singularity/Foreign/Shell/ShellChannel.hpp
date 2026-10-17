#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

namespace Singularity {
namespace Foreign {
namespace Shell {

// True only when the Person launching Earthcall set
// EARTHCALL_ALLOW_PROCESS_CHANNELS=1. Shared by every channel that can launch
// a process (ShellChannel, StreamChannel's "process" pipes). Interim floor;
// see ShellChannel.cpp.
bool processChannelsAllowedByPerson();

class ShellChannel : public Law {
public:
    ShellChannel();
    ~ShellChannel() override = default;

    bool isFirstMover() const override { return true; }

    static void syncRegister(LawManager& laws);

private:
    void buildProperties() override;

    std::string propCommand() const { return _command; }
    void propSetCommand(const std::string& v) { _command = v; }

    double propExecuteRequests() const { return _executeRequests; }
    void propSetExecuteRequests(const double& v);

    std::string propStdout() const { return _stdout; }
    std::string propStderr() const { return _stderr; }
    double propExitCode() const { return _exitCode; }

    std::string _command;
    double _executeRequests = 0.0;
    std::string _stdout;
    std::string _stderr;
    double _exitCode = 0.0;
};

} // namespace Shell
} // namespace Foreign
} // namespace Singularity
