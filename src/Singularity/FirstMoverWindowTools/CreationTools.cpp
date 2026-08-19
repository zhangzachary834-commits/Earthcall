#include "Singularity/FirstMoverWindowTools/CreationTools.hpp"

#include "Singularity/FirstMoverWindowTools/CreatorConsole/CreatorConsoleState.hpp"
#include "Singularity/FirstMoverWindowTools/Tool.hpp"

#include "Singularity/Core/CreationChannel.hpp"
#include "Singularity/Core/Engine.hpp"
#include "ConstructedBeing/Object/Object.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/World/World.hpp"

#include <imgui.h>
#include <GLFW/glfw3.h>

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

// L arms the shape-generator law by writing active3DMode == "Create".
// This used to live in renderDeveloperToolsWindow, above its *open
// return, so it ran without the window — but still inside a render
// function, and without WantCaptureKeyboard, so typing 'l' into any
// text field toggled the world's spawn law.
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
        channel.active3DMode = (channel.active3DMode == "Create") ? "" : "Create";
    }
    lawModeKeyDownLast = lawModeKeyDown;
}

void dispatchActiveTool(GLFWwindow* window, Core::Engine* engine,
                        ZoneManager& zoneMgr, float dt,
                        CreatorConsoleState& state,
                        Singularity::Core::CreationChannel* channel) {
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
