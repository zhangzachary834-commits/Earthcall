#include "ForeignChannel.hpp"
#include "Property.hpp"

ForeignChannel::ForeignChannel(const std::string& foreignAppName)
    : Law(foreignAppName, "foreign-channel." + foreignAppName), 
      _appName(foreignAppName),
      _enabled(true),
      _connected(false),
      _rateLimit(1.0f) {
    buildProperties();
}

void ForeignChannel::buildProperties() {
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<ForeignChannel, bool>>(
        "enabled", this, &ForeignChannel::propEnabled, &ForeignChannel::propSetEnabled));
    
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<ForeignChannel, bool>>(
        "connected", this, &ForeignChannel::propConnected, nullptr));

    _propertyRegistry.push_back(std::make_unique<ComputedProperty<ForeignChannel, float>>(
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
