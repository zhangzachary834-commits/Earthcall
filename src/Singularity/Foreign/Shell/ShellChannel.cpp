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

void ShellChannel::propSetExecuteRequests(const double& v) {
    if (v > _executeRequests) {
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
