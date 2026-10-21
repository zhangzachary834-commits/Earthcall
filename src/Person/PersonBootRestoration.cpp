#include "Person/PersonBootRestoration.hpp"

#include "Person/Person.hpp"
#include "Identity/PersonPresence.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/Serialization/Person/PersonSerialization.hpp"

#include <string>
#include <utility>
#include <vector>

PersonBootRestorationResult restoreUnambiguousPersonProfileForBoot(Person& person) {
    // Only an unambiguous local profile may supply boot-time display/body/
    // motion data. Loading a profile does not make a keyed Person present:
    // the separate key-unlock path retains sole authentication authority.
    std::vector<nlohmann::json> candidates;
    for (const auto& info : SaveSystem::listWorlds(SaveSystem::SaveType::PERSON)) {
        nlohmann::json profile = SaveSystem::readSaveData(info.path);
        if (!profile.is_object()) continue;
        if (!profile.contains("personId") &&
            Identity::legacyProfileSuperseded(profile.value("displayName", std::string{}))) {
            continue;
        }
        candidates.push_back(std::move(profile));
    }

    if (candidates.empty()) return PersonBootRestorationResult::NoProfile;
    if (candidates.size() != 1) return PersonBootRestorationResult::Ambiguous;
    personFromJson(candidates.front(), person);
    return PersonBootRestorationResult::Restored;
}
