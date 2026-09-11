#include "ZoneManager.hpp"
#include "HomesOfEarth/Home.hpp"
#include "ConstructedBeing/CategoryManager.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/Serialization.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/LawAuditLogger.hpp"
#include "Singularity/Core/Logger.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Object/Creation/ObjectConcept.hpp"
#include "ConstructedBeing/Singular/Object/Object/ObjectIdentity.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/TransferPolicy.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/Tool.hpp"
#include "Person/Person.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "Singularity/Storage/Schema/Earthcall_generated.h"
#include <flatbuffers/flatbuffers.h>
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include <fstream>
#include <ctime>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <set>
#include <functional>
#include <chrono>
#include <openssl/sha.h>

extern MaterialManager materials;
extern CategoryManager categories;

void ZoneManager::addZone(std::shared_ptr<Zone> zone)
{
    _zones.push_back(std::move(zone));
}

bool ZoneManager::switchTo(size_t index)
{
    if (index < _zones.size()) {
        // Resolve the complete authored Law closure before changing any live
        // state. A missing/malformed root, absent author/target, or identifier
        // collision leaves the current Zone and Law register untouched.
        struct PreparedLaw {
            std::string id;
            std::shared_ptr<Law> law;
            std::vector<std::string> triggers;
        };
        std::vector<PreparedLaw> prepared;
        std::unordered_set<std::string> requestedLawIds;

        const auto& targetZone = _zones[index];
        if (!targetZone) {
            std::cerr << "[zones] REFUSED activation: target Zone is null. "
                         "Current Zone remains active.\n";
            return false;
        }
        nlohmann::json identity;
        identity = targetZone->isHome()
            ? SaveSystem::readHomeIdentity(targetZone->getIdentifier())
            : SaveSystem::readZoneIdentity(targetZone->getIdentifier());
        const nlohmann::json lawRefs = identity.is_object()
            ? identity.value("lawRefs", nlohmann::json::array())
            : nlohmann::json::array();
        if (!lawRefs.is_array()) {
            std::cerr << "[zones] REFUSED activation of '"
                      << targetZone->getIdentifier()
                      << "': lawRefs is not an array. Current Zone remains active.\n";
            return false;
        }
        if (!lawRefs.empty() && !_lawManager) {
            std::cerr << "[zones] REFUSED activation: Zone names authored Laws but no "
                         "LawManager is bound. Current Zone remains active.\n";
            return false;
        }

        const auto resolveReference = [&](const std::string& id,
                                          bool requirePerson) -> Singular* {
            if (id.empty()) return nullptr;
            std::vector<Singular*> candidates;
            const auto consider = [&](Singular* being) {
                if (!being) return;
                bool matches = being->getIdentifier() == id;
                if (auto* person = dynamic_cast<Person*>(being)) {
                    matches = person->matchesIdentifier(id);
                } else if (requirePerson) {
                    matches = false;
                }
                if (matches && std::find(candidates.begin(), candidates.end(), being) == candidates.end()) {
                    candidates.push_back(being);
                }
            };
            for (Singular* being : Universe::instance().beings()) consider(being);
            // Target-Zone objects are not yet in the active Universe. They
            // are still valid references in the closure being preflighted.
            for (const auto& zone : _zones) {
                if (!zone) continue;
                consider(zone.get());
                for (const auto& object : zone->getOwnedObjects()) consider(object.get());
            }
            return candidates.size() == 1 ? candidates.front() : nullptr;
        };

        try {
            for (const auto& refJson : lawRefs) {
                if (!refJson.is_string()) throw std::runtime_error("lawRef is not a string");
                const std::string ref = refJson.get<std::string>();
                if (ref.empty() || !requestedLawIds.insert(ref).second) {
                    throw std::runtime_error("empty or duplicate lawRef '" + ref + "'");
                }
                const nlohmann::json root = SaveSystem::readLawIdentity(ref);
                if (!root.is_object()) {
                    throw std::runtime_error("missing Law root '" + ref + "'");
                }
                if (root.value("identifier", std::string{}) != ref ||
                    !root.contains("law") || !root["law"].is_object()) {
                    throw std::runtime_error("Law root identity mismatch for '" + ref + "'");
                }
                auto law = Law::fromJson(root["law"]);
                if (!law || law->getIdentifier() != ref) {
                    throw std::runtime_error("serialized Law id mismatch for '" + ref + "'");
                }

                const auto& lawJson = root["law"];
                if (!lawJson.contains("authors") || !lawJson["authors"].is_array() ||
                    lawJson["authors"].empty()) {
                    throw std::runtime_error("Law '" + ref + "' has no recorded author");
                }
                for (const auto& authorJson : lawJson["authors"]) {
                    if (!authorJson.is_string()) {
                        throw std::runtime_error("Law '" + ref + "' has a non-string author ref");
                    }
                    Singular* author = resolveReference(authorJson.get<std::string>(), true);
                    if (!author) {
                        throw std::runtime_error("Law '" + ref + "' cannot resolve Person author '" +
                                                 authorJson.get<std::string>() + "'");
                    }
                    law->addAuthor(*author);
                }
                if (lawJson.contains("targets")) {
                    if (!lawJson["targets"].is_array()) {
                        throw std::runtime_error("Law '" + ref + "' targets is not an array");
                    }
                    for (const auto& targetJson : lawJson["targets"]) {
                        if (!targetJson.is_string()) {
                            throw std::runtime_error("Law '" + ref + "' has a non-string target ref");
                        }
                        Singular* target = resolveReference(targetJson.get<std::string>(), false);
                        if (!target) {
                            throw std::runtime_error("Law '" + ref + "' cannot resolve target '" +
                                                     targetJson.get<std::string>() + "'");
                        }
                        law->addTarget(*target);
                    }
                }

                std::vector<std::string> triggers;
                const auto triggerJson = root.value("triggers", nlohmann::json::array());
                if (!triggerJson.is_array()) {
                    throw std::runtime_error("Law '" + ref + "' triggers is not an array");
                }
                for (const auto& trigger : triggerJson) {
                    if (!trigger.is_string() || trigger.get<std::string>().empty()) {
                        throw std::runtime_error("Law '" + ref + "' has an invalid trigger");
                    }
                    triggers.push_back(trigger.get<std::string>());
                }
                if (law->activation() == Law::Activation::OnEvent && triggers.empty()) {
                    throw std::runtime_error("OnEvent Law '" + ref + "' names no trigger");
                }

                Law* existing = _lawManager ? _lawManager->find(ref) : nullptr;
                if (existing && _activeZoneLawIds.count(ref) == 0) {
                    throw std::runtime_error("Law id collision for '" + ref + "'");
                }
                prepared.push_back(PreparedLaw{ref, std::move(law), std::move(triggers)});
            }
        } catch (const std::exception& e) {
            std::cerr << "[zones] REFUSED activation of '"
                      << targetZone->getIdentifier()
                      << "': " << e.what() << ". Current Zone and Laws remain active.\n";
            return false;
        }

        if (!_zones.empty() && _currentIndex < _zones.size() && _currentIndex != index) {
            Core::EventBus::instance().publish(
                ECA::Event{"zone-exited", _zones[_currentIndex].get(), nullptr, std::time(nullptr)});
        }

        if (_lawManager) {
            for (const auto& id : _activeZoneLawIds) {
                if (requestedLawIds.count(id) == 0) _lawManager->remove(id);
            }
            for (auto& incoming : prepared) {
                if (_activeZoneLawIds.count(incoming.id) != 0 &&
                    _lawManager->find(incoming.id)) {
                    continue;
                }
                _lawManager->add(incoming.law);
                for (const auto& trigger : incoming.triggers) {
                    _lawManager->bindTrigger(incoming.id, trigger);
                }
            }
        }
        _activeZoneLawIds = std::move(requestedLawIds);

        _currentIndex = index;
        std::cout << "🔀 Switching to zone [" << index << "]..." << std::endl;

        // Repopulate active zone's world with global objects that belong to it or its parents
        std::vector<std::string> activeZones;
        std::string currentZoneId = _zones[_currentIndex]->getIdentifier();
        while (!currentZoneId.empty()) {
            activeZones.push_back(currentZoneId);
            std::string parent = "";
            for (const auto& z : _zones) {
                if (z->getIdentifier() == currentZoneId) {
                    parent = z->getParentZone();
                    break;
                }
            }
            if (parent == currentZoneId || parent.empty()) break;
            currentZoneId = parent;
        }

        auto& worldObjs = _zones[_currentIndex]->getOwnedObjectsMutable();
        worldObjs.clear();
        for (const auto& obj : globalObjects) {
            bool matches = false;
            for (const auto& az : activeZones) {
                if (obj->belongsToZone(az)) {
                    matches = true;
                    break;
                }
            }
            if (matches) {
                worldObjs.push_back(obj);
            }
        }

        try {
            _zones[_currentIndex]->load();
            Core::EventBus::instance().publish(
                ECA::Event{"zone-loaded", _zones[_currentIndex].get(), nullptr, std::time(nullptr)});
        } catch (...) { std::cerr << "⚠️  Zone load failed." << std::endl; }
        describeCurrent();
        // The zone is a being: laws hear arrival (subject: the zone itself).
        Core::EventBus::instance().publish(
            ECA::Event{"zone-entered", _zones[_currentIndex].get(), nullptr, std::time(nullptr)});
        return true;
    }
    std::cerr << "⚠️ Invalid zone index!" << std::endl;
    return false;
}

void ZoneManager::describeCurrent() const
{
    if (!_zones.empty())
    {
        _zones[_currentIndex]->describe();
    }
    else
    {
        std::cout << "⚠️ No zones available." << std::endl;
    }
}

void ZoneManager::loadZone()
{
    if (_currentIndex < _zones.size())
    {
        // Unload previous zone if necessary
        _zones[_currentIndex]->load();
    }
    else
    {
        std::cerr << "⚠️ Cannot load zone: index out of bounds!" << std::endl;
    }
}

Zone& ZoneManager::active() { return *_zones[_currentIndex]; }

std::vector<std::shared_ptr<Zone>>& ZoneManager::zones() { return _zones; }

const std::vector<std::shared_ptr<Zone>>& ZoneManager::zones() const { return _zones; }

// Save/Load methods moved from Game

namespace {
ZoneManager* g_liveZones = nullptr;
} // namespace

void ZoneManager::bindLive() { g_liveZones = this; }

ZoneManager* ZoneManager::live() { return g_liveZones; }

Zone* ZoneManager::findPrimaryHome(const std::string& personId) {
    return const_cast<Zone*>(
        static_cast<const ZoneManager*>(this)->findPrimaryHome(personId));
}

const Zone* ZoneManager::findPrimaryHome(const std::string& personId) const {
    if (personId.empty()) return nullptr;
    for (const auto& zone : _zones) {
        if (!zone) continue;
        if (zone->isPrimaryHome() && zone->owner() == personId) return zone.get();
    }
    for (const auto& zone : _zones) {
        if (!zone) continue;
        if (zone->name() == "Home" && zone->owner() == personId
            && !zone->isOurverseGathering() && !zone->isCommunityHome()
            && !zone->isCommunityZone()) {
            return zone.get();
        }
    }
    return nullptr;
}

void ZoneManager::ensureHomeZone(const std::string& personId) {
    if (personId.empty()) return;

    if (Zone* existing = findPrimaryHome(personId)) {
        existing->markPrimaryHome();
        if (existing->owner().empty()) existing->setOwner(personId, Zone::kOwnerKindPerson);
        return;
    }

    // A save from before ownership existed may hold an unowned "Home" —
    // claim it instead of minting a name-twin (identifiers must stay unique).
    for (auto& zone : _zones) {
        if (zone && zone->name() == "Home" && zone->owner().empty()
            && !zone->isOurverseGathering()) {
            zone->markPrimaryHome();
            zone->setOwner(personId, Zone::kOwnerKindPerson);
            return;
        }
    }

    bool homeSlugFree = true;
    for (const auto& zone : _zones) {
        if (zone && zone->getIdentifier() == "Home") {
            homeSlugFree = false;
            break;
        }
    }
    const std::string id = homeSlugFree ? std::string("Home")
                                        : std::string("Home_of_") + personId;
    auto home = std::make_shared<Home>(id, "strict");
    home->markPrimaryHome();
    home->setOwner(personId, Zone::kOwnerKindPerson);
    addZone(home);
    printf("[Init] Home established for '%s' (zone count now %zu)\n",
           personId.c_str(), _zones.size());
}

