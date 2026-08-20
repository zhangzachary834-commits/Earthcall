#pragma once

struct GLFWwindow;
namespace Core { class Engine; }

namespace Rendering {

// Developer test-observation window ("Developer: Test World Saves").
// Scans saves/tests/ and loads a dump into an isolated observation Zone
// via ZoneManager::loadTestObservation — never loadState, which would
// replace Home.
//
// Also owns the LAW path's activation input, which is deliberately NOT an
// ImGui button: pressing L (edge-triggered, independent of whatever
// ImGui window has focus) toggles the CreationChannel's active3DMode between
// "Create" and "", and publishes onMouseClicked on left-click while armed so
// a loaded "Tool: Shape Generator 3D" law (saves/tests/
// shape_generator_3d_law.json) can fire through Law::applyTo.
void renderDeveloperToolsWindow(bool* open, GLFWwindow* window, Core::Engine* engine);

} // namespace Rendering
