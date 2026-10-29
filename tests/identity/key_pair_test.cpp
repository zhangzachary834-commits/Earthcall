#include "Identity/KeyPair.hpp"

#include <iostream>
#include <vector>

using namespace Identity;

namespace {
bool expect(bool condition, const char* witness) {
    if (!condition) std::cerr << "key_pair_test: " << witness << "\n";
    return condition;
}
}

int main() {
    // Native CI must exercise real Ed25519, never pass vacuously.
    if (!expect(cryptoAvailable(), "native Ed25519 backend unavailable")) return 1;

    PrivateKey key = PrivateKey::generate();
    const PublicKey publicKey = key.publicKey();
    if (!expect(key.isValid() && publicKey.isValid(), "generated key validity")) return 1;
    if (!expect(publicKey.id() == key.id(), "public/private identity agreement")) return 1;

    const std::vector<uint8_t> message = {'h', 'e', 'l', 'l', 'o'};
    const auto signature = key.sign(message);
    if (!expect(signature.size() == 64, "Ed25519 signature length")) return 1;
    if (!expect(publicKey.verify(message, signature), "valid signature")) return 1;

    const PublicKey recovered = PublicKey::fromId(key.id());
    if (!expect(recovered.isValid() && recovered.raw() == publicKey.raw(),
                "public key recovered from self-certifying id")) return 1;
    if (!expect(recovered.verify(message, signature), "recovered key verifies")) return 1;
    if (!expect(!PublicKey::fromId(SingularId::mintOpaque()).isValid(),
                "opaque id cannot masquerade as public key")) return 1;

    auto tamperedMessage = message;
    tamperedMessage[0] ^= 1;
    if (!expect(!publicKey.verify(tamperedMessage, signature), "message tampering rejected")) return 1;
    auto tamperedSignature = signature;
    tamperedSignature[0] ^= 1;
    if (!expect(!publicKey.verify(message, tamperedSignature), "signature tampering rejected")) return 1;
    tamperedSignature.pop_back();
    if (!expect(!publicKey.verify(message, tamperedSignature), "malformed signature rejected")) return 1;

    PrivateKey stranger = PrivateKey::generate();
    if (!expect(!stranger.publicKey().verify(message, signature),
                "different identity cannot verify signature")) return 1;

    PrivateKey restored = PrivateKey::fromRawSeed(key.rawSeed());
    if (!expect(restored.id() == key.id(), "seed restores identity")) return 1;
    if (!expect(publicKey.verify(message, restored.sign(message)),
                "restored key signs for original identity")) return 1;
    return 0;
}