std::shared_ptr<Zone> ZoneManager::authorZone(const std::string& identifier,
                                              const std::string& ownerId,
                                              const std::string& kind,
                                              const std::string& ownerKind) {
    const std::string id = SaveSystem::sanitizeLabel(identifier);
    if (id.empty()) {
        std::cerr << "[zones] REFUSED authorZone: identifier sanitizes away.\n";
        return nullptr;
    }
    if (kind == Zone::kGatheringKind) {
        std::cerr << "[zones] REFUSED authorZone '" << id
                  << "': the gathering place is minted by Ourverse, not authored "
                     "as a Home or Zone (OURVERSE.md).\n";
        return nullptr;
    }
    if (id == "Home" || (kind == Zone::kHomeKind && identifier == "Home")) {
        std::cerr << "[zones] REFUSED authorZone 'Home': the primary Home is "
                     "ensureHomeZone, not an authored extra.\n";
        return nullptr;
    }
    for (const auto& z : _zones) {
        if (z && z->getIdentifier() == id) {
            std::cerr << "[zones] REFUSED authorZone: '" << id << "' already lives here.\n";
            return nullptr;
        }
    }
    if (SaveSystem::zoneIdentityExists(id)) {
        std::cerr << "[zones] REFUSED authorZone: '" << id
                  << "' already has an identity file. Load or fork it.\n";
        return nullptr;
    }

    std::string resolvedKind = ownerKind;
    if (kind == Zone::kCommunityHomeKind || kind == Zone::kCommunityZoneKind) {
        if (resolvedKind.empty()) resolvedKind = Zone::kOwnerKindCommunity;
        if (resolvedKind != Zone::kOwnerKindCommunity) {
            std::cerr << "[zones] REFUSED authorZone '" << id
                      << "': Community Homes/Zones are owned by a Community.\n";
            return nullptr;
        }
        if (ownerId.empty()) {
            std::cerr << "[zones] REFUSED authorZone '" << id
                      << "': a Community Home/Zone needs a Community owner.\n";
            return nullptr;
        }
    }
    if (kind == Zone::kHomeKind && resolvedKind == Zone::kOwnerKindCommunity) {
        std::cerr << "[zones] REFUSED authorZone '" << id
                  << "': a Community dwelling is kind=community-home, not kind=home.\n";
        return nullptr;
    }

    const bool dwelling = (kind == Zone::kHomeKind || kind == Zone::kCommunityHomeKind);
    std::shared_ptr<Zone> zone = dwelling
        ? std::shared_ptr<Zone>(std::make_shared<Home>(id, "strict"))
        : std::make_shared<Zone>(id, "strict");
    if (kind == Zone::kHomeKind) {
        zone->setQuality("kind", Zone::kHomeKind);
    } else if (kind == Zone::kCommunityHomeKind) {
        zone->markCommunityHome();
    } else if (kind == Zone::kCommunityZoneKind) {
        zone->markCommunityZone();
    } else if (!kind.empty() && !dwelling) {
        zone->setQuality("kind", kind);
    }
    if (!ownerId.empty()) {
        if (resolvedKind.empty()) resolvedKind = Zone::kOwnerKindPerson;
        zone->setOwner(ownerId, resolvedKind);
    }
    addZone(zone);
    for (const auto& obj : zone->getOwnedObjects()) {
        if (!obj) continue;
        obj->addZoneDesignation(zone->name());
        obj->addZoneDesignation(zone->getIdentifier());
        globalObjects.push_back(obj);
    }
    persistZones();
    Core::EventBus::instance().publish(
        ECA::Event{"zone-authored", zone.get(), nullptr, std::time(nullptr)});
    return zone;
}

void ZoneManager::updateSaveFiles() {
    _saveLoad.saveDirectory = SaveSystem::ensureSaveTypeFolder(SaveSystem::SaveType::WORLD);
    _saveLoad.files = SaveSystem::listFiles(SaveSystem::SaveType::WORLD);
}

void ZoneManager::setSaveDirectory(const std::string& dir) {
    _saveLoad.saveDirectory = dir;
}

std::string ZoneManager::getSaveDirectory() const {
    return _saveLoad.saveDirectory;
}

// ------------------------------------------------------------------
// Helper function for save/load logging
// ------------------------------------------------------------------
namespace {
void logIo(const std::string& line) {
    std::ofstream log("saves/earthcall-io.log", std::ios::app);
    if (!log) return;
    std::time_t now = std::time(nullptr);
    char stamp[32];
    std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    log << stamp << "  " << line << "\n";
}

nlohmann::json readSaveJsonFile(const std::string& filename) {
    std::filesystem::path path(filename);
    std::string name = path.stem().string();
    std::string gameFolder = SaveSystem::ensureSaveTypeFolder(SaveSystem::SaveType::WORLD);
    std::string unpackedPath = gameFolder + "/" + name + "_unpacked";
    if (SaveSystem::isUnpackedDirectoryNewer(unpackedPath, filename)) {
        nlohmann::json j = SaveSystem::compileSaveFromDirectory(unpackedPath);
        SaveSystem::writeSaveDataAsync(j, name, SaveSystem::SaveType::WORLD);
        std::cout << "[load] Compiled newer unpacked directory back into monolithic save.\n";
        return j;
    }
    return SaveSystem::readSaveData(filename);
}

// Latch: person.position vs camera.pos - eyeHeight.
// LocomotionChannel::step treats a mismatch as a teleport and SNAPS THE
// CAMERA BACK onto the Person. Writers that move the camera without writing
// person.position (this helper's callers: loadState, loadTestObservation)
// look like a no-op — the next frame undoes them. Keep both sides in step.
void settlePersonToCamera(SaveContext& ctx) {
    if (!ctx.person || !ctx.camera) return;
    const float eyeH = ctx.person->getBody().getEyeHeight();
    ctx.person->position() = ctx.camera->pos - glm::vec3(0.0f, eyeH, 0.0f);
    ctx.person->cameraPos = ctx.camera->pos;
    ctx.person->cameraForward = ctx.camera->front;
    ctx.person->velocity() = glm::vec3(0.0f);
    ctx.person->updatePose();
}

void applyLook(SaveContext& ctx, const glm::vec3& eye, const glm::vec3& target) {
    if (!ctx.camera) return;
    ctx.camera->pos = eye;
    glm::vec3 dir = target - eye;
    if (glm::length(dir) < 1e-4f) dir = glm::vec3(0.0f, 0.0f, -1.0f);
    else dir = glm::normalize(dir);
    ctx.camera->front = dir;
    if (ctx.mouseHandler) {
        const float rad2deg = 57.2957795f;
        ctx.mouseHandler->setPitch(std::asin(glm::clamp(dir.y, -0.999f, 0.999f)) * rad2deg);
        ctx.mouseHandler->setYaw(std::atan2(dir.z, dir.x) * rad2deg);
    }
    settlePersonToCamera(ctx);
}

void lookAtWorld(SaveContext& ctx, const Zone& zone) {
    glm::vec3 minP(1e9f), maxP(-1e9f);
    int n = 0;
    for (const auto& obj : zone.getOwnedObjects()) {
        if (!obj) continue;
        const glm::vec3 p = obj->getPosition();
        minP = glm::min(minP, p);
        maxP = glm::max(maxP, p);
        ++n;
    }
    if (n == 0) return;
    const glm::vec3 center = 0.5f * (minP + maxP);
    float radius = 0.5f * glm::length(maxP - minP);
    if (radius < 1.5f) radius = 1.5f;
    const glm::vec3 eye = center + glm::vec3(0.0f, radius * 0.45f + 1.6f, radius * 2.2f + 3.0f);
    applyLook(ctx, eye, center);
}

bool cameraIsDumpDefault(const nlohmann::json& j) {
    if (!j.contains("cameraPos") || !j["cameraPos"].is_array() || j["cameraPos"].size() < 3)
        return true;
    const float x = j["cameraPos"][0].get<float>();
    const float y = j["cameraPos"][1].get<float>();
    const float z = j["cameraPos"][2].get<float>();
    return std::fabs(x) < 1e-4f && std::fabs(y) < 1e-4f && std::fabs(z) < 1e-4f;
}

std::string observationZoneName(const std::string& stem) {
    return "test." + stem;
}

bool isBeforeLoadSnapshot(const std::string& filename) {
    std::error_code ec;
    const auto incoming = std::filesystem::weakly_canonical(std::filesystem::path(filename), ec);
    const auto stash = std::filesystem::weakly_canonical(
        std::filesystem::path(ZoneManager::beforeLoadSnapshotPath()), ec);
    if (!ec && incoming == stash) return true;
    const auto p = std::filesystem::path(filename);
    return p.stem() == "before-load" &&
           p.parent_path().filename() == "backups";
}

std::size_t liveObjectCount(const ZoneManager& mgr) {
    std::size_t n = 0;
    for (const auto& z : mgr.zones()) {
        if (!z) continue;
        n += z->getOwnedObjects().size();
    }
    return n;
}

bool isObservationZone(const Zone& zone) {
    const auto& q = zone.getQualities();
    auto it = q.find("kind");
    return it != q.end() && it->second == "test-observation";
}

// zoneIdFromJson now lives in ZoneSerialization.cpp/.hpp — the single
// shared resolution makeZoneFromJson and every admission check here must
// agree on (see that header's comment for why a second copy is exactly
// how this drifted before).

const char* kZoneIdentityFormat = "zone-identity-v1";

// Keep-live Home objects survive a session load, but FaceTextures live on
// Material beings. If the identity file never carried those materials
// (the pre-embed persist), the object's faceColors are still there and
// the named own-material is gone — the draw path then resolves
// material.default (white). Reinstatement is the same fill from_json
// already does when textures are not already present: not a brush stroke.
void reinstatedMissingOwnMaterials(Zone& zone) {
    for (const auto& obj : zone.getOwnedObjects()) {
        if (!obj) continue;
        const std::string ownId = "material." + obj->getIdentifier();
        if (obj->materialId() != ownId) continue;
        if (materials.get(ownId)) continue;
        const int faces = std::min(obj->getFaces() > 0 ? obj->getFaces() : 6, 6);
        for (int f = 0; f < faces; ++f) {
            obj->setFaceColor(f,
                              obj->faceColors[f][0],
                              obj->faceColors[f][1],
                              obj->faceColors[f][2]);
        }
    }
}
} // namespace

std::string ZoneManager::beforeLoadSnapshotPath() {
    return SaveSystem::ensureSaveTypeFolder(SaveSystem::SaveType::BACKUP) +
           "/before-load.ecform";
}

void ZoneManager::persistZones() const {
    for (const auto& z : _zones) {
        if (!z) continue;
        if (isObservationZone(*z)) continue;
        const std::string id = z->getIdentifier();
        if (id.empty()) continue;
        // An empty live Zone must not erase a populated identity. Boot
        // mints empty Home/Sanctum; a session save of that empty bag
        // used to be how loading another "world" wiped Home.
        const bool dwelling = z->isHome();
        const bool identityExists = dwelling ? SaveSystem::homeIdentityExists(id)
                                             : SaveSystem::zoneIdentityExists(id);
        if (z->getOwnedObjects().empty() && identityExists) {
            nlohmann::json existing = dwelling ? SaveSystem::readHomeIdentity(id)
                                               : SaveSystem::readZoneIdentity(id);
            std::size_t stored = 0;
            if (existing.is_object()) {
                if (existing.contains("world") && existing["world"].contains("objects") &&
                    existing["world"]["objects"].is_array()) {
                    stored = existing["world"]["objects"].size();
                } else if (existing.contains("objects") && existing["objects"].is_array()) {
                    stored = existing["objects"].size();
                }
            }
            if (stored > 0) {
                std::cerr << "[zones] REFUSED to persist empty "
                          << (dwelling ? "Home" : "Zone") << " '" << id
                          << "' over a stored identity that still has "
                          << stored << " being(s).\n";
                continue;
            }
        }
        nlohmann::json doc = zoneToJson(*z);
        nlohmann::json priorIdentity;
        if (identityExists) {
            priorIdentity = dwelling ? SaveSystem::readHomeIdentity(id)
                                     : SaveSystem::readZoneIdentity(id);
            // lawRefs are authored Zone membership, not a projection of the
            // currently global Law register. Preserve them across an ordinary
            // Zone save until an authoring surface explicitly changes them.
            if (priorIdentity.contains("lawRefs")) {
                doc["lawRefs"] = priorIdentity["lawRefs"];
            }
        }

        // Bug #7's guard of last resort: never stamp an empty relation
        // graph or lexeme set over a stored identity that still holds one.
        // Fixes above should make the live Zone's graph correct before it
        // gets here, but this is the check that would have caught the loss
        // the day it happened, so it stays even if it now looks redundant.
        if (identityExists) {
            nlohmann::json existingGraph = priorIdentity;
            if (existingGraph.is_object()) {
                const std::size_t storedRelations =
                    existingGraph.value("formationRelations", nlohmann::json::array()).size();
                const std::size_t storedLexemes =
                    existingGraph.value("lexemes", nlohmann::json::array()).size();
                const std::size_t docRelations =
                    doc.value("formationRelations", nlohmann::json::array()).size();
                const std::size_t docLexemes =
                    doc.value("lexemes", nlohmann::json::array()).size();
                if ((docRelations == 0 && storedRelations > 0) ||
                    (docLexemes == 0 && storedLexemes > 0)) {
                    std::cerr << "[zones] REFUSED to persist "
                              << (dwelling ? "Home" : "Zone") << " '" << id
                              << "': live formation has " << docRelations
                              << " relation(s)/" << docLexemes << " lexeme(s), "
                              << "stored identity has " << storedRelations
                              << " relation(s)/" << storedLexemes << " lexeme(s). "
                              << "Protected the stored graph; nothing written.\n";
                    continue;
                }
            }
        }

        const bool wrote = dwelling ? SaveSystem::writeHomeIdentity(id, doc)
                                    : SaveSystem::writeZoneIdentity(id, doc);
        if (!wrote) {
            std::cerr << "[zones] REFUSED or failed to persist "
                      << (dwelling ? "Home" : "Zone") << " '" << id << "'\n";
        } else {
            if (_lawManager && doc.contains("lawRefs") && doc["lawRefs"].is_array()) {
                for (const auto& refJson : doc["lawRefs"]) {
                    if (!refJson.is_string()) continue;
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
                        std::cerr << "[zones] REFUSED or failed to persist shared Law '"
                                  << lawId << "' named by Zone '" << id << "'.\n";
                    }
                }
            }
            std::size_t paintedObjects = 0;
            std::size_t totalFaceTextures = 0;
            for (const auto& obj : z->getOwnedObjects()) {
                if (!obj) continue;
                if (auto mat = materials.get(obj->materialId())) {
                    if (!mat->faceTextures.empty()) {
                        ++paintedObjects;
                        totalFaceTextures += mat->faceTextures.size();
                    }
                }
            }
            std::size_t missingMaterials = 0;
            for (const auto& obj : z->getOwnedObjects()) {
                if (!obj || obj->materialId().empty()) continue;
                if (!materials.get(obj->materialId())) ++missingMaterials;
            }
            logIo("PERSIST " + std::string(dwelling ? "Home '" : "Zone '") + id + "': " +
                  std::to_string(z->getOwnedObjects().size()) + " object(s), " +
                  std::to_string(paintedObjects) + " painted object(s), " +
                  std::to_string(totalFaceTextures) + " face texture(s)" +
                  (missingMaterials ? (", " + std::to_string(missingMaterials) +
                                       " object(s) name a material that is not in the registry")
                                    : ""));
        }
    }
}

