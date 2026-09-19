// Identity foundation: self-certifying ids and signed claims.
//
// These cases are written as the attacks they exist to stop, not as feature
// demos. Each one is something that succeeded against the old string-identifier
// model, so a regression here is a real loss of ground.
#include "Identity/Claim.hpp"
#include "Identity/KeyStore.hpp"
#include "Identity/KeyPair.hpp"
#include "Identity/SingularId.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <set>

using namespace Identity;

static void testIdRoundTrip() {
    SingularId opaque = SingularId::mintOpaque();
    assert(opaque.isValid());
    assert(opaque.kind() == SingularId::Kind::Opaque);
    assert(!opaque.canAuthenticate()); // an object can be referenced, not impersonated

    SingularId parsed = SingularId::parse(opaque.toString());
    assert(parsed.isValid());
    assert(parsed == opaque);

    PrivateKey key = PrivateKey::generate();
    SingularId did = key.id();
    assert(did.canAuthenticate());
    assert(SingularId::parse(did.toString()) == did);

    // The two kinds must never be confusable by text form.
    assert(did.toString().rfind("did:earthcall:", 0) == 0);
    assert(opaque.toString().rfind("ec1:", 0) == 0);

    std::cout << "  id round-trip OK (" << did.abbreviated() << ")\n";
}

static void testIdsAreUnguessable() {
    // The old objectID was a sequential counter, so ids from one save collided
    // with ids from another and the next one was always predictable.
    std::set<std::string> seen;
    for (int i = 0; i < 2000; ++i) {
        assert(seen.insert(SingularId::mintOpaque().toString()).second);
    }
    std::cout << "  2000 minted ids, no collisions OK\n";
}

static void testMalformedIdsRejected() {
    const char* bad[] = {
        "",
        "Zach",                        // a bare name, the old identifier
        "did:earthcall:",              // empty body
        "did:earthcall:!!!!",          // non-alphabet
        "ec1:aaaa",                    // right prefix, wrong length
        "did:earthcall:aaaa",          // truncated key
        "ec2:aaaaaaaaaaaaaaaaaaaaaaaaaa",
    };
    for (const char* s : bad) {
        assert(!SingularId::parse(s).isValid());
    }
    std::cout << "  malformed ids rejected OK\n";
}

static void testSignAndVerify() {
    PrivateKey key = PrivateKey::generate();
    std::vector<uint8_t> msg = {'h', 'e', 'l', 'l', 'o'};

    std::vector<uint8_t> sig = key.sign(msg);
    assert(sig.size() == 64);
    assert(key.publicKey().verify(msg, sig));

    // Recovering the key from the id alone must work -- that is what makes the
    // id self-certifying rather than a lookup into a trusted registry.
    assert(PublicKey::fromId(key.id()).verify(msg, sig));

    std::vector<uint8_t> tampered = msg;
    tampered[0] = 'H';
    assert(!key.publicKey().verify(tampered, sig));

    std::vector<uint8_t> badSig = sig;
    badSig[0] ^= 0x01;
    assert(!key.publicKey().verify(msg, badSig));

    std::cout << "  sign/verify + tamper detection OK\n";
}

static void testSeedRestoresIdentity() {
    // The key store persists the seed; restoring it must yield the same being.
    PrivateKey original = PrivateKey::generate();
    PrivateKey restored = PrivateKey::fromRawSeed(original.rawSeed());
    assert(restored.id() == original.id());

    std::vector<uint8_t> msg = {'c', 'o', 'v', 'e', 'n', 'a', 'n', 't'};
    assert(original.publicKey().verify(msg, restored.sign(msg)));

    std::cout << "  seed restores identity OK\n";
}

static void testOwnershipClaimVerifies() {
    PrivateKey zach = PrivateKey::generate();
    SingularId home = SingularId::mintOpaque();

    Claim owns = Claim::issue(zach, home, "owns", zach.id(), 1000);
    assert(owns.verify());
    assert(owns.issuer() == zach.id());
    assert(owns.subject() == home);

    // Survives the save-file round trip.
    Claim reloaded = Claim::fromJson(owns.toJson());
    assert(reloaded.verify());
    assert(reloaded.subject() == home);

    std::cout << "  ownership claim verifies through save round-trip OK\n";
}

