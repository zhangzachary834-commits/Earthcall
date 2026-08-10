// Game.cpp – Constructor, destructor, and small accessors.
// All substantial method implementations live in the Game*.cpp split files:
//   GameInit.cpp        – init(), registerCallbacks(), GLFW callbacks
//   GameUpdate.cpp      – update()
//   GameRender.cpp      – render()
//   GameToolbar.cpp     – renderCreatorToolbar()
//   GamePolyhedron.cpp  – buildCurrentPolyhedron(), _polyhedron.generateCustom()
//   Save/load methods moved to ZoneManager

#include "Game.hpp"
#include "Form/Object/Object.hpp"
#include "OurVerse/AdvancedFacePaint.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <algorithm>
#include <cstdio>
#include <unordered_set>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

extern ZoneManager mgr;

namespace Core {

Game::Game()
    : _elementalToolHandler(&mgr) {}

Game::~Game() {
    printf("[Shutdown] Game dtor\n");

    // Cleanup Advanced Face Paint System
    AdvancedFacePaint::cleanupAdvancedPainter();
}

// ------------------------------------------------------------------
// shutdown - Automatically save game state upon shutdown
// ------------------------------------------------------------------
void Game::shutdown() {
    // Automatically save game state upon shutdown
    saveStateWithLog();
}

// ---- Simple accessors ------------------------------------------------

bool Game::getAdvanced2DBrush() const {
    return _brush.useAdvanced2D;
}

void Game::setAdvanced2DBrush(bool value) {
    _brush.useAdvanced2D = value;
}

bool Game::getUseLegacy2DTools() const {
    return _useLegacy2DTools;
}

void Game::setUseLegacy2DTools(bool value) {
    _useLegacy2DTools = value;
}

bool Game::getMouseLeftPressedLast() const {
    return _mouseLeftPressedLast;
}

void Game::setMouseLeftPressedLast(bool value) {
    _mouseLeftPressedLast = value;
}

void Game::setPlacementMode(BrushPlacementMode mode) {
    _placement.mode = mode;
}

glm::mat4 Game::buildBrushCreateTransform(const glm::vec3& position) const {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
    transform = glm::rotate(transform, glm::radians(_brush.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(_brush.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(_brush.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    transform = glm::scale(transform, glm::vec3(_brush.scale.x * _brush.size,
                                                _brush.scale.y * _brush.size,
                                                _brush.scale.z * _brush.size));
    return transform;
}

float Game::getBrushCreateSurfaceOffset(const glm::vec3& normal) const {
    glm::vec3 n = glm::length(normal) > 1e-6f ? glm::normalize(normal) : glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 rotation(1.0f);
    rotation = glm::rotate(rotation, glm::radians(_brush.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(_brush.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(_brush.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    const glm::vec3 half = glm::vec3(_brush.scale.x * _brush.size,
                                     _brush.scale.y * _brush.size,
                                     _brush.scale.z * _brush.size) * 0.5f;
    const glm::vec3 axisX = glm::normalize(glm::vec3(rotation * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));
    const glm::vec3 axisY = glm::normalize(glm::vec3(rotation * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
    const glm::vec3 axisZ = glm::normalize(glm::vec3(rotation * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)));

    return std::abs(glm::dot(n, axisX)) * half.x +
           std::abs(glm::dot(n, axisY)) * half.y +
           std::abs(glm::dot(n, axisZ)) * half.z +
           0.01f;
}

void Game::setSelectedObject3D(Object* obj) {
    clearSelection3D();
    if (obj) {
        selectObject3D(obj, true);
    }
}

void Game::selectObject3D(Object* obj, bool extendSelection) {
    if (!extendSelection) {
        clearSelection3D();
    }

    if (!obj) return;

    Singular* member = static_cast<Singular*>(obj);
    if (extendSelection && _selectedFormation3D.hasMember(member)) {
        _selectedFormation3D.removeMember(member);
        if (_selectedObject3D == obj) {
            _selectedObject3D = nullptr;
            for (auto* existing : _selectedFormation3D.getMembers()) {
                if (auto* existingObject = dynamic_cast<Object*>(existing)) {
                    _selectedObject3D = existingObject;
                    break;
                }
            }
        }
        return;
    }

    _selectedFormation3D.addMember(member);
    _selectedObject3D = obj;
}

void Game::clearSelection3D() {
    _selectedObject3D = nullptr;
    _selectedFormation3D.clear();
}

std::unordered_set<std::string> Game::getSelectedObjectIds3D() const {
    std::unordered_set<std::string> ids;
    for (auto* member : _selectedFormation3D.getMembers()) {
        auto* obj = dynamic_cast<Object*>(member);
        if (!obj) continue;
        const std::string id = obj->getIdentifier();
        if (!id.empty()) ids.insert(id);
    }
    return ids;
}

void Game::syncSelectedFormationRelations(const Zone& zone) {
    _selectedFormation3D.clearRelations();
    if (_selectedFormation3D.getMembers().empty()) return;

    std::unordered_set<std::string> selectedIds = getSelectedObjectIds3D();
    if (selectedIds.empty()) return;

    for (const auto& relation : zone.formation().relations().getAll()) {
        if (!relation) continue;
        if (selectedIds.find(relation->entityA) == selectedIds.end()) continue;
        if (selectedIds.find(relation->entityB) == selectedIds.end()) continue;
        _selectedFormation3D.addRelation(relation);
    }
}

} // namespace Core
