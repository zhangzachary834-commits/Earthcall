#include "Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.hpp"
#include "Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.hpp"
#include "Singularity/Storage/Serialization/Relation/FormationSerialization.hpp"
#include "Singularity/Storage/Serialization/ZonesOfEarth/HomeSerialization.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "ZonesOfEarth/HomesOfEarth/Home.hpp"
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

extern MaterialManager materials;

std::string zoneIdFromJson(const nlohmann::json& zj) {
    return zj.value("identifier", zj.value("name", std::string{}));
}

// ------------------------------------------------------------------
// Zone object bag — on disk still `{"objects":[...]}` (the old World shape).
// ------------------------------------------------------------------
nlohmann::json zoneObjectsToJson(const Zone& zone) {
    nlohmann::json j = nlohmann::json{};
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& ptr : zone.objects()) {
        if (ptr) arr.push_back(*ptr);
    }
    j["objects"] = arr;
    return j;
}

void zoneObjectsFromJson(const nlohmann::json& j, Zone& zone) {
    const nlohmann::json* arr = nullptr;
    if (j.contains("objects") && j["objects"].is_array()) arr = &j["objects"];
    else if (j.is_array()) arr = &j;
    if (!arr) return;
    for (const auto& oj : *arr) {
        std::string id;
        if (oj.contains("objectID") && oj["objectID"].is_string()) {
            id = oj["objectID"].get<std::string>();
        } else if (oj.contains("id") && oj["id"].is_string()) {
            id = oj["id"].get<std::string>();
        }
        std::shared_ptr<Object> obj = std::make_shared<Object>(id);
        from_json(oj, *obj);
        zone.addObject(std::move(obj));
    }

    // Re-link composition once every object exists. Elements are remembered by
    // identifier, so this pass is order-independent; an element that is not in
    // the Zone is simply not re-linked (composition is a covenant between
    // beings that are present, never a pointer to something absent).
    auto& owned = zone.getOwnedObjectsMutable();
    for (auto& holder : owned) {
        if (!holder || holder->getPendingElementIds().empty()) continue;
        for (const auto& id : holder->getPendingElementIds()) {
            for (auto& candidate : owned) {
                if (candidate && candidate.get() != holder.get() &&
                    candidate->getIdentifier() == id) {
                    holder->addElement(candidate.get());
                    break;
                }
            }
        }
        holder->getPendingElementIds().clear();
    }
}

namespace {
Zone::Scope scopeFromName(const std::string& name) {
    if (name == "Global") return Zone::Scope::Global;
    if (name == "World") return Zone::Scope::World;
    if (name == "Regional") return Zone::Scope::Regional;
    if (name == "UI") return Zone::Scope::UI;
    return Zone::Scope::Local;
}

} // namespace