static void testSaveFileTamperingFails() {
    // The attack that worked before: open the save, change the owner field.
    PrivateKey zach = PrivateKey::generate();
    PrivateKey mallory = PrivateKey::generate();
    SingularId home = SingularId::mintOpaque();

    Claim owns = Claim::issue(zach, home, "owns", zach.id(), 1000);
    nlohmann::json j = owns.toJson();

    // Rewrite the beneficiary, keeping Zach's signature.
    j["object"] = mallory.id().toString();
    assert(!Claim::fromJson(j).verify());

    // Rewrite the issuer to Mallory, keeping Zach's signature.
    nlohmann::json j2 = owns.toJson();
    j2["issuer"] = mallory.id().toString();
    assert(!Claim::fromJson(j2).verify());

    // Retarget the claim at a different zone.
    nlohmann::json j3 = owns.toJson();
    j3["subject"] = SingularId::mintOpaque().toString();
    assert(!Claim::fromJson(j3).verify());

    // Escalate the right being asserted.
    nlohmann::json j4 = owns.toJson();
    j4["predicate"] = "may-delete";
    assert(!Claim::fromJson(j4).verify());

    // Backdate it, to win an ordering comparison against a real later claim.
    nlohmann::json j5 = owns.toJson();
    j5["issuedAt"] = 1;
    assert(!Claim::fromJson(j5).verify());

    // Strip the signature entirely -- an unsigned claim must not read as valid.
    nlohmann::json j6 = owns.toJson();
    j6["signature"] = "";
    assert(!Claim::fromJson(j6).verify());

    std::cout << "  save-file tampering rejected on all 6 fields OK\n";
}

static void testNameSquattingFails() {
    // The other attack that worked: just call yourself the same thing.
    // Two Persons may share a display name; they can never share an identity.
    PrivateKey realZach = PrivateKey::generate();
    PrivateKey fakeZach = PrivateKey::generate();
    assert(realZach.id() != fakeZach.id());

    SingularId home = SingularId::mintOpaque();
    Claim real = Claim::issue(realZach, home, "owns", realZach.id(), 1000);
    Claim fake = Claim::issue(fakeZach, home, "owns", fakeZach.id(), 2000);

    // Both are genuinely signed -- so signature validity alone is not
    // ownership. The authority layer must still ask *which* issuer held the
    // zone before. This is the distinction the header warns about.
    assert(real.verify() && fake.verify());
    assert(real.issuer() != fake.issuer());

    std::cout << "  distinct identities under one display name OK\n";
}

static void testCanonicalizationAttack() {
    // Length-prefixing is what stops a field boundary from being moved. If the
    // preimage were delimiter-joined, a predicate carrying the delimiter could
    // make one claim's bytes equal another's.
    PrivateKey key = PrivateKey::generate();
    SingularId a = SingularId::mintOpaque();

    Claim c1 = Claim::issue(key, a, "owns", a, 1000);
    nlohmann::json shifted = c1.toJson();
    shifted["predicate"] = std::string("owns\0extra", 10);
    assert(!Claim::fromJson(shifted).verify());

    Claim c2 = Claim::issue(key, a, std::string("owns\0extra", 10), a, 1000);
    assert(c2.verify());
    // The two must not be interchangeable despite sharing a prefix.
    assert(c1.toJson()["signature"] != c2.toJson()["signature"]);

    std::cout << "  canonicalization attack rejected OK\n";
}

static void testGarbageClaimsRejected() {
    assert(!Claim::fromJson(nlohmann::json::object()).verify());
    assert(!Claim::fromJson(nlohmann::json::array()).verify());
    assert(!Claim::fromJson(nlohmann::json("string")).verify());

    nlohmann::json wrongTypes;
    wrongTypes["issuer"] = 42;
    wrongTypes["predicate"] = nlohmann::json::object();
    assert(!Claim::fromJson(wrongTypes).verify());

    std::cout << "  garbage and wrong-typed claims rejected OK\n";
}


