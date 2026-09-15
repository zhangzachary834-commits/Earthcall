#include "ZonesOfEarth/ZoneManager.hpp"

#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <iostream>
#include <string>

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

    // lawRefs are authored Zone membership. They are deliberately not inferred
    // from whichever Laws happen to be globally registered this frame.
    if (priorIdentity.is_object() && priorIdentity.contains("lawRefs")) {
        doc["lawRefs"] = priorIdentity["lawRefs"];
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

    const bool wrote = dwelling ? SaveSystem::writeHomeIdentity(id, doc)
                                : SaveSystem::writeZoneIdentity(id, doc);
    if (!wrote) {
        std::cerr << "[zones] REFUSED or failed Save Zone for '" << id << "'.\n";
        return false;
    }

    // A Zone names shared Law roots by stable id. Save only those roots named
    // by THIS Zone; an unrelated Zone's Laws are outside this transaction.
    if (_lawManager && doc.contains("lawRefs") && doc["lawRefs"].is_array()) {
        for (const auto& refJson : doc["lawRefs"]) {
            if (!refJson.is_string()) {
                std::cerr << "[zones] REFUSED shared Law persistence for Zone '" << id
                          << "': lawRef is not a string. Zone identity was already written; "
                             "repair the authored reference before relying on this closure.\n";
                return false;
            }
            const std::string lawId = refJson.get<std::string>();
            Law* law = _lawManager->find(lawId);
            if (!law || law->isFirstMover()) continue;

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
            if (!SaveSystem::writeLawIdentity(lawId, lawRoot)) {
                std::cerr << "[zones] Failed to persist shared Law '" << lawId
                          << "' named by Zone '" << id << "'.\n";
                return false;
            }
        }
    }

    std::cout << "[zones] Saved Zone '" << id
              << "' without creating or rewriting a conglomerate session file.\n";
    return true;
}

bool ZoneManager::persistActiveZone() const {
    return persistZone(_currentIndex);
}
