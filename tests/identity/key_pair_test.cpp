#include "Identity/KeyPair.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <cstring>

using namespace Identity;

int main() {
    if (!cryptoAvailable()) {
        std::cout << "Crypto not available, skipping KeyPair tests." << std::endl;
        return 0;
    }

    // Generate
    PrivateKey priv = PrivateKey::generate();
    assert(priv.isValid());

    // Public Key derived from private
    PublicKey pub = priv.publicKey();
    assert(pub.isValid());

    // IDs
    assert(priv.id() == pub.id());

    // Roundtrip from raw seed
    auto seed = priv.rawSeed();
    PrivateKey priv2 = PrivateKey::fromRawSeed(seed);
    assert(priv2.isValid());
    assert(priv2.id() == priv.id());

    // Sign and Verify
    std::vector<uint8_t> message = {'h', 'e', 'l', 'l', 'o'};
    std::vector<uint8_t> signature = priv.sign(message);

    assert(signature.size() == 64);
    assert(pub.verify(message, signature));

    // Modify signature should fail verification
    signature[0] ^= 0xFF;
    assert(!pub.verify(message, signature));

    std::cout << "KeyPair tests passed!" << std::endl;
    return 0;
}
