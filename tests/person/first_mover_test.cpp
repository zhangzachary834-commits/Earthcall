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
    std::filesystem::create_directories(dir / "worlds");
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

    assert(reg.trustAuthenticatedPerson(zach));
    assert(reg.recognize(zach, FirstMover::Kind::Person, model.id(),
                         FirstMover::Kind::Model, "claude-fable-5",
                         {"fixtures/**", "worlds/test_*.ecsave"}, 1000));

    // Inside the grant.
    assert(reg.mayWrite(model.id(), root / "fixtures" / "seed.ecsave"));
    assert(reg.mayWrite(model.id(), root / "fixtures" / "deep" / "nested.ecsave"));
    assert(reg.mayWrite(model.id(), root / "worlds" / "test_world.ecsave"));

    // Outside it. Recognition is not blanket permission -- this is the whole
    // point of scoping a model rather than merely trusting one.
    assert(!reg.mayWrite(model.id(), root / "worlds" / "real_world.ecsave"));
    assert(!reg.mayWrite(model.id(), root / "persons" / "zach.json"));

    // '*' must not span a separator, or "worlds/test_*" would reach anywhere
    // below worlds/.
    assert(!reg.mayWrite(model.id(), root / "worlds" / "sub" / "test_x.ecsave"));

    std::cout << "  model writes only inside its granted scopes OK\n";
}

