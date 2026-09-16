// Test: the Creator Console visibility fixture Law and its live native bridge.
//
// The Law Author must be able to make the Creator Console disappear/reappear by
// toggling one named first-mover Law, while F8/menu/dock gestures continue to
// work and write back into that same truth instead of creating a second latch.

#include "Person/Person.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/CreatorConsoleFixtureLaw.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <cstdio>
#include <string>
#include <utility>

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
    std::printf("Running Creator Console fixture law test...\n");

    Soul soul("Player");
    Body body("humanoid", "default");
    Person player(std::move(soul), std::move(body), "default");
    LawManager laws;

    bool consoleOpen = false;
    bool lastSyncedVisible = false;

    Law* fixture = Rendering::syncRegisterCreatorConsoleFixtureLaw(
        laws, player, consoleOpen);

    check(fixture != nullptr, "fixture law is registered");
    if (!fixture) return 1;

    check(fixture->getIdentifier() == Rendering::kCreatorConsoleFixtureLawId,
          "fixture has a stable law identifier");
    check(fixture->isFirstMover(),
          "fixture is a first mover whose truth stays in the engine");
    check(fixture->isAuthored(),
          "fixture is authored by the present Person");
    check(!fixture->isEnabled(),
          "fixture starts down when the native console starts closed");

    // Registration is first-wins. A per-frame sync must never replace the Law
    // and erase the Person's current visibility choice.
    Law* same = Rendering::syncRegisterCreatorConsoleFixtureLaw(laws, player, true);
    check(same == fixture, "sync registration preserves the existing fixture being");
    check(!fixture->isEnabled(),
          "sync registration does not reset the existing enabled state");

    // Path A: Law Author / authored governance -> human-facing native fixture.
    fixture->setEnabled(true);
    Rendering::reconcileCreatorConsoleFixtureVisibility(
        *fixture, consoleOpen, lastSyncedVisible);
    check(consoleOpen,
          "enabling the fixture law makes the native Creator Console visible");
    check(lastSyncedVisible,
          "bridge records the new shared visible state");

    fixture->setEnabled(false);
    Rendering::reconcileCreatorConsoleFixtureVisibility(
        *fixture, consoleOpen, lastSyncedVisible);
    check(!consoleOpen,
          "disabling the fixture law makes the native Creator Console disappear");

    // Path B: F8/menu/dock native gesture -> the same legible Law.
    consoleOpen = true;
    Rendering::reconcileCreatorConsoleFixtureVisibility(
        *fixture, consoleOpen, lastSyncedVisible);
    check(fixture->isEnabled(),
          "native open gesture writes back into fixture-law enabled");

    consoleOpen = false;
    Rendering::reconcileCreatorConsoleFixtureVisibility(
        *fixture, consoleOpen, lastSyncedVisible);
    check(!fixture->isEnabled(),
          "native close gesture writes back into fixture-law enabled");

    // First-mover truth is not serialized into a Zone/world payload, but the
    // live first mover survives LawManager's replace-all load path.
    const nlohmann::json saved = laws.toJson();
    bool serialized = false;
    for (const auto& lawJson : saved["laws"]) {
        if (lawJson.value("id", std::string()) ==
            Rendering::kCreatorConsoleFixtureLawId) {
            serialized = true;
        }
    }
    check(!serialized, "fixture law is omitted from world serialization");

    laws.loadFromJson(saved);
    check(laws.find(Rendering::kCreatorConsoleFixtureLawId) == fixture,
          "fixture law survives a LawManager load without being forged from the save");

    std::printf(g_failures == 0 ? "creator_console_fixture_law_test: ALL OK\n"
                                : "creator_console_fixture_law_test: FAILURES\n");
    return g_failures > 0 ? 1 : 0;
}
