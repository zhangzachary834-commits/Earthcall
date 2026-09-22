#pragma once

#include "json.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include <memory>

// The single, shared resolution of a Zone JSON record's STABLE identifier —
// "identifier" field preferred, falling back to "name" only when a record
// predates the split (Zach, 2026-09-09: "Zone should absolutely have a real
// identifier/name split. It's a Singular."). makeZoneFromJson and every
// admission/dedup check in ZoneManager.cpp must resolve identity through
// this ONE function — a second, independently-written copy of this
// priority order is exactly how it drifted out of sync before (see
// Zone_identity_store_field_level_merge.md's "Invariant 6, made concrete"
// section: makeZoneFromJson preferred "name" while this function preferred
// "identifier", so a record whose two fields differed could never be
// recognized as already-live, minting an unbounded number of duplicate
// live Zone objects that all shared the same eventual getIdentifier()).
std::string zoneIdFromJson(const nlohmann::json& zj);

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