static void testUnknownMoverRefused() {
    FirstMoverRegister reg;
    reg.setSaveRoot(scratchRoot());
    PrivateKey stranger = PrivateKey::generate();

    assert(!reg.mayWrite(stranger.id(), scratchRoot() / "worlds" / "anything.ecsave"));
    assert(reg.explain(stranger.id(), scratchRoot() / "worlds" / "a.ecsave")
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
    assert(reg.trustAuthenticatedPerson(zach));
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
    assert(!reloaded.mayWrite(model.id(), root / "worlds" / "real_world.ecsave"));
    assert(!reloaded.mayWrite(model.id(), root / "fixtures" / "seed.ecsave"));

    std::cout << "  scope widening detected, entry quarantined not dropped OK\n";
}

static void testInvalidGrantGracefulRejection() {
    // Tests that a grant with a syntactically invalid or cryptographically
    // invalid signature is handled gracefully, quarantined, and refused,
    // without throwing exceptions.
    FirstMoverRegister reg;
    const auto root = scratchRoot();
    reg.setSaveRoot(root);

    PrivateKey zach = PrivateKey::generate();
    PrivateKey model = PrivateKey::generate();

    assert(reg.trustAuthenticatedPerson(zach));
    assert(reg.recognize(zach, FirstMover::Kind::Person, model.id(),
                         FirstMover::Kind::Model, "m", {"fixtures/**"}, 1000));

    // Create scenarios with invalid signatures
    nlohmann::json saved = reg.toJson();

    auto checkRejection = [&](const std::string& badSignature) {
        nlohmann::json badJson = saved;
        badJson["movers"][0]["grant"]["signature"] = badSignature;

        FirstMoverRegister reloaded;
        reloaded.setSaveRoot(root);
        reloaded.loadFromJson(badJson);

        assert(reloaded.movers().size() == 1);
        assert(reloaded.isQuarantined(model.id()));
        assert(!reloaded.mayWrite(model.id(), root / "fixtures" / "seed.ecsave"));

        std::string explanation = reloaded.explain(model.id(), root / "fixtures" / "seed.ecsave");
        assert(explanation.find("grant signature does not verify") != std::string::npos);
    };

    checkRejection(""); // Empty signature
    checkRejection("not_base_64!"); // Malformed base64
    checkRejection("aGVsbG8="); // Wrong length (too short)
    checkRejection("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"); // Valid base64 but wrong decoded length (63 bytes)

    std::cout << "  invalid grants gracefully rejected OK\n";
}

static void testForgedGrantRefused() {
    FirstMoverRegister reg;
    const auto root = scratchRoot();
    reg.setSaveRoot(root);

    PrivateKey zach = PrivateKey::generate();
    PrivateKey mallory = PrivateKey::generate();
    PrivateKey model = PrivateKey::generate();

    assert(reg.trustAuthenticatedPerson(zach));
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
    assert(reg.trustAuthenticatedPerson(zach));
    assert(reg.recognize(zach, FirstMover::Kind::Person, model.id(),
                         FirstMover::Kind::Model, "m", {"**"}, 1000));

    assert(reg.mayWrite(model.id(), root / "worlds" / "ok.ecsave"));

    assert(!reg.mayWrite(model.id(), root / ".." / "src" / "main.cpp"));
    assert(!reg.mayWrite(model.id(), root / "worlds" / ".." / ".." / "secret"));
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
    assert(reg.trustAuthenticatedPerson(zach));
    assert(reg.recognize(zach, FirstMover::Kind::Person, model.id(),
                         FirstMover::Kind::Model, "m", {}, 1000));

    assert(!reg.isQuarantined(model.id()));
    assert(!reg.mayWrite(model.id(), root / "worlds" / "x.ecsave"));

    std::cout << "  empty scope grants nothing OK\n";
}

static void testGlobSemantics() {
    assert(matchesGlob("worlds/*.ecsave", "worlds/a.ecsave"));
    assert(!matchesGlob("worlds/*.ecsave", "worlds/sub/a.ecsave"));
    assert(matchesGlob("worlds/**", "worlds/sub/a.ecsave"));
    assert(matchesGlob("**", "anything/at/all"));
    assert(!matchesGlob("worlds/*", "worlds"));
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
    assert(reg.trustAuthenticatedPerson(zach));
    assert(reg.recognize(zach, FirstMover::Kind::Person, model.id(),
                         FirstMover::Kind::Model, "claude", {"fixtures/**"}, 1000));

    FirstMoverRegister reloaded;
    reloaded.setSaveRoot(root);
    reloaded.loadFromJson(reg.toJson());
    // Trusted Person roots are runtime-only; the Person re-authenticates.
    assert(reloaded.standing(model.id()) == Standing::GrantorNotAuthenticated);
    assert(reloaded.trustAuthenticatedPerson(zach));

    assert(reloaded.movers().size() == 1);
    assert(reloaded.movers()[0]->displayName == "claude");
    assert(!reloaded.isQuarantined(model.id()));
    assert(reloaded.mayWrite(model.id(), root / "fixtures" / "seed.ecsave"));
    assert(!reloaded.mayWrite(model.id(), root / "worlds" / "x.ecsave"));

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
    assert(built.trustAuthenticatedPerson(zach));
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
    assert(reloaded.trustAuthenticatedPerson(zach));
    // Even if modelA's key were somehow trusted as a root, its model entry in
    // the register keeps it from granting.
    assert(reloaded.trustAuthenticatedPerson(modelA));

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
                                                        SaveSystem::SaveType::WORLD);
    assert(!engineWrote.empty());
    assert(std::filesystem::exists(engineWrote));

    reg.setSaveRoot(std::filesystem::current_path() / "saves");

    PrivateKey zach = PrivateKey::generate();
    PrivateKey model = PrivateKey::generate();
    assert(reg.trustAuthenticatedPerson(zach));
    assert(reg.recognize(zach, FirstMover::Kind::Person, model.id(),
                         FirstMover::Kind::Model, "claude-fable-5",
                         {"worlds/test_*.ecform"}, 1000));

    {
        Identity::FirstMoverSession session(reg, model.id());

        // 2. Inside its scope: allowed.
        std::string ok = SaveSystem::writeSaveData(payload, "test_fixture",
                                                   SaveSystem::SaveType::WORLD);
        assert(!ok.empty());
        assert(std::filesystem::exists(ok));

        // 3. Outside its scope: refused, and nothing written.
        std::string refused = SaveSystem::writeSaveData(payload, "production_world",
                                                        SaveSystem::SaveType::WORLD);
        assert(refused.empty());
        assert(!std::filesystem::exists("saves/worlds/production_world.ecform"));

        // 4. A different save TYPE is a different directory, so the scope
        //    does not reach it even with a matching stem.
        std::string wrongType = SaveSystem::writeSaveData(payload, "test_person",
                                                          SaveSystem::SaveType::PERSON);
        assert(wrongType.empty());
    }

    // 5. The session is scoped: once it ends the engine writes freely again.
    assert(!reg.hasActiveMover());
    std::string afterWrote = SaveSystem::writeSaveData(payload, "engine_again",
                                                       SaveSystem::SaveType::WORLD);
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
                                         SaveSystem::SaveType::WORLD).empty());
    }

    std::filesystem::current_path(previousCwd);
    std::filesystem::remove_all(sandbox);
    reg.clear();
    reg.clearActiveMover();
    std::cout << "  unregistered agent refused every write OK\n";
}


// --- 2026-09-24: trust root, stable movers, covenant floor -----------------
// Phase 0/1/2 of docs/plans/MCP_FIRST_MOVER_GOVERNANCE_IMPLEMENTATION_PLAN_2026-09-18.md.

