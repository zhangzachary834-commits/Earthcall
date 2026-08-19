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

// Write the console's 3D mode onto the chrome state and, when a channel is
// present, onto @creation-channel.activeTool. Shared by the mode buttons
// and by the first-mover step.
void apply3DMode(CreatorConsoleState& state,
                 Singularity::Core::CreationChannel* channel,
                 Mode3D mode);

// Slug a law can read on @creation-channel.activeTool for this mode.
// "Create" is NOT this string — that bit still belongs to active3DMode
// and is armed by L. Unifying the two is the remaining one-Create-bit
// work (audit §2.6 / near-term 7(c)).
const char* toolNameForMode(Mode3D mode);

} // namespace Rendering
