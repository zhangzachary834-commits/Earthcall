#pragma once

#include "json.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"

// Relation/Formation links are a second hydration phase.  Endpoints are resolved
// against already-created Singulars and never treated as Zone-owned children.
void applyFormationRelations(Zone& zone, const nlohmann::json& j);
