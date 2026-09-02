#include "PhysicalChannel.hpp"

#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

namespace Singularity {
namespace Physical {

PhysicalChannel::PhysicalChannel() : Law("physical-channel"), _name("physical-channel"), _enabled(false) {
}

bool PhysicalChannel::propEnabled() const {
    return _enabled;
}

void PhysicalChannel::propSetEnabled(const bool& v) {
    _enabled = v;
}

void PhysicalChannel::buildProperties() {
    registerProperty(std::make_unique<ComputedProperty<PhysicalChannel, bool>>(
        "enabled", this, &PhysicalChannel::propEnabled, &PhysicalChannel::propSetEnabled));
}

void PhysicalChannel::syncRegister(LawManager& laws) {
    bool bridged = false;
    for (const auto& law : laws.getAll()) {
        auto* bridge = dynamic_cast<PhysicalChannel*>(law.get());
        if (bridge && bridge->name() == "physical-channel") {
            bridged = true;
            break;
        }
    }
    if (!bridged) {
        laws.add(std::make_shared<PhysicalChannel>());
    }
}

} // namespace Physical
} // namespace Singularity
