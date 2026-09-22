#include "ForeignChannel.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"

// buildProperties is NOT called here: Singular builds the registry lazily
// behind _propertiesBuilt, so calling it from the constructor registers every
// property twice. PhysicsLawBridge and PhysicalChannel both leave it to the
// lazy path.
ForeignChannel::ForeignChannel(const std::string& foreignAppName)
    : Law("foreign: " + foreignAppName),
      _appName(foreignAppName),
      _enabled(true),
      _connected(false),
      _rateLimit(1.0f) {}

void ForeignChannel::buildProperties() {
    registerProperty(std::make_unique<ComputedProperty<ForeignChannel, bool>>(
        "enabled", this, &ForeignChannel::propEnabled, &ForeignChannel::propSetEnabled));
    
    registerProperty(std::make_unique<ComputedProperty<ForeignChannel, bool>>(
        "connected", this, &ForeignChannel::propConnected, nullptr));

    registerProperty(std::make_unique<ComputedProperty<ForeignChannel, float>>(
        "rate_limit", this, &ForeignChannel::propRateLimit, &ForeignChannel::propSetRateLimit));
}

bool ForeignChannel::propEnabled() const {
    return _enabled;
}

void ForeignChannel::propSetEnabled(const bool& v) {
    _enabled = v;
}

bool ForeignChannel::propConnected() const {
    // In a real implementation, this checks the actual external app's API state
    return _connected;
}

float ForeignChannel::propRateLimit() const {
    return _rateLimit;
}

void ForeignChannel::propSetRateLimit(const float& v) {
    _rateLimit = v;
}

void ForeignChannel::syncRegister(LawManager& laws) {
    // Scaffold for registering the bridge into the LawManager
    // e.g. finding connected external apps and spawning a ForeignChannel for them
}
