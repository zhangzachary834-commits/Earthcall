#pragma once

#include "Identity/SingularId.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace Identity {

// Verifying half of an identity. Safe to persist, broadcast, and put in a save
// file -- it is the identity, publicly.
class PublicKey {
public:
    PublicKey() = default;
    explicit PublicKey(const std::array<uint8_t, 32>& raw) : _raw(raw), _valid(true) {}

    // Recover the key from the id itself. This is what "self-certifying" buys:
    // no directory lookup, so there is no directory to poison.
    static PublicKey fromId(const SingularId& id);

    bool isValid() const { return _valid; }
    const std::array<uint8_t, 32>& raw() const { return _raw; }
    SingularId id() const { return SingularId::fromPublicKey(_raw); }

    // Constant-time inside OpenSSL. Returns false on any malformed input
    // rather than throwing -- verification runs on untrusted data by
    // definition, so a bad signature is an expected value, not an error.
    bool verify(const std::vector<uint8_t>& message,
                const std::vector<uint8_t>& signature) const;

private:
    std::array<uint8_t, 32> _raw{};
    bool _valid = false;
};

// Signing half. Never serialize this into a save file or send it anywhere;
// it belongs in the platform key store (see KeyStore).
class PrivateKey {
public:
    PrivateKey() = default;
    ~PrivateKey();

    PrivateKey(PrivateKey&&) noexcept;
    PrivateKey& operator=(PrivateKey&&) noexcept;
    PrivateKey(const PrivateKey&) = delete;
    PrivateKey& operator=(const PrivateKey&) = delete;

    // Mint a new identity.
    static PrivateKey generate();

    // Restore from 32 raw seed bytes recovered from the key store.
    static PrivateKey fromRawSeed(const std::array<uint8_t, 32>& seed);

    bool isValid() const { return _pkey != nullptr; }

    PublicKey publicKey() const;
    SingularId id() const { return publicKey().id(); }

    // Export the private seed for the key store to encrypt. The returned
    // buffer is the secret; the caller must not let it reach disk in the clear.
    std::array<uint8_t, 32> rawSeed() const;

    std::vector<uint8_t> sign(const std::vector<uint8_t>& message) const;

private:
    // void* rather than EVP_PKEY* so OpenSSL headers stay out of this header
    // and out of every translation unit that merely holds an identity.
    void* _pkey = nullptr;
};

// True where a real cryptographic guarantee can actually be produced (Ed25519
// sign/verify, a CSPRNG). False on the wasm build, which links no OpenSSL --
// see CMakeLists.txt's `if (NOT EMSCRIPTEN)` guard around find_package
// (OpenSSL). Callers that need to tell "no crypto on this platform" apart
// from "checked, and it's invalid" should ask this before trusting a false
// or a caught throw from PublicKey::verify() / Claim::verify().
bool cryptoAvailable();

} // namespace Identity