void ZoneManager::hydrateFromZoneStore() {
    // Invariant 6, storage boundary (Sol, agent intercom "Basic Pixel
    // Changer Zone Identity Bug 9-7-26", 2026-09-09, Stage 1): the
    // directory key each identity was actually enumerated under must stay
    // explicit all the way through admission — never re-derived from the
    // document's own content, which can diverge from the folder it lives
    // in (saves/zones/BasicPixelChanger/ once carried a document identifier
    // of "Basic Pixel Changer", a space-containing display string; fixed
    // 2026-09-09 with Zach's authorization, but nothing structurally
    // prevented a future save from drifting the same way again).
    //
    // Two checks, both refuse before ANY live Zone is constructed — zero
    // writes, no phantom live Zone, per Sol's acceptance criterion:
    //   1. the document's own resolved identity must equal the directory
    //      key it was read from;
    //   2. two different directory entries may not both claim the same
    //      stable identity.
    // Neither check picks a "first winner" by iteration/directory order —
    // that is exactly the last-record-wins shape Sol's invariants forbid
    // elsewhere. A duplicate identity refuses EVERY directory claiming it,
    // the same way a duplicate composite matter key refuses every entity
    // sharing it (matter_semantic_precedence_test.cpp).
    const auto homeRecords = SaveSystem::listHomeIdentityRecords();
    const auto zoneRecords = SaveSystem::listZoneIdentityRecords();

    struct Claimant { std::string path; std::string directoryKey; };
    std::unordered_map<std::string, std::vector<Claimant>> claimants; // identity -> [claimant,...]
    auto validate = [&](const std::string& directoryKey, const nlohmann::json& zj, const std::string& path) {
        if (!zj.is_object() || directoryKey.empty()) return;
        const std::string documentIdentity = zoneIdFromJson(zj);
        if (documentIdentity.empty()) {
            std::cerr << "[ZoneManager] hydrateFromZoneStore: REFUSED '" << path
                      << "' (directory key '" << directoryKey << "') — the document names no "
                         "identifier or name at all. Zero writes, no phantom live Zone.\n";
            return;
        }
        if (documentIdentity != directoryKey) {
            std::cerr << "[ZoneManager] hydrateFromZoneStore: REFUSED '" << path
                      << "' — directory key '" << directoryKey << "' does not match the "
                         "document's own identifier ('" << documentIdentity << "'). Zero writes, "
                         "no phantom live Zone. This is a save-file inconsistency and needs a "
                         "Person-authorized repair (edit the file's \"identifier\" field, or "
                         "rename the folder, so the two agree) — never auto-derived or "
                         "auto-renamed.\n";
            return;
        }
        claimants[documentIdentity].push_back(Claimant{path, directoryKey});
    };
    for (const auto& rec : homeRecords) {
        validate(rec.directoryKey, rec.document,
                 SaveSystem::homeDirectory(rec.directoryKey) + "/home.json");
    }
    for (const auto& rec : zoneRecords) {
        validate(rec.directoryKey, rec.document,
                 SaveSystem::zoneDirectory(rec.directoryKey) + "/zone.json");
    }

    std::unordered_set<std::string> admittable;
    for (const auto& [identity, claimList] : claimants) {
        if (claimList.size() == 1) {
            admittable.insert(identity);
            continue;
        }
        std::string listed;
        for (std::size_t i = 0; i < claimList.size(); ++i) {
            if (i) listed += ", ";
            listed += "'" + claimList[i].path + "'";
        }
        std::cerr << "[ZoneManager] hydrateFromZoneStore: REFUSED identity '" << identity
                  << "' — claimed by " << claimList.size() << " identity records ("
                  << listed << "). Zero writes, no phantom live Zone; refusing all of "
                     "them rather than guessing which is authoritative.\n";
    }

    auto admit = [&](const std::string& id, const nlohmann::json& zj) {
        std::shared_ptr<Zone> live;
        for (auto& z : _zones) {
            if (z && z->getIdentifier() == id) {
                live = z;
                break;
            }
        }
        if (live) {
            // Objects already here stay (keep-live). Still merge this
            // identity's materials so FaceTextures are not a leftover
            // from whatever session was last loaded.
            if (zj.contains("materials")) materials.mergeFromJson(zj["materials"]);
            if (live->getOwnedObjects().empty()) applyZoneJson(*live, zj, true);
            else reinstatedMissingOwnMaterials(*live);
        } else {
            live = makeZoneFromJson(zj);
            addZone(live);
        }
        if (live) {
            std::size_t paintedObjects = 0;
            std::size_t totalFaceTextures = 0;
            for (const auto& obj : live->getOwnedObjects()) {
                if (!obj) continue;
                if (auto mat = materials.get(obj->materialId())) {
                    if (!mat->faceTextures.empty()) {
                        ++paintedObjects;
                        totalFaceTextures += mat->faceTextures.size();
                    }
                }
            }
            logIo("HYDRATE Zone '" + id + "': " +
                  std::to_string(live->getOwnedObjects().size()) + " object(s), " +
                  std::to_string(paintedObjects) + " painted object(s), " +
                  std::to_string(totalFaceTextures) + " face texture(s)");
        }
    };
    // Homes first: dwelling memory lives under saves/homes/, not zones/.
    // Each record's directoryKey IS its identity here — validate() already
    // refused anything where that wasn't true, so no re-derivation from
    // document content happens at admission time either.
    for (const auto& rec : homeRecords) {
        if (admittable.count(rec.directoryKey)) admit(rec.directoryKey, rec.document);
    }
    for (const auto& rec : zoneRecords) {
        if (admittable.count(rec.directoryKey)) admit(rec.directoryKey, rec.document);
    }
    globalObjects.clear();
    for (const auto& z : _zones) {
        if (!z) continue;
        for (const auto& obj : z->getOwnedObjects()) {
            if (!obj) continue;
            obj->addZoneDesignation(z->name());
            obj->addZoneDesignation(z->getIdentifier());
            globalObjects.push_back(obj);
        }
    }
}

bool ZoneManager::forkZone(const std::string& sourceId, const std::string& newId) {
    if (sourceId.empty() || newId.empty() || sourceId == newId) return false;
    if (SaveSystem::sanitizeLabel(newId).empty()) return false;
    for (const auto& z : _zones) {
        if (z && z->getIdentifier() == newId) {
            std::cerr << "[zones] REFUSED fork: '" << newId << "' already lives here.\n";
            return false;
        }
    }
    persistZones();
    nlohmann::json src;
    for (const auto& z : _zones) {
        if (z && z->getIdentifier() == sourceId) {
            src = zoneToJson(*z);
            break;
        }
    }
    if (src.is_null() || src.empty()) {
        src = SaveSystem::readHomeIdentity(sourceId);
        if (src.is_null() || src.empty()) src = SaveSystem::readZoneIdentity(sourceId);
    }
    if (!src.is_object()) {
        std::cerr << "[zones] REFUSED fork: source '" << sourceId << "' not found.\n";
        return false;
    }
    src["name"] = newId;
    src["identifier"] = newId;
    nlohmann::json qualities = src.value("qualities", nlohmann::json::object());
    qualities["forkedFrom"] = sourceId;
    // A fork of the primary Home is an extra dwelling, not a second lock.
    qualities.erase("primary");
    src["qualities"] = qualities;
    src["primary"] = false;
    const bool dwelling = src.value("being", std::string{}) == "home"
        || qualities.value("kind", std::string{}) == Zone::kHomeKind
        || qualities.value("kind", std::string{}) == Zone::kCommunityHomeKind;
    const bool wrote = dwelling ? SaveSystem::writeHomeIdentity(newId, src)
                                : SaveSystem::writeZoneIdentity(newId, src);
    if (!wrote) return false;
    auto forked = makeZoneFromJson(src);
    addZone(forked);
    for (const auto& obj : forked->getOwnedObjects()) {
        if (!obj) continue;
        obj->addZoneDesignation(forked->name());
        obj->addZoneDesignation(forked->getIdentifier());
        globalObjects.push_back(obj);
    }
    return true;
}

nlohmann::json ZoneManager::diffZones(const std::string& aId, const std::string& bId) const {
    auto objectIds = [this](const std::string& id) -> std::set<std::string> {
        std::set<std::string> ids;
        for (const auto& z : _zones) {
            if (!z || z->getIdentifier() != id) continue;
            for (const auto& o : z->getOwnedObjects()) {
                if (o) ids.insert(o->getIdentifier());
            }
            return ids;
        }
        nlohmann::json zj = SaveSystem::readZoneIdentity(id);
        if (zj.is_object()) {
            const nlohmann::json* arr = nullptr;
            if (zj.contains("world") && zj["world"].contains("objects"))
                arr = &zj["world"]["objects"];
            else if (zj.contains("objects"))
                arr = &zj["objects"];
            if (arr && arr->is_array()) {
                for (const auto& o : *arr) {
                    if (o.contains("objectID")) ids.insert(o["objectID"].get<std::string>());
                    else if (o.contains("identifier")) ids.insert(o["identifier"].get<std::string>());
                }
            }
        }
        return ids;
    };
    const auto a = objectIds(aId);
    const auto b = objectIds(bId);
    nlohmann::json d;
    d["a"] = aId;
    d["b"] = bId;
    d["onlyInA"] = nlohmann::json::array();
    d["onlyInB"] = nlohmann::json::array();
    d["shared"] = nlohmann::json::array();
    for (const auto& id : a) {
        if (b.count(id)) d["shared"].push_back(id);
        else d["onlyInA"].push_back(id);
    }
    for (const auto& id : b) {
        if (!a.count(id)) d["onlyInB"].push_back(id);
    }
    return d;
}

// ------------------------------------------------------------------
// buildSaveJson - moved from Game
// ------------------------------------------------------------------
nlohmann::json ZoneManager::buildSaveJson(const SaveContext& ctx) const {
    using json = nlohmann::json;
    json j;

    j["saveFormat"] = kZoneIdentityFormat;
    j["currentZone"] = _currentIndex;
    if (_currentIndex < _zones.size() && _zones[_currentIndex]) {
        j["currentZoneId"] = _zones[_currentIndex]->getIdentifier();
    }
    json zonesJson = json::array();
    json zoneRefs = json::array();
    for (const auto& z : _zones) {
        if (!z) continue;
        json zj = zoneToJson(*z);
        zonesJson.push_back(zj);
        json ref;
        ref["identifier"] = z->getIdentifier();
        const auto& q = z->getQualities();
        auto kind = q.find("kind");
        if (kind != q.end()) ref["kind"] = kind->second;
        zoneRefs.push_back(std::move(ref));
    }
    writeSemanticRoots(j, std::move(zonesJson), std::move(zoneRefs),
                       ctx.person, ctx.ourverse);

    j["materials"] = materials.toJson();
    j["categories"] = categories.toJson();

    // Camera and player view - accessed through SaveContext
    j["cameraPos"]   = {ctx.camera->pos.x, ctx.camera->pos.y, ctx.camera->pos.z};
    j["cameraFront"] = {ctx.camera->front.x, ctx.camera->front.y, ctx.camera->front.z};
    j["cameraUp"]    = {ctx.camera->up.x, ctx.camera->up.y, ctx.camera->up.z};
    j["yaw"]   = ctx.mouseHandler->getYaw();
    j["pitch"] = ctx.mouseHandler->getPitch();

    j["currentColor"] = {ctx.currentColor[0], ctx.currentColor[1], ctx.currentColor[2]};

    // Save physics laws
    {
        json lawsJ = json::array();
        for (const auto& law : Physics::getLaws()) {
            json lj;
            lj["id"] = law.id; lj["name"] = law.name;
            lj["type"] = static_cast<int>(law.type); lj["enabled"] = law.enabled;
            lj["strength"] = law.strength; lj["damping"] = law.damping;
            lj["direction"] = {law.direction.x, law.direction.y, law.direction.z};
            const auto& t = law.target;
            json tj;
            tj["allObjects"] = t.allObjects;
            tj["limitByGeometry"] = t.limitByGeometry;
            tj["limitBySpatialKind"] = t.limitBySpatialKind;
            tj["limitByObjectType"] = t.limitByObjectType;
            tj["limitByAttribute"] = t.limitByAttribute;
            tj["limitByTag"] = t.limitByTag;
            tj["limitByExplicitList"] = t.limitByExplicitList;
            tj["geometryTypes"] = json::array();
            for (auto g : t.geometryTypes) tj["geometryTypes"].push_back(static_cast<int>(g));
            tj["spatialKinds"] = json::array();
            for (auto kind : t.spatialKinds) tj["spatialKinds"].push_back(static_cast<int>(kind));
            tj["objectTypes"] = t.objectTypes;
            tj["attributeKey"] = t.attributeKey;
            tj["attributeValue"] = t.attributeValue;
            tj["tag"] = t.tag;
            tj["objectIdentifiers"] = t.objectIdentifiers;
            lj["target"] = tj;
            lawsJ.push_back(lj);
        }
        j["physicsLaws"] = lawsJ;
    }
    j["flying"] = Physics::getFlying();

    // Authored register
    j["authoredLaws"] = ctx.lawManager->toJson();
    j["concepts"] = ConceptRegistry::instance().toJson();
    j["transferPolicy"] = TransferPolicy::instance().toJson();
    j["mathFunctions"] = OntoMath::FunctionRegistry::instance().toJson();
    j["worldTime"] = *ctx.worldTime;

    return j;
}

