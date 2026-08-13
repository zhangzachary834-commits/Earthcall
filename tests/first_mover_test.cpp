// First Mover Register: who may write the substrate directly, and to which
// files. Implements the gates specified in FIRST_MOVER_AUTHORING.md 8a-8d.
//
// Written as refusals rather than permissions: the interesting behaviour of an
// authorization layer is everything it says no to.
#include "Identity/FirstMoverRegister.hpp"
#include "Identity/KeyPair.hpp"
#include "Singularity/Storage/SaveSystem.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>

using namespace Identity;

namespace {

std::filesystem::path scratchRoot() {
    auto dir = std::filesystem::temp_directory_path() / "earthcall_fm_saves";
    std::filesystem::create_directories(dir / "games");
    std::filesystem::create_directories(dir / "fixtures");
    return dir;
}

} // namespace

static void testModelWritesOnlyInsideItsScope() {
    FirstMoverRegister reg;
    const auto root = scratchRoot();
    reg.setSaveRoot(root);

    PrivateKey zach = PrivateKey::generate();
    PrivateKey model = PrivateKey::generate();

    assert(reg.recognize(zach, FirstMover::Kind::Person, model.id(),
                         FirstMover::Kind::Model, "claude-fable-5",
                         {"fixtures/**", "games/test_*.ecsave"}, 1000));

    // Inside the grant.
    assert(reg.mayWrite(model.id(), root / "fixtures" / "seed.ecsave"));
    assert(reg.mayWrite(model.id(), root / "fixtures" / "deep" / "nested.ecsave"));
    assert(reg.mayWrite(model.id(), root / "games" / "test_world.ecsave"));

    // Outside it. Recognition is not blanket permission -- this is the whole
    // point of scoping a model rather than merely trusting one.
    assert(!reg.mayWrite(model.id(), root / "games" / "real_world.ecsave"));
    assert(!reg.mayWrite(model.id(), root / "persons" / "zach.json"));

    // '*' must not span a separator, or "games/test_*" would reach anywhere
    // below games/.
    assert(!reg.mayWrite(model.id(), root / "games" / "sub" / "test_x.ecsave"));

    std::cout << "  model writes only inside its granted scopes OK\n";
}

static void testUnknownMoverRefused() {
    FirstMoverRegister reg;
    reg.setSaveRoot(scratchRoot());
    PrivateKey stranger = PrivateKey::generate();

    assert(!reg.mayWrite(stranger.id(), scratchRoot() / "games" / "anything.ecsave"));
    assert(reg.explain(stranger.id(), scratchRoot() / "games" / "a.ecsave")
               .find("not in the First Mover Register") != std::string::npos);

    std::cout << "  unregistered mover refused OK\n";
}

static void testSelfAttestationRefused() {
    // 8d: "No First Mover may attest itself. The chain terminates in a Person
    // or it does not terminate."
    FirstMoverRegister reg;
    reg.setSaveRoot(scratchRoot());
    PrivateKey model = PrivateKey::generate();

    assert(!reg.recognize(model, FirstMover::Kind::Person, model.id(),
                          FirstMover::Kind::Model, "self", {"**"}, 1000));
    assert(reg.movers().empty());

    std::cout << "  self-attestation refused OK\n";
}

static void testModelCannotRecognizeAnother() {
    // A model's recognition is delegated. If a model could widen the circle,
    // one compromised model would be enough to admit any number more.
    FirstMoverRegister reg;
    reg.setSaveRoot(scratchRoot());
    PrivateKey modelA = PrivateKey::generate();
    PrivateKey modelB = PrivateKey::generate();

    assert(!reg.recognize(modelA, FirstMover::Kind::Model, modelB.id(),
                          FirstMover::Kind::Model, "b", {"**"}, 1000));
    assert(reg.movers().empty());

    std::cout << "  model cannot recognise another mover OK\n";
}

