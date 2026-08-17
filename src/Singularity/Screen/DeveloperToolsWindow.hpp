#pragma once

struct GLFWwindow;
namespace Core { class Engine; }

namespace Rendering {

// Developer-only panel driving the restored Tool::ShapeGenerator3D (the
// pre-law direct-spawn 3D create tool -- see AGENTS.md's 2026-08-13 restore
// notes and Tool.cpp). Exposes the CreationChannel fields the tool reads
// (shape kind, colour, placement mode) and fires the spawn on left-click
// while `*open` is true.
//
// Also owns the LAW path's activation input, which is deliberately NOT this
// panel's spawn button: pressing L (edge-triggered, independent of whatever
// ImGui window has focus) toggles the CreationChannel's active3DMode between
// "Create" and "", and publishes onMouseClicked on left-click while armed so
// a loaded "Tool: Shape Generator 3D" law (saves/tests/
// shape_generator_3d_law.json) can fire through Law::applyTo instead of the
// direct spawn below. Two different inputs, two different authorship
// stories: this window's button is the CreationChannel First Mover acting
// directly; the L-armed click is an ordinary Person-authored law firing off
// a Person-published event.
void renderDeveloperToolsWindow(bool* open, GLFWwindow* window, Core::Engine* engine);

} // namespace Rendering
