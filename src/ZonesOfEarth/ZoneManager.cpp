#include "ZoneManager.hpp"
#include "ConstructedBeing/CategoryManager.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/Serialization.hpp"
#include "Singularity/Storage/BinaryPack.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/LawAuditLogger.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Object/Creation/ObjectConcept.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/TransferPolicy.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Input/MouseHandler.hpp"
#include "OurVerse/Tool.hpp"
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

extern MaterialManager materials;
extern CategoryManager categories;

void ZoneManager::addZone(Zone&& zone) noexcept
{
    // Avoid copy of potentially large internal members by moving in
    _zones.push_back(std::move(zone));
}

void ZoneManager::addZone(const Zone& zone)
{
    _zones.emplace_back(zone); // invokes Zone copy-ctor
}

void ZoneManager::switchTo(size_t index)
{
    if (index < _zones.size())
    {
        _currentIndex = index;
        std::cout << "🔀 Switching to zone [" << index << "]..." << std::endl;
        
        // Repopulate active zone's world with global objects that belong to it or its parents
        std::vector<std::string> activeZones;
        std::string currentZoneId = _zones[_currentIndex].getIdentifier();
        while (!currentZoneId.empty()) {
            activeZones.push_back(currentZoneId);
            std::string parent = "";
            for (const auto& z : _zones) {
                if (z.getIdentifier() == currentZoneId) {
                    parent = z.getParentZone();
                    break;
                }
            }
            if (parent == currentZoneId || parent.empty()) break;
            currentZoneId = parent;
        }

        auto& worldObjs = _zones[_currentIndex].world().getOwnedObjectsMutable();
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

        try { _zones[_currentIndex].load(); } catch (...) { std::cerr << "⚠️  Zone load failed." << std::endl; }
        describeCurrent();
        // The zone is a being: laws hear arrival (subject: the zone itself).
        Core::EventBus::instance().publish(
            ECA::Event{"zone-entered", &_zones[_currentIndex], nullptr, std::time(nullptr)});
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
        _zones[_currentIndex].describe();
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
        _zones[_currentIndex].load();
    }
    else
    {
        std::cerr << "⚠️ Cannot load zone: index out of bounds!" << std::endl;
    }
}

Zone& ZoneManager::active() { return ZoneManager::_zones[ZoneManager::_currentIndex]; }

std::vector<Zone>& ZoneManager::zones() { return _zones; }

const std::vector<Zone>& ZoneManager::zones() const { return _zones; }

// Save/Load methods moved from Game

void ZoneManager::ensureHomeZone(const std::string& playerId) {
    if (playerId.empty()) return;

    for (const auto& zone : _zones) {
        if (zone.owner() == playerId) return;
    }
    // A save from before ownership existed may hold an unowned "Home" —
    // claim it instead of minting a name-twin (identifiers must stay unique).
    for (auto& zone : _zones) {
        if (zone.name() == "Home" && zone.owner().empty()) {
            zone.setOwner(playerId);
            return;
        }
    }
    Zone home("Home", "strict", 0.08f, 0.06f, 0.12f);
    home.setOwner(playerId);
    home.setQuality("kind", "home");
    addZone(std::move(home));
    printf("[Init] Home established for '%s' (zone count now %zu)\n",
           playerId.c_str(), _zones.size());
}

void ZoneManager::updateSaveFiles() {
    _saveLoad.files = SaveSystem::listFiles(SaveSystem::SaveType::GAME);
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
} // namespace

