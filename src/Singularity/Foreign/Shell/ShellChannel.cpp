#include "ShellChannel.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include <cstdlib>
#include <cstdio>
#include <array>
#include <iostream>

namespace Singularity {
namespace Foreign {
namespace Shell {

ShellChannel::ShellChannel() : Law("shell-channel") {}

void ShellChannel::syncRegister(LawManager& laws) {
    if (!laws.find("shell-channel")) {
        laws.add(std::make_shared<ShellChannel>());
    }
}

void ShellChannel::buildProperties() {
    registerProperty(std::make_unique<ComputedProperty<ShellChannel, std::string>>(
        "command", this, &ShellChannel::propCommand, &ShellChannel::propSetCommand));
    registerProperty(std::make_unique<ComputedProperty<ShellChannel, double>>(
        "executeRequests", this, &ShellChannel::propExecuteRequests, &ShellChannel::propSetExecuteRequests));
    registerProperty(std::make_unique<ComputedProperty<ShellChannel, std::string>>(
        "stdout", this, &ShellChannel::propStdout));
    registerProperty(std::make_unique<ComputedProperty<ShellChannel, std::string>>(
        "stderr", this, &ShellChannel::propStderr));
    registerProperty(std::make_unique<ComputedProperty<ShellChannel, double>>(
        "exitCode", this, &ShellChannel::propExitCode));
}

// INTERIM KERNEL FLOOR (FULL_SYSTEM_AUDIT_2026-10-16 F1). Any firing Law can
// write `command` and raise `executeRequests`, including a Law that arrived
// inside someone else's Zone, and no authority office yet decides who may
// launch a process. Until Zach answers that ⚑ (task: Channels and Language/
// Process_Launching_Channels_Authority), launching refuses — loudly, into
// this channel's own state — unless the Person starting Earthcall set
// EARTHCALL_ALLOW_PROCESS_CHANNELS=1. Like the body guards, this acts in C++
// beneath Law; no authored policy can open it. It is not a permission
// system: the real gate belongs to Foreign::authorizeForeignActuation.
bool processChannelsAllowedByPerson() {
    const char* allow = std::getenv("EARTHCALL_ALLOW_PROCESS_CHANNELS");
    return allow && std::string(allow) == "1";
}

void ShellChannel::propSetExecuteRequests(const double& v) {
    if (v > _executeRequests && !processChannelsAllowedByPerson()) {
        _stdout.clear();
        _stderr = "refused: process launch is closed until a Person grants it "
                  "(set EARTHCALL_ALLOW_PROCESS_CHANNELS=1 when starting Earthcall)";
        _exitCode = -1.0;
        std::cerr << "[ShellChannel] " << _stderr << "\n";
    } else if (v > _executeRequests) {
        _stdout.clear();
        _stderr.clear();

        FILE* pipe = popen((_command + " 2>&1").c_str(), "r");
        if (!pipe) {
            _stderr = "popen failed";
            _exitCode = -1.0;
        } else {
            std::array<char, 128> buffer;
            while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
                _stdout += buffer.data();
            }
            int returnCode = pclose(pipe);
            _exitCode = static_cast<double>(WEXITSTATUS(returnCode));
        }
    }
    _executeRequests = v;
}

} // namespace Shell
} // namespace Foreign
} // namespace Singularity