// ------------------------------------------------------------------
// Matter generation coupling (Sol, Invariant 4 — agent intercom "Basic
// Pixel Changer Zone Identity Bug 9-7-26", 2026-09-08/09):
//
// "generations are atomic": the semantic root (.ecform) and physical
// substrate (.ecmatter) must carry the SAME opaque snapshot id, and the
// root must record the matter chunk's hash/length/schema version so a
// half-written or swapped sidecar is detected and refused BEFORE it
// touches any live Zone — never silently applied, never silently
// dropped as "empty".
//
// Content-addressed naming (snapshotId = a prefix of the matter bytes'
// own SHA-256) makes the coupling and the atomicity fall out together:
// the matter file is written under a name nothing else on disk can
// already claim, written+flushed BEFORE the semantic root ever names
// it, and the semantic root's own commit is a single atomic rename —
// so a crash anywhere in this sequence leaves either the previous
// generation (still fully valid, still loadable) or the new one, never
// a root that names matter that was never finished.
//
// Deliberately scoped to the two live save paths, saveState and
// saveStateWithLog — the legacy JSON splitter (loadState's one-time
// migration write) already writes matter before form and is a rarer,
// already-append-only event; extending it is future work, not
// required to close the gap Sol identified. Legacy fixed-name pairs
// (no "matterGeneration" key on the root) remain readable exactly as
// before — this is additive, not a forced migration of any existing
// save.
namespace {
constexpr int kMatterSchemaVersion = 1;

// Mirrors Singularity::Storage::FileChannel::computeSha256's exact
// OpenSSL + hex-encoding approach. Not called directly: FileChannel is a
// Law (Singularity::Storage), and ZonesOfEarth depending on a Law just to
// borrow a hash utility would be a backward dependency edge for a
// one-function need.
std::string sha256Hex(const std::vector<uint8_t>& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(data.data(), data.size(), hash);
    static const char hexDigits[] = "0123456789abcdef";
    std::string out;
    out.reserve(SHA256_DIGEST_LENGTH * 2);
    for (unsigned char byte : hash) {
        out.push_back(hexDigits[(byte >> 4) & 0x0F]);
        out.push_back(hexDigits[byte & 0x0F]);
    }
    return out;
}

// Writes `bytes` to `finalPath` via write-temp-then-atomic-rename, so a
// crash mid-write never leaves a truncated file at `finalPath` itself.
// Returns false (finalPath untouched) on any failure.
bool atomicWriteFile(const std::filesystem::path& finalPath, const std::vector<uint8_t>& bytes) {
    std::filesystem::path tmp = finalPath;
    tmp += ".tmp-" + std::to_string(reinterpret_cast<uintptr_t>(&bytes)) + "-" +
           std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    {
        std::ofstream out(tmp, std::ios::binary);
        if (!out.is_open()) return false;
        out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        out.flush();
        if (!out) { std::error_code ec; std::filesystem::remove(tmp, ec); return false; }
    }
    std::error_code ec;
    std::filesystem::rename(tmp, finalPath, ec);
    if (ec) {
        // Cross-device or other rename failure: fall back to copy+remove,
        // still finishing with the destination fully written before any
        // caller can observe it under its final name.
        std::filesystem::copy_file(tmp, finalPath, std::filesystem::copy_options::overwrite_existing, ec);
        std::filesystem::remove(tmp, ec);
        if (ec) return false;
    }
    return true;
}

bool atomicWriteFile(const std::filesystem::path& finalPath, const std::string& text) {
    std::vector<uint8_t> bytes(text.begin(), text.end());
    return atomicWriteFile(finalPath, bytes);
}

// Commits `matterBytes` as a new generation of `ecformPath`'s matter
// sidecar and stamps `j` with the metadata needed to verify it on load.
// Matter is written+flushed FIRST, under a content-addressed name; the
// caller commits the semantic root (which now names this generation)
// LAST. On success, removes the previous generation named in `j`'s prior
// "matterGeneration" (if any and if different) — cleanup happens only
// after the new root's atomic rename has already succeeded, per Sol's
// "keep the prior generation until the new pointer commits."
void commitMatterGeneration(const std::filesystem::path& ecformPath,
                             const std::vector<uint8_t>& matterBytes,
                             nlohmann::json& j) {
    if (matterBytes.empty()) return;

    const std::string hash = sha256Hex(matterBytes);
    const std::string snapshotId = hash.substr(0, 16);
    const std::string stem = ecformPath.stem().string();
    const std::filesystem::path matterPath =
        ecformPath.parent_path() / (stem + "." + snapshotId + ".ecmatter");

    std::error_code ec;
    if (!std::filesystem::exists(matterPath, ec)) {
        // Content-addressed: if a prior save already produced byte-identical
        // matter, its generation file is already correct and untouched.
        if (!atomicWriteFile(matterPath, matterBytes)) {
            std::cerr << "[ZoneManager] commitMatterGeneration: failed to write "
                      << matterPath << " — leaving prior generation as the "
                      << "semantic root's committed reference.\n";
            return;
        }
    }

    // Read the CURRENT on-disk root's prior generation (not `j`, which for
    // saveStateWithLog is built fresh each call and never carries one) so
    // cleanup targets the actual predecessor, not this call's own value.
    std::string previousGenerationId;
    {
        std::error_code readEc;
        if (std::filesystem::exists(ecformPath, readEc)) {
            std::ifstream in(ecformPath);
            if (in.is_open()) {
                try {
                    nlohmann::json prior = nlohmann::json::parse(in, nullptr, false);
                    if (!prior.is_discarded() && prior.contains("matterGeneration")) {
                        previousGenerationId = prior["matterGeneration"].value("snapshotId", std::string{});
                    }
                } catch (...) { /* malformed prior root: nothing to clean up */ }
            }
        }
    }

    j["matterGeneration"] = {
        {"snapshotId", snapshotId},
        {"sha256", hash},
        {"byteLength", matterBytes.size()},
        {"schemaVersion", kMatterSchemaVersion}
    };

    if (!previousGenerationId.empty() && previousGenerationId != snapshotId) {
        std::filesystem::path oldMatterPath =
            ecformPath.parent_path() / (stem + "." + previousGenerationId + ".ecmatter");
        std::error_code rmEc;
        std::filesystem::remove(oldMatterPath, rmEc);
        // Not finding it is fine (already cleaned, or the root predates
        // generation coupling); a real removal failure is logged, not fatal —
        // an orphaned old generation is disk waste, not a correctness bug.
        if (rmEc && std::filesystem::exists(oldMatterPath)) {
            std::cerr << "[ZoneManager] commitMatterGeneration: could not remove "
                      << "superseded generation " << oldMatterPath << ": " << rmEc.message() << "\n";
        }
    }
}

// Read-side counterpart of commitMatterGeneration. `j` is the already-
// parsed semantic root. Returns the verified matter bytes, or empty if
// `j` names no generation at all (the legacy-compat signal — caller
// falls back to the fixed-name .ecmatter path unchanged). Refuses (logs,
// returns empty, mutates nothing) on any missing/mismatched/truncated/
// future-schema sidecar — the world is never partially hydrated from a
// generation that failed verification.
std::vector<uint8_t> readVerifiedMatterGeneration(const std::filesystem::path& ecformPath,
                                                   const nlohmann::json& j) {
    if (!j.contains("matterGeneration")) return {};
    const auto& gen = j["matterGeneration"];
    const std::string snapshotId = gen.value("snapshotId", std::string{});
    const std::string expectedHash = gen.value("sha256", std::string{});
    const std::size_t expectedLength = gen.value("byteLength", std::size_t{0});
    const int schemaVersion = gen.value("schemaVersion", 0);

    if (snapshotId.empty()) {
        std::cerr << "[ZoneManager] readVerifiedMatterGeneration: '" << ecformPath
                  << "' names a matterGeneration with no snapshotId — refusing.\n";
        return {};
    }
    if (schemaVersion > kMatterSchemaVersion) {
        std::cerr << "[ZoneManager] readVerifiedMatterGeneration: '" << ecformPath
                  << "' names matter schema version " << schemaVersion
                  << ", newer than this build understands (" << kMatterSchemaVersion
                  << ") — refusing rather than misreading it.\n";
        return {};
    }

    const std::filesystem::path matterPath =
        ecformPath.parent_path() / (ecformPath.stem().string() + "." + snapshotId + ".ecmatter");
    std::error_code ec;
    if (!std::filesystem::exists(matterPath, ec)) {
        std::cerr << "[ZoneManager] readVerifiedMatterGeneration: '" << ecformPath
                  << "' names generation '" << snapshotId << "' but " << matterPath
                  << " is missing — refusing to hydrate physical matter.\n";
        return {};
    }

    std::ifstream in(matterPath.string(), std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "[ZoneManager] readVerifiedMatterGeneration: could not open " << matterPath << "\n";
        return {};
    }
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    if (bytes.size() != expectedLength) {
        std::cerr << "[ZoneManager] readVerifiedMatterGeneration: " << matterPath
                  << " is " << bytes.size() << " bytes, root expected " << expectedLength
                  << " — truncated or swapped sidecar, refusing.\n";
        return {};
    }
    const std::string actualHash = sha256Hex(bytes);
    if (actualHash != expectedHash) {
        std::cerr << "[ZoneManager] readVerifiedMatterGeneration: " << matterPath
                  << " hash mismatch (root names " << expectedHash << ", file hashes to "
                  << actualHash << ") — refusing.\n";
        return {};
    }
    return bytes;
}
} // namespace

// ------------------------------------------------------------------
// saveState
// ------------------------------------------------------------------
void ZoneManager::saveState(const std::string& filename, SaveContext& ctx) {
    // A before-load stash is a snapshot of the previous session, not an
    // evolution of Zone identity. Writing it into saves/zones/ would let a
    // load rewind Home as a side effect.
    if (!isBeforeLoadSnapshot(filename)) persistZones();
    nlohmann::json j = buildSaveJson(ctx);

    std::filesystem::path p(filename);
    if (!isBeforeLoadSnapshot(filename) && p.extension() != ".ecform") {
        p.replace_extension(".ecform");
    }

    // Invariant 4 (Sol): matter is written+flushed under its own
    // content-addressed name BEFORE the semantic root commits, and the
    // root's own commit is the atomic rename below — never a plain
    // ofstream that a crash mid-write can leave truncated in place.
    if (!isBeforeLoadSnapshot(filename)) {
        std::vector<uint8_t> matter = buildMatterFlatBuffer();
        commitMatterGeneration(p, matter, j);
    }

    if (!atomicWriteFile(p, j.dump(2))) {
        std::cerr << "[ZoneManager] saveState: failed to commit " << p << "\n";
        return;
    }

    logIo("SAVE " + p.string() + ": " +
          std::to_string(ctx.lawManager->getAll().size()) + " law(s), " +
          std::to_string(ConceptRegistry::instance().getAll().size()) + " concept(s)");
}

// ------------------------------------------------------------------
// saveStateWithLog
// ------------------------------------------------------------------
void ZoneManager::saveStateWithLog(const std::string& customName, SaveContext& ctx) {
    persistZones();
    nlohmann::json j = buildSaveJson(ctx);

    // Top-level objects: the active zone's whole world.
    auto& zone = active();
    nlohmann::json objArr = nlohmann::json::array();
    for (const auto& o : zone.getOwnedObjects()) {
        if (!o) continue;
        nlohmann::json oj = *o;
        objArr.push_back(std::move(oj));
    }
    j["objects"] = objArr;

    // Use SaveSystem to write the files
    std::string actualName = customName;
    if (actualName.empty()) {
        if (!_saveLoad.loadedSaveName.empty()) {
            actualName = SaveSystem::timestamp() + "_" + _saveLoad.loadedSaveName;
        } else {
            actualName = SaveSystem::timestamp() + "_QuickSave";
        }
    }
    
    // Invariant 4 (Sol): the matter generation must be written+flushed
    // and named inside `j` BEFORE the semantic root commits, not after —
    // resolve the .ecform's destination path the same way writeSaveData
    // will (same helper, same sanitized label/folder) so the matter file
    // lands beside it under the correct stem.
    const std::string ecformPath = SaveSystem::makeFilename(actualName, SaveSystem::SaveType::WORLD, ".ecform");
    if (!ecformPath.empty()) {
        std::vector<uint8_t> matterBuffer = buildMatterFlatBuffer();
        commitMatterGeneration(std::filesystem::path(ecformPath), matterBuffer, j);
    }

    // Semantic Text Substrate (.ecform), now carrying matterGeneration
    // metadata for whatever matter was committed above.
    const std::string path = SaveSystem::writeSaveData(j, actualName, SaveSystem::SaveType::WORLD);
    if (path.empty()) {
        _saveLoad.lastSaveReport = "Save refused or failed for '" + actualName + "'.";
        logIo("SAVE FAILED '" + actualName + "'");
        return;
    }

    _saveLoad.lastSaveReport = "Wrote " + path;
    _saveLoad.loadedSaveName = actualName;
    if (ctx.unpackForAuthoring) {
        std::string gameFolder = SaveSystem::ensureSaveTypeFolder(SaveSystem::SaveType::WORLD);
        std::string unpackedPath = gameFolder + "/" + actualName + "_unpacked";
        SaveSystem::unpackSaveToDirectory(j, unpackedPath);
        _saveLoad.lastSaveReport += " (unpacked " + unpackedPath + ")";
    }
    
    ECA::Logger::instance().setActiveWorld(actualName);
    logIo("SAVE (log) '" + actualName + "' -> " + path + ": " +
          std::to_string(ctx.lawManager->getAll().size()) + " law(s), " +
          std::to_string(ConceptRegistry::instance().getAll().size()) + " concept(s)");
}