// A Zone with a per-identity store (saves/zones/<id>/) is built from the
// store first, so a Person's own in-place edits — a moved position, a
// painted texture — survive re-loading an older World snapshot. That is
// the point of the store; see the comment at its call site in
// ZoneManager.cpp. But the store is a snapshot taken at some past moment,
// not a diff — a field the World authors LATER, which the store never
// captured, used to be silently dropped in favour of the raw C++ default
// rather than the World's authored value, because objects only ever came
// from ONE side or the other, never both.
//
// This merges per object, per field, by RFC 7386 JSON merge-patch
// (nlohmann::json::merge_patch): the World's authored JSON is the base,
// the store's object overlays it, and only fields the overlay actually
// specifies win. A field absent from the overlay — because it postdates
// the snapshot — falls through to the World's authored value instead of
// vanishing to a hardcoded default. An object the World added after the
// snapshot (no id in the store at all) is admitted fresh rather than
// dropped, since there is no "unsaved work" to protect here — this is the
// Zone's first appearance this run, its objects fresh out of
// makeZoneFromJson a moment ago, not yet ticked by any Law.
//
// ONLY safe here for exactly that reason. Deliberately NOT called from
// ZoneManager's findLive branch, and not safe to call there: a Zone kept
// LIVE from the running session may already carry Rete facts, selection
// state, and other runtime history that from_json (called again here on
// an "existing" match) knows nothing about and will silently drop —
// chess_click_geometry_test caught exactly this when it was tried
// (2026-09-07): every piece stopped registering isSelected after a click.
// unsaved_preserve_test also requires another file's snapshot of the same
// live Zone to introduce nothing of its own, which this function's
// new-object admission would violate there too. See faceColors' own fix
// in ObjectSerialization.cpp's to_json for the findLive-reachable case
// instead: making the field round-trip through every save means the store
// itself is correct from its first write, so no runtime merge is needed
// once a Zone is already live.
void mergeZoneObjectsFromJson(const nlohmann::json& j, Zone& zone) {
    const nlohmann::json* arr = nullptr;
    if (j.contains("objects") && j["objects"].is_array()) arr = &j["objects"];
    else if (j.is_array()) arr = &j;
    if (!arr) return;
    const nlohmann::json& worldObjects = *arr;
    auto& owned = zone.getOwnedObjectsMutable();
    for (const auto& oj : worldObjects) {
        std::string id;
        if (oj.contains("objectID") && oj["objectID"].is_string()) {
            id = oj["objectID"].get<std::string>();
        } else if (oj.contains("id") && oj["id"].is_string()) {
            id = oj["id"].get<std::string>();
        }
        if (id.empty()) continue;

        std::shared_ptr<Object> existing;
        for (auto& holder : owned) {
            if (holder && holder->getIdentifier() == id) { existing = holder; break; }
        }

        if (existing) {
            nlohmann::json merged = oj;
            merged.merge_patch(nlohmann::json(*existing));
            from_json(merged, *existing);
        } else {
            std::shared_ptr<Object> obj = std::make_shared<Object>(id);
            from_json(oj, *obj);
            zone.addObject(std::move(obj));
        }
    }

    // Re-link composition for anything newly admitted above — mirrors the
    // pass at the end of zoneObjectsFromJson.
    for (auto& holder : owned) {
        if (!holder || holder->getPendingElementIds().empty()) continue;
        for (const auto& elementId : holder->getPendingElementIds()) {
            for (auto& candidate : owned) {
                if (candidate && candidate.get() != holder.get() &&
                    candidate->getIdentifier() == elementId) {
                    holder->addElement(candidate.get());
                    break;
                }
            }
        }
        holder->getPendingElementIds().clear();
    }
}

nlohmann::json zoneToJson(const Zone& zone) {
    nlohmann::json zj;
    zj["name"] = zone.name();
    zj["identifier"] = zone.getIdentifier();
    zj["owner"] = zone.owner();
    zj["parentZone"] = zone.getParentZone();
    zj["scope"] = zone.scopeName();
    nlohmann::json qualities = nlohmann::json::object();
    for (const auto& kv : zone.getQualities()) {
        qualities[kv.first] = kv.second;
    }
    zj["qualities"] = qualities;
    nlohmann::json del = nlohmann::json::object();
    for (const auto& kv : zone.getDeletability()) {
        del[kv.first] = kv.second;
    }
    zj["deletable"] = del;
    zj["world"] = zoneObjectsToJson(zone);

    // The Zone's continuous field root used to exist live, participate in the
    // Formation, expose PropertyPaths, and then simply disappear from saves.
    // Persist the being itself — including its OntoMath ASTs and authored
    // dynamic properties — so a field that governs reality remains real
    // across the temporal boundary too.
    if (const auto* root = zone.spatialRoot()) {
        zj["spatialRoot"] = root->toJson();
    }

    nlohmann::json lexemes = nlohmann::json::array();
    for (Singular* member : zone.formation().getMembers()) {
        auto* lexeme = dynamic_cast<Singularity::Language::Lexeme*>(member);
        if (!lexeme) continue;
        lexemes.push_back({
            {"id", lexeme->getIdentifier()},
            {"symbol", lexeme->getSymbol()}
        });
    }
    zj["lexemes"] = lexemes;
    zj["formationRelations"] = zone.formation().relations().toJson();

    // Paint lives on Material beings. Objects only store a materialId.
    // If those materials stay only in the session bag, loading another
    // save replaces them and Home comes back white. Carry the materials
    // this Zone/Home's objects name, so the identity file is the surface.
    nlohmann::json mats = nlohmann::json::array();
    std::unordered_set<std::string> seen;
    for (const auto& obj : zone.objects()) {
        if (!obj) continue;
        const std::string& mid = obj->materialId();
        if (mid.empty() || !seen.insert(mid).second) continue;
        if (auto m = materials.get(mid)) mats.push_back(m->toJson());
    }
    if (!mats.empty()) zj["materials"] = std::move(mats);

    if (const auto* home = dynamic_cast<const Home*>(&zone)) {
        homeToJson(zj, *home);
    }
    return zj;
}

