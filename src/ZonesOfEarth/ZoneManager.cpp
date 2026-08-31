#include "ZoneManager.hpp"
#include "HomesOfEarth/Home.hpp"
#include "ConstructedBeing/CategoryManager.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/Serialization.hpp"
#include "Singularity/Storage/BinaryPack.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/LawAuditLogger.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Object/Creation/ObjectConcept.hpp"
#include "ConstructedBeing/Singular/Object/Object/ObjectIdentity.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/TransferPolicy.hpp"
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
#include <filesystem>
#include <fstream>
#include <ctime>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <unordered_set>
#include <algorithm>
#include <set>
#include <functional>

extern MaterialManager materials;
extern CategoryManager categories;

void ZoneManager::addZone(std::shared_ptr<Zone> zone)
{
    _zones.push_back(std::move(zone));
}

void ZoneManager::switchTo(size_t index)
{
    if (index < _zones.size())
    {
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

        try { _zones[_currentIndex]->load(); } catch (...) { std::cerr << "⚠️  Zone load failed." << std::endl; }
        describeCurrent();
        // The zone is a being: laws hear arrival (subject: the zone itself).
        Core::EventBus::instance().publish(
            ECA::Event{"zone-entered", _zones[_currentIndex].get(), nullptr, std::time(nullptr)});
    }
    else
    {
        std::cerr << "⚠️ Invalid zone index!" << std::endl;
    }
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

std::string zoneIdFromJson(const nlohmann::json& zj) {
    return zj.value("identifier", zj.value("name", std::string{}));
}

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
           "/before-load.json";
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
        const nlohmann::json doc = zoneToJson(*z);

        // Bug #7's guard of last resort: never stamp an empty relation
        // graph or lexeme set over a stored identity that still holds one.
        // Fixes above should make the live Zone's graph correct before it
        // gets here, but this is the check that would have caught the loss
        // the day it happened, so it stays even if it now looks redundant.
        if (identityExists) {
            nlohmann::json existingGraph = dwelling ? SaveSystem::readHomeIdentity(id)
                                                     : SaveSystem::readZoneIdentity(id);
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
    std::unordered_set<std::string> loaded;
    auto admit = [&](const std::string& id, nlohmann::json zj) {
        if (!zj.is_object() || id.empty() || loaded.count(id)) return;
        loaded.insert(id);
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
    for (const auto& id : SaveSystem::listHomeIdentities()) {
        admit(id, SaveSystem::readHomeIdentity(id));
    }
    for (const auto& id : SaveSystem::listZoneIdentities()) {
        admit(id, SaveSystem::readZoneIdentity(id));
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
    j["zones"] = zonesJson;
    j["zoneRefs"] = zoneRefs;

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

    // Player avatar body
    j["playerBody"] = bodyToJson(ctx.person->getBody());

    // Authored register
    j["authoredLaws"] = ctx.lawManager->toJson();
    j["concepts"] = ConceptRegistry::instance().toJson();
    j["transferPolicy"] = TransferPolicy::instance().toJson();
    j["mathFunctions"] = OntoMath::FunctionRegistry::instance().toJson();
    j["worldTime"] = *ctx.worldTime;

    return j;
}

// ------------------------------------------------------------------
// saveState
// ------------------------------------------------------------------
void ZoneManager::saveState(const std::string& filename, SaveContext& ctx) {
    // A before-load stash is a snapshot of the previous session, not an
    // evolution of Zone identity. Writing it into saves/zones/ would let a
    // load rewind Home as a side effect.
    if (!isBeforeLoadSnapshot(filename)) persistZones();
    nlohmann::json j = buildSaveJson(ctx);
    std::ofstream out(filename);
    out << j.dump(2);
    logIo("SAVE " + filename + ": " +
          std::to_string(ctx.lawManager->getAll().size()) + " law(s), " +
          std::to_string(ConceptRegistry::instance().getAll().size()) + " concept(s)");
}

// ------------------------------------------------------------------
// saveStateWithLog
// ------------------------------------------------------------------
void ZoneManager::saveStateWithLog(const std::string& customName, SaveContext& ctx) {
    persistZones();
    nlohmann::json j = buildSaveJson(ctx);

    // Top-level objects: the active zone's whole world. This used to skip
    // index 0 and 1 as "baseline cube & ground", but those live on Ourverse
    // now (EngineInit), so the skip ate the first two beings a Person
    // spawned. Zone JSON still had them; the top-level array is the fallback
    // for empty zone worlds (legacy saves). Write what is actually there.
    auto& zone = active();
    nlohmann::json objArr = nlohmann::json::array();
    for (const auto& o : zone.getOwnedObjects()) {
        if (!o) continue;
        nlohmann::json oj = *o;
        objArr.push_back(std::move(oj));
    }
    j["objects"] = objArr;

    // Use the new SaveSystem to write the file
    std::string actualName = customName;
    if (actualName.empty()) {
        if (!_saveLoad.loadedSaveName.empty()) {
            actualName = SaveSystem::timestamp() + "_" + _saveLoad.loadedSaveName;
        } else {
            actualName = SaveSystem::timestamp() + "_QuickSave";
        }
    }
    // Console / menu Save As used to fire-and-forget async, so a Person
    // who pressed the button and looked at saves/worlds/ saw nothing and
    // concluded the gesture failed. Write on this thread; report the path.
    const std::string path = SaveSystem::writeSaveData(j, actualName, SaveSystem::SaveType::WORLD);
    if (path.empty()) {
        _saveLoad.lastSaveReport = "Save refused or failed for '" + actualName + "'.";
        logIo("SAVE FAILED '" + actualName + "'");
        return;
    }
    // The console Save As path is authoring: write the readable JSON next to
    // the binary .ecsave so looking in saves/worlds/ is not a blank folder
    // of opaque files.
    {
        std::filesystem::path jsonPath(path);
        jsonPath.replace_extension(".json");
        std::ofstream jsonOut(jsonPath);
        if (jsonOut) jsonOut << j.dump(2);
    }
    _saveLoad.lastSaveReport = "Wrote " + path;
    _saveLoad.loadedSaveName = actualName;
    if (ctx.unpackForAuthoring) {
        std::string gameFolder = SaveSystem::ensureSaveTypeFolder(SaveSystem::SaveType::WORLD);
        std::string unpackedPath = gameFolder + "/" + actualName + "_unpacked";
        SaveSystem::unpackSaveToDirectory(j, unpackedPath);
        _saveLoad.lastSaveReport += " (unpacked " + unpackedPath + ")";
    }
    
    // Phase 4: Save dirty delta chunk as FlatBuffers
    std::vector<uint8_t> deltaChunk = buildSaveChunkFlatBuffer();
    if (!deltaChunk.empty()) {
        SaveSystem::writeSaveDataAsync(deltaChunk, actualName + "_delta", ".ecsave", SaveSystem::SaveType::WORLD);
    }
    
    ECA::LawAuditLogger::instance().setActiveWorld(actualName);
    logIo("SAVE (log) '" + actualName + "' -> " + path + ": " +
          std::to_string(ctx.lawManager->getAll().size()) + " law(s), " +
          std::to_string(ConceptRegistry::instance().getAll().size()) + " concept(s)");
}

// ------------------------------------------------------------------
// loadState
// ------------------------------------------------------------------
void ZoneManager::loadState(const std::string& filename, SaveContext& ctx) {
    ECA::LawAuditLogger::instance().setActiveWorld(filename);
    
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
            std::ifstream probe(stash);
            if (!probe) {
                _saveLoad.lastLoadReport =
                    "REFUSED load: could not preserve unsaved work to " + stash +
                    ". The current world was not replaced.";
                std::cerr << "[load] " << _saveLoad.lastLoadReport << "\n";
                logIo("LOAD end:   " + _saveLoad.lastLoadReport);
                return;
            }
            preservedPath = stash;
            logIo("PRESERVE unsaved -> " + stash);
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
                    return;
                }
            }
            auto z = makeZoneFromJson(zj);
            addZone(z);
            if (!snapshotRestore && !isObservationZone(*z)) {
                SaveSystem::writeZoneIdentity(id, zoneToJson(*z));
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

        // Player avatar body
        stage("player-body", [&] {
            if (j.contains("playerBody")) {
                bodyFromJson(j["playerBody"], ctx.person->getBody());
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

// ------------------------------------------------------------------
// buildSaveChunkFlatBuffer – Serialize dirty objects to FlatBuffer
// ------------------------------------------------------------------
std::vector<uint8_t> ZoneManager::buildSaveChunkFlatBuffer() {
    flatbuffers::FlatBufferBuilder builder(1024);
    
    std::vector<flatbuffers::Offset<Earthcall::Schema::Entity>> entity_offsets;
    const auto& objs = active().getOwnedObjects();
    
    for (size_t i = 2; i < objs.size(); ++i) {
        const auto& o = objs[i];
        if (!o->getIsDirty()) continue;
        
        // Mark as clean since we are saving it
        o->clearDirty();
        
        // 1. Strings
        auto id_str = builder.CreateString(o->getIdentifier());
        auto name_str = builder.CreateString(o->getObjectType());
        
        // 2. Transform matrix (16 floats)
        glm::mat4 t = o->getTransform();
        std::vector<float> tf_data(16);
        const float* t_ptr = (const float*)glm::value_ptr(t);
        for(int m=0; m<16; m++) tf_data[m] = t_ptr[m];
        auto tf_vec = builder.CreateVector(tf_data);
        
        // 3. Polyhedron Data
        const auto& poly = o->getPolyhedronData();
        std::vector<Earthcall::Schema::Vec3> fbs_verts;
        for (const auto& v : poly.vertices) {
            fbs_verts.push_back(Earthcall::Schema::Vec3(v.x, v.y, v.z));
        }
        auto verts_vec = builder.CreateVectorOfStructs(fbs_verts);
        
        std::vector<int> face_data;
        std::vector<int> face_offsets;
        for (const auto& face : poly.faces) {
            face_offsets.push_back(face_data.size());
            for (int v_idx : face) {
                face_data.push_back(v_idx);
            }
        }
        face_offsets.push_back(face_data.size()); // end offset
        
        auto face_data_vec = builder.CreateVector(face_data);
        auto face_offsets_vec = builder.CreateVector(face_offsets);
        
        auto poly_data = Earthcall::Schema::CreatePolyhedronData(
            builder, verts_vec, face_data_vec, face_offsets_vec);
            
        // 4. Entity
        auto entity = Earthcall::Schema::CreateEntity(
            builder,
            id_str,
            name_str,
            tf_vec,
            poly_data
            // laws left empty for now to test serialization
        );
        
        entity_offsets.push_back(entity);
    }
    
    auto chunk_id = builder.CreateString("zone_" + std::to_string(_currentIndex) + "_delta_" + SaveSystem::timestamp());
    auto entities_vec = builder.CreateVector(entity_offsets);
    auto chunk = Earthcall::Schema::CreateSaveChunk(builder, chunk_id, entities_vec);
    
    builder.Finish(chunk);
    
    uint8_t* buf = builder.GetBufferPointer();
    int size = builder.GetSize();
    return std::vector<uint8_t>(buf, buf + size);
}

// ------------------------------------------------------------------
// loadSaveChunkFlatBuffer
// ------------------------------------------------------------------
void ZoneManager::loadSaveChunkFlatBuffer(const std::vector<uint8_t>& buffer) {
    // TODO: implement loading
}