// ------------------------------------------------------------------
// loadState
// ------------------------------------------------------------------
void ZoneManager::loadState(const std::string& filename, SaveContext& ctx) {
    ECA::Logger::instance().setActiveWorld(filename);
    
    std::filesystem::path path(filename);
    std::string name = path.stem().string();
    // Strip a YYYYMMDD_HHMMSS_ prefix if Quick Save stamped one. Do not
    // commit loadedSaveName until the read actually succeeds — a refused
    // load of an empty twin used to retitle the live world as that file.

    // Loading is LOUD: every stage reports, and one stage's failure never
    // silently discards the stages after it (a swallowed exception between
    // the world and the registers once cost a field-test law).
    _saveLoad.lastLoadReport.clear();
    logIo("LOAD begin: " + filename);
    std::string failures;
    const auto stage = [&](const char* name, const std::function<void()>& body) {
        try {
            body();
        } catch (const std::exception& e) {
            failures += std::string(name) + ": " + e.what() + "  ";
            std::cerr << "[load] stage '" << name << "' failed: " << e.what() << "\n";
        }
    };
    try {
        using json = nlohmann::json;
        json j = readSaveJsonFile(filename);
        
        if (j.is_null()) {
            _saveLoad.lastLoadReport = "COULD NOT OPEN OR READ: " + filename;
            std::cerr << "Could not open or read " << filename << "\n";
            logIo("LOAD end:   " + _saveLoad.lastLoadReport);
            return;
        }
        // An empty file, a delta chunk, or a non-world JSON must not clear
        // the live Zones. Callers: AssetsConsole loadWorld (the Person's
        // Load / Save Manager). Switching worlds went funky when
        // saves/worlds/ourverse.json (0 bytes) and .ecsave twins were
        // offered as independent worlds and a failed read still replaced
        // nothing — or, worse, a `{}` would have wiped every Zone.
        std::string semanticRootsError;
        const SemanticRootsReadResult semanticRoots = materializeSemanticRoots(j, &semanticRootsError);
        if (semanticRoots == SemanticRootsReadResult::Malformed) {
            _saveLoad.lastLoadReport = "REFUSED: '" + filename +
                "' advertises malformed semantic roots: " + semanticRootsError +
                ". The current world was not replaced.";
            std::cerr << "[load] " << _saveLoad.lastLoadReport << "\n";
            logIo("LOAD end:   " + _saveLoad.lastLoadReport);
            return;
        }

        const bool looksLikeWorld =
            j.is_object() &&
            ((j.contains("zones") && j["zones"].is_array()) ||
             (j.contains("objects") && j["objects"].is_array()) ||
             (j.contains("zoneRefs") && j["zoneRefs"].is_array()) ||
             j.value("saveFormat", std::string{}) == kZoneIdentityFormat);
        if (!looksLikeWorld) {
            _saveLoad.lastLoadReport =
                "REFUSED: '" + filename + "' is not a world save (no zones, no objects). "
                "The current world was not replaced.";
            std::cerr << "[load] " << _saveLoad.lastLoadReport << "\n";
            logIo("LOAD end:   " + _saveLoad.lastLoadReport);
            return;
        }

        ObjectIdentity::reportAndResetVolatileCount();

        // Preserve unsaved live work BEFORE zonesVec.clear(). The CRITICAL
        // save-system fear: load used to erase the present world with no
        // copy. The dedicated slot is backups/before-load.json — one place,
        // overwritten each load, recoverable by loading that path. Skip when
        // the incoming file IS that slot, or recovery would stash the loaded
        // world on top of the unsaved one. Skip when there is nothing to
        // keep (empty boot). If the write fails, refuse the load rather than
        // overwrite anyway.
        std::string preservedPath;
        if (!isBeforeLoadSnapshot(filename) && liveObjectCount(*this) > 0) {
            const std::string stash = beforeLoadSnapshotPath();
            if (stash.empty()) {
                _saveLoad.lastLoadReport =
                    "REFUSED load: could not create the before-load save zone. "
                    "The current world was not replaced.";
                std::cerr << "[load] " << _saveLoad.lastLoadReport << "\n";
                logIo("LOAD end:   " + _saveLoad.lastLoadReport);
                return;
            }
            saveState(stash, ctx);
            std::filesystem::path p(stash);
            std::string actualEcform = stash;
            if (p.extension() != ".ecform") {
                actualEcform = (p.parent_path() / (p.stem().string() + ".ecform")).string();
            }
            std::ifstream probe(actualEcform);
            if (!probe) {
                _saveLoad.lastLoadReport =
                    "REFUSED load: could not preserve unsaved work to " + actualEcform +
                    ". The current world was not replaced.";
                std::cerr << "[load] " << _saveLoad.lastLoadReport << "\n";
                logIo("LOAD end:   " + _saveLoad.lastLoadReport);
                return;
            }
            preservedPath = actualEcform;
            logIo("PRESERVE unsaved -> " + actualEcform);
        }

        // Reset physics registries
        Physics::resetRigidBodies();
        Physics::clearBonds();

        size_t currentZoneIdx = j.value("currentZone", 0);
        std::string currentZoneId = j.value("currentZoneId", std::string{});
        const bool snapshotRestore = isBeforeLoadSnapshot(filename);

        // Materials are beings objects name by id. FaceTextures live on them,
        // not on the Object. A session file still carries a materials bag;
        // REPLACE would wipe paint on a live Home when another save loaded.
        // Snapshot restore rewinds; every other load merges, then Home/Zone
        // identity files re-apply their own surfaces (source of truth).
        if (j.contains("materials")) {
            if (snapshotRestore) materials.loadFromJson(j["materials"]);
            else materials.mergeFromJson(j["materials"]);
        }
        if (j.contains("categories")) categories.loadFromJson(j["categories"]);

        // Snapshot restore (before-load) rewinds the working set from the
        // embedded copy. Every other load treats Zones as identities: a
        // live Zone is kept, else the Zone store, else the snapshot is
        // migrated into the store. Home is never replaced by a session
        // file's private copy — EarthcallOurverse.md: one Singularity-fixed
        // Home, evolved across files, not minted per snapshot.
        auto findLive = [this](const std::string& id) -> std::shared_ptr<Zone> {
            if (id.empty()) return nullptr;
            for (auto& z : _zones) {
                if (z && z->getIdentifier() == id) return z;
            }
            return nullptr;
        };
        auto admitFromJson = [&](const nlohmann::json& zj) {
            const std::string id = zoneIdFromJson(zj);
            if (id.empty()) return;
            if (auto live = findLive(id)) {
                // A LIVE zone still needs the session's zone JSON merged into
                // it. Boot runs hydrateFromZoneStore() (EngineInit.cpp) before
                // any world is loaded, so every Zone in saves/zones/ is already
                // live by the time a Person clicks Load — and this branch used
                // to return here, which meant the session snapshot was skipped
                // entirely for exactly the Zones a Person keeps coming back to.
                //
                // That mattered most for the relation graph, and it is Bug #7
                // arriving through the door its fix did not cover. Categories
                // are world data: they load in loadState (categories
                // .loadFromJson), NOT at boot. So when hydration binds a Zone's
                // formation relations, "category.chess.piece" does not exist
                // yet, every instance-of edge comes back with an unbound
                // endpoint, and Formation::add REFUSES it. The Chess zone went
                // live with zero relations, and nothing ever tried again:
                // "instance-of category.chess.piece" was false for every piece
                // for the rest of the run, so law-chess-click answered
                // CONDITIONS FAILED on a pawn while still succeeding on the
                // board (whose test is the isBoard property, not a relation).
                //
                // replaceObjects=false: the live objects stay authoritative,
                // exactly as the store-hit branch below keeps the store's.
                // applyFormationRelations is idempotent by type + endpoint ids,
                // so re-running it adds only what is genuinely missing.
                applyZoneJson(*live, zj, /*replaceObjects=*/snapshotRestore);
                // Tried and reverted (2026-09-07): re-running from_json on an
                // object that is already live corrupts it. from_json is only
                // safe on a just-constructed Object — a live one may already
                // carry Rete facts, selection flags, and other runtime state
                // from_json knows nothing about and will not preserve.
                // chess_click_geometry_test caught this immediately: every
                // piece stopped registering isSelected after a click, because
                // reapplying from_json here ran during Chess's own boot
                // hydration + load sequence, which — unlike this file's own
                // tests — actually ticks Laws in between. See faceColors'
                // fix in ObjectSerialization.cpp's to_json instead: the real
                // gap was that field never round-tripping through ANY save at
                // all, so the store itself carries the right value from its
                // very first write and no runtime merge is needed here.
                return;
            }
            if (!snapshotRestore && SaveSystem::zoneIdentityExists(id)) {
                nlohmann::json identity = SaveSystem::readZoneIdentity(id);
                if (identity.is_object()) {
                    auto z = makeZoneFromJson(identity);
                    addZone(z);
                    // Bug #7: the store wins on objects (that is the point
                    // of per-Zone identity), but the store may be missing
                    // things the session snapshot still holds — most
                    // critically the formation relation graph, which used
                    // to have no load path of its own. Merge rather than
                    // discard `zj` whole; replaceObjects=false keeps the
                    // store's objects authoritative.
                    applyZoneJson(*z, zj, /*replaceObjects=*/false);
                    // The store wins per-FIELD, not per-object: a field the
                    // World authors after this identity snapshot was taken —
                    // faceColors added to an object the snapshot predates,
                    // say — must not regress to a hardcoded default just
                    // because the snapshot never recorded it. See
                    // mergeZoneObjectsFromJson's own comment for the mechanism.
                    if (zj.contains("world")) {
                        mergeZoneObjectsFromJson(zj["world"], *z);
                    } else if (zj.contains("objects")) {
                        mergeZoneObjectsFromJson(zj, *z);
                    }
                    return;
                }
            }
            auto z = makeZoneFromJson(zj);
            addZone(z);
            if (!snapshotRestore && !isObservationZone(*z)) {
                // A session's embedded zones[] can equally name a Home
                // (e.g. a first save from a fresh Person, before Home has
                // ever been persisted on its own). Route to the SAME store
                // persistZones() uses — writeZoneIdentity unconditionally
                // here used to leak a Home's first identity write into
                // saves/zones/ instead of saves/homes/, so the SAME "Home"
                // ended up claimed by both stores. Invariant 6's boundary
                // validation (hydrateFromZoneStore) now refuses a stable
                // identity claimed by more than one identity record rather
                // than silently tolerating it, which is what surfaced this.
                if (z->isHome()) {
                    SaveSystem::writeHomeIdentity(id, zoneToJson(*z));
                } else {
                    SaveSystem::writeZoneIdentity(id, zoneToJson(*z));
                }
            }
        };

        if (snapshotRestore) {
            _zones.clear();
        }

        if (j.contains("zones") && j["zones"].is_array()) {
            for (const auto& zj : j["zones"]) admitFromJson(zj);
        }
        if (j.contains("zoneRefs") && j["zoneRefs"].is_array()) {
            for (const auto& ref : j["zoneRefs"]) {
                std::string id;
                if (ref.is_string()) id = ref.get<std::string>();
                else if (ref.is_object()) id = ref.value("identifier", std::string{});
                if (id.empty() || findLive(id)) continue;
                nlohmann::json identity = SaveSystem::readZoneIdentity(id);
                if (identity.is_object()) addZone(makeZoneFromJson(identity));
            }
        }

        if (_zones.empty()) {
            _zones.push_back(std::make_shared<Zone>("Default Zone", "default"));
        }

        if (currentZoneId.empty() && j.contains("zones") && j["zones"].is_array() &&
            currentZoneIdx < j["zones"].size()) {
            currentZoneId = zoneIdFromJson(j["zones"][currentZoneIdx]);
        }

        // saveStateWithLog also writes a top-level objects array. If a
        // zone's world came in empty, fold those into the active zone so
        // a Person's spawned shapes survive the round-trip. Identity-stable
        // Zones that already have beings are left alone.
        if (j.contains("objects") && j["objects"].is_array() && !_zones.empty()) {
            size_t foldIdx = std::min(currentZoneIdx, _zones.size() - 1);
            if (!currentZoneId.empty()) {
                for (size_t i = 0; i < _zones.size(); ++i) {
                    if (_zones[i] && _zones[i]->getIdentifier() == currentZoneId) {
                        foldIdx = i;
                        break;
                    }
                }
            }
            auto& loadZone = *_zones[foldIdx];
            if (loadZone.getOwnedObjects().empty()) {
                zoneObjectsFromJson(j, loadZone);
            }
        }

        if (!snapshotRestore) {
            hydrateFromZoneStore();
            // Home/Zone identity is the surface of truth. Session merge
            // already kept live paint; this re-applies paint from the
            // identity file so a Home's FaceTextures cannot be a different
            // world's leftover bag.
            for (const auto& z : _zones) {
                if (!z) continue;
                const std::string id = z->getIdentifier();
                nlohmann::json idj = z->isHome()
                    ? SaveSystem::readHomeIdentity(id)
                    : SaveSystem::readZoneIdentity(id);
                if (idj.contains("materials"))
                    materials.mergeFromJson(idj["materials"]);
                reinstatedMissingOwnMaterials(*z);
            }
        }
        if (ctx.person) ensureHomeZone(ctx.person->getIdentifier());
        if (snapshotRestore) persistZones();

        // switchTo CLEARS the active world's objects and refills from
        // globalObjects. Load used to skip this catalog, so every successful
        // read then wiped the world. Stamp zone membership and fill the
        // catalog BEFORE switching.
        globalObjects.clear();
        for (const auto& z : _zones) {
            if (!z) continue;
            for (const auto& obj : z->getOwnedObjects()) {
                if (!obj) continue;
                obj->addZoneDesignation(z->name());
                obj->addZoneDesignation(z->getIdentifier());
                globalObjects.push_back(obj);
            }
        }
        size_t switchIdx = std::min(currentZoneIdx, _zones.size() - 1);
        if (!currentZoneId.empty()) {
            for (size_t i = 0; i < _zones.size(); ++i) {
                if (_zones[i] && _zones[i]->getIdentifier() == currentZoneId) {
                    switchIdx = i;
                    break;
                }
            }
        }
        switchTo(switchIdx);

        // A session's camera is the canonical visible pose. Hydrate the
        // Person root (including Body) before reconciling that pose, so the
        // current body eye-height participates in the Person/camera latch.
        // Profiles still use personFromJson directly and retain their own
        // position and velocity without this session-specific reconciliation.
        stage("person", [&] {
            if (ctx.person && j.contains("person")) {
                personFromJson(j["person"], *ctx.person);
            }
        });

        // Player avatar body (legacy bridge). Old sessions have no semantic
        // Person root, so establish their Body before the same camera latch.
        stage("player-body", [&] {
            if (ctx.person && !j.contains("person") && j.contains("playerBody")) {
                bodyFromJson(j["playerBody"], ctx.person->getBody());
            }
        });

        // Load camera and player view
        if (j.contains("cameraPos")) {
            ctx.camera->pos = glm::vec3(j["cameraPos"][0], j["cameraPos"][1], j["cameraPos"][2]);
        }
        if (j.contains("cameraFront")) {
            ctx.camera->front = glm::vec3(j["cameraFront"][0], j["cameraFront"][1], j["cameraFront"][2]);
        }
        if (j.contains("cameraUp")) {
            ctx.camera->up = glm::vec3(j["cameraUp"][0], j["cameraUp"][1], j["cameraUp"][2]);
        }
        ctx.mouseHandler->setYaw(j.value("yaw", -90.0f));
        ctx.mouseHandler->setPitch(j.value("pitch", 0.0f));
        // Engine::update overwrites camera.front from yaw/pitch each frame, so
        // the JSON front is only a hint; the mouse handler is the look office.
        if (ctx.camera) {
            ctx.camera->front = ctx.mouseHandler->calculateCameraFront();
        }
        settlePersonToCamera(ctx);

        if (j.contains("currentColor")) {
            ctx.currentColor[0] = j["currentColor"][0];
            ctx.currentColor[1] = j["currentColor"][1];
            ctx.currentColor[2] = j["currentColor"][2];
        }

        Physics::setFlying(j.value("flying", false));

        // Load physics laws
        stage("physics-laws", [&] {
        if (j.contains("physicsLaws")) {
            std::vector<int> ids;
            for (const auto& law : Physics::getLaws()) ids.push_back(law.id);
            for (int id : ids) Physics::removeLaw(id);
            for (const auto& lj : j["physicsLaws"]) {
                Physics::PhysicsLaw law;
                law.name = lj.value("name", std::string("Law"));
                law.type = static_cast<Physics::LawType>(lj.value("type", 0));
                law.enabled = lj.value("enabled", true);
                law.strength = lj.value("strength", 9.81f);
                law.damping = lj.value("damping", 0.1f);
                auto dir = lj.value("direction", std::vector<float>{0, -1, 0});
                if (dir.size() == 3) law.direction = glm::vec3(dir[0], dir[1], dir[2]);
                const auto& tj = lj["target"];
                law.target.allObjects = tj.value("allObjects", true);
                law.target.limitByGeometry = tj.value("limitByGeometry", false);
                law.target.limitBySpatialKind = tj.value("limitBySpatialKind", false);
                law.target.limitByObjectType = tj.value("limitByObjectType", false);
                law.target.limitByAttribute = tj.value("limitByAttribute", false);
                law.target.limitByTag = tj.value("limitByTag", false);
                law.target.limitByExplicitList = tj.value("limitByExplicitList", false);
                law.target.geometryTypes.clear();
                if (tj.contains("geometryTypes")) {
                    for (const auto& gi : tj["geometryTypes"])
                        law.target.geometryTypes.push_back(static_cast<Object::ShapeKind>(gi.get<int>()));
                }
                law.target.spatialKinds.clear();
                if (tj.contains("spatialKinds")) {
                    for (const auto& ki : tj["spatialKinds"])
                        law.target.spatialKinds.push_back(static_cast<Object::SpatialKind>(ki.get<int>()));
                }
                law.target.objectTypes.clear();
                if (tj.contains("objectTypes")) {
                    for (const auto& s : tj["objectTypes"])
                        law.target.objectTypes.push_back(s.get<std::string>());
                }
                law.target.attributeKey = tj.value("attributeKey", std::string(""));
                law.target.attributeValue = tj.value("attributeValue", std::string(""));
                law.target.tag = tj.value("tag", std::string(""));
                law.target.objectIdentifiers.clear();
                if (tj.contains("objectIdentifiers")) {
                    for (const auto& s : tj["objectIdentifiers"])
                        law.target.objectIdentifiers.push_back(s.get<std::string>());
                }
                Physics::addLaw(law);
            }
        }
        });

        // The authored register
        stage("world-clock", [&] {
            if (ctx.worldTime) {
                *ctx.worldTime = j.value("worldTime", 0.0);
                Universe::instance().setClock(*ctx.worldTime, 0.0);
            }
        });
        stage("concepts", [&] {
            if (j.contains("concepts")) {
                ConceptRegistry::instance().loadFromJson(j["concepts"]);
            }
        });
        stage("transfer-policy", [&] {
            if (j.contains("transferPolicy")) {
                TransferPolicy::instance().loadFromJson(j["transferPolicy"]);
            }
        });
        stage("math-functions", [&] {
            if (j.contains("mathFunctions")) {
                OntoMath::FunctionRegistry::instance().loadFromJson(j["mathFunctions"]);
            }
        });
        stage("authored-laws", [&] {
            if (j.contains("authoredLaws")) {
                ctx.lawManager->loadFromJson(j["authoredLaws"]);
            }

            // Zone graphs hydrate before authored Laws because Objects and
            // categories must exist first. A saved Law -> instance-of ->
            // category edge therefore has one legitimately missing endpoint
            // during that first pass and relation hydration defers it. Re-run the
            // idempotent relation hydration now that the Law register exists.
            // This is what makes authored Law categories persisted ontology,
            // not JSON decoration visible only to a file reader.
            if (!j.contains("zones") || !j["zones"].is_array()) return;
            for (const auto& zoneJson : j["zones"]) {
                const std::string id = zoneIdFromJson(zoneJson);
                if (id.empty()) continue;
                for (const auto& zone : _zones) {
                    if (zone && zone->getIdentifier() == id) {
                        applyFormationRelations(*zone, zoneJson);
                        break;
                    }
                }
            }
        });
        stage("ourverse", [&] {
            if (!ctx.ourverse || !j.contains("ourverse")) return;

            auto resolveZone = [this](const std::string& id) -> std::shared_ptr<Zone> {
                if (id.empty()) return nullptr;
                for (const auto& zone : _zones) {
                    if (zone && zone->getIdentifier() == id) return zone;
                }
                return nullptr;
            };
            auto resolveMember = [&ctx, &resolveZone](const std::string& id) -> Singular* {
                if (id.empty()) return nullptr;
                if (auto zone = resolveZone(id)) return zone.get();
                if (ctx.person && ctx.person->getIdentifier() == id) return ctx.person;
                if (ctx.lawManager) {
                    if (auto* law = ctx.lawManager->find(id)) return law;
                }
                if (auto category = categories.get(id)) return category.get();
                if (auto material = materials.get(id)) return material.get();
                auto& language = Singularity::Language::LanguageSystem::instance();
                if (auto lexeme = language.findById(id)) {
                    return lexeme.get();
                }
                if (auto lexeme = language.findBySymbol(id)) {
                    return lexeme.get();
                }
                for (Singular* being : Universe::instance().beings()) {
                    if (being && being->getIdentifier() == id) return being;
                }
                return nullptr;
            };
            if (!ourverseFromJson(*ctx.ourverse, j["ourverse"],
                                 resolveZone, resolveMember)) {
                throw std::runtime_error("Ourverse root contained no loadable state");
            }
            logIo("Ourverse semantic root hydrated after Zones and laws.");
        });
        stage("physical-matter", [&] {
            // Invariant 4 (Sol): a root that names a matterGeneration must
            // be verified (generation file exists, byte length + sha256 +
            // schema version all match) before its matter is ever applied
            // to a live Zone. Absence of the key is the legacy-compat
            // signal — read the fixed-name .ecmatter exactly as before.
            const bool namesGeneration = j.contains("matterGeneration");
            std::vector<uint8_t> matterBytes = namesGeneration
                ? readVerifiedMatterGeneration(std::filesystem::path(filename), j)
                : SaveSystem::readMatterData(filename);
            if (!matterBytes.empty()) {
                applyMatterFlatBuffer(matterBytes);
                logIo("Physical matter (.ecmatter) hydrated successfully.");
            } else if (namesGeneration) {
                // A named generation that failed verification is refused,
                // not silently re-migrated — falling through to the legacy
                // splitter below would paper over exactly the failure this
                // invariant exists to surface. See stderr for which check
                // failed (missing file, length, hash, or schema version).
                _saveLoad.lastLoadReport = "REFUSED matter generation for '" + filename +
                    "': verification failed (see stderr). Physical matter was not hydrated; "
                    "the current world's physical state for this file was not replaced.";
                std::cerr << "[load] " << _saveLoad.lastLoadReport << "\n";
            } else if (!snapshotRestore && looksLikeWorld) {
                // Legacy JSON splitter: transparent migration on load
                std::filesystem::path p(filename);
                std::string stem = p.stem().string();

                // Invariant 1 (Sol, agent intercom "Basic Pixel Changer Zone
                // Identity Bug 9-7-26", 2026-09-08): scope the freshly-minted
                // matter buffer to exactly the Zones THIS legacy file names
                // (its own "zones"/"zoneRefs"), not every Zone hydrated into
                // _zones this session — boot hydration alone can pull in
                // every Zone under saves/zones/. This is the actual
                // mechanism by which the real basic_pixel_changer.ecmatter
                // reached 1,441 entities: a World naming one Zone, migrated
                // while dozens of unrelated Zones were also live.
                std::unordered_set<std::string> scopeIds;
                if (j.contains("zones") && j["zones"].is_array()) {
                    for (const auto& zj : j["zones"]) {
                        const std::string zid = zoneIdFromJson(zj);
                        if (!zid.empty()) scopeIds.insert(zid);
                    }
                }
                if (j.contains("zoneRefs") && j["zoneRefs"].is_array()) {
                    for (const auto& ref : j["zoneRefs"]) {
                        std::string zid;
                        if (ref.is_string()) zid = ref.get<std::string>();
                        else if (ref.is_object()) zid = ref.value("identifier", std::string{});
                        if (!zid.empty()) scopeIds.insert(zid);
                    }
                }
                // An empty scope means this legacy file named no Zone at
                // all — degenerate, not "fall back to everything": that
                // fallback is exactly the bug this scoping exists to close.
                std::vector<uint8_t> newMatter = scopeIds.empty()
                    ? std::vector<uint8_t>{}
                    : buildMatterFlatBuffer(scopeIds);
                if (!newMatter.empty()) {
                    SaveSystem::writeMatterData(newMatter, stem, SaveSystem::SaveType::WORLD);
                    std::filesystem::path formPath(filename);
                    formPath.replace_extension(".ecform");
                    std::ofstream formOut(formPath);
                    if (formOut) formOut << j.dump(2);
                    logIo("Migrated legacy save '" + filename + "' to split substrate (.ecform + .ecmatter).");
                }
            }
        });

        // Build report
        std::size_t objectCount = 0;
        for (const auto& zone : _zones) {
            objectCount += zone->getOwnedObjects().size();
        }
        std::size_t authoredCount = 0;
        for (const auto& law : ctx.lawManager->getAll()) {
            if (law && law->isAuthored()) ++authoredCount;
        }
        if (name.length() >= 15 && name[8] == '_') {
            _saveLoad.loadedSaveName = (name.length() > 16) ? name.substr(16) : std::string{};
        } else {
            _saveLoad.loadedSaveName = name;
        }
        if (!_saveLoad.loadedSaveName.empty()) {
            std::strncpy(_saveLoad.customName, _saveLoad.loadedSaveName.c_str(), sizeof(_saveLoad.customName) - 1);
            _saveLoad.customName[sizeof(_saveLoad.customName) - 1] = '\0';
        }

        const std::string zoneName = _zones.empty() ? std::string("?") : _zones[_currentIndex]->name();
        _saveLoad.lastLoadReport =
            "Loaded session '" + _saveLoad.loadedSaveName + "' (Zones of Earth stay "
            "identity-stable under saves/zones/). Now in " + zoneName + ": " +
            std::to_string(_zones.size()) + " zone(s), " +
            std::to_string(objectCount) + " object(s), " +
            std::to_string(ctx.lawManager->getAll().size()) + " law(s) (" +
            std::to_string(authoredCount) + " authored), " +
            std::to_string(ConceptRegistry::instance().getAll().size()) +
            " concept(s), worldTime " +
            std::to_string(ctx.worldTime ? *ctx.worldTime : 0.0);
        if (!preservedPath.empty()) {
            _saveLoad.lastLoadReport +=
                "  Unsaved work preserved to " + preservedPath + ".";
        }
        if (!failures.empty()) {
            _saveLoad.lastLoadReport += "  |  FAILED stages: " + failures;
        }
        uint64_t volatileCount = ObjectIdentity::reportAndResetVolatileCount();
        if (volatileCount > 0) {
            _saveLoad.lastLoadReport += " (" + std::to_string(volatileCount) + " beings took volatile IDs)";
        }
        std::cerr << "[load] " << _saveLoad.lastLoadReport << "\n";
        logIo("LOAD end:   " + _saveLoad.lastLoadReport);

    } catch (const std::exception& e) {
        _saveLoad.lastLoadReport = std::string("LOAD FAILED: ") + e.what();
        std::cerr << "Error loading state: " << e.what() << "\n";
        logIo("LOAD end:   " + _saveLoad.lastLoadReport);
    }
}

