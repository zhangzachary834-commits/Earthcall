#include "Identity/KeyPair.hpp"

#ifndef __EMSCRIPTEN__
#include <openssl/evp.h>
#endif

#include <cstring>
#include <stdexcept>

namespace Identity {

#ifndef __EMSCRIPTEN__

namespace {
EVP_PKEY* asKey(void* p) { return static_cast<EVP_PKEY*>(p); }
} // namespace

// --- PublicKey -------------------------------------------------------------

PublicKey PublicKey::fromId(const SingularId& id) {
    if (!id.canAuthenticate() || id.bytes().size() != 32) return PublicKey{};
    std::array<uint8_t, 32> raw{};
    std::memcpy(raw.data(), id.bytes().data(), 32);
    return PublicKey(raw);
}

bool PublicKey::verify(const std::vector<uint8_t>& message,
                       const std::vector<uint8_t>& signature) const {
    if (!_valid || signature.size() != 64) return false;

    EVP_PKEY* pkey = EVP_PKEY_new_raw_public_key(
        EVP_PKEY_ED25519, nullptr, _raw.data(), _raw.size());
    if (!pkey) return false;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        EVP_PKEY_free(pkey);
        return false;
    }

    bool ok = false;
    if (EVP_DigestVerifyInit(ctx, nullptr, nullptr, nullptr, pkey) == 1) {
        ok = EVP_DigestVerify(ctx, signature.data(), signature.size(),
                              message.data(), message.size()) == 1;
    }

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return ok;
}

// --- PrivateKey ------------------------------------------------------------

PrivateKey::~PrivateKey() {
    if (_pkey) EVP_PKEY_free(asKey(_pkey));
}

PrivateKey::PrivateKey(PrivateKey&& other) noexcept : _pkey(other._pkey) {
    other._pkey = nullptr;
}

PrivateKey& PrivateKey::operator=(PrivateKey&& other) noexcept {
    if (this != &other) {
        if (_pkey) EVP_PKEY_free(asKey(_pkey));
        _pkey = other._pkey;
        other._pkey = nullptr;
    }
    return *this;
}

PrivateKey PrivateKey::generate() {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);
    if (!ctx) throw std::runtime_error("Identity: cannot create Ed25519 context");

    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_keygen_init(ctx) != 1 || EVP_PKEY_keygen(ctx, &pkey) != 1) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Identity: Ed25519 key generation failed");
    }
    EVP_PKEY_CTX_free(ctx);

    PrivateKey out;
    out._pkey = pkey;
    return out;
}

PrivateKey PrivateKey::fromRawSeed(const std::array<uint8_t, 32>& seed) {
    EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(
        EVP_PKEY_ED25519, nullptr, seed.data(), seed.size());
    if (!pkey) throw std::runtime_error("Identity: malformed Ed25519 seed");

    PrivateKey out;
    out._pkey = pkey;
    return out;
}

PublicKey PrivateKey::publicKey() const {
    if (!_pkey) return PublicKey{};

    std::array<uint8_t, 32> raw{};
    size_t len = raw.size();
    if (EVP_PKEY_get_raw_public_key(asKey(_pkey), raw.data(), &len) != 1 || len != raw.size()) {
        return PublicKey{};
    }
    return PublicKey(raw);
}

std::array<uint8_t, 32> PrivateKey::rawSeed() const {
    std::array<uint8_t, 32> raw{};
    if (!_pkey) return raw;

    size_t len = raw.size();
    if (EVP_PKEY_get_raw_private_key(asKey(_pkey), raw.data(), &len) != 1 || len != raw.size()) {
        throw std::runtime_error("Identity: cannot export private seed");
    }
    return raw;
}

std::vector<uint8_t> PrivateKey::sign(const std::vector<uint8_t>& message) const {
    if (!_pkey) throw std::runtime_error("Identity: cannot sign with an empty key");

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) throw std::runtime_error("Identity: cannot create signing context");

    std::vector<uint8_t> signature(64);
    size_t siglen = signature.size();

    // Ed25519 is a one-shot scheme in OpenSSL: DigestSign, never DigestSignUpdate.
    if (EVP_DigestSignInit(ctx, nullptr, nullptr, nullptr, asKey(_pkey)) != 1 ||
        EVP_DigestSign(ctx, signature.data(), &siglen, message.data(), message.size()) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Identity: signing failed");
    }

    EVP_MD_CTX_free(ctx);
    signature.resize(siglen);
    return signature;
}

#else

PublicKey PublicKey::fromId(const SingularId& id) { return PublicKey{}; }
bool PublicKey::verify(const std::vector<uint8_t>& message, const std::vector<uint8_t>& signature) const { return false; }
PrivateKey::~PrivateKey() {}
PrivateKey::PrivateKey(PrivateKey&& other) noexcept : _pkey(nullptr) {}
PrivateKey& PrivateKey::operator=(PrivateKey&& other) noexcept { return *this; }
PrivateKey PrivateKey::generate() { throw std::runtime_error("Identity: No crypto in WASM"); }
PrivateKey PrivateKey::fromRawSeed(const std::array<uint8_t, 32>& seed) { throw std::runtime_error("Identity: No crypto in WASM"); }
PublicKey PrivateKey::publicKey() const { return PublicKey{}; }
std::array<uint8_t, 32> PrivateKey::rawSeed() const { return std::array<uint8_t, 32>{}; }
std::vector<uint8_t> PrivateKey::sign(const std::vector<uint8_t>& message) const { return std::vector<uint8_t>{}; }

#endif

} // namespace Identity
