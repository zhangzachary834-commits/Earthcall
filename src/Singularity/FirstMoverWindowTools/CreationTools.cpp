#include "Singularity/FirstMoverWindowTools/CreationTools.hpp"

#include "Singularity/FirstMoverWindowTools/CreatorConsole/CreatorConsoleState.hpp"
#include "Singularity/FirstMoverWindowTools/Tool.hpp"

#include "Singularity/Core/CreationChannel.hpp"
#include "Singularity/Core/Engine.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ConstructedBeing/Object/Object.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/World/World.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Screen/GL/GluCompat.hpp"
#include "Singularity/Screen/HighlightSystem.hpp"

#include <imgui.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <vector>

namespace Rendering {

const char* toolNameForMode(Mode3D mode) {
    switch (mode) {
        case Mode3D::BrushCreate: return "ShapeGenerator3D";
        case Mode3D::Selection:   return "Selection3D";
        case Mode3D::FaceBrush:   return "FaceBrush";
        case Mode3D::FacePaint:   return "FacePaint";
        case Mode3D::Pottery:     return "Pottery3D";
        case Mode3D::Rotation:    return "Rotate3D";
        case Mode3D::Morph:       return "Morph";
        case Mode3D::Combine:     return "Combine";
        case Mode3D::Sculpt:      return "Sculpt";
        case Mode3D::Clay:        return "Clay";
        case Mode3D::Graph:       return "Graph";
        case Mode3D::None:        return "";
    }
    return "";
}

const char* activeModeFor(Mode3D mode) {
    switch (mode) {
        case Mode3D::BrushCreate: return "Create";
        case Mode3D::Selection:   return "Select";
        case Mode3D::FaceBrush:   return "FaceBrush";
        case Mode3D::FacePaint:   return "FacePaint";
        case Mode3D::Pottery:     return "Pottery";
        case Mode3D::Rotation:    return "Rotate";
        case Mode3D::Morph:       return "Morph";
        case Mode3D::Combine:     return "Combine";
        case Mode3D::Sculpt:      return "Sculpt";
        case Mode3D::Clay:        return "Clay";
        case Mode3D::Graph:       return "Graph";
        case Mode3D::None:        return "";
    }
    return "";
}

void apply3DMode(CreatorConsoleState& state,
                 Singularity::Core::CreationChannel* channel,
                 Mode3D mode) {
    state.current3DMode = mode;
    state.combineOperandA = nullptr;
    state.clayGrabbed = nullptr;
    state.clayTarget = nullptr;

    if (mode == Mode3D::FacePaint) {
        state.currentTool = Tool(Tool::Type::FacePaint);
    } else if (mode == Mode3D::FaceBrush) {
        state.currentTool = Tool(Tool::Type::FaceBrush);
    } else if (mode == Mode3D::Graph) {
        state.showLawAuthor = true;
    }

    if (channel) {
        channel->activeTool = toolNameForMode(mode);
        channel->active3DMode = activeModeFor(mode);
    }
}

namespace {

std::vector<Object*> collectWorldTargets(ZoneManager& zoneMgr) {
    std::vector<Object*> targets;
    for (const auto& up : zoneMgr.active().world().getOwnedObjects()) {
        if (up && !(up->hasAttribute("baseline") &&
                    up->getAttribute("baseline") == std::string("ground"))) {
            targets.push_back(up.get());
        }
    }
    return targets;
}

// L arms the spawn law only. Does not change console Create mode.
// Callers of spawnLawArmed: CreationChannel::spawnLawArmed.
void handleCreateLawKey(GLFWwindow* window, Core::Engine& engine,
                        Singularity::Core::CreationChannel& channel) {
    if (!window) return;
    if (engine.getMainMenu().isOpen()) return;
    if (ImGui::GetIO().WantCaptureKeyboard) return;

    // BENEATH THE KERNEL: edge latch so a held L publishes once, not
    // as a per-frame level. Same reason Tool::ShapeGenerator3D tracks
    // its mouse button before any gate.
    static bool lawModeKeyDownLast = false;
    const bool lawModeKeyDown = glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS;
    if (lawModeKeyDown && !lawModeKeyDownLast) {
        channel.spawnLawArmed = !channel.spawnLawArmed;
    }
    lawModeKeyDownLast = lawModeKeyDown;
}

void stepMorphTool(GLFWwindow* window, Core::Engine* engine,
                   CreatorConsoleState& state) {
    Object* obj = state.selectedObject3D;
    if (!obj || !window || !engine || !engine->getCamera()) return;
    if (ImGui::GetIO().WantCaptureMouse) return;

    static bool morphMouseLeftPressedLast = false;
    const bool mouseLeftNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool pressEdge = mouseLeftNow && !morphMouseLeftPressedLast;
    morphMouseLeftPressedLast = mouseLeftNow;

    double xpos = 0.0, ypos = 0.0;
    glfwGetCursorPos(window, &xpos, &ypos);
    const int* vp = engine->getCamera()->getViewport();
    const GLdouble* mv = engine->getCamera()->getModelview();
    const GLdouble* pr = engine->getCamera()->getProjection();
    double winX = xpos;
    double winY = vp[3] - ypos;

    if (obj->isBinaryField()) {
        engine->handleFieldGizmos(obj, pressEdge, mouseLeftNow, winX, winY);
        return;
    }

    glm::mat4 xf = obj->getTransform();
    if (obj->getShapeKind() == ObjectTypes::ShapeKind::Polyhedron &&
        obj->getPolyhedronVertexCount() > 0) {
        if (pressEdge) {
            int best = -1; double bestD = 1e18;
            for (int i = 0; i < obj->getPolyhedronVertexCount(); ++i) {
                glm::vec3 w = glm::vec3(xf * glm::vec4(obj->getPolyhedronVertexLocal(i), 1.0f));
                GLdouble sx, sy, sz;
                if (ecgl::project(w.x, w.y, w.z, mv, pr, vp, &sx, &sy, &sz)) {
                    double d = (sx - winX) * (sx - winX) + (sy - winY) * (sy - winY);
                    if (d < bestD) { bestD = d; best = i; }
                }
            }
            if (best >= 0 && bestD < 40.0 * 40.0) state.morphVertexIndex = best;
        }
        if (mouseLeftNow && state.morphVertexIndex >= 0 &&
            state.morphVertexIndex < obj->getPolyhedronVertexCount()) {
            glm::vec3 wv = glm::vec3(xf * glm::vec4(obj->getPolyhedronVertexLocal(state.morphVertexIndex), 1.0f));
            GLdouble sx, sy, sz;
            if (ecgl::project(wv.x, wv.y, wv.z, mv, pr, vp, &sx, &sy, &sz)) {
                GLdouble nx, ny, nz;
                if (ecgl::unProject(winX, winY, sz, mv, pr, vp, &nx, &ny, &nz)) {
                    glm::vec3 local = glm::vec3(glm::inverse(xf) *
                        glm::vec4((float)nx, (float)ny, (float)nz, 1.0f));
                    obj->setPolyhedronVertexLocal(state.morphVertexIndex, local);
                }
            }
        }
        return;
    }

    if (obj->isPatch() && obj->getPatchControlCount() > 0) {
        if (pressEdge) {
            int best = -1; double bestD = 1e18;
            for (int i = 0; i < obj->getPatchControlCount(); ++i) {
                glm::vec3 w = glm::vec3(xf * glm::vec4(obj->getPatchControlLocal(i), 1.0f));
                GLdouble sx, sy, sz;
                if (ecgl::project(w.x, w.y, w.z, mv, pr, vp, &sx, &sy, &sz)) {
                    double d = (sx - winX) * (sx - winX) + (sy - winY) * (sy - winY);
                    if (d < bestD) { bestD = d; best = i; }
                }
            }
            if (best >= 0 && bestD < 40.0 * 40.0) state.patchCtrlIndex = best;
        }
        if (mouseLeftNow && state.patchCtrlIndex >= 0 &&
            state.patchCtrlIndex < obj->getPatchControlCount()) {
            glm::vec3 wv = glm::vec3(xf * glm::vec4(obj->getPatchControlLocal(state.patchCtrlIndex), 1.0f));
            GLdouble sx, sy, sz;
            if (ecgl::project(wv.x, wv.y, wv.z, mv, pr, vp, &sx, &sy, &sz)) {
                GLdouble nx, ny, nz;
                if (ecgl::unProject(winX, winY, sz, mv, pr, vp, &nx, &ny, &nz)) {
                    glm::vec3 local = glm::vec3(glm::inverse(xf) *
                        glm::vec4((float)nx, (float)ny, (float)nz, 1.0f));
                    obj->setPatchControlLocal(state.patchCtrlIndex, local);
                }
            }
        }
    }
}

bool creatorToolIsUp(Core::Engine* engine, Mode3D mode) {
    if (!engine || !engine->getLawManager()) return true;
    const char* id = Singularity::Core::creatorToolLawIdForMode(activeModeFor(mode));
    if (!id || !*id) return true;
    Law* law = engine->getLawManager()->find(id);
    return !law || law->isEnabled();
}

void dispatchActiveTool(GLFWwindow* window, Core::Engine* engine,
                        ZoneManager& zoneMgr, float dt,
                        CreatorConsoleState& state,
                        Singularity::Core::CreationChannel* channel) {
    if (!creatorToolIsUp(engine, state.current3DMode)) return;
    switch (state.current3DMode) {
        case Mode3D::BrushCreate: {
            if (channel) {
                Tool::ShapeGenerator3D(window, engine, zoneMgr, *channel, nullptr);
            }
            break;
        }
        case Mode3D::Selection: {
            Tool::Selection3D(window, engine, collectWorldTargets(zoneMgr));
            break;
        }
        case Mode3D::FaceBrush: {
            Tool::FaceBrush(window, engine, zoneMgr, dt, collectWorldTargets(zoneMgr));
            break;
        }
        case Mode3D::FacePaint: {
            Tool::FacePaint(window, engine, zoneMgr, dt, collectWorldTargets(zoneMgr));
            break;
        }
        case Mode3D::Pottery: {
            Tool::Pottery3D(window, engine, zoneMgr, dt, collectWorldTargets(zoneMgr), nullptr);
            break;
        }
        case Mode3D::Rotation: {
            Tool::Rotate3D(window, engine, zoneMgr, dt, collectWorldTargets(zoneMgr), nullptr);
            break;
        }
        case Mode3D::Morph:
            stepMorphTool(window, engine, state);
            break;
        case Mode3D::Combine: {
            std::vector<Object*> targets = collectWorldTargets(zoneMgr);
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
                state.combineOperandA = nullptr;
            }
            if (!ImGui::GetIO().WantCaptureMouse) {
                // BENEATH THE KERNEL: mouse-edge latch for the combine pick.
                static bool combineMouseLeftPressedLast = false;
                const bool mouseLeftNow =
                    glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
                if (mouseLeftNow && !combineMouseLeftPressedLast) {
                    Object* pick = Tool::PickObject3D(window, engine, targets);
                    if (pick) {
                        if (!state.combineOperandA) {
                            state.combineOperandA = pick;
                        } else if (pick != state.combineOperandA) {
                            if (engine) engine->fuseObjects(state.combineOperandA, pick);
                        }
                    }
                }
                combineMouseLeftPressedLast = mouseLeftNow;
            }
            break;
        }
        case Mode3D::Sculpt:
        case Mode3D::Clay: {
            std::vector<Object*> targets = collectWorldTargets(zoneMgr);
            if (!ImGui::GetIO().WantCaptureMouse) {
                // BENEATH THE KERNEL: mouse-edge latch for the clay grab.
                static bool clayMouseLeftPressedLast = false;
                const bool mouseLeftNow =
                    glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
                if (mouseLeftNow && !clayMouseLeftPressedLast) {
                    state.clayGrabbed = Tool::PickObject3D(window, engine, targets);
                }
                if (!mouseLeftNow && state.clayGrabbed) {
                    if (state.clayTarget && state.clayTarget != state.clayGrabbed) {
                        if (engine) engine->fuseObjects(state.clayTarget, state.clayGrabbed);
                    }
                    state.clayGrabbed = nullptr;
                    state.clayTarget = nullptr;
                }
                clayMouseLeftPressedLast = mouseLeftNow;
            }
            break;
        }
        case Mode3D::Graph:
            state.showLawAuthor = true;
            break;
        case Mode3D::None:
            break;
    }
}

} // namespace

