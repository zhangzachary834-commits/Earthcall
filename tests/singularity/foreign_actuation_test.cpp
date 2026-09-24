// Foreign actuation: a process outside Earthcall (the MCP bridge a language
// model drives) proves it holds a Person-granted First Mover's key, and every
// mutation it asks for is decided by the register + TransferPolicy -- nothing
// else. Written as refusals first: the interesting behaviour of a gate is
// everything it says no to.
//
// Plan: docs/plans/MCP_FIRST_MOVER_GOVERNANCE_IMPLEMENTATION_PLAN_2026-09-18.md
// sections 4.4, 8, 10, 15.2-15.3. Implemented 2026-09-24 (Claude Opus 5.5).
#include "Singularity/Foreign/ForeignActuationGuard.hpp"
#include "Identity/FirstMoverRegister.hpp"
#include "Identity/KeyPair.hpp"
#include "Singularity/TransferPolicy.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>

using namespace Identity;
using namespace Singularity::Foreign;

namespace {

std::filesystem::path root() {
    auto dir = std::filesystem::temp_directory_path() / "earthcall_foreign_saves";
    std::filesystem::create_directories(dir / "laws");
    std::filesystem::create_directories(dir / "zones");
    return dir;
}

struct World {
    FirstMoverRegister reg;
    PrivateKey zach = PrivateKey::generate();
    PrivateKey sonnet = PrivateKey::generate();
    World() {
        reg.setSaveRoot(root());
        assert(reg.trustAuthenticatedPerson(zach));
        assert(reg.recognize(zach, FirstMover::Kind::Person, sonnet.id(),
                             FirstMover::Kind::Model, "Claude Sonnet 4.5",
                             {"laws/sonnet-*/**", "zones/Sonnet*/**"}, 1000));
    }
};

std::string sign(const PrivateKey& key, const ForeignChallenge& c) {
    return hexEncode(key.sign(foreignSessionTranscript(c.challengeId, c.nonce, c.connection, key.id())));
}

} // namespace

static void testHandshakeBindsTheMover() {
    World w;
    ForeignSessionAuthenticator auth;
    const auto c = auth.issueChallenge("conn-1", 0);
    assert(!c.challengeId.empty() && !c.nonce.empty() && c.challengeId != c.nonce);
    assert(auth.moverFor("conn-1") == nullptr);

    const auto r = auth.authenticate("conn-1", c.challengeId, w.sonnet.id().toString(),
                                     sign(w.sonnet, c), w.reg, 10);
    assert(r.ok);
    assert(auth.moverFor("conn-1") && *auth.moverFor("conn-1") == w.sonnet.id());
    assert(auth.moverFor("conn-2") == nullptr);   // bound to ONE connection

    auth.drop("conn-1");
    assert(auth.moverFor("conn-1") == nullptr);   // disconnect ends it
    std::cout << "  challenge -> signature binds the mover to its connection OK\n";
}

static void testBareMoverIdIsNotAuthentication() {
    // 4.4: naming a real registered mover without proving its key is a
    // username with no password.
    World w;
    ForeignSessionAuthenticator auth;
    PrivateKey impostor = PrivateKey::generate();

    auto c = auth.issueChallenge("conn", 0);
    auto r = auth.authenticate("conn", c.challengeId, w.sonnet.id().toString(), "", w.reg, 1);
    assert(!r.ok && r.reasonCode == "signature-invalid");

    c = auth.issueChallenge("conn", 0);
    r = auth.authenticate("conn", c.challengeId, w.sonnet.id().toString(), sign(impostor, c), w.reg, 1);
    assert(!r.ok && r.reasonCode == "signature-invalid");
    assert(auth.moverFor("conn") == nullptr);
    std::cout << "  bare / wrongly signed mover id refused OK\n";
}

