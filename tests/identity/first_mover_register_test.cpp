#include "Identity/FirstMoverRegister.hpp"
#include "Identity/KeyPair.hpp"
#include <iostream>
#include <cassert>

using namespace Identity;

int main() {
    if (!cryptoAvailable()) {
        std::cout << "Crypto not available, skipping FirstMoverRegister tests." << std::endl;
        return 0;
    }

    FirstMoverRegister reg;

    PrivateKey personKey = PrivateKey::generate();
    PrivateKey moverKey = PrivateKey::generate();

    assert(reg.standing(moverKey.id()) == Standing::NotRegistered);

    reg.trustAuthenticatedPerson(personKey);

    assert(reg.recognize(personKey, FirstMover::Kind::Person, moverKey.id(), FirstMover::Kind::Model, "Test Model", {"test/*"}, 100));

    assert(reg.standing(moverKey.id()) == Standing::Recognized);
    assert(!reg.isQuarantined(moverKey.id()));

    reg.revoke(personKey, moverKey.id());
    assert(reg.standing(moverKey.id()) == Standing::NotRegistered);

    // Repopulate for load test
    reg.recognize(personKey, FirstMover::Kind::Person, moverKey.id(), FirstMover::Kind::Model, "Test Model", {"test/*"}, 100);

    auto j = reg.toJson();

    FirstMoverRegister reg2;
    reg2.loadFromJson(j);
    // After load, grantor is NOT authenticated, so it should be GrantorNotAuthenticated
    assert(reg2.standing(moverKey.id()) == Standing::GrantorNotAuthenticated);

    // Now trust the person again
    reg2.trustAuthenticatedPerson(personKey);
    assert(reg2.standing(moverKey.id()) == Standing::Recognized);

    std::cout << "FirstMoverRegister tests passed!" << std::endl;
    return 0;
}