// ------------------------------------------------------------------
// buildSaveJson - moved from Game
// ------------------------------------------------------------------
nlohmann::json ZoneManager::buildSaveJson(const SaveContext& ctx) const {
    using json = nlohmann::json;
    json j;

    j["currentZone"] = _currentIndex;
    json zonesJson = json::array();
    for (const auto& z : _zones) {
        json zj; zj["name"] = z.name();
        zj["r"] = z.r; zj["g"] = z.g; zj["b"] = z.b;
        zj["owner"] = z.owner();
        BinaryPack::Writer bw;
        bw.write(static_cast<uint32_t>(z.strokes.size()));
        for (const auto& s : z.strokes) {
            bw.write(s.r); bw.write(s.g); bw.write(s.b); bw.write(s.lineWidth);
            bw.writeArray(s.points);
        }
        zj["strokesBinary"] = bw.toBinaryJson();
        zj["world"] = z.world();
        zj["formationRelations"] = z.formation().relations().toJson();
        zonesJson.push_back(zj);
    }
    j["zones"] = zonesJson;

    j["materials"] = materials.toJson();
    j["categories"] = categories.toJson();

    // Camera and player view - accessed through SaveContext
    j["cameraPos"]   = {ctx.camera->pos.x, ctx.camera->pos.y, ctx.camera->pos.z};
    j["cameraFront"] = {ctx.camera->front.x, ctx.camera->front.y, ctx.camera->front.z};
    j["cameraUp"]    = {ctx.camera->up.x, ctx.camera->up.y, ctx.camera->up.z};
    j["yaw"]   = ctx.mouseHandler->getYaw();
    j["pitch"] = ctx.mouseHandler->getPitch();

    j["currentColor"] = {ctx.currentColor[0], ctx.currentColor[1], ctx.currentColor[2]};
    j["currentTool"]  = static_cast<int>(ctx.currentTool->getType());

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
    j["playerBody"] = bodyToJson(ctx.player->getBody());

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
    nlohmann::json j = buildSaveJson(ctx);

    // Dynamic objects (skip baseline 0 & 1) – only added by the "log" variant
    auto& zoneWorld = active().world();
    nlohmann::json objArr = nlohmann::json::array();
    const auto& objs = zoneWorld.getOwnedObjects();
    for (size_t i = 2; i < objs.size(); ++i) {
        const auto& o = objs[i];
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
    SaveSystem::writeSaveDataAsync(j, actualName, SaveSystem::SaveType::GAME);
    
    // Phase 4: Save dirty delta chunk as FlatBuffers
    std::vector<uint8_t> deltaChunk = buildSaveChunkFlatBuffer();
    if (!deltaChunk.empty()) {
        SaveSystem::writeSaveDataAsync(deltaChunk, actualName + "_delta", ".ecsave", SaveSystem::SaveType::GAME);
    }
    
    ECA::LawAuditLogger::instance().setActiveWorld(actualName);
    logIo("SAVE (log) '" + actualName + "': " +
          std::to_string(ctx.lawManager->getAll().size()) + " law(s), " +
          std::to_string(ConceptRegistry::instance().getAll().size()) + " concept(s)");
}

// ------------------------------------------------------------------
// loadState
// ------------------------------------------------------------------
void ZoneManager::loadState(const std::string& filename, SaveContext& ctx) {
    ECA::LawAuditLogger::instance().setActiveWorld(filename);
    
    std::filesystem::path path(filename);
    std::string name = path.filename().string();
    if (name.length() > 5 && name.substr(name.length() - 5) == ".json") {
        name = name.substr(0, name.length() - 5);
    }
    if (name.length() >= 15 && name[8] == '_') {
        if (name.length() > 16) {
            _saveLoad.loadedSaveName = name.substr(16);
        } else {
            _saveLoad.loadedSaveName = "";
        }
    } else {
        _saveLoad.loadedSaveName = name;
    }
    if (!_saveLoad.loadedSaveName.empty()) {
        std::strncpy(_saveLoad.customName, _saveLoad.loadedSaveName.c_str(), sizeof(_saveLoad.customName) - 1);
        _saveLoad.customName[sizeof(_saveLoad.customName) - 1] = '\0';
    }

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
        json j = SaveSystem::readSaveData(filename);
        if (j.is_null()) {
            _saveLoad.lastLoadReport = "COULD NOT OPEN OR READ: " + filename;
            std::cerr << "Could not open or read " << filename << "\n";
            return;
        }

        // Reset physics registries
        Physics::resetRigidBodies();
        Physics::clearBonds();

        // Load material beings
        if (j.contains("materials")) materials.loadFromJson(j["materials"]);
        if (j.contains("categories")) categories.loadFromJson(j["categories"]);

        size_t currentZoneIdx = j.value("currentZone", 0);
        auto& zonesVec = _zones; zonesVec.clear();
        if (j.contains("zones")) {
            for (const auto& zj : j["zones"]) {
                std::string name = zj.value("name", "Untitled Zone");
                Zone z(name, "strict");
                z.r = zj.value("r", 0.05f);
                z.g = zj.value("g", 0.05f);
                z.b = zj.value("b", 0.1f);
                z.setOwner(zj.value("owner", std::string{}));
                if (zj.contains("strokesBinary")) {
                    BinaryPack::Reader br(zj["strokesBinary"].get_binary());
                    uint32_t numStrokes = br.read<uint32_t>();
                    for (uint32_t i = 0; i < numStrokes; ++i) {
                        Zone::Stroke s;
                        s.r = br.read<float>();
                        s.g = br.read<float>();
                        s.b = br.read<float>();
                        s.lineWidth = br.read<float>();
                        br.readArray(s.points);
                        z.strokes.push_back(std::move(s));
                    }
                } else if (zj.contains("strokes")) {
                    for (const auto& sj : zj["strokes"]) {
                        Zone::Stroke s;
                        s.lineWidth = 1.0f;
                        auto col = sj.value("color", std::vector<float>{1, 1, 1});
                        if (col.size() >= 3) { s.r = col[0]; s.g = col[1]; s.b = col[2]; }
                        s.points = sj.value("points", std::vector<float>{});
                        z.strokes.push_back(std::move(s));
                    }
                }
                if (zj.contains("world")) {
                    from_json(zj["world"], z.world());
                }
                if (zj.contains("formationRelations")) {
                    // MEMBERS BEFORE RELATIONS. Zone::syncFormationMembers does
                    // not run until the frame loop, so a relation added here used
                    // to find neither of its endpoints: it got a subformation with
                    // no members, and since subformations were matched only by
                    // member lookup, that empty set could never match anything
                    // again. Every world loaded from disk came up with set-to-set
                    // grouping already broken. The objects exist now — say so
                    // before the bonds between them are read.
                    z.syncFormationMembers();
                    size_t refused = 0;
                    for (const auto& relJson : zj["formationRelations"]) {
                        if (!z.formation().add(std::make_shared<Relation>(Relation::fromJson(relJson)))) {
                            ++refused;
                        }
                    }
                    if (refused > 0) {
                        std::cout << "⚠️  Zone '" << z.name() << "': " << refused
                                  << " saved formation relation(s) were REFUSED on load "
                                  << "(self-ground or a directed cycle). They are not in "
                                  << "the formation and will not be written back on the "
                                  << "next save. Fix them in the save file to keep them."
                                  << std::endl;
                    }
                }
                zonesVec.push_back(std::move(z));
            }
        }

        if (zonesVec.empty()) {
            zonesVec.push_back(Zone("Default Zone", "default"));
        }
        // Home survives every load
        ensureHomeZone(ctx.player->getIdentifier());
        switchTo(std::min(currentZoneIdx, zonesVec.size() - 1));

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

        if (j.contains("currentColor")) {
            ctx.currentColor[0] = j["currentColor"][0];
            ctx.currentColor[1] = j["currentColor"][1];
            ctx.currentColor[2] = j["currentColor"][2];
        }
        *ctx.currentTool = Tool(static_cast<Tool::Type>(j.value("currentTool", static_cast<int>(Tool::Type::Brush))));

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
                bodyFromJson(j["playerBody"], ctx.player->getBody());
            }
        });

        // The authored register
        stage("world-clock", [&] {
            *ctx.worldTime = j.value("worldTime", 0.0);
            Universe::instance().setClock(*ctx.worldTime, 0.0);
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
            objectCount += zone.world().getOwnedObjects().size();
        }
        std::size_t authoredCount = 0;
        for (const auto& law : ctx.lawManager->getAll()) {
            if (law && law->isAuthored()) ++authoredCount;
        }
        _saveLoad.lastLoadReport =
            "Loaded: " + std::to_string(objectCount) + " object(s), " +
            std::to_string(ctx.lawManager->getAll().size()) + " law(s) (" +
            std::to_string(authoredCount) + " authored), " +
            std::to_string(ConceptRegistry::instance().getAll().size()) +
            " concept(s), worldTime " + std::to_string(*ctx.worldTime);
        if (!failures.empty()) {
            _saveLoad.lastLoadReport += "  |  FAILED stages: " + failures;
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
// buildSaveChunkFlatBuffer – Serialize dirty objects to FlatBuffer
// ------------------------------------------------------------------
std::vector<uint8_t> ZoneManager::buildSaveChunkFlatBuffer() {
    flatbuffers::FlatBufferBuilder builder(1024);
    
    std::vector<flatbuffers::Offset<Earthcall::Schema::Entity>> entity_offsets;
    auto& zoneWorld = active().world();
    const auto& objs = zoneWorld.getOwnedObjects();
    
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
