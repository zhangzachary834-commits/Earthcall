// The Creation first mover's modularity seams, held to their word.
//
// Tool dispatch and the console's live selection used to live inside
// render3DConsole. Collapsing the Creator Console froze every 3D tool, and
// the six fields spawn laws read were only copied while that tab was on
// screen. The door is now CreationChannel::writeLiveSelection, called from
// Rendering::stepCreationTools (Engine::update), never from a render
// function. This test cannot boot the window; it holds the door itself:
// find() is how every caller locates the first mover, writeLiveSelection
// is how the chrome writes registered paths, apply3DMode writes the one
// Create bit (active3DMode == "Create") that L also writes.

#include "ConstructedBeing/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Singularity/Core/CreationChannel.hpp"
#include "Singularity/FirstMoverWindowTools/CreationTools.hpp"
#include "Singularity/FirstMoverWindowTools/CreatorConsole/CreatorConsoleState.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <cstdio>
#include <string>

namespace {

int g_failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        ++g_failures;
        std::printf("  FAILED: %s\n", what.c_str());
        return;
    }
    std::printf("  ok: %s\n", what.c_str());
}

} // namespace

int main() {
    std::printf("Running Creation tools modularity test...\n");

    LawManager laws;
    check(Singularity::Core::CreationChannel::find(laws) == nullptr,
          "find is empty before the first mover is registered");

    Singularity::Core::CreationChannel::syncRegister(laws);
    auto* channel = Singularity::Core::CreationChannel::find(laws);
    check(channel != nullptr, "find returns the registered CreationChannel");
    check(channel == Singularity::Core::CreationChannel::find(laws),
          "find is stable — the same first mover, not a new one");
    check(channel->getIdentifier() == "creation-channel",
          "stable identifier law text can name");

    Singularity::Core::CreationChannel::syncRegister(laws);
    check(laws.getAll().size() == 1,
          "syncRegister is idempotent — a second call does not mint another");

    // ---- writeLiveSelection is the door onto registered paths --------------
    channel->writeLiveSelection(
        "ShapeGenerator3D",
        "Create",
        static_cast<int>(Object::ShapeKind::Sphere),
        glm::vec3(10.0f, 20.0f, 30.0f),
        glm::vec3(2.0f, 3.0f, 4.0f),
        true,
        0.5f,
        glm::vec3(0.25f, 0.5f, 0.75f));

    auto read = [&](const char* path, PropertyValue& v) {
        return PropertyPath::parse(path).getValue(*channel, v) ==
               PropertyPath::PathResult::Ok;
    };

    PropertyValue v;
    check(read("activeTool", v) && std::get<std::string>(v) == "ShapeGenerator3D",
          "activeTool is registered and holds the written slug");
    check(read("active3DMode", v) && std::get<std::string>(v) == "Create",
          "active3DMode is registered and holds the written Create bit");
    check(read("activeShapeKind", v) &&
              std::get<int>(v) == static_cast<int>(Object::ShapeKind::Sphere),
          "activeShapeKind is registered and holds the written kind");
    check(read("cursorSpawnRot", v) &&
              std::get<glm::vec3>(v) == glm::vec3(10.0f, 20.0f, 30.0f),
          "cursorSpawnRot is registered and holds the written rotation");
    check(read("cursorSpawnScale", v) &&
              std::get<glm::vec3>(v) == glm::vec3(2.0f, 3.0f, 4.0f),
          "cursorSpawnScale is registered and holds the written scale");
    check(read("gridSnap", v) && std::get<bool>(v),
          "gridSnap is registered and holds the written flag");
    check(read("gridSnapSize", v) && std::get<float>(v) == 0.5f,
          "gridSnapSize is registered and holds the written size");
    check(read("activeColor", v) &&
              std::get<glm::vec3>(v) == glm::vec3(0.25f, 0.5f, 0.75f),
          "activeColor is registered and holds the written colour");

    // ---- one Create bit: console Create and L write the same mode ----------
    check(std::string(Rendering::toolNameForMode(Rendering::Mode3D::BrushCreate)) ==
              "ShapeGenerator3D",
          "console Create maps to the ShapeGenerator3D tool slug");
    check(std::string(Rendering::activeModeFor(Rendering::Mode3D::BrushCreate)) ==
              "Create",
          "console Create maps to active3DMode \"Create\" — the spawn law's gate");
    check(std::string(Rendering::toolNameForMode(Rendering::Mode3D::Selection)) ==
              "Selection3D",
          "console Select maps to Selection3D");
    check(std::string(Rendering::activeModeFor(Rendering::Mode3D::Selection)) ==
              "Select",
          "Select is not Create — the spawn law stays down");

    Rendering::CreatorConsoleState state;
    check(state.current3DMode == Rendering::Mode3D::None,
          "console boots with no tool armed — BrushCreate is a choice, not a default actuation");
    channel->active3DMode = "";
    Rendering::apply3DMode(state, channel, Rendering::Mode3D::BrushCreate);
    check(state.current3DMode == Rendering::Mode3D::BrushCreate,
          "apply3DMode writes the chrome mode");
    check(channel->activeTool == "ShapeGenerator3D",
          "apply3DMode writes @creation-channel.activeTool");
    check(channel->active3DMode == "Create",
          "console Create arms the spawn law — same bit L writes");

    Rendering::apply3DMode(state, channel, Rendering::Mode3D::Selection);
    check(channel->activeTool == "Selection3D",
          "switching mode updates the registered tool");
    check(channel->active3DMode == "Select",
          "leaving Create takes the spawn law down");
    check(state.combineOperandA == nullptr && state.clayGrabbed == nullptr,
          "switching mode clears per-gesture pick operands");

    // ---- Creator Console tools as first movers -----------------------------
    Object author("creator-tools-author");
    Singularity::Core::syncRegisterCreatorTools(laws, author);
    check(laws.find("shape-generator-3d-law") != nullptr,
          "Shape Generator remains the Create first mover");
    check(laws.find("tool-select-3d-law") != nullptr,
          "Select is a first mover the console arms");
    check(laws.find("tool-morph-3d-law") != nullptr,
          "Morph is a first mover the console arms");
    check(std::string(Singularity::Core::creatorToolLawIdForMode("Select")) ==
              "tool-select-3d-law",
          "active3DMode Select maps to the Select first mover slug");
    check(std::string(Singularity::Core::creatorToolLawIdForMode("Create")) ==
              "shape-generator-3d-law",
          "active3DMode Create maps to the spawn first mover, not a twin");
    const size_t afterFirst = laws.getAll().size();
    Singularity::Core::syncRegisterCreatorTools(laws, author);
    check(laws.getAll().size() == afterFirst,
          "syncRegisterCreatorTools is idempotent");
    Law* selectLaw = laws.find("tool-select-3d-law");
    check(selectLaw && selectLaw->isFirstMover(),
          "Select is first-mover — engine truth, not a world save");
    check(selectLaw && selectLaw->isEnabled(),
          "a tool first mover boots up — set-down is a Person choice");

    std::printf(g_failures == 0 ? "creation_tools_test: ALL OK\n"
                                : "creation_tools_test: FAILURES\n");
    return g_failures > 0 ? 1 : 0;
}
