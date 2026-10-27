#include "Identity/KeyPair.hpp"
#include <iostream>

using namespace Identity;

int main() {
    if (cryptoAvailable()) {
        auto sk = PrivateKey::generate();
        auto pk = sk.publicKey();
        std::vector<uint8_t> msg = {'h', 'e', 'l', 'l', 'o'};
        auto sig = sk.sign(msg);
        bool valid = pk.verify(msg, sig);
        if (!valid) return 1;

        auto sig_bad = sig;
        if (!sig_bad.empty()) {
            sig_bad[0] ^= 1;
            bool invalid = pk.verify(msg, sig_bad);
            if (invalid) return 1;
        }

        auto seed = sk.rawSeed();
        auto sk2 = PrivateKey::fromRawSeed(seed);
        auto pk2 = sk2.publicKey();

        if (pk.raw() != pk2.raw()) return 1;

    }
    return 0;
}
