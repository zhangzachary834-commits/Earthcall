#include "ZonesOfEarth/ZoneManager.hpp"

#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <iostream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

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
            preparedLawRoots.push_back(PreparedLawRoot{lawId, std::move(lawRoot)});
        }
    }

    const bool wrote = dwelling ? SaveSystem::writeHomeIdentity(id, doc)
                                : SaveSystem::writeZoneIdentity(id, doc);
    if (!wrote) {
        std::cerr << "[zones] REFUSED or failed Save Zone for '" << id << "'.\n";
        return false;
    }

    // Save only the shared Law roots named by THIS Zone; an unrelated Zone's
    // Laws are outside this operation. Each root writer is itself atomic.
    for (const auto& prepared : preparedLawRoots) {
        if (!SaveSystem::writeLawIdentity(prepared.identifier, prepared.document)) {
            std::cerr << "[zones] Failed to persist shared Law '" << prepared.identifier
                      << "' named by Zone '" << id
                      << "'. Zone identity was written; whole-closure transactional commit "
                         "remains an open serialization rung.\n";
            return false;
        }
    }

    std::cout << "[zones] Saved Zone '" << id
              << "' without creating or rewriting a conglomerate session file.\n";
    return true;
}

bool ZoneManager::persistActiveZone() const {
    return persistZone(_currentIndex);
}