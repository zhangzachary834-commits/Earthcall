#pragma once

#include <memory>

class Law;
class LawManager;
class Singular;

namespace Rendering {

// Stable first-mover identity for the Creator Console's on-screen fixture.
// The law's ordinary `enabled` property is the authored/legible visibility
// latch: enabled = the fixture is present, disabled = the fixture is set down.
inline constexpr const char* kCreatorConsoleFixtureLawId =
    "fixture-creator-console-visibility-law";

// Factory kept outside Engine::update so tests instantiate exactly the being
// the live engine uses. The Person is still the author: first-mover means the
// truth is engine-backed and not serialized, never "authorless".
std::shared_ptr<Law> createCreatorConsoleFixtureLaw(Singular& author,
                                                    bool initiallyVisible);

// Idempotently place the fixture law into the live LawManager. Returns the
// existing law first-wins so a Person's current enabled/disabled choice is not
// reset every frame.
Law* syncRegisterCreatorConsoleFixtureLaw(LawManager& laws,
                                          Singular& author,
                                          bool initiallyVisible);

// Reconcile the native ImGui/Dock bool with the fixture Law's enabled bit.
// `lastSyncedVisible` lets the bridge distinguish which side changed:
//   - Law Author / another law changed `enabled` -> drive the native window.
//   - F8 / menu / dock chrome changed the bool -> drive the fixture law.
// No side silently wins merely because it happened to be sampled second.
void reconcileCreatorConsoleFixtureVisibility(Law& fixtureLaw,
                                              bool& consoleOpen,
                                              bool& lastSyncedVisible);

} // namespace Rendering
