#include "Singularity/Storage/Serialization/Relation/FormationSerialization.hpp"

#include "ConstructedBeing/CategoryManager.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "Relation/Relation.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "Singularity/Storage/Serialization/Common/SingularPropertySerialization.hpp"

#include <iostream>
#include <memory>
#include <set>
#include <tuple>
#include <unordered_set>

extern MaterialManager materials;
extern CategoryManager categories;

namespace {

void internZoneLexemes(Zone& zone, const nlohmann::json& zj) {
    if (!zj.contains("lexemes") || !zj["lexemes"].is_array()) return;
    auto& language = Singularity::Language::LanguageSystem::instance();
    for (const auto& item : zj["lexemes"]) {
        if (!item.is_object()) continue;
        const std::string id = item.value("id", std::string{});
        const std::string symbol = item.value("symbol", std::string{});
        if (id.empty() || symbol.empty()) continue;
        auto lexeme = language.intern(symbol, id);
        Singularity::Storage::readSingularProperties(item, *lexeme);
        zone.addToFormation(lexeme.get());
    }
}

Singular* resolveZoneEndpoint(Zone& zone, const std::string& id) {
    if (id.empty()) return nullptr;
    // A Zone is itself a legitimate Relation endpoint. During first hydration
    // makeZoneFromJson has not admitted it to ZoneManager yet, so Universe
    // cannot find it. Resolve self directly or persisted Zone→Person relations
    // such as owned-by remain pending until a second pass that may never come.
    if (zone.getIdentifier() == id) return &zone;
    if (Singular* member = zone.formation().findMemberByIdentifier(id)) return member;
    for (const auto& obj : zone.getOwnedObjects()) {
        if (obj && obj->getIdentifier() == id) return obj.get();
    }
    for (const auto& relation : zone.formation().relations().getAll()) {
        if (relation && relation->getIdentifier() == id) return relation.get();
    }
    if (auto cat = categories.get(id)) return cat.get();
    if (auto mat = materials.get(id)) return mat.get();
    auto& language = Singularity::Language::LanguageSystem::instance();
    if (auto lexeme = language.findById(id)) {
        zone.addToFormation(lexeme.get());
        return lexeme.get();
    }
    if (auto lexeme = language.findBySymbol(id)) {
        zone.addToFormation(lexeme.get());
        return lexeme.get();
    }
    for (Singular* being : Universe::instance().beings()) {
        if (being && being->getIdentifier() == id) return being;
    }
    return nullptr;
}

void resolveDeferredZoneProperties(Zone& zone) {
    auto resolve = [&](const std::string& id) -> Singular* {
        return resolveZoneEndpoint(zone, id);
    };
    std::unordered_set<Singular*> rebound;
    auto bindDeferred = [&](Singular* being) {
        if (!being || !rebound.insert(being).second) return;
        Singularity::Storage::resolveDeferredSingularProperties(*being, resolve);
    };

    bindDeferred(&zone);
    bindDeferred(zone.spatialRoot());
    for (const auto& object : zone.getOwnedObjects()) {
        bindDeferred(object.get());
        if (object) {
            if (auto material = materials.get(object->materialId())) {
                bindDeferred(material.get());
            }
        }
    }
    for (Singular* member : zone.formation().getMembers()) bindDeferred(member);
    for (const auto& relation : zone.formation().relations().getAll()) {
        bindDeferred(relation.get());
    }
    for (Singular* being : Universe::instance().beings()) bindDeferred(being);
}

} // namespace

void applyFormationRelations(Zone& zone, const nlohmann::json& zj) {
    // Member/Lexeme hydration is independent of whether this Zone has any
    // Relation records. A Zone containing only Lexemes must still restore
    // those Singulars and their semantic properties.
    zone.syncFormationMembers();
    internZoneLexemes(zone, zj);

    if (!zj.contains("formationRelations") || !zj["formationRelations"].is_array()) {
        resolveDeferredZoneProperties(zone);
        return;
    }

    std::set<std::tuple<std::string, std::string, std::string>> existing;
    for (const auto& relation : zone.formation().relations().getAll()) {
        if (relation) existing.insert({relation->type, relation->aId(), relation->bId()});
    }

    size_t refused = 0;
    size_t unbound = 0;
    for (const auto& relationJson : zj["formationRelations"]) {
        auto relation = std::make_shared<Relation>(Relation::fromJson(
            relationJson, [&](const std::string& id) {
                return resolveZoneEndpoint(zone, id);
            }));
        const auto key = std::make_tuple(relation->type,
                                         relation->aId(), relation->bId());
        if (existing.count(key)) continue;
        if (!zone.formation().add(relation)) {
            ++refused;
            if (!relation->hasEndpoints()) ++unbound;
        } else {
            existing.insert(key);
        }
    }

    // Second hydration phase: every root is now present, so identity-valued
    // Properties deferred during codec hydration can bind through the SAME
    // resolver the Relation graph trusts.
    resolveDeferredZoneProperties(zone);

    if (refused == 0) return;
    std::cout << "⚠️  Zone '" << zone.name() << "': " << refused
              << " saved relation(s) remain pending after this hydration pass";
    if (unbound > 0) {
        std::cout << " — " << unbound
                  << " because an endpoint is not in the world yet";
        if (unbound < refused) std::cout << ", the rest";
    }
    if (unbound < refused) {
        std::cout << " because the edge is a self-ground or closes a directed cycle";
    }
    std::cout << ". A later hydration pass may bind newly available endpoints; any "
              << "still unresolved at save time will not be written back."
              << std::endl;
}
