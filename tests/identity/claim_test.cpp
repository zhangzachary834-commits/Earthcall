#include "Identity/Claim.hpp"
#include <iostream>
#include <cassert>

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

int main() {
    std::cout << "claim_test:\n";
    testClaimIssueAndVerify();
    std::cout << "claim_test: ALL OK\n";
    return 0;
}