static void testScopeWideningIsDetected() {
    // The attack this layer exists to stop: edit the save, give yourself "**".
    FirstMoverRegister reg;
    const auto root = scratchRoot();
    reg.setSaveRoot(root);

    PrivateKey zach = PrivateKey::generate();
    PrivateKey model = PrivateKey::generate();
    assert(reg.recognize(zach, FirstMover::Kind::Person, model.id(),
                         FirstMover::Kind::Model, "m", {"fixtures/**"}, 1000));

    nlohmann::json saved = reg.toJson();
    saved["movers"][0]["scopes"] = nlohmann::json::array({"**"});

    FirstMoverRegister reloaded;
    reloaded.setSaveRoot(root);
    reloaded.loadFromJson(saved);

    // Present and listed -- 8c forbids silently discarding it ...
    assert(reloaded.movers().size() == 1);
    assert(reloaded.isQuarantined(model.id()));
    // ... and inert.
    assert(!reloaded.mayWrite(model.id(), root / "games" / "real_world.ecsave"));
    assert(!reloaded.mayWrite(model.id(), root / "fixtures" / "seed.ecsave"));

    std::cout << "  scope widening detected, entry quarantined not dropped OK\n";
}

static void testForgedGrantRefused() {
    FirstMoverRegister reg;
    const auto root = scratchRoot();
    reg.setSaveRoot(root);

    PrivateKey zach = PrivateKey::generate();
    PrivateKey mallory = PrivateKey::generate();
    PrivateKey model = PrivateKey::generate();

    assert(reg.recognize(zach, FirstMover::Kind::Person, model.id(),
                         FirstMover::Kind::Model, "m", {"fixtures/**"}, 1000));

    // Claim the grant came from someone else while keeping Zach's signature.
    nlohmann::json saved = reg.toJson();
    saved["movers"][0]["grantedBy"] = mallory.id().toString();

    FirstMoverRegister reloaded;
    reloaded.setSaveRoot(root);
    reloaded.loadFromJson(saved);
    assert(!reloaded.mayWrite(model.id(), root / "fixtures" / "seed.ecsave"));

    // Re-point a valid grant at a different mover.
    PrivateKey other = PrivateKey::generate();
    nlohmann::json swapped = reg.toJson();
    swapped["movers"][0]["id"] = other.id().toString();

    FirstMoverRegister reloaded2;
    reloaded2.setSaveRoot(root);
    reloaded2.loadFromJson(swapped);
    assert(!reloaded2.mayWrite(other.id(), root / "fixtures" / "seed.ecsave"));

    std::cout << "  forged and re-pointed grants refused OK\n";
}

static void testCannotEscapeSaveRoot() {
    // Even a mover granted "**" cannot leave the save root. The pattern is a
    // filter within the floor, never a way through it.
    FirstMoverRegister reg;
    const auto root = scratchRoot();
    reg.setSaveRoot(root);

    PrivateKey zach = PrivateKey::generate();
    PrivateKey model = PrivateKey::generate();
    assert(reg.recognize(zach, FirstMover::Kind::Person, model.id(),
                         FirstMover::Kind::Model, "m", {"**"}, 1000));

    assert(reg.mayWrite(model.id(), root / "games" / "ok.ecsave"));

    assert(!reg.mayWrite(model.id(), root / ".." / "src" / "main.cpp"));
    assert(!reg.mayWrite(model.id(), root / "games" / ".." / ".." / "secret"));
    assert(!reg.mayWrite(model.id(), "/etc/passwd"));
    assert(!reg.mayWrite(model.id(), root)); // the root itself is not a file in it

    std::cout << "  save-root floor holds even with a '**' grant OK\n";
}

static void testEmptyScopeGrantsNothing() {
    // Recognised but scoped to nothing is a legitimate state, not an error:
    // it is what "we know who you are, you may not write yet" looks like.
    FirstMoverRegister reg;
    const auto root = scratchRoot();
    reg.setSaveRoot(root);

    PrivateKey zach = PrivateKey::generate();
    PrivateKey model = PrivateKey::generate();
    assert(reg.recognize(zach, FirstMover::Kind::Person, model.id(),
                         FirstMover::Kind::Model, "m", {}, 1000));

    assert(!reg.isQuarantined(model.id()));
    assert(!reg.mayWrite(model.id(), root / "games" / "x.ecsave"));

    std::cout << "  empty scope grants nothing OK\n";
}

