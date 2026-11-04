#include "Identity/Claim.hpp"
#include "Identity/KeyPair.hpp"
#include <iostream>
#include <cassert>
#include <chrono>

using namespace Identity;

int main() {
    if (!cryptoAvailable()) {
        std::cout << "Crypto not available, skipping Claim tests." << std::endl;
        return 0;
    }

    PrivateKey priv = PrivateKey::generate();
    PublicKey pub = priv.publicKey();

    SingularId subject = SingularId::mintOpaque();
    SingularId object = SingularId::mintOpaque();

    int64_t now = std::chrono::system_clock::now().time_since_epoch().count();

    Claim claim = Claim::issue(priv, subject, "owns", object, now);

    assert(claim.issuer() == pub.id());
    assert(claim.subject() == subject);
    assert(claim.predicate() == "owns");
    assert(claim.object() == object);
    assert(claim.issuedAt() == now);
    assert(claim.isWellFormed());
    assert(claim.verify());

    // Serialize and deserialize
    auto j = claim.toJson();
    Claim loadedClaim = Claim::fromJson(j);

    assert(loadedClaim.verify());
    assert(loadedClaim.issuer() == pub.id());
    assert(loadedClaim.subject() == subject);

    std::cout << "Claim tests passed!" << std::endl;
    return 0;
}