static void testAbsentGrantorIsNotATrustRoot() {
    // The 2026-08-20 finding: a valid signature from a grantor ABSENT from the
    // register was never proven to terminate in a Person. It verified, so it
    // stood. It must not.
    const auto root = scratchRoot();
    PrivateKey zach = PrivateKey::generate();
    PrivateKey model = PrivateKey::generate();

    FirstMoverRegister built;
    built.setSaveRoot(root);
    assert(built.trustAuthenticatedPerson(zach));
    assert(built.recognize(zach, FirstMover::Kind::Person, model.id(),
                           FirstMover::Kind::Model, "m", {"fixtures/**"}, 1000));

    FirstMoverRegister loaded;   // a fresh process: nobody has authenticated
    loaded.setSaveRoot(root);
    loaded.loadFromJson(built.toJson());
    assert(loaded.standing(model.id()) == Standing::GrantorNotAuthenticated);
    assert(!loaded.mayWrite(model.id(), root / "fixtures" / "seed.ecsave"));
    // Inert, but not quarantined: nothing about the data is wrong.
    assert(!loaded.isQuarantined(model.id()));
    assert(loaded.explain(model.id(), root / "fixtures" / "seed.ecsave")
               .find("has not authenticated") != std::string::npos);

    // The hostile save: mallory mints her own key, writes herself in as
    // `kind: person`, and signs a grant. Every signature verifies.
    PrivateKey mallory = PrivateKey::generate();
    PrivateKey puppet = PrivateKey::generate();
    FirstMover fakePerson;
    fakePerson.id = mallory.id();
    fakePerson.kind = FirstMover::Kind::Person;
    fakePerson.displayName = "Zach";          // a label, not a root
    fakePerson.grantedBy = zach.id();
    FirstMover forged;
    forged.id = puppet.id();
    forged.kind = FirstMover::Kind::Model;
    forged.grantedBy = mallory.id();
    forged.scopes = {"**"};
    forged.grant = Claim::issue(mallory, puppet.id(), forged.grantPredicate(), mallory.id(), 3000);
    assert(forged.grant.verify());

    nlohmann::json hostile = built.toJson();
    hostile["movers"].push_back(fakePerson.toJson());
    hostile["movers"].push_back(forged.toJson());
    hostile["authenticatedPersons"] = nlohmann::json::array({mallory.id().toString()});

    FirstMoverRegister victim;
    victim.setSaveRoot(root);
    assert(victim.trustAuthenticatedPerson(zach));   // the real Person is present
    victim.loadFromJson(hostile);
    assert(!victim.isAuthenticatedPerson(mallory.id()));   // the file cannot seed a root
    assert(victim.standing(puppet.id()) == Standing::GrantorNotAuthenticated);
    assert(!victim.mayWrite(puppet.id(), root / "worlds" / "real.ecsave"));
    // Zach's genuine grant still stands.
    assert(victim.mayWrite(model.id(), root / "fixtures" / "seed.ecsave"));

    std::cout << "  absent / self-minted grantor is not a trust root OK\n";
}

static void testRecognizeRequiresAuthenticatedGrantor() {
    FirstMoverRegister reg;
    reg.setSaveRoot(scratchRoot());
    PrivateKey someone = PrivateKey::generate();
    PrivateKey model = PrivateKey::generate();
    // Holding a key and saying "Person" is not enough.
    assert(!reg.recognize(someone, FirstMover::Kind::Person, model.id(),
                          FirstMover::Kind::Model, "m", {"**"}, 1000));
    assert(reg.movers().empty());
    // An invalid key can never be a root.
    assert(!reg.trustAuthenticatedPerson(PrivateKey{}));
    assert(reg.authenticatedPersons().empty());

    // Roots are never in the serialized register.
    assert(reg.trustAuthenticatedPerson(someone));
    assert(reg.toJson().dump().find(someone.id().toString()) == std::string::npos);
    std::cout << "  recognize requires an authenticated grantor; roots never serialize OK\n";
}

