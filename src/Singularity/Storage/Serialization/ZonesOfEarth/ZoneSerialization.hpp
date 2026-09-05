#pragma once

#include "json.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include <memory>

// A Zone identity record is a persistence root.  The historical `world` / `objects`
// envelope is retained here solely for dual-read compatibility.
nlohmann::json zoneObjectsToJson(const Zone& zone);
void zoneObjectsFromJson(const nlohmann::json& j, Zone& zone);
nlohmann::json zoneToJson(const Zone& zone);
void applyZoneJson(Zone& zone, const nlohmann::json& j, bool replaceObjects);
std::shared_ptr<Zone> makeZoneFromJson(const nlohmann::json& j);
