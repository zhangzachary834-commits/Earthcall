#include "Singularity/Storage/Serialization/Person/PersonSerialization.hpp"

#include "Person/Person.hpp"
#include "Singularity/Storage/Serialization/Person/BodySerialization.hpp"

nlohmann::json personToJson(const Person& person) {
    nlohmann::json j;
    j["displayName"] = person.getDisplayName();
    if (person.called()) j["displayLexemeId"] = person.called()->getIdentifier();
    // Keep this legacy alias: older profile readers still use it as a label.
    j["soulName"] = person.getDisplayName();
    if (person.personId().canAuthenticate()) j["personId"] = person.personId().toString();
    const auto& position = person.position();
    const auto& velocity = person.velocity();
    j["position"] = {position.x, position.y, position.z};
    j["velocity"] = {velocity.x, velocity.y, velocity.z};
    j["body"] = bodyToJson(person.getBody());
    return j;
}

void personFromJson(const nlohmann::json& j, Person& person) {
    // Type-checked: profile/session files are untrusted by construction.
    if (j.contains("displayName") && j["displayName"].is_string()) {
        person.setDisplayName(j["displayName"].get<std::string>());
    } else if (j.contains("soulName") && j["soulName"].is_string()) {
        person.setDisplayName(j["soulName"].get<std::string>());
    }

    // A personId read from a file is only a claim. Signature/authority
    // verification remains outside this storage codec, as it did before.
    if (j.contains("personId") && j["personId"].is_string()) {
        Identity::SingularId claimed =
            Identity::SingularId::parse(j["personId"].get<std::string>());
        if (claimed.canAuthenticate()) person.setPersonId(claimed);
    }
    if (j.contains("position") && j["position"].is_array() && j["position"].size() >= 3) {
        person.position() = glm::vec3(j["position"][0], j["position"][1], j["position"][2]);
    }
    if (j.contains("velocity") && j["velocity"].is_array() && j["velocity"].size() >= 3) {
        person.velocity() = glm::vec3(j["velocity"][0], j["velocity"][1], j["velocity"][2]);
    }
    if (j.contains("body")) {
        bodyFromJson(j["body"], person.getBody());
    }
}

#include "Singularity/Storage/SaveSystem.hpp"
#include "Person/PersonDatabase.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

