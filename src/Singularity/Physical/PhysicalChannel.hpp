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

private:
    void buildProperties() override;

    bool propEnabled() const;
    void propSetEnabled(const bool& v);

    std::string _name;
    bool _enabled;
};

} // namespace Physical
} // namespace Singularity
