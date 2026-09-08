#pragma once

#include "json.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include <memory>

// A Zone identity record is a persistence root.  The historical `world` / `objects`
// envelope is retained here solely for dual-read compatibility.
nlohmann::json zoneObjectsToJson(const Zone& zone);
void zoneObjectsFromJson(const nlohmann::json& j, Zone& zone);
// Per-object, per-field JSON merge-patch of a World's authored objects onto
// a Zone just built from its identity store. Safe ONLY there — see the
// definition in ZoneSerialization.cpp for why it must not be called on a
// Zone that has been live for any length of time.
void mergeZoneObjectsFromJson(const nlohmann::json& j, Zone& zone);
nlohmann::json zoneToJson(const Zone& zone);
void applyZoneJson(Zone& zone, const nlohmann::json& j, bool replaceObjects);
std::shared_ptr<Zone> makeZoneFromJson(const nlohmann::json& j);