static void testGlobSemantics() {
    assert(matchesGlob("games/*.ecsave", "games/a.ecsave"));
    assert(!matchesGlob("games/*.ecsave", "games/sub/a.ecsave"));
    assert(matchesGlob("games/**", "games/sub/a.ecsave"));
    assert(matchesGlob("**", "anything/at/all"));
    assert(!matchesGlob("games/*", "games"));
    assert(matchesGlob("test_*.ecsave", "test_world.ecsave"));
    assert(!matchesGlob("test_*.ecsave", "real_world.ecsave"));
    std::cout << "  glob semantics OK\n";
}

static void testRoundTripPreservesGrants() {
    FirstMoverRegister reg;
    const auto root = scratchRoot();
    reg.setSaveRoot(root);

    PrivateKey zach = PrivateKey::generate();
    PrivateKey model = PrivateKey::generate();
    assert(reg.recognize(zach, FirstMover::Kind::Person, model.id(),
                         FirstMover::Kind::Model, "claude", {"fixtures/**"}, 1000));

    FirstMoverRegister reloaded;
    reloaded.setSaveRoot(root);
    reloaded.loadFromJson(reg.toJson());

    assert(reloaded.movers().size() == 1);
    assert(reloaded.movers()[0].displayName == "claude");
    assert(!reloaded.isQuarantined(model.id()));
    assert(reloaded.mayWrite(model.id(), root / "fixtures" / "seed.ecsave"));
    assert(!reloaded.mayWrite(model.id(), root / "games" / "x.ecsave"));

    std::cout << "  register round-trips through a save OK\n";
}


static void testModelSignedGrantRefusedOnLoad() {
    // recognize() blocks a model from recognising anyone, but a register read
    // from a save was not necessarily built by recognize(). A model holds a
    // real key, so a grant it signs VERIFIES -- the signature is genuine and
    // only the authority behind it is not. The register must catch that on the
    // load path too, or one compromised model admits any number more.
    const auto root = scratchRoot();

    PrivateKey zach = PrivateKey::generate();
    PrivateKey modelA = PrivateKey::generate();
    PrivateKey modelB = PrivateKey::generate();

    // Zach legitimately recognises modelA.
    FirstMoverRegister built;
    built.setSaveRoot(root);
    assert(built.recognize(zach, FirstMover::Kind::Person, modelA.id(),
                           FirstMover::Kind::Model, "a", {"fixtures/**"}, 1000));

    // modelA forges a register entry recognising modelB, signing with its own
    // real key. The Claim itself is valid.
    FirstMover forged;
    forged.id = modelB.id();
    forged.kind = FirstMover::Kind::Model;
    forged.displayName = "b";
    forged.grantedBy = modelA.id();
    forged.scopes = {"fixtures/**"};
    forged.grant = Claim::issue(modelA, modelB.id(), forged.grantPredicate(),
                                modelA.id(), 2000);
    assert(forged.grant.verify()); // genuinely signed ...

    nlohmann::json saved = built.toJson();
    saved["movers"].push_back(forged.toJson());

    FirstMoverRegister reloaded;
    reloaded.setSaveRoot(root);
    reloaded.loadFromJson(saved);

    // ... and still refused, because modelA is a model.
    assert(!reloaded.mayWrite(modelB.id(), root / "fixtures" / "seed.ecsave"));
    assert(reloaded.explain(modelB.id(), root / "fixtures" / "seed.ecsave")
               .find("not a Person") != std::string::npos);

    // The legitimately recognised model is unaffected.
    assert(reloaded.mayWrite(modelA.id(), root / "fixtures" / "seed.ecsave"));

    std::cout << "  model-signed grant refused on load OK\n";
}