static void testMoverPointersAreStable() {
    // A Law's author Formation holds a Singular*. If a mover moved when the
    // register grew, re-granted, reloaded, or revoked, that author would dangle.
    FirstMoverRegister reg;
    const auto root = scratchRoot();
    reg.setSaveRoot(root);
    PrivateKey zach = PrivateKey::generate();
    PrivateKey model = PrivateKey::generate();
    assert(reg.trustAuthenticatedPerson(zach));
    assert(reg.recognize(zach, FirstMover::Kind::Person, model.id(),
                         FirstMover::Kind::Model, "sonnet", {"laws/**"}, 1000));
    const FirstMover* author = reg.find(model.id());
    assert(author);

    for (int i = 0; i < 64; ++i) {
        PrivateKey other = PrivateKey::generate();
        assert(reg.recognize(zach, FirstMover::Kind::Person, other.id(),
                             FirstMover::Kind::Model, "m" + std::to_string(i), {}, 1000 + i));
    }
    assert(reg.find(model.id()) == author);

    // Re-grant with wider scope: same object, new scope.
    assert(reg.recognize(zach, FirstMover::Kind::Person, model.id(),
                         FirstMover::Kind::Model, "sonnet", {"laws/**", "zones/**"}, 2000));
    assert(reg.find(model.id()) == author);
    assert(author->scopes.size() == 2);

    // Reload: same object.
    reg.loadFromJson(reg.toJson());
    assert(reg.find(model.id()) == author);
    assert(reg.standing(model.id()) == Standing::Recognized);

    // Revoke: gone from the register, but the object survives (retired).
    PrivateKey stranger = PrivateKey::generate();
    assert(!reg.revoke(stranger, model.id()));     // not an authenticated grantor
    assert(reg.trustAuthenticatedPerson(stranger));
    assert(!reg.revoke(stranger, model.id()));     // authenticated, but not THE grantor
    assert(reg.revoke(zach, model.id()));
    assert(reg.find(model.id()) == nullptr);
    assert(author->id == model.id());              // still a valid Singular
    assert(reg.standing(model.id()) == Standing::NotRegistered);
    std::cout << "  mover pointers survive growth, re-grant, reload and revoke OK\n";
}

static void testRegisterIsNotWritableByInjection() {
    // 8d: "The register is not writable by injection." Even a '**' mover.
    FirstMoverRegister reg;
    const auto root = scratchRoot();
    reg.setSaveRoot(root);
    PrivateKey zach = PrivateKey::generate();
    PrivateKey model = PrivateKey::generate();
    assert(reg.trustAuthenticatedPerson(zach));
    assert(reg.recognize(zach, FirstMover::Kind::Person, model.id(),
                         FirstMover::Kind::Model, "m", {"**"}, 1000));
    assert(reg.mayWrite(model.id(), root / "laws" / "x" / "law.json"));
    assert(!reg.mayWrite(model.id(), root / FirstMoverRegister::kRegisterFile));
    assert(!reg.mayWrite(model.id(), root / "identity" / "anything.json"));
    assert(reg.explain(model.id(), root / FirstMoverRegister::kRegisterFile)
               .find("covenant") != std::string::npos);
    std::cout << "  register directory refuses even a '**' mover OK\n";
}

static void testNestedSessionsRestore() {
    FirstMoverRegister reg;
    PrivateKey a = PrivateKey::generate();
    PrivateKey b = PrivateKey::generate();
    assert(!reg.hasActiveMover());
    {
        FirstMoverSession sa(reg, a.id());
        assert(reg.activeMover() == a.id());
        {
            FirstMoverSession sb(reg, b.id());
            assert(reg.activeMover() == b.id());
        }
        assert(reg.activeMover() == a.id());
    }
    assert(!reg.hasActiveMover());
    std::cout << "  nested sessions restore the previous mover OK\n";
}

static void testSessionTranscriptIsUnambiguous() {
    PrivateKey m = PrivateKey::generate();
    const auto t1 = foreignSessionTranscript("ab", "c", "conn", m.id());
    const auto t2 = foreignSessionTranscript("a", "bc", "conn", m.id());
    assert(t1 != t2);   // length-prefixing: no field can bleed into the next
    assert(foreignSessionTranscript("ab", "c", "conn", m.id()) == t1);
    const std::string head(t1.begin(), t1.begin() + 35);
    assert(head.find("earthcall-first-mover-session-v1") != std::string::npos);
    std::cout << "  session transcript is length-prefixed and domain-separated OK\n";
}

int main() {
    std::cout << "first_mover_test:\n";
    testGlobSemantics();
    testModelWritesOnlyInsideItsScope();
    testUnknownMoverRefused();
    testSelfAttestationRefused();
    testModelCannotRecognizeAnother();
    testScopeWideningIsDetected();
    testInvalidGrantGracefulRejection();
    testForgedGrantRefused();
    testCannotEscapeSaveRoot();
    testEmptyScopeGrantsNothing();
    testRoundTripPreservesGrants();
    testModelSignedGrantRefusedOnLoad();
    testSaveSystemEnforcesTheRegister();
    testUnregisteredAgentCannotWriteAtAll();
    testAbsentGrantorIsNotATrustRoot();
    testRecognizeRequiresAuthenticatedGrantor();
    testMoverPointersAreStable();
    testRegisterIsNotWritableByInjection();
    testNestedSessionsRestore();
    testSessionTranscriptIsUnambiguous();
    std::filesystem::remove_all(std::filesystem::temp_directory_path() / "earthcall_fm_saves");
    std::cout << "first_mover_test: ALL OK\n";
    return 0;
}
