#include "Ourverse.hpp"
#include <iostream>
#include "imgui.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Form/Object/Object.hpp"
#include <unordered_set>
#include "Rendering/HighlightSystem.hpp"
#include "Rendering/ShadingSystem.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include <unordered_map>

extern ZoneManager mgr;

// This class is for the entire digital existence of Earthcall
// It's called "Ourverse" because it is our creation, an embodiment of everything as we relate to it

void Ourverse::addZone(Zone zone) {
    zones.push_back(zone);
}

void Ourverse::addHome(Home home) {
    homes.push_back(home);
}

void Ourverse::relate(Relation relation) {
    relations.push_back(relation);
}

void Ourverse::display() const {
    std::cout << "🌐 OURVERSE STATUS 🌐" << std::endl;
    for (const auto& z : zones) z.describe();
    for (const auto& h : homes) h.welcome();
    for (const auto& r : relations) r.describe();
}


void Ourverse::renderModeUI() {
    ImGui::Separator();
    ImGui::Text("Game Mode:");
    int mode = static_cast<int>(this->getMode());
    const char* modes[] = {"Creative", "Survival", "Spectator"};
    if (ImGui::Combo("##GameMode", &mode, modes, IM_ARRAYSIZE(modes))) {
        this->setMode(static_cast<Ourverse::GameMode>(mode));
    }

    ImGui::SameLine();
    bool physics = this->isPhysicsEnabled();
    if (ImGui::Checkbox("Physics Enabled", &physics)) {
        this->togglePhysics();
        
    }

    // Lighting toggle
    ImGui::SameLine();
    bool lighting = ShadingSystem::isEnabled();
    if (ImGui::Checkbox("Lighting", &lighting)) {
        ShadingSystem::setEnabled(lighting);
    }

    // --------------------------------------------------------------
    // Physics Laws Manager UI
    // --------------------------------------------------------------
    if (ImGui::CollapsingHeader("Physics Laws")) {
        using namespace Physics;
        static int selectedLawId = 0;

        // List existing laws
        if (ImGui::BeginChild("##laws_list", ImVec2(0, 150), true)) {
            const auto& laws = getLaws();
            for (const auto& law : laws) {
                ImGui::PushID(law.id);
                bool sel = (selectedLawId == law.id);
                std::string label = (law.enabled ? std::string("🟢 ") : std::string("🔴 ")) + law.name + (" [#" + std::to_string(law.id) + "]");
                if (ImGui::Selectable(label.c_str(), sel)) {
                    selectedLawId = law.id;
                }
                ImGui::PopID();
            }
        }
        ImGui::EndChild();

        // Create new law
        if (ImGui::TreeNode("Create New Law")) {
            static char nameBuf[128] = "";
            static int typeIdx = 0; // Gravity, AirResistance, Collision, CustomForce
            static float strength = 9.81f;
            static float damping = 0.1f;
            static glm::vec3 direction(0.0f, -1.0f, 0.0f);
            ImGui::InputText("Name", nameBuf, IM_ARRAYSIZE(nameBuf));
            const char* typeNames[] = {"Gravity", "AirResistance", "Collision", "CustomForce", "GravityField", "CenterGravity"};
            ImGui::Combo("Type", &typeIdx, typeNames, IM_ARRAYSIZE(typeNames));
            ImGui::DragFloat("Strength", &strength, 0.01f, -1000.f, 1000.f);
            ImGui::DragFloat("Damping", &damping, 0.01f, 0.f, 10.f);
            ImGui::DragFloat3("Direction", &direction[0], 0.01f, -1.f, 1.f);

            // Targeting
            static bool allObjects = true;
            static bool byGeom = false; static bool geomCube=true, geomSphere=false, geomCyl=false, geomCone=false, geomPoly=false;
            static bool byType = false; static char typeBuf[128]="";
            static bool byAttr = false; static char attrKey[128]=""; static char attrVal[128]="";
            static bool byTag = false; static char tagBuf[128]="";
            ImGui::Checkbox("All objects", &allObjects);
            ImGui::Checkbox("Filter by Geometry", &byGeom);
            if (byGeom) {
                ImGui::SameLine(); ImGui::Checkbox("Cube", &geomCube);
                ImGui::SameLine(); ImGui::Checkbox("Sphere", &geomSphere);
                ImGui::SameLine(); ImGui::Checkbox("Cylinder", &geomCyl);
                ImGui::SameLine(); ImGui::Checkbox("Cone", &geomCone);
                ImGui::SameLine(); ImGui::Checkbox("Polyhedron", &geomPoly);
            }
            ImGui::Checkbox("Filter by Object Type", &byType);
            if (byType) ImGui::InputText("Type equals", typeBuf, IM_ARRAYSIZE(typeBuf));
            ImGui::Checkbox("Filter by Attribute", &byAttr);
            if (byAttr) {
                ImGui::InputText("Attr Key", attrKey, IM_ARRAYSIZE(attrKey));
                ImGui::InputText("Attr Value", attrVal, IM_ARRAYSIZE(attrVal));
            }
            ImGui::Checkbox("Filter by Tag", &byTag);
            if (byTag) ImGui::InputText("Tag", tagBuf, IM_ARRAYSIZE(tagBuf));

            if (ImGui::Button("Create")) {
                PhysicsLaw law;
                law.name = nameBuf[0] ? nameBuf : std::string("Law ") + std::to_string((int)getLaws().size()+1);
                law.type = static_cast<LawType>(typeIdx);
                law.enabled = true;
                law.strength = strength;
                law.damping = damping;
                law.direction = direction;
                law.target.allObjects = allObjects;
                law.target.limitByGeometry = byGeom;
                law.target.geometryTypes.clear();
                if (geomCube) law.target.geometryTypes.push_back(Object::GeometryType::Cube);
                if (geomSphere) law.target.geometryTypes.push_back(Object::GeometryType::Sphere);
                if (geomCyl) law.target.geometryTypes.push_back(Object::GeometryType::Cylinder);
                if (geomCone) law.target.geometryTypes.push_back(Object::GeometryType::Cone);
                if (geomPoly) law.target.geometryTypes.push_back(Object::GeometryType::Polyhedron);
                law.target.limitByObjectType = byType;
                if (byType && typeBuf[0]) law.target.objectTypes = { std::string(typeBuf) };
                law.target.limitByAttribute = byAttr;
                if (byAttr) { law.target.attributeKey = attrKey; law.target.attributeValue = attrVal; }
                law.target.limitByTag = byTag;
                if (byTag && tagBuf[0]) law.target.tag = tagBuf;
                addLaw(law);
                nameBuf[0] = '\0'; typeIdx = 0; strength = 9.81f; damping = 0.1f; direction = glm::vec3(0, -1, 0);
            }
            ImGui::TreePop();
        }

        // Edit selected law
        if (PhysicsLaw* sel = getLawById(selectedLawId)) {
            ImGui::Separator();
            ImGui::Text("Edit Law #%d", sel->id);
            char nameBuf[128];
            std::snprintf(nameBuf, sizeof(nameBuf), "%s", sel->name.c_str());
            if (ImGui::InputText("Name##edit", nameBuf, IM_ARRAYSIZE(nameBuf))) sel->name = nameBuf;
            int tIdx = static_cast<int>(sel->type);
            const char* typeNames[] = {"Gravity", "AirResistance", "Collision", "CustomForce", "GravityField", "CenterGravity"};
            if (ImGui::Combo("Type##edit", &tIdx, typeNames, IM_ARRAYSIZE(typeNames))) sel->type = static_cast<LawType>(tIdx);
            ImGui::Checkbox("Enabled", &sel->enabled);
            ImGui::DragFloat("Strength##edit", &sel->strength, 0.01f, -1000.f, 1000.f);
            ImGui::DragFloat("Damping##edit", &sel->damping, 0.01f, 0.f, 10.f);
            ImGui::DragFloat3("Direction##edit", &sel->direction[0], 0.01f, -1.f, 1.f);

            if (ImGui::Button("Delete Law")) {
                removeLaw(sel->id);
                selectedLawId = 0;
            }

            // Targets quick view
            if (ImGui::TreeNode("Targets")) {
                ImGui::Text("All objects: %s", sel->target.allObjects ? "Yes" : "No");
                ImGui::Text("By Geometry: %s", sel->target.limitByGeometry ? "Yes" : "No");
                ImGui::Text("By Type: %s", sel->target.limitByObjectType ? "Yes" : "No");
                ImGui::Text("By Attribute: %s", sel->target.limitByAttribute ? "Yes" : "No");
                ImGui::Text("By Tag: %s", sel->target.limitByTag ? "Yes" : "No");
                // Integrate current 3D selection (via hover as a proxy)
                extern ZoneManager mgr;
                if (ImGui::Button("Add current 3D selection")) {
                    // We can't reach Core::Game to ask for _selectedObject3D here without ref wiring.
                    // Minimal non-breaking approach: find a uniquely hovered object (if any) and add its ID.
                    // As a simple proxy, add the first object whose getIsHovered() is true.
                    const auto& objects = mgr.active().world().getOwnedObjects();
                    for (const auto& up : objects) {
                        if (!up) continue;
                        const Object* o = up.get();
                        if (o->getIsHovered()) {
                            std::string id = o->getIdentifier();
                            if (!id.empty()) {
                                sel->target.limitByExplicitList = true;
                                sel->target.objectIdentifiers.push_back(id);
                            }
                            break;
                        }
                    }
                }
                // Also publish selected law candidate IDs to rendering highlight system
                /*
                {
                    std::unordered_set<std::string> ids;
                    for (const auto& id : sel->target.objectIdentifiers) ids.insert(id);
                    // Avoid direct include dependency to keep UI isolated; forward-declare linkage is not practical here.
                    // Instead, update via a simple global hook exposed in HighlightSystem
                    extern void HighlightSystem_setLawIds(const std::unordered_set<std::string>&);
                    HighlightSystem_setLawIds(ids);
                }*/
                 // Defer highlight list update to the checkbox section where we already aggregate selected IDs

                ImGui::TreePop();
            }
        }

        // Global gravity tunables & visualization
        if (ImGui::TreeNode("Gravity Field Settings")) {
            float G, eps; Physics::getGravityConstants(G, eps);
            if (ImGui::DragFloat("G (strength)", &G, 0.01f, 0.0f, 1000.0f)) Physics::setGravityConstants(G, eps);
            if (ImGui::DragFloat("Softening Epsilon", &eps, 0.001f, 0.0f, 10.0f)) Physics::setGravityConstants(G, eps);
            bool viz = Physics::getGravityVisualization();
            if (ImGui::Checkbox("Visualize Gravity Field", &viz)) Physics::setGravityVisualization(viz);
            int dens = Physics::getGravityVisualizationDensity();
            if (ImGui::DragInt("Viz Density (per axis)", &dens, 1, 2, 32)) Physics::setGravityVisualizationDensity(dens);
            ImGui::TreePop();
        }

        // Zone objects browser
        if (ImGui::CollapsingHeader("Zone Objects")) {
            ImGui::Text("Browse objects in active zone:");
            auto& objs = mgr.active().world().getOwnedObjects();
            static char filterType[128] = ""; static char filterAttrKey[128] = ""; static char filterAttrVal[128] = ""; static char filterTag[128] = "";
            static std::unordered_map<std::string, bool> selectedById; // only for objects with identifiers
            ImGui::InputText("Type contains", filterType, IM_ARRAYSIZE(filterType));
            ImGui::InputText("Attr Key", filterAttrKey, IM_ARRAYSIZE(filterAttrKey));
            ImGui::InputText("Attr Value", filterAttrVal, IM_ARRAYSIZE(filterAttrVal));
            ImGui::InputText("Tag contains", filterTag, IM_ARRAYSIZE(filterTag));
            if (ImGui::BeginChild("##obj_list", ImVec2(0, 150), true)) {
                for (const auto& up : objs) {
                    if (!up) continue; const Object* o = up.get();
                    bool pass = true;
                    if (filterType[0] && o->getObjectType().find(filterType) == std::string::npos) pass = false;
                    if (pass && filterAttrKey[0]) {
                        if (!o->hasAttribute(filterAttrKey)) pass = false;
                        else if (filterAttrVal[0] && o->getAttribute(filterAttrKey) != std::string(filterAttrVal)) pass = false;
                    }
                    if (pass && filterTag[0]) {
                        bool any = false; for (const auto& tg : o->getTags()) { if (tg.find(filterTag) != std::string::npos) { any = true; break; } }
                        if (!any) pass = false;
                    }
                    if (!pass) continue;
                    std::string id = o->getIdentifier();
                    if (id.empty()) {
                        ImGui::Text("%s", "(unnamed)"); ImGui::SameLine(); ImGui::TextDisabled("type=%s", o->getObjectType().c_str());
                    } else {
                        bool sel = selectedById[id];
                        if (ImGui::Checkbox(id.c_str(), &sel)) selectedById[id] = sel;
                        ImGui::SameLine(); ImGui::TextDisabled("type=%s", o->getObjectType().c_str());
                    }
                }
            }
            ImGui::EndChild();

            // Attach selection to law (from checkbox list)
            if (PhysicsLaw* sel = getLawById(selectedLawId)) {
                if (ImGui::Button("Apply selection to law")) {
                    sel->target.limitByExplicitList = true;
                    sel->target.objectIdentifiers.clear();
                    for (const auto& kv : selectedById) if (kv.second) sel->target.objectIdentifiers.push_back(kv.first);
                    // Publish list to highlight system for red outline
                    Rendering::HighlightSystem::setLawCandidateIds({});
                    {
                        std::unordered_set<std::string> ids;
                        for (const auto& kv : selectedById) if (kv.second) ids.insert(kv.first);
                        Rendering::HighlightSystem::setLawCandidateIds(ids);
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear selection from law")) {
                    sel->target.limitByExplicitList = false;
                    sel->target.objectIdentifiers.clear();
                    Rendering::HighlightSystem::setLawCandidateIds({});
                }
                // Minimal integration: also allow adding current 3D selection by identifier
                // We read the last selected object via mgr.active().world() by checking for an object whose id matches a known selection source is not available here.
                // Provide a small input to paste the selected id quickly and a button to add it, without removing the existing input UX
                static char idBuf[128] = "";
                ImGui::InputText("Add Object ID", idBuf, IM_ARRAYSIZE(idBuf));
                ImGui::SameLine(); if (ImGui::Button("Add ID to Law")) {
                    if (idBuf[0]) {
                        sel->target.limitByExplicitList = true;
                        sel->target.objectIdentifiers.push_back(std::string(idBuf));
                        std::unordered_set<std::string> ids;
                        for (const auto& kv : selectedById) if (kv.second) ids.insert(kv.first);
                        ids.insert(std::string(idBuf));
                        Rendering::HighlightSystem::setLawCandidateIds(ids);
                        idBuf[0] = '\0';
                    }
                }
                if (ImGui::Button("Enable law and apply now")) {
                    sel->enabled = true;
                }
            }
        }
    }
}

void Ourverse::updateObjectCollisions(glm::vec3& position, const Object& obj, const glm::mat4& transform) const {
    obj.updateCollisionZone(transform);
    if (obj.isPointInside(position)) {
        // Push the position out to the nearest face of the cube's AABB
        glm::vec3 minCorner = obj.collisionZone.corners[0];
        glm::vec3 maxCorner = obj.collisionZone.corners[0];
        for (int i = 1; i < 8; ++i) {
            minCorner = glm::min(minCorner, obj.collisionZone.corners[i]);
            maxCorner = glm::max(maxCorner, obj.collisionZone.corners[i]);
        }
        // Find the closest face and move the position to that face
        float dx = std::min(std::abs(position.x - minCorner.x), std::abs(position.x - maxCorner.x));
        float dy = std::min(std::abs(position.y - minCorner.y), std::abs(position.y - maxCorner.y));
        float dz = std::min(std::abs(position.z - minCorner.z), std::abs(position.z - maxCorner.z));
        if (dx <= dy && dx <= dz) {
            position.x = (std::abs(position.x - minCorner.x) < std::abs(position.x - maxCorner.x)) ? minCorner.x : maxCorner.x;
        } else if (dy <= dx && dy <= dz) {
            position.y = (std::abs(position.y - minCorner.y) < std::abs(position.y - maxCorner.y)) ? minCorner.y : maxCorner.y;
        } else {
            position.z = (std::abs(position.z - minCorner.z) < std::abs(position.z - maxCorner.z)) ? minCorner.z : maxCorner.z;
        }
    }
}

void Ourverse::onUpdate(float deltaTime) {
    if (!cameraPos) return;
    // Determine the visible ground height so physics collisions align with the rendered plane
    float groundY = 0.0f;
    if (ownedObjects.size() > 1 && ownedObjects[1]) {
        const glm::mat4& gT = ownedObjects[1]->getTransform();
        // Column 1 represents the Y axis after scaling/rotation; its length is the current scale on Y
        float scaleY = glm::length(glm::vec3(gT[1]));
        groundY = gT[3][1] + 0.5f * scaleY; // translation Y + half the total height
    }

    Physics::applyGravity(*cameraPos,
                          physicsEnabled,
                          static_cast<Physics::GameMode>(mode),
                          deltaTime,
                          groundY);
    // Disallow flying in Survival
    if (mode == GameMode::Survival && Physics::getFlying()) {
        Physics::setFlying(false);
    }
    if (physicsEnabled) {
        // Ensure every owned object has a physics body (zone-level toggle is true by default)
        for (const auto& upObj : ownedObjects) {
            if (upObj) {
                Physics::getBodyFor(upObj.get());
            }
        }

        // Step physics for all object bodies and bonds
        Physics::updateBodies(ownedObjects, deltaTime, /*gravityAccel*/ 9.81f, /*airResistance*/ 0.1f, groundY);

        Physics::enforceCollisions(*cameraPos, ownedObjects);
    }
}

void Ourverse::clearDynamicObjects() {
    if (ownedObjects.size() > 2) {
        ownedObjects.erase(ownedObjects.begin() + 2, ownedObjects.end());
    }
}