// GameSaveLoad.cpp – Save / Load serialization, UI dialogs, shutdown
// Split from Game.cpp during refactor.

#include "Game.hpp"
#include "Form/Object/Object.hpp"
#include "Form/Object/Creation/ObjectConcept.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/TransferPolicy.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "Form/Material/MaterialManager.hpp"
#include "Util/SaveSystem.hpp"
#include "Util/Serialization.hpp"
#include "Util/BinaryPack.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/LawAuditLogger.hpp"
#include "Util/Schema/Earthcall_generated.h"

extern MaterialManager materials; // global Material beings (globals.cpp)

#include <imgui.h>
#include <glm/glm.hpp>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <ctime>
#include <cstring>
#include <iostream>
#include <ctime>
#include <string>
#include <vector>

extern ZoneManager mgr;

namespace Core {

namespace {
// Persistent testimony: every save/load appends one line here, so what
// happened in a session is readable AFTER it — stderr dies with the
// terminal, the log does not.
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
// Shared JSON builder – used by both saveState and saveStateWithLog
// to eliminate duplicated serialization code.
// ------------------------------------------------------------------
nlohmann::json Game::buildSaveJson() const {
    using json = nlohmann::json;
    json j;

    j["currentZone"] = mgr.currentIndex();
    json zonesJson = json::array();
    for (const auto& z : mgr.zones()) {
        json zj; zj["name"] = z.name();
        zj["r"] = z.r; zj["g"] = z.g; zj["b"] = z.b;
        zj["owner"] = z.owner();   // ownership is covenant: it persists
        BinaryPack::Writer bw;
        bw.write(static_cast<uint32_t>(z.strokes.size()));
        for (const auto& s : z.strokes) {
            bw.write(s.r); bw.write(s.g); bw.write(s.b); bw.write(s.lineWidth);
            bw.writeArray(s.points);
        }
        zj["strokesBinary"] = bw.toBinaryJson();
        // Serialize the 3-D world owned by this zone
        zj["world"] = z.world();
        zj["formationRelations"] = z.formation().relations().toJson();
        zonesJson.push_back(zj);
    }
    j["zones"] = zonesJson;

    // Material beings are global (shared across zones), so they save once here,
    // not inside any zone. Objects carry only the identifier string.
    j["materials"] = materials.toJson();

    // Camera and player view
    j["cameraPos"]   = {_camera.pos.x, _camera.pos.y, _camera.pos.z};
    j["cameraFront"] = {_camera.front.x, _camera.front.y, _camera.front.z};
    j["cameraUp"]    = {_camera.up.x, _camera.up.y, _camera.up.z};
    j["yaw"]   = _mouseHandler.getYaw();
    j["pitch"] = _mouseHandler.getPitch();

    j["currentColor"] = {_currentColor[0], _currentColor[1], _currentColor[2]};
    j["currentTool"]  = static_cast<int>(_currentTool.getType());

    j["worldMode"]    = static_cast<int>(_world.getMode());
    j["worldPhysics"] = _world.isPhysicsEnabled();

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

    // Player avatar body (includes per-face textures on each body part)
    j["playerBody"] = bodyToJson(_player.getBody());

    // The authored register: laws (with their models, scope, drives, and
    // trigger bindings) and captured concepts — a saved world keeps its
    // covenant, not just its furniture. The world clock rides along so
    // "time" doesn't restart at zero.
    j["authoredLaws"] = _lawManager.toJson();
    j["concepts"] = ConceptRegistry::instance().toJson();
    j["transferPolicy"] = TransferPolicy::instance().toJson();
    j["mathFunctions"] = OntoMath::FunctionRegistry::instance().toJson();
    j["worldTime"] = _worldTime;

    return j;
}

// ------------------------------------------------------------------
// buildSaveChunkFlatBuffer – Serialize dirty objects to FlatBuffer
// ------------------------------------------------------------------
std::vector<uint8_t> Game::buildSaveChunkFlatBuffer() {
    flatbuffers::FlatBufferBuilder builder(1024);
    
    std::vector<flatbuffers::Offset<Earthcall::Schema::Entity>> entity_offsets;
    auto& zoneWorld = mgr.active().world();
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
    
    auto chunk_id = builder.CreateString("zone_" + std::to_string(mgr.currentIndex()) + "_delta_" + SaveSystem::timestamp());
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
void Game::loadSaveChunkFlatBuffer(const std::vector<uint8_t>& buffer) {
    // TODO: implement loading
}

// ------------------------------------------------------------------
// saveState – write JSON to an explicit filename
// ------------------------------------------------------------------
void Game::saveState(const std::string& filename) {
    nlohmann::json j = buildSaveJson();
    std::ofstream out(filename);
    out << j.dump(2);
    logIo("SAVE " + filename + ": " +
          std::to_string(_lawManager.getAll().size()) + " law(s), " +
          std::to_string(ConceptRegistry::instance().getAll().size()) + " concept(s)");
}

// ------------------------------------------------------------------
// saveStateWithLog – write JSON via SaveSystem (timestamped file)
// ------------------------------------------------------------------
void Game::saveStateWithLog(const std::string& customName) {
    nlohmann::json j = buildSaveJson();

    // Dynamic objects (skip baseline 0 & 1) – only added by the "log" variant
    auto& zoneWorld = mgr.active().world();
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
          std::to_string(_lawManager.getAll().size()) + " law(s), " +
          std::to_string(ConceptRegistry::instance().getAll().size()) + " concept(s)");
}

// ------------------------------------------------------------------
// loadState
// ------------------------------------------------------------------
void Game::loadState(const std::string& filename) {
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
    logIo("LOAD begin: " + filename);   // even a crash mid-load leaves this trace
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

        // Reset physics registries to avoid stale velocities/bonds affecting freshly loaded objects
        Physics::resetRigidBodies();
        Physics::clearBonds();

        // Load material beings before objects that reference them (older saves
        // without a "materials" key keep the current set + material.default).
        if (j.contains("materials")) materials.loadFromJson(j["materials"]);

        size_t currentZoneIdx = j.value("currentZone", 0);
        auto& zonesVec = mgr.zones(); zonesVec.clear();
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
                        s.lineWidth = 1.0f; // legacy default
                        auto col = sj.value("color", std::vector<float>{1, 1, 1});
                        if (col.size() >= 3) { s.r = col[0]; s.g = col[1]; s.b = col[2]; }
                        s.points = sj.value("points", std::vector<float>{});
                        z.strokes.push_back(std::move(s));
                    }
                }
                // Load 3-D world objects for this zone
                if (zj.contains("world")) {
                    from_json(zj["world"], z.world());
                }
                if (zj.contains("formationRelations")) {
                    for (const auto& relJson : zj["formationRelations"]) {
                        z.formation().add(std::make_shared<Relation>(Relation::fromJson(relJson)));
                    }
                }
                zonesVec.push_back(std::move(z));
            }
        }

        if (zonesVec.empty()) {
            zonesVec.push_back(Zone("Default Zone", "default"));
        }
        // Home survives every load (and pre-ownership saves get one).
        // Must run BEFORE switchTo: addZone may reallocate the zone vector,
        // and switchTo publishes a pointer-carrying zone-entered event.
        ensureHomeZone();
        mgr.switchTo(std::min(currentZoneIdx, zonesVec.size() - 1));

        // Load camera and player view
        if (j.contains("cameraPos")) {
            _camera.pos = glm::vec3(j["cameraPos"][0], j["cameraPos"][1], j["cameraPos"][2]);
        }
        if (j.contains("cameraFront")) {
            _camera.front = glm::vec3(j["cameraFront"][0], j["cameraFront"][1], j["cameraFront"][2]);
        }
        if (j.contains("cameraUp")) {
            _camera.up = glm::vec3(j["cameraUp"][0], j["cameraUp"][1], j["cameraUp"][2]);
        }
        _mouseHandler.setYaw(j.value("yaw", -90.0f));
        _mouseHandler.setPitch(j.value("pitch", 0.0f));

        if (j.contains("currentColor")) {
            _currentColor[0] = j["currentColor"][0];
            _currentColor[1] = j["currentColor"][1];
            _currentColor[2] = j["currentColor"][2];
        }
        _currentTool = Tool(static_cast<Tool::Type>(j.value("currentTool", static_cast<int>(Tool::Type::Brush))));

        _world.setMode(static_cast<Ourverse::GameMode>(j.value("worldMode", static_cast<int>(Ourverse::GameMode::Creative))));
        bool phys = j.value("worldPhysics", true);
        if (_world.isPhysicsEnabled() != phys) _world.togglePhysics();
        Physics::setFlying(j.value("flying", false));

        // Load physics laws
        stage("physics-laws", [&] {
        if (j.contains("physicsLaws")) {
            std::vector<int> ids;
            for (const auto& L : Physics::getLaws()) ids.push_back(L.id);
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
                        law.target.geometryTypes.push_back(static_cast<Object::GeometryType>(gi.get<int>()));
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

        // Player avatar body (includes per-face textures on each body part)
        stage("player-body", [&] {
            if (j.contains("playerBody")) {
                bodyFromJson(j["playerBody"], _player.getBody());
            }
        });

        // The authored register — AFTER the world, so the Universe can
        // resolve the saved identifiers when authors and targets reattach.
        // Restore the clock first: laws must never see time run backward.
        stage("world-clock", [&] {
            _worldTime = j.value("worldTime", 0.0);
            Universe::instance().setClock(_worldTime, 0.0);
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
                _lawManager.loadFromJson(j["authoredLaws"]);
            }
        });

        // Say what actually happened — the loader's testimony.
        std::size_t objectCount = 0;
        for (const auto& zone : mgr.zones()) {
            objectCount += zone.world().getOwnedObjects().size();
        }
        std::size_t authoredCount = 0;
        for (const auto& law : _lawManager.getAll()) {
            if (law && law->isAuthored()) ++authoredCount;
        }
        _saveLoad.lastLoadReport =
            "Loaded: " + std::to_string(objectCount) + " object(s), " +
            std::to_string(_lawManager.getAll().size()) + " law(s) (" +
            std::to_string(authoredCount) + " authored), " +
            std::to_string(ConceptRegistry::instance().getAll().size()) +
            " concept(s), worldTime " + std::to_string(_worldTime);
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
// shutdown
// ------------------------------------------------------------------
void Game::shutdown() {
    // Automatically save game state upon shutdown
    saveStateWithLog();
}

// ------------------------------------------------------------------
// updateSaveFiles
// ------------------------------------------------------------------
void Game::updateSaveFiles() {
    _saveLoad.files = SaveSystem::listFiles(SaveSystem::SaveType::GAME);
}

// ------------------------------------------------------------------
// drawLoadWindow
// ------------------------------------------------------------------
void Game::drawLoadWindow() {
    if (!_saveLoad.showLoadWindow) return;
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Load Game State", &_saveLoad.showLoadWindow, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Select a save file to load:");
        if (!_saveLoad.lastLoadReport.empty()) {
            const bool trouble =
                _saveLoad.lastLoadReport.find("FAIL") != std::string::npos ||
                _saveLoad.lastLoadReport.find("COULD NOT") != std::string::npos;
            ImGui::TextColored(trouble ? ImVec4(1.0f, 0.5f, 0.4f, 1.0f)
                                       : ImVec4(0.5f, 0.9f, 0.5f, 1.0f),
                               "%s", _saveLoad.lastLoadReport.c_str());
        }
        ImGui::Separator();

        // Get save metadata for better display
        auto saveMetadata = SaveSystem::getSaveMetadata(SaveSystem::SaveType::GAME);

        if (saveMetadata.empty()) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No save files found.");
        } else {
            for (const auto& meta : saveMetadata) {
                // Format timestamp for display
                std::time_t time = meta.creationTime;
                std::tm* tm = std::localtime(&time);
                char timeStr[64];
                std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm);

                // Format file size
                std::string sizeStr;
                if (meta.fileSize < 1024) {
                    sizeStr = std::to_string(meta.fileSize) + " B";
                } else if (meta.fileSize < 1024 * 1024) {
                    sizeStr = std::to_string(meta.fileSize / 1024) + " KB";
                } else {
                    sizeStr = std::to_string(meta.fileSize / (1024 * 1024)) + " MB";
                }

                // Create display string
                std::string displayText = meta.customLabel.empty() ?
                    meta.filename : meta.customLabel;
                displayText += " (" + std::string(timeStr) + ", " + sizeStr + ")";

                if (ImGui::Selectable(displayText.c_str())) {
                    loadState(meta.fullPath);
                    _saveLoad.showLoadWindow = false;
                }

                // Show tooltip with full path
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Path: %s", meta.fullPath.c_str());
                }
            }
        }

        ImGui::Separator();

        // Add some management options
        if (ImGui::Button("Refresh")) {
            updateSaveFiles();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clean Old Saves")) {
            SaveSystem::cleanupOldSaves(SaveSystem::SaveType::GAME, 10);
            updateSaveFiles();
        }
        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            _saveLoad.showLoadWindow = false;
        }
    }
    ImGui::End();
}