void updatePriorPersonSerializations(const Person& person, const std::string& oldName) {
    const std::string newName = person.getDisplayName();
    const std::string newId = person.getIdentifier();

    // 1. Update PersonDatabase profile
    PersonDatabase::getInstance().savePerson(person);
    if (!oldName.empty() && oldName != newName) {
        std::string personFolder = SaveSystem::ensureSaveTypeFolder(SaveSystem::SaveType::PERSON);
        if (!personFolder.empty()) {
            std::error_code ec;
            std::string oldPath = personFolder + "/" + SaveSystem::sanitizeLabel(oldName) + ".json";
            if (std::filesystem::exists(oldPath, ec)) {
                std::filesystem::remove(oldPath, ec);
            }
            if (oldName == "Player" || oldName == "player") {
                std::string playerPath = personFolder + "/Player.json";
                if (std::filesystem::exists(playerPath, ec)) std::filesystem::remove(playerPath, ec);
            }
        }
    }

    // 2. Update Live Zones & Home Zone in ZoneManager
    if (ZoneManager* mgr = ZoneManager::live()) {
        for (auto& z : mgr->zones()) {
            if (!z) continue;
            if (z->owner() == oldName || z->owner() == "Player" || z->owner() == "player") {
                z->setOwner(newId, Zone::kOwnerKindPerson);
            }
        }
        mgr->ensureHomeZone(newId);
        mgr->persistZones();
    }

    // 3. Scan and update all prior save files in SaveSystem::saveRoot()
    std::string root = SaveSystem::saveRoot();
    if (root.empty()) root = "saves";
    std::error_code dirEc;
    if (std::filesystem::exists(root, dirEc)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(root, dirEc)) {
            if (dirEc) break;
            if (!entry.is_regular_file()) continue;

            std::string ext = entry.path().extension().string();
            if (ext != ".json" && ext != ".ecform") continue;

            std::string stem = entry.path().stem().string();
            if (entry.path().parent_path().filename() == "persons" && stem == SaveSystem::sanitizeLabel(newName)) {
                continue;
            }

            try {
                std::ifstream inFile(entry.path());
                if (!inFile.is_open()) continue;
                nlohmann::json j;
                inFile >> j;
                inFile.close();

                if (!j.is_object()) continue;
                bool modified = false;

                // Update semanticRoots.person
                if (j.contains("semanticRoots") && j["semanticRoots"].is_object()) {
                    auto& sr = j["semanticRoots"];
                    if (sr.contains("person") && sr["person"].is_object()) {
                        auto& pj = sr["person"];
                        std::string dName = pj.value("displayName", "");
                        std::string sName = pj.value("soulName", "");
                        bool match = false;
                        if (person.hasIdentity() && pj.contains("personId") && pj["personId"].is_string()) {
                            if (pj["personId"].get<std::string>() == person.personId().toString()) match = true;
                        }
                        if (dName == oldName || dName == "Player" || dName == "player" || dName == "Person" ||
                            sName == oldName || sName == "Player" || sName == "player" || sName == "Person" ||
                            oldName.empty()) {
                            match = true;
                        }
                        if (match) {
                            pj["displayName"] = newName;
                            pj["soulName"] = newName;
                            if (person.called()) pj["displayLexemeId"] = person.called()->getIdentifier();
                            if (person.hasIdentity()) pj["personId"] = person.personId().toString();
                            modified = true;
                        }
                    }

                    // Update zones inside semanticRoots
                    if (sr.contains("zones") && sr["zones"].is_array()) {
                        for (auto& zj : sr["zones"]) {
                            if (zj.is_object() && zj.contains("owner") && zj["owner"].is_string()) {
                                std::string o = zj["owner"].get<std::string>();
                                if (o == oldName || o == "Player" || o == "player" ||
                                    (oldName != "Person" && o == "Person" && newName != "Person")) {
                                    zj["owner"] = newId;
                                    modified = true;
                                }
                            }
                        }
                    }
                }

                // Update top-level person
                if (j.contains("person") && j["person"].is_object()) {
                    auto& pj = j["person"];
                    std::string dName = pj.value("displayName", "");
                    std::string sName = pj.value("soulName", "");
                    bool match = false;
                    if (person.hasIdentity() && pj.contains("personId") && pj["personId"].is_string()) {
                        if (pj["personId"].get<std::string>() == person.personId().toString()) match = true;
                    }
                    if (dName == oldName || dName == "Player" || dName == "player" || dName == "Person" ||
                        sName == oldName || sName == "Player" || sName == "player" || sName == "Person" ||
                        oldName.empty()) {
                        match = true;
                    }
                    if (match) {
                        pj["displayName"] = newName;
                        pj["soulName"] = newName;
                        if (person.called()) pj["displayLexemeId"] = person.called()->getIdentifier();
                        if (person.hasIdentity()) pj["personId"] = person.personId().toString();
                        modified = true;
                    }
                }

                // Update top-level zones array
                if (j.contains("zones") && j["zones"].is_array()) {
                    for (auto& zj : j["zones"]) {
                        if (zj.is_object() && zj.contains("owner") && zj["owner"].is_string()) {
                            std::string o = zj["owner"].get<std::string>();
                            if (o == oldName || o == "Player" || o == "player" ||
                                (oldName != "Person" && o == "Person" && newName != "Person")) {
                                zj["owner"] = newId;
                                modified = true;
                            }
                        }
                    }
                }

                // Update top-level owner (home.json / zone.json)
                if (j.contains("owner") && j["owner"].is_string()) {
                    std::string o = j["owner"].get<std::string>();
                    if (o == oldName || o == "Player" || o == "player" ||
                        (oldName != "Person" && o == "Person" && newName != "Person")) {
                        j["owner"] = newId;
                        modified = true;
                    }
                }

                if (modified) {
                    std::ofstream outFile(entry.path(), std::ios::trunc);
                    if (outFile.is_open()) {
                        outFile << j.dump(2);
                        outFile.close();
                    }
                }
            } catch (...) {
                // Ignore parsing errors on non-json or malformed files
            }
        }
    }
}