// ------------------------------------------------------------------
// loadTestObservation
//
// Caller: DeveloperToolsWindow (grave / Toggle Dev Mode).
// Not loadState: that clears _zones and would erase Home, which is the
// CRITICAL fear at the top of the agenda. Observation puts the dump's
// beings into a Zone named test.<stem>, merges missing materials /
// concepts / laws, switches the Person into that Zone, and aims them
// at the cluster so they can see it.
// ------------------------------------------------------------------
void ZoneManager::loadTestObservation(const std::string& filename, SaveContext& ctx) {
    _saveLoad.lastLoadReport.clear();
    logIo("OBSERVE begin: " + filename);
    try {
        using json = nlohmann::json;
        json j = readSaveJsonFile(filename);
        if (j.is_null()) {
            _saveLoad.lastLoadReport = "COULD NOT OPEN OR READ: " + filename;
            std::cerr << "[observe] " << _saveLoad.lastLoadReport << "\n";
            logIo("OBSERVE end: " + _saveLoad.lastLoadReport);
            return;
        }

        const std::string stem = std::filesystem::path(filename).stem().string();
        const std::string zoneName = observationZoneName(stem);

        int materialsAdded = 0;
        if (j.contains("materials") && j["materials"].is_array()) {
            for (const auto& e : j["materials"]) {
                auto m = std::make_shared<Material>(Material::fromJson(e));
                if (!m) continue;
                if (materials.get(m->getIdentifier())) continue;
                materials.add(m);
                ++materialsAdded;
            }
        }

        int conceptsAdded = 0;
        if (j.contains("concepts")) {
            const auto& cj = j["concepts"];
            const auto& arr = (cj.is_object() && cj.contains("concepts")) ? cj["concepts"] : cj;
            if (arr.is_array()) {
                for (const auto& c : arr) {
                    auto concept = ObjectConcept::fromJson(c);
                    if (!concept) continue;
                    if (ConceptRegistry::instance().find(concept->getIdentifier())) continue;
                    ConceptRegistry::instance().add(concept);
                    ++conceptsAdded;
                }
            }
        }

        int lawsAdded = 0;
        int lawsReauthored = 0;
        if (ctx.lawManager && j.contains("authoredLaws")) {
            const auto& al = j["authoredLaws"];
            const auto findBeing = [](const std::string& id) -> Singular* {
                for (Singular* being : Universe::instance().beings()) {
                    if (being && being->getIdentifier() == id) return being;
                }
                return nullptr;
            };
            if (al.contains("laws") && al["laws"].is_array()) {
                for (const auto& lj : al["laws"]) {
                    auto law = Law::fromJson(lj);
                    if (!law) continue;
                    if (ctx.lawManager->find(law->getIdentifier())) continue;
                    if (lj.contains("authors")) {
                        for (const auto& idJson : lj["authors"]) {
                            if (!idJson.is_string()) continue;
                            if (Singular* being = findBeing(idJson.get<std::string>())) {
                                law->addAuthor(*being);
                            }
                        }
                    }
                    if (law->authors().getMembers().empty() && ctx.person) {
                        law->addAuthor(*ctx.person);
                        ++lawsReauthored;
                    }
                    ctx.lawManager->add(law);
                    ++lawsAdded;
                }
            }
            if (al.contains("triggers") && al["triggers"].is_object()) {
                for (auto it = al["triggers"].begin(); it != al["triggers"].end(); ++it) {
                    if (!ctx.lawManager->find(it.key())) continue;
                    for (const auto& type : it.value()) {
                        if (type.is_string()) {
                            ctx.lawManager->bindTrigger(it.key(), type.get<std::string>());
                        }
                    }
                }
            }
        }

        std::shared_ptr<Zone> zone;
        size_t zoneIndex = static_cast<size_t>(-1);
        for (size_t i = 0; i < _zones.size(); ++i) {
            if (_zones[i] && _zones[i]->getIdentifier() == zoneName) {
                zone = _zones[i];
                zoneIndex = i;
                break;
            }
        }
        if (!zone) {
            zone = std::make_shared<Zone>(zoneName, "default", Zone::Scope::Local);
            zone->setQuality("kind", "test-observation");
            addZone(zone);
            zoneIndex = _zones.size() - 1;
        }

        std::unordered_set<Object*> retiring;
        for (const auto& obj : zone->getOwnedObjects()) {
            if (obj) retiring.insert(obj.get());
        }
        zone->getOwnedObjectsMutable().clear();
        globalObjects.erase(
            std::remove_if(globalObjects.begin(), globalObjects.end(),
                           [&](const std::shared_ptr<Object>& obj) {
                               if (!obj) return true;
                               return retiring.count(obj.get()) > 0 || obj->belongsToZone(zoneName);
                           }),
            globalObjects.end());

        if (j.contains("zones") && j["zones"].is_array()) {
            for (const auto& zj : j["zones"]) {
                if (zj.contains("world")) {
                    zoneObjectsFromJson(zj["world"], *zone);
                }
            }
        }
        if (j.contains("objects") && j["objects"].is_array() &&
            zone->getOwnedObjects().empty()) {
            zoneObjectsFromJson(j, *zone);
        }

        for (const auto& obj : zone->getOwnedObjects()) {
            if (!obj) continue;
            obj->addZoneDesignation(zone->name());
            obj->addZoneDesignation(zone->getIdentifier());
            globalObjects.push_back(obj);
        }

        // Step 2: Physical matter injection (.ecmatter FlatBuffer).
        // Invariant 4 (Sol): honor generation metadata here too, since this
        // file may equally have been written by saveState/saveStateWithLog.
        std::vector<uint8_t> matterBytes = j.contains("matterGeneration")
            ? readVerifiedMatterGeneration(std::filesystem::path(filename), j)
            : SaveSystem::readMatterData(filename);
        if (!matterBytes.empty()) {
            applyMatterFlatBuffer(matterBytes);
        }

        switchTo(zoneIndex);

        const std::size_t objectCount = zone->getOwnedObjects().size();
        if (objectCount > 0 && cameraIsDumpDefault(j)) {
            lookAtWorld(ctx, *zone);
        } else if (j.contains("cameraPos") && ctx.camera) {
            ctx.camera->pos = glm::vec3(j["cameraPos"][0], j["cameraPos"][1], j["cameraPos"][2]);
            if (ctx.mouseHandler) {
                ctx.mouseHandler->setYaw(j.value("yaw", -90.0f));
                ctx.mouseHandler->setPitch(j.value("pitch", 0.0f));
                ctx.camera->front = ctx.mouseHandler->calculateCameraFront();
            }
            settlePersonToCamera(ctx);
            if (objectCount > 0) {
                // Even a non-default dump camera may be looking past the
                // cluster (eye at y=0, cubes at y=2). If the camera is more
                // than a few metres from the cluster, aim at it.
                glm::vec3 minP(1e9f), maxP(-1e9f);
                for (const auto& obj : zone->getOwnedObjects()) {
                    if (!obj) continue;
                    const glm::vec3 p = obj->getPosition();
                    minP = glm::min(minP, p);
                    maxP = glm::max(maxP, p);
                }
                const glm::vec3 center = 0.5f * (minP + maxP);
                if (glm::distance(ctx.camera->pos, center) > 12.0f) {
                    lookAtWorld(ctx, *zone);
                }
            }
        } else if (objectCount > 0) {
            lookAtWorld(ctx, *zone);
        }

        _saveLoad.lastLoadReport =
            "Observing '" + zoneName + "': " +
            std::to_string(objectCount) + " object(s) in the active Zone. Home is still here. " +
            std::to_string(materialsAdded) + " material(s) merged, " +
            std::to_string(conceptsAdded) + " concept(s) merged, " +
            std::to_string(lawsAdded) + " law(s) added";
        if (lawsReauthored > 0) {
            _saveLoad.lastLoadReport +=
                " (" + std::to_string(lawsReauthored) +
                " re-authored onto this Person so they can fire)";
        }
        _saveLoad.lastLoadReport += ".";
        if (objectCount == 0) {
            _saveLoad.lastLoadReport +=
                " This dump has no objects — it is a law seed. Load a *_final.json "
                "to see spawned beings, or arm the loaded law and click.";
        } else {
            _saveLoad.lastLoadReport +=
                " Close this window and look around; you are facing the loaded beings.";
        }
        std::cerr << "[observe] " << _saveLoad.lastLoadReport << "\n";
        logIo("OBSERVE end: " + _saveLoad.lastLoadReport);
    } catch (const std::exception& e) {
        _saveLoad.lastLoadReport = std::string("OBSERVE FAILED: ") + e.what();
        std::cerr << "[observe] " << _saveLoad.lastLoadReport << "\n";
        logIo("OBSERVE end: " + _saveLoad.lastLoadReport);
    }
}

