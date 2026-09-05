#pragma once

#include "json.hpp"
#include "ZonesOfEarth/Ourverse/Ourverse.hpp"
#include <functional>
#include <memory>

// Ourverse is a Singular root.  Its record names the gathering Zone and the
// three Formation identities; it does not inline Zones or Relations.
nlohmann::json ourverseToJson(const Ourverse& ourverse);

using OurverseMemberResolver = Formation::MemberResolver;
using OurverseZoneResolver = std::function<std::shared_ptr<Zone>(const std::string&)>;

// Rebind an Ourverse root only after its Zone and other Singular roots exist.
// Missing endpoints are reported by Formation::fromJson and never invented.
bool ourverseFromJson(Ourverse& ourverse,
                      const nlohmann::json& json,
                      const OurverseZoneResolver& resolveZone,
                      const OurverseMemberResolver& resolveMember);