static void testKeyStoreRoundTrip() {
    // Use a scratch directory so the developer's real identity is never touched.
    std::filesystem::path dir =
        std::filesystem::temp_directory_path() / "earthcall_keystore_test";
    std::filesystem::remove_all(dir);
    KeyStore store(dir);

    PrivateKey key = PrivateKey::generate();
    SingularId id = key.id();

    assert(!store.contains(id));
    assert(store.store(key, "correct horse battery staple"));
    assert(store.contains(id));

    auto reopened = store.load(id, "correct horse battery staple");
    assert(reopened.has_value());
    assert(reopened->id() == id);

    // The restored key must actually sign as the same identity.
    std::vector<uint8_t> msg = {'p', 'r', 'o', 'o', 'f'};
    assert(key.publicKey().verify(msg, reopened->sign(msg)));

    assert(store.list().size() == 1);
    assert(store.list()[0] == id);

    std::filesystem::remove_all(dir);
    std::cout << "  key store seals and restores an identity OK\n";
}

static void testKeyStoreRejectsWrongPassphrase() {
    std::filesystem::path dir =
        std::filesystem::temp_directory_path() / "earthcall_keystore_wrongpass";
    std::filesystem::remove_all(dir);
    KeyStore store(dir);

    PrivateKey key = PrivateKey::generate();
    assert(store.store(key, "right"));
    assert(!store.load(key.id(), "wrong").has_value());
    assert(!store.load(key.id(), "").has_value());
    assert(store.load(key.id(), "right").has_value());

    std::filesystem::remove_all(dir);
    std::cout << "  key store rejects wrong passphrase OK\n";
}

static void testKeyStoreDetectsTampering() {
    std::filesystem::path dir =
        std::filesystem::temp_directory_path() / "earthcall_keystore_tamper";
    std::filesystem::remove_all(dir);
    KeyStore store(dir);

    PrivateKey key = PrivateKey::generate();
    assert(store.store(key, "pass"));

    std::filesystem::path file;
    for (const auto& e : std::filesystem::directory_iterator(dir)) {
        if (e.path().extension() == ".key") file = e.path();
    }
    assert(!file.empty());

    auto rewrite = [&](const std::string& field, const nlohmann::json& value) {
        nlohmann::json j;
        { std::ifstream in(file); in >> j; }
        j[field] = value;
        { std::ofstream out(file, std::ios::trunc); out << j.dump(2); }
    };

    // Downgrading the KDF cost would make offline guessing cheap. The header is
    // bound as AAD precisely so this cannot pass.
    rewrite("n", 2);
    assert(!store.load(key.id(), "pass").has_value());

    { std::ofstream out(file, std::ios::trunc); }
    assert(store.store(key, "pass"));

    // Flipping a ciphertext byte must fail the tag rather than yield garbage.
    nlohmann::json j;
    { std::ifstream in(file); in >> j; }
    std::string ct = j["ciphertext"];
    ct[0] = (ct[0] == 'a') ? 'b' : 'a';
    rewrite("ciphertext", ct);
    assert(!store.load(key.id(), "pass").has_value());

    std::filesystem::remove_all(dir);
    std::cout << "  key store detects header and ciphertext tampering OK\n";
}

static void testKeyStoreNeverWritesIntoSaves() {
    // The default location must sit outside the repository, because saves/ is
    // tracked by git and anything under it is one commit from being published.
    std::string dir = KeyStore::defaultDirectory().string();
    assert(dir.find("/saves/") == std::string::npos);
    assert(dir.find(".earthcall") != std::string::npos ||
           dir.find("identity") != std::string::npos);
    std::cout << "  default key directory is outside saves/ OK (" << dir << ")\n";
}

int main() {
    std::cout << "identity_test:\n";
    testIdRoundTrip();
    testIdsAreUnguessable();
    testMalformedIdsRejected();
    testSignAndVerify();
    testSeedRestoresIdentity();
    testOwnershipClaimVerifies();
    testSaveFileTamperingFails();
    testNameSquattingFails();
    testCanonicalizationAttack();
    testGarbageClaimsRejected();
    testKeyStoreRoundTrip();
    testKeyStoreRejectsWrongPassphrase();
    testKeyStoreDetectsTampering();
    testKeyStoreNeverWritesIntoSaves();
    std::cout << "identity_test: ALL OK\n";
    return 0;
}