void stepCreationTools(GLFWwindow* window, Core::Engine* engine,
                       ZoneManager& zoneMgr, float dt,
                       bool creatorConsoleOpen) {
    if (!engine) return;

    auto& state = getCreatorConsoleState();
    auto* channel = engine->getLawManager()
        ? Singularity::Core::CreationChannel::find(*engine->getLawManager())
        : nullptr;

    // Opening the 3D tab with no mode yet is the old chrome default:
    // Create is selected. Doing that here — not in the initializer —
    // keeps boot clicks from taking the developer bypass.
    if (creatorConsoleOpen &&
        state.currentSection == CreatorSection::Create3D &&
        state.current3DMode == Mode3D::None) {
        apply3DMode(state, channel, Mode3D::BrushCreate);
    }

    if (channel) {
        handleCreateLawKey(window, *engine, *channel);
        channel->writeLiveSelection(
            toolNameForMode(state.current3DMode),
            activeModeFor(state.current3DMode),
            static_cast<int>(state.polyhedron.shapeKind),
            state.brush.rotation,
            state.brush.scale * state.brush.size,
            state.brush.gridSnap,
            state.brush.gridSize,
            state.createColor);
        if (window) {
            Tool::UpdateShapeGeneratorPlacement(window, engine, zoneMgr, *channel);
        }
    }

    if (window) {
        dispatchActiveTool(window, engine, zoneMgr, dt, state, channel);
    }
}

} // namespace Rendering