// ------------------------------------------------------------------
// drawSaveWindow
// ------------------------------------------------------------------
void Game::drawSaveWindow() {
    if (!_saveLoad.showSaveWindow) return;
    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Save Game State", &_saveLoad.showSaveWindow, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Save your current game state:");
        ImGui::Separator();

        ImGui::Text("Save Name (optional):");
        ImGui::InputText("##SaveName", _saveLoad.customName, sizeof(_saveLoad.customName));

        ImGui::Separator();

        if (ImGui::Button("Save with Timestamp")) {
            saveStateWithLog("");
            _saveLoad.showSaveWindow = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Save with Custom Name")) {
            saveStateWithLog(_saveLoad.customName);
            _saveLoad.showSaveWindow = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            _saveLoad.showSaveWindow = false;
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Saves are stored in: saves/games/");
    }
    ImGui::End();
}

// ------------------------------------------------------------------
// drawSaveManager
// ------------------------------------------------------------------
void Game::drawSaveManager() {
    if (!_saveLoad.showManager) return;
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Save Manager", &_saveLoad.showManager, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {

        // Tabs for different save types
        if (ImGui::BeginTabBar("SaveTypes")) {

            // Game saves tab
            if (ImGui::BeginTabItem("Game Saves")) {
                auto gameSaves = SaveSystem::getSaveMetadata(SaveSystem::SaveType::GAME);

                ImGui::Text("Game Saves (%zu files)", gameSaves.size());
                ImGui::Separator();

                if (gameSaves.empty()) {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No game saves found.");
                } else {
                    for (const auto& meta : gameSaves) {
                        std::time_t time = meta.creationTime;
                        std::tm* tm = std::localtime(&time);
                        char timeStr[64];
                        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm);

                        std::string sizeStr;
                        if (meta.fileSize < 1024) {
                            sizeStr = std::to_string(meta.fileSize) + " B";
                        } else if (meta.fileSize < 1024 * 1024) {
                            sizeStr = std::to_string(meta.fileSize / 1024) + " KB";
                        } else {
                            sizeStr = std::to_string(meta.fileSize / (1024 * 1024)) + " MB";
                        }

                        std::string displayText = meta.customLabel.empty() ?
                            meta.filename : meta.customLabel;
                        displayText += " (" + std::string(timeStr) + ", " + sizeStr + ")";
                        if (meta.isCloudOnly) displayText += " [CLOUD]";
                        else displayText += " [LOCAL]";

                        if (ImGui::Selectable(displayText.c_str())) {
                            loadState(meta.fullPath);
                        }

                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Path: %s", meta.fullPath.c_str());
                        }
                    }
                }

                ImGui::Separator();
                if (ImGui::Button("Clean Old Saves")) {
                    SaveSystem::cleanupOldSaves(SaveSystem::SaveType::GAME, 10);
                }
                ImGui::SameLine();
                if (ImGui::Button("Sync & Clear Local")) {
                    std::cout << "[UI] Sync & Clear Local clicked. Cloud foundation active.\n";
                }

                ImGui::EndTabItem();
            }

            // Avatar saves tab
            if (ImGui::BeginTabItem("Avatar Saves")) {
                auto avatarSaves = SaveSystem::getSaveMetadata(SaveSystem::SaveType::AVATAR);

                ImGui::Text("Avatar Saves (%zu files)", avatarSaves.size());
                ImGui::Separator();

                if (avatarSaves.empty()) {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No avatar saves found.");
                } else {
                    for (const auto& meta : avatarSaves) {
                        std::time_t time = meta.creationTime;
                        std::tm* tm = std::localtime(&time);
                        char timeStr[64];
                        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm);

                        std::string sizeStr;
                        if (meta.fileSize < 1024) {
                            sizeStr = std::to_string(meta.fileSize) + " B";
                        } else if (meta.fileSize < 1024 * 1024) {
                            sizeStr = std::to_string(meta.fileSize / 1024) + " KB";
                        } else {
                            sizeStr = std::to_string(meta.fileSize / (1024 * 1024)) + " MB";
                        }

                        std::string displayText = meta.customLabel.empty() ?
                            meta.filename : meta.customLabel;
                        displayText += " (" + std::string(timeStr) + ", " + sizeStr + ")";

                        if (ImGui::Selectable(displayText.c_str())) {
                            // TODO: Load avatar save
                        }

                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Path: %s", meta.fullPath.c_str());
                        }
                    }
                }

                ImGui::Separator();
                if (ImGui::Button("Clean Old Saves")) {
                    SaveSystem::cleanupOldSaves(SaveSystem::SaveType::AVATAR, 10);
                }

                ImGui::EndTabItem();
            }

            // Design saves tab
            if (ImGui::BeginTabItem("Design Saves")) {
                auto designSaves = SaveSystem::getSaveMetadata(SaveSystem::SaveType::DESIGN);

                ImGui::Text("Design Saves (%zu files)", designSaves.size());
                ImGui::Separator();

                if (designSaves.empty()) {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No design saves found.");
                } else {
                    for (const auto& meta : designSaves) {
                        std::time_t time = meta.creationTime;
                        std::tm* tm = std::localtime(&time);
                        char timeStr[64];
                        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm);

                        std::string sizeStr;
                        if (meta.fileSize < 1024) {
                            sizeStr = std::to_string(meta.fileSize) + " B";
                        } else if (meta.fileSize < 1024 * 1024) {
                            sizeStr = std::to_string(meta.fileSize / 1024) + " KB";
                        } else {
                            sizeStr = std::to_string(meta.fileSize / (1024 * 1024)) + " MB";
                        }

                        std::string displayText = meta.customLabel.empty() ?
                            meta.filename : meta.customLabel;
                        displayText += " (" + std::string(timeStr) + ", " + sizeStr + ")";

                        if (ImGui::Selectable(displayText.c_str())) {
                            // TODO: Load design save
                        }

                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Path: %s", meta.fullPath.c_str());
                        }
                    }
                }

                ImGui::Separator();
                if (ImGui::Button("Clean Old Saves")) {
                    SaveSystem::cleanupOldSaves(SaveSystem::SaveType::DESIGN, 10);
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::Separator();
        if (ImGui::Button("Close")) {
            _saveLoad.showManager = false;
        }
    }
    ImGui::End();
}

} // namespace Core
