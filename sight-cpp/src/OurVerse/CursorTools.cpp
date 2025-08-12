#include "CursorTools.hpp"
#include "Core/Game.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "Form/Object/Object.hpp"
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

extern ZoneManager mgr;

CursorTools::CursorTools(ZoneManager* mgrPtr) : _mgr(mgrPtr) {}
CursorTools::CursorTools() : _mgr(&mgr) {}

void CursorTools::clearSelection() {
    _primary = nullptr;
    _secondary = nullptr;
}

static bool raycastObject(const Object& obj,
                          const glm::vec3& rayOriginWorld,
                          const glm::vec3& rayDirWorld,
                          float& outT) {
    int faceIdx; glm::vec2 uv;
    float t;
    if (obj.raycastFace(rayOriginWorld, rayDirWorld, t, faceIdx, uv)) { outT = t; return true; }
    return false;
}

Object* CursorTools::pickObjectAtCursor3D(Core::Game& game) const {
    // Build a ray from cursor using View/Projection/Viewport
    const float mouseX = game.getCursorX();
    const float mouseY = game.getCursorY();
    const GLint* vp = game.getCameraViewport();
    const GLdouble* mv = game.getCameraModelview();
    const GLdouble* pr = game.getCameraProjection();

    // Convert GLdouble arrays to glm::mat4 (column-major)
    glm::mat4 V(1.0f), P(1.0f);
    for (int c=0;c<4;++c) for (int r=0;r<4;++r) {
        V[c][r] = static_cast<float>(mv[c*4 + r]);
        P[c][r] = static_cast<float>(pr[c*4 + r]);
    }
    glm::mat4 invVP = glm::inverse(P * V);

    float ndcX = ( (mouseX - static_cast<float>(vp[0])) / static_cast<float>(vp[2]) ) * 2.0f - 1.0f;
    float ndcY = 1.0f - ( (mouseY - static_cast<float>(vp[1])) / static_cast<float>(vp[3]) ) * 2.0f;
    glm::vec4 nearClip(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 farClip (ndcX, ndcY,  1.0f, 1.0f);
    glm::vec4 nearWorld4 = invVP * nearClip;
    glm::vec4 farWorld4  = invVP * farClip;
    if (nearWorld4.w != 0.0f) nearWorld4 /= nearWorld4.w;
    if (farWorld4.w  != 0.0f) farWorld4  /= farWorld4.w;
    glm::vec3 origin = glm::vec3(nearWorld4);
    glm::vec3 dir    = glm::normalize(glm::vec3(farWorld4 - nearWorld4));

    auto& objects = _mgr->active().world().getOwnedObjects();
    float bestT = 1e9f; Object* best = nullptr;
    for (const auto& up : objects) {
        if (!up) continue; Object* obj = up.get();
        float t;
        if (raycastObject(*obj, origin, dir, t)) {
            if (t > 0.0f && t < bestT) { bestT = t; best = obj; }
        }
    }
    return best;
}

void CursorTools::update(Core::Game& game) {
    if (!_enabled) return;
    // Left mouse click selection: use Keyboard/Mouse handler from game to detect click state if needed
    // Simpler: respond to ImGui IsMouseClicked but we need world context; assume click in world when not over UI
    // Require Ctrl held to initiate selection to avoid interfering with creation tools
    if (ImGui::IsMouseClicked(0) && !ImGui::GetIO().WantCaptureMouse && _selectOnClick && ImGui::GetIO().KeyCtrl) {
        Object* hit = pickObjectAtCursor3D(game);
        if (hit) {
            if (_appendWithShift && (ImGui::GetIO().KeyShift)) {
                _secondary = hit;
            } else {
                _primary = hit;
            }
        }
    }
}

void CursorTools::applyLawToSelection(int lawId) {
    using namespace Physics;
    if (PhysicsLaw* law = getLawById(lawId)) {
        // Use runtime explicit pointers
        law->target.explicitObjects.clear();
        if (_primary) law->target.explicitObjects.push_back(_primary);
        if (_secondary) law->target.explicitObjects.push_back(_secondary);
        law->enabled = true;
    }
}

void CursorTools::renderUI(bool& open) {
    if (!open) return;
    ImGui::Begin("🖱 Cursor Tools", &open);
    ImGui::Checkbox("Enable Picking", &_enabled);
    ImGui::Checkbox("Select on Click", &_selectOnClick);
    ImGui::Checkbox("Shift adds Secondary", &_appendWithShift);
    ImGui::Separator();
    ImGui::Text("Primary: %s", _primary ? _primary->getIdentifier().c_str() : "<none>");
    ImGui::Text("Secondary: %s", _secondary ? _secondary->getIdentifier().c_str() : "<none>");
    if (ImGui::Button("Clear Selection")) clearSelection();
    ImGui::Separator();

    // Apply a selected physics law to current selection
    using namespace Physics;
    static int currentLawId = 0;
    const auto& laws = getLaws();
    // Build simple combo from laws
    std::vector<std::string> labels; labels.reserve(laws.size());
    for (const auto& l : laws) labels.push_back("[#" + std::to_string(l.id) + "] " + l.name);
    if (!labels.empty()) {
        // ImGui combo requires const char* array; build transient
        std::vector<const char*> cstrs; cstrs.reserve(labels.size());
        for (auto& s : labels) cstrs.push_back(s.c_str());
        // find index of currentLawId
        int idx = 0; for (size_t i=0;i<laws.size();++i) if (laws[i].id == currentLawId) { idx = (int)i; break; }
        if (ImGui::Combo("Law", &idx, cstrs.data(), (int)cstrs.size())) currentLawId = laws[idx].id;
        if (ImGui::Button("Apply Law to Selection")) applyLawToSelection(currentLawId);
    } else {
        ImGui::TextDisabled("No laws defined.");
    }

    ImGui::End();
}