static void testSaveSystemEnforcesTheRegister() {
    // End to end: the register is only worth having if a write path consults
    // it. This drives the real SaveSystem, not a stand-in.
    auto& reg = Identity::FirstMoverRegister::instance();
    reg.clear();
    reg.clearActiveMover();

    // SaveSystem writes relative to the process cwd, so run inside a scratch
    // directory and point the register at the "saves" it will create there.
    auto sandbox = std::filesystem::temp_directory_path() / "earthcall_save_enforce";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox);
    auto previousCwd = std::filesystem::current_path();
    std::filesystem::current_path(sandbox);

    nlohmann::json payload = {{"hello", "world"}};

    // 1. No session active: the engine's own save must be untouched by this
    //    layer. Fail-open when unset is the whole reason it can ship safely.
    std::string engineWrote = SaveSystem::writeSaveData(payload, "engine_save",
                                                        SaveSystem::SaveType::GAME);
    assert(!engineWrote.empty());
    assert(std::filesystem::exists(engineWrote));

    reg.setSaveRoot(std::filesystem::current_path() / "saves");

    PrivateKey zach = PrivateKey::generate();
    PrivateKey model = PrivateKey::generate();
    assert(reg.recognize(zach, FirstMover::Kind::Person, model.id(),
                         FirstMover::Kind::Model, "claude-fable-5",
                         {"games/test_*.ecsave"}, 1000));

    {
        Identity::FirstMoverSession session(reg, model.id());

        // 2. Inside its scope: allowed.
        std::string ok = SaveSystem::writeSaveData(payload, "test_fixture",
                                                   SaveSystem::SaveType::GAME);
        assert(!ok.empty());
        assert(std::filesystem::exists(ok));

        // 3. Outside its scope: refused, and nothing written.
        std::string refused = SaveSystem::writeSaveData(payload, "production_world",
                                                        SaveSystem::SaveType::GAME);
        assert(refused.empty());
        assert(!std::filesystem::exists("saves/games/production_world.ecsave"));

        // 4. A different save TYPE is a different directory, so the scope
        //    does not reach it even with a matching stem.
        std::string wrongType = SaveSystem::writeSaveData(payload, "test_person",
                                                          SaveSystem::SaveType::PERSON);
        assert(wrongType.empty());
    }

    // 5. The session is scoped: once it ends the engine writes freely again.
    assert(!reg.hasActiveMover());
    std::string afterWrote = SaveSystem::writeSaveData(payload, "engine_again",
                                                       SaveSystem::SaveType::GAME);
    assert(!afterWrote.empty());

    std::filesystem::current_path(previousCwd);
    std::filesystem::remove_all(sandbox);
    reg.clear();
    reg.clearActiveMover();
    std::cout << "  SaveSystem enforces the register, engine saves unaffected OK\n";
}

static void testUnregisteredAgentCannotWriteAtAll() {
    auto& reg = Identity::FirstMoverRegister::instance();
    reg.clear();
    reg.clearActiveMover();

    auto sandbox = std::filesystem::temp_directory_path() / "earthcall_save_unreg";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox);
    auto previousCwd = std::filesystem::current_path();
    std::filesystem::current_path(sandbox);
    reg.setSaveRoot(std::filesystem::current_path() / "saves");

    PrivateKey rogue = PrivateKey::generate();
    {
        Identity::FirstMoverSession session(reg, rogue.id());
        // Never recognised by anyone. Every write refused, including one that
        // would match a scope had it held any.
        assert(SaveSystem::writeSaveData(nlohmann::json{{"a", 1}}, "test_x",
                                         SaveSystem::SaveType::GAME).empty());
    }

    std::filesystem::current_path(previousCwd);
    std::filesystem::remove_all(sandbox);
    reg.clear();
    reg.clearActiveMover();
    std::cout << "  unregistered agent refused every write OK\n";
}

int main() {
    std::cout << "first_mover_test:\n";
    testGlobSemantics();
    testModelWritesOnlyInsideItsScope();
    testUnknownMoverRefused();
    testSelfAttestationRefused();
    testModelCannotRecognizeAnother();
    testScopeWideningIsDetected();
    testForgedGrantRefused();
    testCannotEscapeSaveRoot();
    testEmptyScopeGrantsNothing();
    testRoundTripPreservesGrants();
    testModelSignedGrantRefusedOnLoad();
    testSaveSystemEnforcesTheRegister();
    testUnregisteredAgentCannotWriteAtAll();
    std::filesystem::remove_all(std::filesystem::temp_directory_path() / "earthcall_fm_saves");
    std::cout << "first_mover_test: ALL OK\n";
    return 0;
}
