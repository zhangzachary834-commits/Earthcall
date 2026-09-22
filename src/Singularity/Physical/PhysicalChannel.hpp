#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

namespace Singularity {
namespace Physical {

// PhysicalChannel implements Sense and Act for the Modality::Physical channel.
// It is governed by properties and acts as a bridge between Earthcall and physical robots.
// Resolves targets by name, and acts as a first-mover Law bridge.
class PhysicalChannel : public Law {
public:
    PhysicalChannel();

    bool isFirstMover() const override { return true; }
    const std::string& name() const { return _name; }

    // A STABLE identifier, not the generated law-<N>. First movers are excluded
    // from the save, so their generated ids differ every run — which would break
    // any law-text that addresses this channel by name (`@physical-channel.enabled
    // := false`) on the next launch. The addressing contract requires a slug.
    // See NEW_KIND_FRAMEWORK.md §7b and FIRST_MOVER_AUTHORING.md §6b.
    std::string getIdentifier() const override { return _name; }

    static void syncRegister(LawManager& laws);

private:
    void buildProperties() override;

    bool propEnabled() const;
    void propSetEnabled(const bool& v);

    std::string _name;
    bool _enabled;
};

} // namespace Physical
} // namespace Singularity