static void testChallengeIsSingleUseBoundAndShortLived() {
    World w;
    ForeignSessionAuthenticator auth;

    // Replay: the same good answer twice.
    auto c = auth.issueChallenge("conn", 0);
    const std::string sig = sign(w.sonnet, c);
    assert(auth.authenticate("conn", c.challengeId, w.sonnet.id().toString(), sig, w.reg, 1).ok);
    auto r = auth.authenticate("conn", c.challengeId, w.sonnet.id().toString(), sig, w.reg, 2);
    assert(!r.ok && r.reasonCode == "unknown-challenge");

    // Consumed even by a failed attempt.
    c = auth.issueChallenge("other", 0);
    assert(!auth.authenticate("other", c.challengeId, w.sonnet.id().toString(), "00", w.reg, 1).ok);
    r = auth.authenticate("other", c.challengeId, w.sonnet.id().toString(), sign(w.sonnet, c), w.reg, 2);
    assert(!r.ok && r.reasonCode == "unknown-challenge");

    // Answered from a different connection.
    c = auth.issueChallenge("A", 0);
    r = auth.authenticate("B", c.challengeId, w.sonnet.id().toString(), sign(w.sonnet, c), w.reg, 1);
    assert(!r.ok && r.reasonCode == "challenge-connection-mismatch");

    // Too late.
    c = auth.issueChallenge("late", 0);
    r = auth.authenticate("late", c.challengeId, w.sonnet.id().toString(), sign(w.sonnet, c), w.reg,
                          ForeignSessionAuthenticator::kChallengeLifetimeMs + 1);
    assert(!r.ok && r.reasonCode == "challenge-expired");

    // A new challenge withdraws the connection's previous one.
    auto first = auth.issueChallenge("x", 0);
    auth.issueChallenge("x", 0);
    r = auth.authenticate("x", first.challengeId, w.sonnet.id().toString(), sign(w.sonnet, first), w.reg, 1);
    assert(!r.ok && r.reasonCode == "unknown-challenge");
    std::cout << "  challenges are single-use, connection-bound, and expire OK\n";
}

static void testKeyHolderWithoutStandingIsRefused() {
    // Holding the key is necessary, not sufficient: the mover must stand.
    World w;
    ForeignSessionAuthenticator auth;

    PrivateKey stranger = PrivateKey::generate();
    auto c = auth.issueChallenge("s", 0);
    auto r = auth.authenticate("s", c.challengeId, stranger.id().toString(), sign(stranger, c), w.reg, 1);
    assert(!r.ok && r.reasonCode == "not-registered");

    // The granting Person leaves (their key no longer unlocked).
    w.reg.revokeAuthenticatedPerson(w.zach.id());
    c = auth.issueChallenge("s", 0);
    r = auth.authenticate("s", c.challengeId, w.sonnet.id().toString(), sign(w.sonnet, c), w.reg, 1);
    assert(!r.ok && r.reasonCode == "grantor-not-authenticated");
    std::cout << "  key holder without standing refused OK\n";
}

static void testActuationDecisions() {
    World w;
    const auto R = root();
    const SingularId mover = w.sonnet.id();
    const std::string ownLaw = (R / "laws" / "sonnet-garden" / "law.json").string();
    const std::string zachsLaw = (R / "laws" / "law-art-stroke-draw" / "law.json").string();

    // No session.
    auto d = authorizeForeignActuation(w.reg, nullptr, ownLaw, "", "mcp");
    assert(!d.allowed && d.reasonCode == "no-first-mover-session");

    // Inside its grant.
    d = authorizeForeignActuation(w.reg, &mover, ownLaw, "", "mcp");
    assert(d.allowed && d.moverId == mover.toString());
    d = authorizeForeignActuation(w.reg, &mover, (R / "zones" / "SonnetGarden" / "zone.json").string(), "", "mcp");
    assert(d.allowed);

    // A Law Zach wrote is outside it.
    d = authorizeForeignActuation(w.reg, &mover, zachsLaw, "", "mcp");
    assert(!d.allowed && d.reasonCode == "outside-scope");

    // No honest coordinate: refused, never widened.
    d = authorizeForeignActuation(w.reg, &mover, "", "", "mcp", "Person body");
    assert(!d.allowed && d.reasonCode == "unmapped-resource" && d.reason == "Person body");

    // The register itself, even named directly.
    d = authorizeForeignActuation(w.reg, &mover, (R / FirstMoverRegister::kRegisterFile).string(), "", "mcp");
    assert(!d.allowed && d.reasonCode == "outside-scope");

    // TransferPolicy is consulted for property acts: close a Governable gate.
    auto& tp = TransferPolicy::instance();
    std::string governable;
    for (const auto& [gate, tier] : tp.gates()) {
        if (tier == TransferPolicy::Tier::Governable) { governable = gate; break; }
    }
    if (!governable.empty()) {
        const bool wasOpen = tp.isOpen(governable);
        assert(tp.setOpen(governable, false));
        d = authorizeForeignActuation(w.reg, &mover, ownLaw, governable, "mcp");
        assert(!d.allowed && d.reasonCode == "transfer-policy-closed");
        tp.setOpen(governable, wasOpen);
        d = authorizeForeignActuation(w.reg, &mover, ownLaw, governable, "mcp");
        assert(d.allowed == wasOpen || !wasOpen);
    }

    // Revocation takes effect on the very next act.
    assert(w.reg.revoke(w.zach, mover));
    d = authorizeForeignActuation(w.reg, &mover, ownLaw, "", "mcp");
    assert(!d.allowed && d.reasonCode == "not-registered");

    // The refusal JSON carries no key material.
    const std::string wire = d.toJson().dump();
    assert(wire.find("signature") == std::string::npos);
    assert(wire.find("\"status\":\"refused\"") != std::string::npos);
    std::cout << "  actuation decisions compose register + TransferPolicy only OK\n";
}

