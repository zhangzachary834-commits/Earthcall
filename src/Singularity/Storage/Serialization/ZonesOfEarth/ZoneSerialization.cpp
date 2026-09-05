#include "Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.hpp"
#include "Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.hpp"
#include "Singularity/Storage/Serialization/Relation/FormationSerialization.hpp"
#include "Singularity/Storage/Serialization/ZonesOfEarth/HomeSerialization.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "ZonesOfEarth/HomesOfEarth/Home.hpp"
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

extern MaterialManager materials;
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
    const std::string name = zj.value("name", zj.value("identifier", "Untitled Zone"));
    std::string kind;
    if (zj.contains("qualities") && zj["qualities"].is_object()) {
        kind = zj["qualities"].value("kind", std::string{});
    }
    if (kind.empty()) kind = zj.value("kind", std::string{});
    const bool dwelling = (kind == Zone::kHomeKind || kind == Zone::kCommunityHomeKind
                           || name == "Home" || zj.value("being", std::string{}) == "home");
    std::shared_ptr<Zone> zone = dwelling
        ? std::shared_ptr<Zone>(std::make_shared<Home>(name, "strict"))
        : std::make_shared<Zone>(name, "strict");
    applyZoneJson(*zone, zj, true);
    return zone;
}
