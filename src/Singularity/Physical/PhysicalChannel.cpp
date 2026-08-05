#include "PhysicalChannel.hpp"

namespace Singularity {
namespace Physical {

PhysicalChannel::PhysicalChannel() : _name("physical-channel"), _enabled(false) {
}

bool PhysicalChannel::propEnabled() const {
    return _enabled;
}

void PhysicalChannel::propSetEnabled(const bool& v) {
    _enabled = v;
}

void PhysicalChannel::buildProperties() {
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<PhysicalChannel, bool>>(
        "enabled", this, &PhysicalChannel::propEnabled, &PhysicalChannel::propSetEnabled));
}

} // namespace Physical
} // namespace Singularity