static void testForeignLawAuthorIsTheMover() {
    // 4.3: a Law a model wrote is authored by the model's mover -- never by
    // the Person at the screen, never by whoever the payload names.
    World w;
    auto authors = foreignLawAuthors(w.reg, w.sonnet.id());
    assert(authors.size() == 1);
    assert(authors.front() == w.reg.find(w.sonnet.id()));
    assert(authors.front()->getIdentifier() == w.sonnet.id().toString());

    // No standing, no author -- so no authored Law.
    PrivateKey stranger = PrivateKey::generate();
    assert(foreignLawAuthors(w.reg, stranger.id()).empty());
    w.reg.revokeAuthenticatedPerson(w.zach.id());
    assert(foreignLawAuthors(w.reg, w.sonnet.id()).empty());

    // Reload by identifier resolves back to the same live mover (LawManager
    // and ZoneManager both use authorFor), and only while it stands.
    assert(w.reg.authorFor(w.sonnet.id().toString()) == nullptr);
    assert(w.reg.trustAuthenticatedPerson(w.zach));
    assert(w.reg.authorFor(w.sonnet.id().toString()) == w.reg.find(w.sonnet.id()));
    assert(w.reg.authorFor("gemini-spark") == nullptr);   // slugs fall through
    std::cout << "  foreign Law author is the mover, and reloads only while it stands OK\n";
}

static void testMoverAuthoredLawSurvivesReload() {
    // 19: "FirstMover authors survive save/reload safely." The real
    // LawManager round trip: authored while the mover stands, reattached by
    // cryptographic id on load, and left Unauthored (inert, visible -- 8c)
    // when its Person is not present.
    auto& reg = FirstMoverRegister::instance();
    reg.clear();
    reg.clearAuthenticatedPersons();
    reg.setSaveRoot(root());
    PrivateKey zach = PrivateKey::generate();
    PrivateKey sonnet = PrivateKey::generate();
    assert(reg.trustAuthenticatedPerson(zach));
    assert(reg.recognize(zach, FirstMover::Kind::Person, sonnet.id(), FirstMover::Kind::Model,
                         "Claude Sonnet 4.5", {"laws/sonnet-*/**"}, 1000));

    nlohmann::json saved;
    {
        LawManager lm;
        auto law = lm.createLaw("Sonnet's Garden", foreignLawAuthors(reg, sonnet.id()));
        law->setLawIdentifier("sonnet-garden");
        assert(law->isAuthored());
        saved = lm.toJson();
    }
    assert(saved.dump().find(sonnet.id().toString()) != std::string::npos);

    {
        LawManager back;
        back.loadFromJson(saved);
        Law* law = back.find("sonnet-garden");
        assert(law && law->isAuthored());
        assert(law->authors().getMembers().front() == reg.find(sonnet.id()));
    }

    reg.revokeAuthenticatedPerson(zach.id());
    {
        LawManager absent;
        absent.loadFromJson(saved);
        Law* law = absent.find("sonnet-garden");
        assert(law && !law->isAuthored());   // loads, listed, cannot fire
    }
    reg.clear();
    reg.clearAuthenticatedPersons();
    std::cout << "  mover-authored Law reloads authored only while its mover stands OK\n";
}

int main() {
    std::cout << "foreign_actuation_test:\n";
    testHandshakeBindsTheMover();
    testBareMoverIdIsNotAuthentication();
    testChallengeIsSingleUseBoundAndShortLived();
    testKeyHolderWithoutStandingIsRefused();
    testActuationDecisions();
    testForeignLawAuthorIsTheMover();
    testMoverAuthoredLawSurvivesReload();
    std::filesystem::remove_all(root());
    std::cout << "foreign_actuation_test: ALL OK\n";
    return 0;
}
