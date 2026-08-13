#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include <string>

// A modality channel under Singularity/Foreign that bridges external apps
// (like calendar apps or web browsers) into Earthcall.
// It follows the PhysicsLawBridge pattern: it is a first mover Law
// whose properties are governed by ordinary K4 Laws.
// The K4 Laws govern the channel via a stable identifier like `@foreign-channel.calendar.enabled`.
class ForeignChannel : public Law {
public:
    explicit ForeignChannel(const std::string& foreignAppName);

    const std::string& foreignAppName() const { return _appName; }

    // As a first mover, it's not serialized. Devices and external states persist as devices.
    bool isFirstMover() const override { return true; }

    // A STABLE identifier, not the generated law-<N>. First movers are excluded
    // from the save, so their generated ids differ every run — which would break
    // any law-text that addresses this channel by name
    // (`@foreign-channel.calendar.enabled := false`) on the next launch.
    // resolveBeingToken matches an `@token` against getIdentifier(), so the
    // addressing contract in the comment above is this override, not the name.
    // Same contract as Singularity::Physical::PhysicalChannel.
    std::string getIdentifier() const override { return "foreign-channel." + _appName; }

    // Ensures the channel is registered.
    static void syncRegister(LawManager& laws);

private:
    void buildProperties() override;

    bool propEnabled() const;
    void propSetEnabled(const bool& v);

    bool propConnected() const;

    float propRateLimit() const;
    void propSetRateLimit(const float& v);

    std::string _appName;
    
    bool _enabled;
    bool _connected;
    float _rateLimit;
};