static glm::mat4 vectorToMat4(const std::vector<float>& v){
    glm::mat4 m(1.0f);
    if(v.size()==16){ std::memcpy(glm::value_ptr(m), v.data(), sizeof(float)*16); }
    return m;
}

// ------------------------------------------------------------------
// buildMatterFlatBuffer – Serialize physical geometry to FlatBuffer (.ecmatter)
// ------------------------------------------------------------------
std::vector<uint8_t> ZoneManager::buildMatterFlatBuffer(
        const std::optional<std::unordered_set<std::string>>& scopeZoneIds) const {
    flatbuffers::FlatBufferBuilder builder(4096);

    std::vector<flatbuffers::Offset<Earthcall::Schema::Entity>> entity_offsets;

    for (const auto& zone : _zones) {
        if (!zone) continue;
        if (scopeZoneIds && !scopeZoneIds->count(zone->getIdentifier())) continue;
        for (const auto& o : zone->getOwnedObjects()) {
            if (!o) continue;

            auto id_str = builder.CreateString(o->getIdentifier());
            auto name_str = builder.CreateString(o->getObjectType());
            // owner_identifier: the composite address's other half (Sol's
            // Invariant 2) — see applyMatterFlatBuffer's read-side comment.
            auto owner_id_str = builder.CreateString(zone->getIdentifier());
            // materialId is semantic/Material state — Material::toJson
            // already round-trips it (and faceTextures, and faceColors
            // below) through the JSON path. Not written here any more:
            // see applyMatterFlatBuffer's read-side comment for why this
            // sidecar carrying it was actively harmful, not merely
            // redundant. Left as an empty (0) FlatBuffers offset rather
            // than removed from the schema, so old buffers that still
            // carry a real value stay readable — applyMatterFlatBuffer
            // just never acts on it any more.

            // 1. Transform matrix (16 floats)
            glm::mat4 t = o->getTransform();
            std::vector<float> tf_data(16);
            const float* t_ptr = glm::value_ptr(t);
            for (int m = 0; m < 16; ++m) tf_data[m] = t_ptr[m];
            auto tf_vec = builder.CreateVector(tf_data);

            // 2. Polyhedron Data
            flatbuffers::Offset<Earthcall::Schema::PolyhedronData> poly_offset = 0;
            if (o->getShapeKind() == Object::ShapeKind::Polyhedron) {
                const auto& poly = o->getPolyhedronData();
                std::vector<Earthcall::Schema::Vec3> fbs_verts;
                fbs_verts.reserve(poly.vertices.size());
                for (const auto& v : poly.vertices) {
                    fbs_verts.push_back(Earthcall::Schema::Vec3(v.x, v.y, v.z));
                }
                auto verts_vec = builder.CreateVectorOfStructs(fbs_verts);

                std::vector<int> face_data;
                std::vector<int> face_offsets;
                for (const auto& face : poly.faces) {
                    face_offsets.push_back(static_cast<int>(face_data.size()));
                    for (int v_idx : face) {
                        face_data.push_back(v_idx);
                    }
                }
                face_offsets.push_back(static_cast<int>(face_data.size()));

                auto face_data_vec = builder.CreateVector(face_data);
                auto face_offsets_vec = builder.CreateVector(face_offsets);
                poly_offset = Earthcall::Schema::CreatePolyhedronData(
                    builder, verts_vec, face_data_vec, face_offsets_vec);
            }

            // 3. Bezier Patch Data
            flatbuffers::Offset<Earthcall::Schema::BezierPatch> patch_offset = 0;
            if (o->hasPatch()) {
                const auto& patch = o->getPatchData();
                std::vector<Earthcall::Schema::Vec3> fbs_ctrl;
                fbs_ctrl.reserve(patch.ctrl.size());
                for (const auto& c : patch.ctrl) {
                    fbs_ctrl.push_back(Earthcall::Schema::Vec3(c.x, c.y, c.z));
                }
                auto ctrl_vec = builder.CreateVectorOfStructs(fbs_ctrl);
                patch_offset = Earthcall::Schema::CreateBezierPatch(
                    builder, patch.du, patch.dv, ctrl_vec);
            }

            // 4. Smooth Surface Data
            flatbuffers::Offset<Earthcall::Schema::SmoothSurfaceData> smooth_offset = 0;
            if (o->hasSmoothSurface()) {
                const auto& sm = o->getSmoothData();
                std::vector<float> q_data(16);
                const float* q_ptr = glm::value_ptr(sm.Q);
                for (int m = 0; m < 16; ++m) q_data[m] = q_ptr[m];
                auto q_vec = builder.CreateVector(q_data);
                auto params_vec = builder.CreateVector(sm.params);
                Earthcall::Schema::Vec3 axes(sm.axes.x, sm.axes.y, sm.axes.z);

                smooth_offset = Earthcall::Schema::CreateSmoothSurfaceData(
                    builder,
                    sm.closed,
                    sm.orientable,
                    sm.hasBoundary,
                    sm.isVolume,
                    static_cast<int>(sm.model),
                    q_vec,
                    static_cast<int>(sm.form),
                    static_cast<int>(sm.pkind),
                    &axes,
                    sm.zTrim.x,
                    sm.zTrim.y,
                    params_vec
                );
            }

            // 5. Field Data
            flatbuffers::Offset<Earthcall::Schema::FieldData> field_offset = 0;
            if (o->hasField()) {
                const auto& fd = o->getFieldData();
                Earthcall::Schema::Vec3 f_ext(o->getFieldExtent().x, o->getFieldExtent().y, o->getFieldExtent().z);
                Earthcall::Schema::Vec3 dims(fd.dims.x, fd.dims.y, fd.dims.z);
                Earthcall::Schema::Vec3 offset(fd.offset.x, fd.offset.y, fd.offset.z);
                auto expr_str = builder.CreateString(fd.expr);
                auto root_node = Earthcall::Schema::CreateSdfNode(
                    builder,
                    static_cast<int>(fd.prim),
                    static_cast<int>(fd.op),
                    0, 0, 0,
                    &dims,
                    &offset,
                    fd.p0,
                    fd.p1,
                    fd.t,
                    expr_str
                );
                field_offset = Earthcall::Schema::CreateFieldData(builder, &f_ext, root_node);
            }

            // Face textures and face colors: not written here any more —
            // see the comment above materialId. Left as empty (0)
            // FlatBuffers offsets.

            Earthcall::Schema::Vec3 fbs_center(o->getCenter().x, o->getCenter().y, o->getCenter().z);
            Earthcall::Schema::Vec3 fbs_axis(o->getAuthoritativeAxis().x, o->getAuthoritativeAxis().y, o->getAuthoritativeAxis().z);
            Earthcall::Schema::Vec3 fbs_target_rot(o->getTargetRotationEulerDegrees().x, o->getTargetRotationEulerDegrees().y, o->getTargetRotationEulerDegrees().z);

            auto entity = Earthcall::Schema::CreateEntity(
                builder,
                id_str,
                name_str,
                tf_vec,
                poly_offset,
                patch_offset,
                smooth_offset,
                field_offset,
                0, // face_textures — semantic/Material state, see comment above
                0, // face_colors — semantic/Material state, see comment above
                0, // sdf_nodes
                0, // laws
                0, // material_id — semantic/Material state, see comment above
                &fbs_center,
                &fbs_axis,
                &fbs_target_rot,
                o->getRotationResponsiveness(),
                owner_id_str
            );
            entity_offsets.push_back(entity);
        }
    }

    // Sol's Invariant 1 assertion: "the semantic root and its sidecar must
    // have the same membership set." This function cannot see the .ecform
    // side, so it checks the one divergence it CAN see directly — a caller
    // naming a Zone id in scopeZoneIds that was never actually live to
    // contribute objects, which would otherwise silently mean that Zone's
    // physical state is just absent from the buffer with no signal at all.
    // Logged loudly, not a hard crash: a Storage-mechanism sanity check must
    // not abort a Person's save.
    if (scopeZoneIds) {
        std::unordered_set<std::string> seenZoneIds;
        for (const auto& zone : _zones) {
            if (zone) seenZoneIds.insert(zone->getIdentifier());
        }
        for (const auto& wanted : *scopeZoneIds) {
            if (!seenZoneIds.count(wanted)) {
                std::cerr << "[ZoneManager] buildMatterFlatBuffer: scope named Zone '"
                          << wanted << "' which is not currently live — its physical "
                          << "state (if any) will be absent from this matter buffer.\n";
            }
        }
    }

    auto chunk_id = builder.CreateString("matter_" + SaveSystem::timestamp());
    auto entities_vec = builder.CreateVector(entity_offsets);
    auto chunk = Earthcall::Schema::CreateSaveChunk(builder, chunk_id, entities_vec);

    builder.Finish(chunk);

    const uint8_t* buf = builder.GetBufferPointer();
    size_t size = builder.GetSize();
    return std::vector<uint8_t>(buf, buf + size);
}

