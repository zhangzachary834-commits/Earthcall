#include "Singularity/Storage/Serialization/Relation/FormationSerialization.hpp"

#include "ConstructedBeing/CategoryManager.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "Relation/Relation.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

#include <iostream>
#include <memory>
#include <set>
#include <tuple>

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
        zone.addToFormation(lexeme.get());
    }
}

Singular* resolveZoneEndpoint(Zone& zone, const std::string& id) {
    if (id.empty()) return nullptr;
    if (Singular* member = zone.formation().findMemberByIdentifier(id)) return member;
    for (const auto& obj : zone.getOwnedObjects()) {
        if (obj && obj->getIdentifier() == id) return obj.get();
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

} // namespace

void applyFormationRelations(Zone& zone, const nlohmann::json& zj) {
    if (!zj.contains("formationRelations") || !zj["formationRelations"].is_array()) return;

    // Members must exist before endpoint resolution.  This is deliberately a
    // separate hydration phase: a Relation is a first-class Singular, not a
    // child nested below the Zone that happens to hold its endpoints.
    zone.syncFormationMembers();
    internZoneLexemes(zone, zj);

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

    if (refused == 0) return;
    std::cout << "⚠️  Zone '" << zone.name() << "': " << refused
              << " saved formation relation(s) were REFUSED on load";
    if (unbound > 0) {
        std::cout << " — " << unbound
                  << " because an endpoint is not in the world yet";
        if (unbound < refused) std::cout << ", the rest";
    }
    if (unbound < refused) {
        std::cout << " because the edge is a self-ground or closes a directed cycle";
    }
    std::cout << ". They are not in the formation and will not be written back on "
              << "the next save. Fix them in the save file to keep them."
              << std::endl;
}
