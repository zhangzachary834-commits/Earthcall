#include "Identity/Claim.hpp"
#include <iostream>
#include <cassert>
#include <string>

using namespace Identity;

static void testClaimIssueAndVerify() {
    PrivateKey key = PrivateKey::generate();
    SingularId subject = SingularId::mintOpaque();
    SingularId object = SingularId::mintOpaque();

    Claim claim = Claim::issue(key, subject, "owns", object, 1000);
    assert(claim.verify());
    assert(claim.issuer() == key.id());
    assert(claim.subject() == subject);
    assert(claim.predicate() == "owns");
    assert(claim.object() == object);
    assert(claim.issuedAt() == 1000);

    std::cout << "  claim issue and verify OK\n";
}

static void testRoundTripAndTamperRefusal() {
    PrivateKey key = PrivateKey::generate();
    SingularId subject = SingularId::mintOpaque();
    SingularId object = SingularId::mintOpaque();
    Claim original = Claim::issue(key, subject, "owns", object, 1000);
    auto json = original.toJson();
    assert(Claim::fromJson(json).verify());

    auto changed = json;
    changed["predicate"] = "administers";
    assert(!Claim::fromJson(changed).verify());
    changed = json;
    changed["subject"] = SingularId::mintOpaque().toString();
    assert(!Claim::fromJson(changed).verify());
    changed = json;
    changed["issuedAt"] = 1001;
    assert(!Claim::fromJson(changed).verify());
    changed = json;
    changed["signature"] = "00";
    assert(!Claim::fromJson(changed).verify());
    changed = json;
    changed["issuer"] = false;
    assert(!Claim::fromJson(changed).verify());

    Claim reissued = Claim::issue(key, subject, "owns", object, 1000);
    assert(reissued.verify());
    assert(reissued.toJson()["nonce"] != json["nonce"]);
    std::cout << "  signed round-trip, tamper refusal and nonce freshness OK\n";
}

int main() {
    std::cout << "claim_test:\n";
    testClaimIssueAndVerify();
    testRoundTripAndTamperRefusal();
    std::cout << "claim_test: ALL OK\n";
    return 0;
}