void applyZoneJson(Zone& zone, const nlohmann::json& zj, bool replaceObjects) {
    if (zj.contains("materials")) {
        materials.mergeFromJson(zj["materials"]);
    }
    if (zj.contains("owner")) {
        zone.setOwner(zj.value("owner", std::string{}));
    }
    if (zj.contains("parentZone")) {
        zone.setParentZone(zj.value("parentZone", std::string{}));
    }
    if (zj.contains("scope") && zj["scope"].is_string()) {
        zone.setScope(scopeFromName(zj["scope"].get<std::string>()));
    }
    if (zj.contains("qualities") && zj["qualities"].is_object()) {
        for (auto it = zj["qualities"].begin(); it != zj["qualities"].end(); ++it) {
            if (it.value().is_string()) {
                zone.setQuality(it.key(), it.value().get<std::string>());
            }
        }
    }
    if (zj.contains("deletable") && zj["deletable"].is_object()) {
        for (auto it = zj["deletable"].begin(); it != zj["deletable"].end(); ++it) {
            if (it.value().is_boolean()) {
                zone.setDeletable(it.key(), it.value().get<bool>());
            }
        }
    }

    // Restore into the Zone's already-owned FieldNode instead of replacing
    // the pointer. Formation membership and any lazily materialised PropertyRef
    // bridges therefore stay valid while the mathematical state comes back.
    if (zj.contains("spatialRoot") && zj["spatialRoot"].is_object()) {
        if (auto* root = zone.spatialRoot()) root->applyJson(zj["spatialRoot"]);
    }

    if (replaceObjects) {
        zone.getOwnedObjectsMutable().clear();
    }
    if (zone.getOwnedObjects().empty()) {
        if (zj.contains("world")) {
            zoneObjectsFromJson(zj["world"], zone);
        } else if (zj.contains("objects")) {
            zoneObjectsFromJson(zj, zone);
        }
    }
    // A non-empty zone here can mean two different things this function
    // cannot tell apart from replaceObjects alone: a Zone kept LIVE from the
    // running session (unsaved_preserve_test: another file's snapshot of the
    // same Zone must NOT touch it), or a Zone just hydrated from its identity
    // store a moment ago (where the World's later authoring SHOULD fill in
    // fields the store never captured — see mergeZoneObjectsFromJson).
    // Those callers are distinguishable at the call site, not here, so the
    // merge is invoked explicitly by ZoneManager's identity-store branch
    // rather than folded into this general-purpose function.
    // Bug #7: this used to be nested in the empty-objects branch above, so
    // a Zone that already held objects (kept live, or just hydrated from
    // the store) could never receive its relation graph or lexemes. Now
    // idempotent (see applyFormationRelations), it always runs.
    applyFormationRelations(zone, zj);
    if (auto* home = dynamic_cast<Home*>(&zone)) {
        homeFromJson(zj, *home);
    }
}

std::shared_ptr<Zone> makeZoneFromJson(const nlohmann::json& zj) {
    // Identity, not display: MUST agree with zoneIdFromJson, the same
    // resolution every admission/dedup check in ZoneManager.cpp uses to
    // ask "is a Zone with this id already live?" Before this fix, this
    // function preferred "name" while zoneIdFromJson preferred
    // "identifier" — a record whose two fields differed (the common case
    // once identity/name are actually distinct concepts, e.g. the
    // BasicPixelChanger/"Basic Pixel Changer" case, or the
    // Basic2DButtonZone/"Basic 2D Button Zone" one that surfaced this)
    // could never be recognized as already-live, so every load of a
    // World naming it minted a fresh duplicate live Zone object sharing
    // the same eventual getIdentifier() as one already there — exactly
    // the ambiguity applyMatterFlatBuffer's Invariant 3 refusal surfaced.
    const std::string identifier = zoneIdFromJson(zj);
    const std::string safeIdentifier = identifier.empty() ? "Untitled Zone" : identifier;
    const std::string displayName = zj.value("name", safeIdentifier);
    std::string kind;
    if (zj.contains("qualities") && zj["qualities"].is_object()) {
        kind = zj["qualities"].value("kind", std::string{});
    }
    if (kind.empty()) kind = zj.value("kind", std::string{});
    const bool dwelling = (kind == Zone::kHomeKind || kind == Zone::kCommunityHomeKind
                           || safeIdentifier == "Home" || zj.value("being", std::string{}) == "home");
    std::shared_ptr<Zone> zone = dwelling
        ? std::shared_ptr<Zone>(std::make_shared<Home>(safeIdentifier, "strict"))
        : std::make_shared<Zone>(safeIdentifier, "strict");
    if (!displayName.empty() && displayName != safeIdentifier) {
        zone->setName(displayName);
    }
    applyZoneJson(*zone, zj, true);
    return zone;
}