// ------------------------------------------------------------------
// applyMatterFlatBuffer – Hydrate physical geometry from FlatBuffer (.ecmatter)
// ------------------------------------------------------------------
void ZoneManager::applyMatterFlatBuffer(const std::vector<uint8_t>& buffer) {
    if (buffer.empty()) return;
    flatbuffers::Verifier verifier(buffer.data(), buffer.size());
    if (!Earthcall::Schema::VerifySaveChunkBuffer(verifier)) {
        std::cerr << "[ZoneManager] Matter FlatBuffer verification failed!\n";
        return;
    }

    const auto* chunk = Earthcall::Schema::GetSaveChunk(buffer.data());
    if (!chunk || !chunk->entities()) return;

    // Resolve by (owner Zone/Home identifier, bare object id) — the
    // canonical composite address (Sol's Invariant 2, agent intercom
    // "Basic Pixel Changer Zone Identity Bug 9-7-26", 2026-09-08). Bare id
    // alone is not unique: the real basic_pixel_changer.ecmatter had 382
    // duplicate bare ids across Zones, including two records for
    // "basic-pixel-canvas" itself — one correct, one the legacy cube-face
    // red default — and whichever the old bare-id map's insertion visited
    // last silently won, overwriting a correct semantic-JSON-loaded value
    // with a stale one every time, regardless of which was right.
    //
    // byBareId also tracks every live (zoneId, Object) pair sharing a bare
    // id, so a LEGACY entity with no owner_identifier can still be resolved
    // when — and only when — that id is unambiguous among currently-live
    // objects; when it is not, this refuses (skips, logs every candidate)
    // rather than guessing which one was meant. No iteration order, no
    // unordered_map replacement, no last-record-wins.
    std::unordered_map<std::string, std::shared_ptr<Object>> byComposite;
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::shared_ptr<Object>>>> byBareId;
    for (const auto& zone : _zones) {
        if (!zone) continue;
        const std::string zoneId = zone->getIdentifier();
        for (const auto& o : zone->getOwnedObjects()) {
            if (!o) continue;
            const std::string objId = o->getIdentifier();
            byComposite[zoneId + "::" + objId] = o;
            byBareId[objId].push_back({zoneId, o});
        }
    }

    // Pass 1: resolve every entity to (Object, composite key) WITHOUT
    // mutating anything yet, so a composite key that resolves more than
    // once in THIS buffer (a literal duplicate record — the real
    // basic_pixel_changer.ecmatter's two "basic-pixel-canvas" entities are
    // exactly this, both legacy/ownerless) can be refused in its entirety
    // rather than letting whichever the loop reaches second silently win.
    const auto* entities = chunk->entities();
    std::vector<std::shared_ptr<Object>> resolvedObj(entities->size());
    std::vector<std::string> resolvedKey(entities->size());
    std::unordered_map<std::string, int> keyCount;
    for (size_t i = 0; i < entities->size(); ++i) {
        const auto* entity = entities->Get(i);
        if (!entity || !entity->id()) continue;
        const std::string id = entity->id()->str();

        std::shared_ptr<Object> o;
        std::string key;
        if (entity->owner_identifier() && entity->owner_identifier()->size() > 0) {
            auto it = byComposite.find(entity->owner_identifier()->str() + "::" + id);
            if (it != byComposite.end()) {
                o = it->second;
                key = entity->owner_identifier()->str() + "::" + id;
            }
            // else: the named owner does not match any live Zone holding
            // this bare id right now. That is not necessarily staleness —
            // loadTestObservation deliberately re-parents a dump's objects
            // into a freshly-named "test.<stem>" Zone, different from
            // whatever Zone owned them when the .ecmatter was written, so a
            // legitimately-moved object's owner_identifier will never match
            // post-move. Fall through to the same unambiguous-bare-id
            // resolution a legacy (ownerless) record gets, rather than
            // refusing outright: owner_identifier disambiguates when there
            // IS a live collision, it does not veto a resolution that is
            // otherwise perfectly safe.
        }
        if (!o) {
            auto it = byBareId.find(id);
            if (it != byBareId.end()) {
                if (it->second.size() == 1) {
                    o = it->second.front().second;
                    key = it->second.front().first + "::" + id;
                } else {
                    std::string candidates;
                    for (const auto& c : it->second) {
                        if (!candidates.empty()) candidates += ", ";
                        candidates += c.first;
                    }
                    std::cerr << "[ZoneManager] applyMatterFlatBuffer: entity '" << id
                              << "' has no exact owner match and matches " << it->second.size()
                              << " live objects across Zones (" << candidates
                              << ") — refusing to guess, skipping this entity.\n";
                }
            }
        }
        if (!o) continue;
        resolvedObj[i] = o;
        resolvedKey[i] = key;
        ++keyCount[key];
    }

    // Pass 2: apply fields, but only for entities whose composite key was
    // unique in this buffer. Sol's Invariant 3: "do not use iteration
    // order, unordered_map replacement, or last-record-wins anywhere."
    std::unordered_set<std::string> loggedDuplicates;
    for (size_t i = 0; i < entities->size(); ++i) {
        auto& o = resolvedObj[i];
        if (!o) continue;
        const std::string& key = resolvedKey[i];
        if (keyCount[key] > 1) {
            if (loggedDuplicates.insert(key).second) {
                std::cerr << "[ZoneManager] applyMatterFlatBuffer: composite key '" << key
                          << "' appears " << keyCount[key] << " times in this matter buffer — "
                          << "refusing all of them rather than guessing which is authoritative.\n";
            }
            continue;
        }
        const auto* entity = entities->Get(i);

        // Material ID, face textures, and face colors are semantic/Material
        // state (Material::toJson already round-trips all three), not
        // physical matter — deliberately not applied from this sidecar.
        // materialId/faceColors/faceTextures are still READABLE here for
        // old buffers, only never acted on: this schema field stays
        // append-only rather than removed.

        // 1. Transform & Pose
        if (entity->transform() && entity->transform()->size() == 16) {
            std::vector<float> tvals(entity->transform()->begin(), entity->transform()->end());
            o->setTransform(vectorToMat4(tvals));
        }
        if (entity->center()) {
            o->setCenter(glm::vec3(entity->center()->x(), entity->center()->y(), entity->center()->z()));
        }
        if (entity->authoritative_axis()) {
            o->setAuthoritativeAxis(glm::vec3(entity->authoritative_axis()->x(), entity->authoritative_axis()->y(), entity->authoritative_axis()->z()));
        }
        if (entity->target_rotation()) {
            o->setTargetRotationEulerDegrees(glm::vec3(entity->target_rotation()->x(), entity->target_rotation()->y(), entity->target_rotation()->z()));
        }
        o->setRotationResponsiveness(entity->rotation_responsiveness());

        // 2. Polyhedron
        if (entity->polyhedron() && entity->polyhedron()->vertices() && entity->polyhedron()->face_data() && entity->polyhedron()->face_offsets()) {
            const auto* poly = entity->polyhedron();
            std::vector<glm::vec3> verts;
            verts.reserve(poly->vertices()->size());
            for (const auto* v : *poly->vertices()) {
                verts.emplace_back(v->x(), v->y(), v->z());
            }

            const auto* fData = poly->face_data();
            const auto* fOffsets = poly->face_offsets();
            std::vector<std::vector<int>> faces;
            if (fOffsets->size() >= 2) {
                faces.reserve(fOffsets->size() - 1);
                for (size_t i = 0; i + 1 < fOffsets->size(); ++i) {
                    int start = fOffsets->Get(i);
                    int end = fOffsets->Get(i + 1);
                    std::vector<int> face;
                    face.reserve(end - start);
                    for (int fi = start; fi < end && fi < (int)fData->size(); ++fi) {
                        face.push_back(fData->Get(fi));
                    }
                    faces.push_back(std::move(face));
                }
            }
            if (!verts.empty() && !faces.empty()) {
                o->setPolyhedronData(PolyhedronData::createCustomPolyhedron(verts, faces));
            }
        }

        // 3. Bezier Patch
        if (entity->patch() && entity->patch()->ctrl()) {
            geom::BezierPatch patch;
            patch.du = entity->patch()->du();
            patch.dv = entity->patch()->dv();
            patch.ctrl.reserve(entity->patch()->ctrl()->size());
            for (const auto* c : *entity->patch()->ctrl()) {
                patch.ctrl.emplace_back(c->x(), c->y(), c->z());
            }
            if (patch.valid()) {
                o->setBezierPatch(patch);
            }
        }

        // 4. Smooth Surface
        if (entity->smooth_data() && entity->smooth_data()->quadric_matrix()) {
            const auto* sm = entity->smooth_data();
            geom::SmoothSurfaceData sd;
            sd.closed = sm->closed();
            sd.orientable = sm->orientable();
            sd.hasBoundary = sm->has_boundary();
            sd.isVolume = sm->is_volume();
            sd.model = static_cast<geom::SmoothSurfaceData::Model>(sm->model());
            if (sm->quadric_matrix()->size() == 16) {
                std::vector<float> qv(sm->quadric_matrix()->begin(), sm->quadric_matrix()->end());
                sd.Q = vectorToMat4(qv);
            }
            sd.form = static_cast<geom::SmoothSurfaceData::QuadricForm>(sm->quadric_form());
            sd.pkind = static_cast<geom::SmoothSurfaceData::ParametricKind>(sm->parametric_kind());
            if (sm->axes()) {
                sd.axes = glm::vec3(sm->axes()->x(), sm->axes()->y(), sm->axes()->z());
            }
            sd.zTrim = glm::vec2(sm->z_trim_min(), sm->z_trim_max());
            if (sm->params()) {
                sd.params.assign(sm->params()->begin(), sm->params()->end());
            }
            o->setSmoothSurface(sd);
        }

        // 5. Field Shape
        if (entity->field() && entity->field()->root_node()) {
            const auto* fbsField = entity->field();
            const auto* root = fbsField->root_node();
            geom::SdfNode node;
            node.prim = static_cast<geom::SdfPrim>(root->type());
            node.op = static_cast<geom::SdfOp>(root->operation());
            if (root->dims()) node.dims = glm::vec3(root->dims()->x(), root->dims()->y(), root->dims()->z());
            if (root->offset()) node.offset = glm::vec3(root->offset()->x(), root->offset()->y(), root->offset()->z());
            node.p0 = root->p0();
            node.p1 = root->p1();
            node.t = root->t();
            if (root->expr()) node.expr = root->expr()->str();
            glm::vec3 extent(1.0f);
            if (fbsField->extent()) extent = glm::vec3(fbsField->extent()->x(), fbsField->extent()->y(), fbsField->extent()->z());
            o->setFieldShape(node, extent);
        }

        // Face textures and face colors: not applied. See the comment
        // above the Material-ID field at the top of this loop.
    }
}

std::vector<uint8_t> ZoneManager::buildSaveChunkFlatBuffer() {
    return buildMatterFlatBuffer();
}

void ZoneManager::loadSaveChunkFlatBuffer(const std::vector<uint8_t>& buffer) {
    applyMatterFlatBuffer(buffer);
}
