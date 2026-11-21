#pragma once

#include "json.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"

// Relation/Formation links are a second hydration phase.  Endpoints are resolved
// against already-created Singulars and never treated as Zone-owned children.
// Intern references before decoding graphs. Ordinary admission hydrates only
// newly created canonical Lexemes; an explicit snapshot restore may rewind them.
void hydrateZoneLexemes(Zone& zone, const nlohmann::json& j, bool restoreExisting = false);
void applyFormationRelations(Zone& zone, const nlohmann::json& j, bool restoreExistingLexemes = false);
