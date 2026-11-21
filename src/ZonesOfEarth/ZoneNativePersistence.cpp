#include "ZonesOfEarth/ZoneManager.hpp"

#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"

#include <iostream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

extern MaterialManager materials;

namespace {

bool isObservationZoneForNativeSave(const Zone& zone) {
    const auto& qualities = zone.getQualities();
    const auto it = qualities.find("kind");
    return it != qualities.end() && it->second == "test-observation";
}

std::size_t storedObjectCount(const nlohmann::json& identity) {
    if (!identity.is_object()) return 0;
    if (identity.contains("world") && identity["world"].is_object() &&
        identity["world"].contains("objects") && identity["world"]["objects"].is_array()) {
        return identity["world"]["objects"].size();
    }
    if (identity.contains("objects") && identity["objects"].is_array()) {
        return identity["objects"].size();
    }
    return 0;
}

struct PreparedLawRoot {
    std::string identifier;
    nlohmann::json document;
};

struct PreparedMaterialRoot {
    std::string identifier;
    nlohmann::json document;
};

std::string materialStem(const std::string& id) {
    static const std::string prefix = "material.";
    return id.rfind(prefix, 0) == 0 ? id.substr(prefix.size()) : id;
}

} // namespace

bool ZoneManager::persistZone(size_t index) const {
    if (index >= _zones.size() || !_zones[index]) {
        std::cerr << "[zones] REFUSED Save Zone: invalid Zone index " << index << ".\n";
        return false;
    }

    const auto& zone = _zones[index];
    if (isObservationZoneForNativeSave(*zone)) {
        std::cerr << "[zones] REFUSED Save Zone for observation Zone '"
                  << zone->getIdentifier()
                  << "': test observations are not authored Zone identities.\n";
        return false;
    }

    const std::string id = zone->getIdentifier();
    if (id.empty()) {
        std::cerr << "[zones] REFUSED Save Zone: Zone has no stable identifier.\n";
        return false;
    }

    const bool dwelling = zone->isHome();
    const bool identityExists = dwelling ? SaveSystem::homeIdentityExists(id)
                                         : SaveSystem::zoneIdentityExists(id);
    nlohmann::json priorIdentity;
    if (identityExists) {
        priorIdentity = dwelling ? SaveSystem::readHomeIdentity(id)
                                 : SaveSystem::readZoneIdentity(id);
    }

    // Keep the same anti-erasure covenant as the bulk compatibility writer.
    // A boot-minted empty Zone/Home may not stamp emptiness over an authored
    // identity simply because that identity has not been admitted correctly.
    if (zone->getOwnedObjects().empty() && identityExists) {
        const std::size_t stored = storedObjectCount(priorIdentity);
        if (stored > 0) {
            std::cerr << "[zones] REFUSED Save Zone for '" << id
                      << "': live Zone is empty while its identity still has "
                      << stored << " being(s). Stored identity left untouched.\n";
            return false;
        }
    }

    nlohmann::json doc = zoneToJson(*zone);

    // lawRefs are authored Zone membership. They are deliberately NOT inferred
    // from whichever Laws happen to be globally registered this frame.
    //
    // There is one additional authored source for the ACTIVE Zone: Laws born
    // through universal Singular creation are explicitly adopted into
    // `_activeZoneLawIds`. That set is the same closure switchTo loaded from
    // lawRefs; appending its new members is therefore recording an authored
    // membership mutation, not projecting the global register back to disk.
    if (priorIdentity.is_object() && priorIdentity.contains("lawRefs")) {
        doc["lawRefs"] = priorIdentity["lawRefs"];
        // A Law retired from this Zone by an authored act (a confirmed
        // deletion) leaves its membership; its own file stays as history.
        if (doc["lawRefs"].is_array()) {
            nlohmann::json kept = nlohmann::json::array();
            for (const auto& ref : doc["lawRefs"]) {
                if (ref.is_string() && isLawRetiredFrom(id, ref.get<std::string>())) continue;
                kept.push_back(ref);
            }
            doc["lawRefs"] = std::move(kept);
        }
    }
    if (index == _currentIndex && !_activeZoneLawIds.empty()) {
        if (!doc.contains("lawRefs")) doc["lawRefs"] = nlohmann::json::array();
        if (!doc["lawRefs"].is_array()) {
            std::cerr << "[zones] REFUSED Save Zone for '" << id
                      << "': lawRefs is not an array. Nothing written.\n";
            return false;
        }

        std::unordered_set<std::string> alreadyNamed;
        for (const auto& ref : doc["lawRefs"]) {
            if (ref.is_string()) alreadyNamed.insert(ref.get<std::string>());
        }
        for (const auto& activeLawId : _activeZoneLawIds) {
            if (!activeLawId.empty() && alreadyNamed.insert(activeLawId).second) {
                doc["lawRefs"].push_back(activeLawId);
            }
        }
    }

    // Relation/lexeme loss has happened before. Refuse rather than turning a
    // live hydration failure into permanent authored history.
    if (identityExists && priorIdentity.is_object()) {
        const std::size_t storedRelations =
            priorIdentity.value("formationRelations", nlohmann::json::array()).size();
        const std::size_t storedLexemes =
            priorIdentity.value("lexemes", nlohmann::json::array()).size();
        const std::size_t liveRelations =
            doc.value("formationRelations", nlohmann::json::array()).size();
        const std::size_t liveLexemes =
            doc.value("lexemes", nlohmann::json::array()).size();
        if ((liveRelations == 0 && storedRelations > 0) ||
            (liveLexemes == 0 && storedLexemes > 0)) {
            std::cerr << "[zones] REFUSED Save Zone for '" << id
                      << "': live formation has " << liveRelations << " relation(s)/"
                      << liveLexemes << " lexeme(s), while stored identity has "
                      << storedRelations << " relation(s)/" << storedLexemes
                      << " lexeme(s). Stored graph left untouched.\n";
            return false;
        }
    }

    // materialRefs are authored membership exactly as lawRefs are: a Material
    // two Zones share lives ONCE at saves/materials/<stem>/material.json. The
    // Zone keeps naming it and does not embed a private copy (Per-Zone
    // pathway proof 5). A ref whose Material is no longer live refuses the
    // save -- dropping it silently would make the next Move to Zone refuse.
    std::vector<PreparedMaterialRoot> preparedMaterialRoots;
    if (priorIdentity.is_object() && priorIdentity.contains("materialRefs")) {
        if (!priorIdentity["materialRefs"].is_array()) {
            std::cerr << "[zones] REFUSED Save Zone for '" << id
                      << "': materialRefs is not an array. Nothing written.\n";
            return false;
        }
        doc["materialRefs"] = priorIdentity["materialRefs"];
        std::unordered_set<std::string> sharedStems;
        for (const auto& refJson : doc["materialRefs"]) {
            if (!refJson.is_string() || refJson.get<std::string>().empty()) {
                std::cerr << "[zones] REFUSED Save Zone for '" << id
                          << "': materialRef is not a non-empty string. Nothing written.\n";
                return false;
            }
            const std::string ref = refJson.get<std::string>();
            auto live = materials.get(ref);
            if (!live) {
                std::cerr << "[zones] REFUSED Save Zone for '" << id << "': named shared Material '"
                          << ref << "' is not in the live Material register. Nothing written.\n";
                return false;
            }
            sharedStems.insert(materialStem(ref));
            nlohmann::json root{{"identifier", "material." + live->name()},
                                {"material", live->toJson()}};
            const nlohmann::json existing = SaveSystem::readMaterialIdentity(ref);
            if (existing.is_object() && existing.contains("injected_by")) {
                root["injected_by"] = existing["injected_by"];
            }
            // Byte-stability: an unchanged shared root is not rewritten, so
            // saving one Zone never touches a Material it merely shares.
            if (existing != root) preparedMaterialRoots.push_back(PreparedMaterialRoot{ref, std::move(root)});
        }
        if (doc.contains("materials") && doc["materials"].is_array()) {
            nlohmann::json embedded = nlohmann::json::array();
            for (const auto& entry : doc["materials"]) {
                if (entry.is_object() && sharedStems.count(Material::fromJson(entry).name()) != 0) continue;
                embedded.push_back(entry);
            }
            if (embedded.empty()) doc.erase("materials");
            else doc["materials"] = std::move(embedded);
        }
    }

    // Preflight the shared Law portion of this Zone's closure BEFORE writing
    // the Zone identity. This does not yet make the multi-file write a fully
    // detached transaction (that larger rung remains open), but semantic
    // refusal cannot happen after we have already mutated the Zone file.
    std::vector<PreparedLawRoot> preparedLawRoots;
    if (doc.contains("lawRefs")) {
        if (!doc["lawRefs"].is_array()) {
            std::cerr << "[zones] REFUSED Save Zone for '" << id
                      << "': lawRefs is not an array. Nothing written.\n";
            return false;
        }
        if (!doc["lawRefs"].empty() && !_lawManager) {
            std::cerr << "[zones] REFUSED Save Zone for '" << id
                      << "': Zone names authored Laws but no LawManager is bound. Nothing written.\n";
            return false;
        }

        for (const auto& refJson : doc["lawRefs"]) {
            if (!refJson.is_string() || refJson.get<std::string>().empty()) {
                std::cerr << "[zones] REFUSED Save Zone for '" << id
                          << "': lawRef is not a non-empty string. Nothing written.\n";
                return false;
            }
            const std::string lawId = refJson.get<std::string>();
            Law* law = _lawManager->find(lawId);
            if (!law) {
                std::cerr << "[zones] REFUSED Save Zone for '" << id
                          << "': named Law '" << lawId
                          << "' is not in the running Law register. Nothing written.\n";
                return false;
            }
            if (law->isFirstMover()) {
                // First Movers are engine-owned substrate, not Zone-authored
                // shared roots. Preserve the reference if legacy data names
                // one, but do not serialize the engine into saves/laws/.
                continue;
            }

            const nlohmann::json lawJson = law->toJson();
            nlohmann::json lawRoot{
                {"identifier", lawId},
                {"authors", lawJson.value("authors", nlohmann::json::array())},
                {"law", lawJson},
                {"triggers", _lawManager->triggersOf(lawId)}
            };
            const nlohmann::json existingRoot = SaveSystem::readLawIdentity(lawId);
            if (existingRoot.is_object() && existingRoot.contains("injected_by")) {
                lawRoot["injected_by"] = existingRoot["injected_by"];
            }
            // Unchanged shared roots are not rewritten (byte-stable): saving one
            // Zone must not touch a Law it merely shares with another.
            if (existingRoot != lawRoot) {
                preparedLawRoots.push_back(PreparedLawRoot{lawId, std::move(lawRoot)});
            }
        }
    }

    // Dependencies first, dependents last. Every shared root this Zone names is
    // written (each atomically) BEFORE the Zone identity, so a failure partway
    // leaves the previous Zone identity pointing at roots that all exist,
    // never a Zone that names a root which was not written. Only the roots
    // named by THIS Zone are touched; an unrelated Zone's closure is outside
    // this operation.
    for (const auto& prepared : preparedMaterialRoots) {
        if (!SaveSystem::writeMaterialIdentity(prepared.identifier, prepared.document)) {
            std::cerr << "[zones] Failed to persist shared Material '" << prepared.identifier
                      << "' named by Zone '" << id << "'. Zone identity left untouched.\n";
            return false;
        }
    }
    for (const auto& prepared : preparedLawRoots) {
        if (!SaveSystem::writeLawIdentity(prepared.identifier, prepared.document)) {
            std::cerr << "[zones] Failed to persist shared Law '" << prepared.identifier
                      << "' named by Zone '" << id << "'. Zone identity left untouched.\n";
            return false;
        }
    }

    const bool wrote = dwelling ? SaveSystem::writeHomeIdentity(id, doc)
                                : SaveSystem::writeZoneIdentity(id, doc);
    if (!wrote) {
        std::cerr << "[zones] REFUSED or failed Save Zone for '" << id << "'.\n";
        return false;
    }

    std::cout << "[zones] Saved Zone '" << id
              << "' without creating or rewriting a conglomerate session file.\n";
    return true;
}

bool ZoneManager::persistActiveZone() const {
    return persistZone(_currentIndex);
}