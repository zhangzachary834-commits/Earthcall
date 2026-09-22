#pragma once

struct GLFWwindow;
class ZoneManager;

namespace Core { class Engine; }
namespace Singularity { namespace Core { class CreationChannel; } }

namespace Rendering {

struct CreatorConsoleState;
enum class Mode3D;

// The first-mover step for 3D tools.
//
// Sense (placement, L) and Act (the developer bypass tools) used to run
// inside render functions: UpdateShapeGeneratorPlacement and the L-key
// lived in DeveloperToolsWindow, and the tool switch lived in
// render3DConsole. Collapsing the Creator Console, or leaving the 3D tab,
// froze every tool while the chrome still showed the mode as armed.
//
// Engine::update calls this every frame. Rendering does not.
void stepCreationTools(GLFWwindow* window, Core::Engine* engine,
                       ZoneManager& zoneMgr, float dt,
                       bool creatorConsoleOpen);

// Write the console's 3D mode onto the chrome state and the channel.
// BrushCreate writes active3DMode == "Create" (console bypass only).
// Does not touch spawnLawArmed. Callers: mode buttons, F4/F5.
void apply3DMode(CreatorConsoleState& state,
                 Singularity::Core::CreationChannel* channel,
                 Mode3D mode);

// Slug a law can read on @creation-channel.activeTool for this mode.
const char* toolNameForMode(Mode3D mode);

// Console mode string. BrushCreate is "Create" — the bypass, not the
// spawn law. The spawn law reads spawnLawArmed.
const char* activeModeFor(Mode3D mode);

} // namespace Rendering
