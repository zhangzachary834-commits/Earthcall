#include "Identity/KeyStore.hpp"

#ifndef __EMSCRIPTEN__
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>
#endif

#include <cstdlib>
#include <fstream>
#include <iostream>

#include "json.hpp"

#if !defined(_WIN32)
#include <sys/stat.h>
#endif

namespace Identity {

namespace {

constexpr int kFormatVersion = 1;

#ifndef __EMSCRIPTEN__

// scrypt cost. N=32768/r=8 is ~32 MB per attempt, which is the point: it makes
// offline guessing against a stolen file expensive rather than merely slow.
constexpr uint64_t kScryptN = 32768;
constexpr uint64_t kScryptR = 8;
constexpr uint64_t kScryptP = 1;
constexpr uint64_t kScryptMaxMem = 96ull * 1024 * 1024;

constexpr size_t kSaltBytes = 16;
constexpr size_t kIvBytes = 12;  // GCM's native nonce size
constexpr size_t kTagBytes = 16;
constexpr size_t kKeyBytes = 32; // AES-256

bool randomBytes(std::vector<uint8_t>& out, size_t n) {
    out.resize(n);
    return RAND_bytes(out.data(), static_cast<int>(n)) == 1;
}

// Passphrase -> AES key. Returns false rather than throwing so callers can
// treat every failure on this path identically.
bool deriveKey(const std::string& passphrase,
               const std::vector<uint8_t>& salt,
               std::vector<uint8_t>& out) {
    out.assign(kKeyBytes, 0);

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_SCRYPT, nullptr);
    if (!ctx) return false;

    size_t outLen = out.size();
    bool ok =
        EVP_PKEY_derive_init(ctx) == 1 &&
        EVP_PKEY_CTX_set1_pbe_pass(ctx, passphrase.data(),
                                   static_cast<int>(passphrase.size())) == 1 &&
        EVP_PKEY_CTX_set1_scrypt_salt(ctx, salt.data(),
                                      static_cast<int>(salt.size())) == 1 &&
        EVP_PKEY_CTX_set_scrypt_N(ctx, kScryptN) == 1 &&
        EVP_PKEY_CTX_set_scrypt_r(ctx, kScryptR) == 1 &&
        EVP_PKEY_CTX_set_scrypt_p(ctx, kScryptP) == 1 &&
        EVP_PKEY_CTX_set_scrypt_maxmem_bytes(ctx, kScryptMaxMem) == 1 &&
        EVP_PKEY_derive(ctx, out.data(), &outLen) == 1 &&
        outLen == out.size();

    EVP_PKEY_CTX_free(ctx);
    return ok;
}

bool aesGcmSeal(const std::vector<uint8_t>& key,
                const std::vector<uint8_t>& iv,
                const std::vector<uint8_t>& aad,
                const std::vector<uint8_t>& plaintext,
                std::vector<uint8_t>& ciphertext,
                std::vector<uint8_t>& tag) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;

    ciphertext.assign(plaintext.size(), 0);
    tag.assign(kTagBytes, 0);
    int len = 0;
    int total = 0;
    bool ok = false;

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(iv.size()), nullptr) == 1 &&
        EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()) == 1 &&
        EVP_EncryptUpdate(ctx, nullptr, &len, aad.data(), static_cast<int>(aad.size())) == 1 &&
        EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(),
                          static_cast<int>(plaintext.size())) == 1) {
        total = len;
        if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + total, &len) == 1) {
            total += len;
            ciphertext.resize(static_cast<size_t>(total));
            ok = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG,
                                     static_cast<int>(tag.size()), tag.data()) == 1;
        }
    }

    EVP_CIPHER_CTX_free(ctx);
    return ok;
}

bool aesGcmOpen(const std::vector<uint8_t>& key,
                const std::vector<uint8_t>& iv,
                const std::vector<uint8_t>& aad,
                const std::vector<uint8_t>& ciphertext,
                const std::vector<uint8_t>& tag,
                std::vector<uint8_t>& plaintext) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;

    plaintext.assign(ciphertext.size(), 0);
    int len = 0;
    int total = 0;
    bool ok = false;

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(iv.size()), nullptr) == 1 &&
        EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()) == 1 &&
        EVP_DecryptUpdate(ctx, nullptr, &len, aad.data(), static_cast<int>(aad.size())) == 1 &&
        EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(),
                          static_cast<int>(ciphertext.size())) == 1) {
        total = len;
        // Setting the expected tag before Final is what makes this
        // authenticated: Final returns failure if the tag does not match, so a
        // tampered file cannot decrypt to attacker-chosen bytes.
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, static_cast<int>(tag.size()),
                                const_cast<uint8_t*>(tag.data())) == 1 &&
            EVP_DecryptFinal_ex(ctx, plaintext.data() + total, &len) == 1) {
            total += len;
            plaintext.resize(static_cast<size_t>(total));
            ok = true;
        }
    }

    EVP_CIPHER_CTX_free(ctx);
    if (!ok) plaintext.clear();
    return ok;
}

// Best-effort scrub. Not a guarantee under an optimizing compiler, but it
// shortens the window in which a seed sits in reusable heap memory.
void scrub(std::vector<uint8_t>& buf) {
    if (!buf.empty()) OPENSSL_cleanse(buf.data(), buf.size());
    buf.clear();
}
#endif

std::string stringField(const nlohmann::json& j, const char* key) {
    auto it = j.find(key);
    if (it == j.end() || !it->is_string()) return {};
    return it->get<std::string>();
}

} // namespace

std::filesystem::path KeyStore::defaultDirectory() {
    if (const char* home = std::getenv("EARTHCALL_HOME")) {
        if (*home) return std::filesystem::path(home) / "identity";
    }
    if (const char* home = std::getenv("HOME")) {
        if (*home) return std::filesystem::path(home) / ".earthcall" / "identity";
    }
    // Last resort. Still outside saves/, which is the directory git tracks.
    return std::filesystem::path(".earthcall") / "identity";
}

KeyStore::KeyStore(std::filesystem::path directory) : _directory(std::move(directory)) {}

std::filesystem::path KeyStore::pathFor(const SingularId& id) const {
    // base32 of the public key: filesystem-safe by construction, and no
    // caller-supplied text ever reaches this path.
    return _directory / (base32Encode(id.bytes()) + ".key");
}

bool KeyStore::store(const PrivateKey& key, const std::string& passphrase) {
#ifndef __EMSCRIPTEN__
    if (!key.isValid()) return false;

    std::error_code ec;
    std::filesystem::create_directories(_directory, ec);
    if (ec) {
        std::cerr << "[KeyStore] cannot create " << _directory << ": " << ec.message() << "\n";
        return false;
    }
#if !defined(_WIN32)
    ::chmod(_directory.c_str(), 0700);
#endif

    std::vector<uint8_t> salt, iv;
    if (!randomBytes(salt, kSaltBytes) || !randomBytes(iv, kIvBytes)) {
        std::cerr << "[KeyStore] CSPRNG unavailable\n";
        return false;
    }

    std::vector<uint8_t> aesKey;
    if (!deriveKey(passphrase, salt, aesKey)) {
        std::cerr << "[KeyStore] key derivation failed\n";
        return false;
    }

    const SingularId id = key.id();

    // The header is authenticated but not encrypted, so it must be bound as
    // AAD. Otherwise the KDF parameters could be swapped for weaker ones, or
    // the file relabelled as a different identity, without breaking the tag.
    nlohmann::json header = {
        {"version", kFormatVersion},
        {"id", id.toString()},
        {"kdf", "scrypt"},
        {"n", kScryptN},
        {"r", kScryptR},
        {"p", kScryptP},
        {"cipher", "aes-256-gcm"},
        {"salt", hexEncode(salt)},
        {"iv", hexEncode(iv)},
    };
    const std::string headerBytes = header.dump();
    const std::vector<uint8_t> aad(headerBytes.begin(), headerBytes.end());

    std::array<uint8_t, 32> seed = key.rawSeed();
    std::vector<uint8_t> plaintext(seed.begin(), seed.end());
    OPENSSL_cleanse(seed.data(), seed.size());

    std::vector<uint8_t> ciphertext, tag;
    const bool sealed = aesGcmSeal(aesKey, iv, aad, plaintext, ciphertext, tag);
    scrub(plaintext);
    scrub(aesKey);

    if (!sealed) {
        std::cerr << "[KeyStore] sealing failed\n";
        return false;
    }

    nlohmann::json file = header;
    file["ciphertext"] = hexEncode(ciphertext);
    file["tag"] = hexEncode(tag);

    const std::filesystem::path path = pathFor(id);

    // Create with 0600 from the outset rather than widening then narrowing:
    // between the two there would be a window where the sealed key is
    // world-readable.
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            std::cerr << "[KeyStore] cannot write " << path << "\n";
            return false;
        }
        out << file.dump(2) << "\n";
    }
#if !defined(_WIN32)
    ::chmod(path.c_str(), 0600);
#endif

    return true;
#else
    return false;
#endif
}

std::optional<PrivateKey> KeyStore::load(const SingularId& id,
                                         const std::string& passphrase) const {
#ifndef __EMSCRIPTEN__
    const std::filesystem::path path = pathFor(id);
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return std::nullopt;

    nlohmann::json file;
    try {
        in >> file;
    } catch (const std::exception&) {
        return std::nullopt;
    }
    if (!file.is_object()) return std::nullopt;

    auto versionIt = file.find("version");
    if (versionIt == file.end() || !versionIt->is_number_integer() ||
        versionIt->get<int>() != kFormatVersion) {
        return std::nullopt;
    }

    // The stored id must match what was asked for. Without this a renamed file
    // could hand back a different identity than the caller believes it opened.
    if (SingularId::parse(stringField(file, "id")) != id) return std::nullopt;

    const std::vector<uint8_t> salt = hexDecode(stringField(file, "salt"));
    const std::vector<uint8_t> iv = hexDecode(stringField(file, "iv"));
    const std::vector<uint8_t> ciphertext = hexDecode(stringField(file, "ciphertext"));
    const std::vector<uint8_t> tag = hexDecode(stringField(file, "tag"));
    if (salt.size() != kSaltBytes || iv.size() != kIvBytes ||
        tag.size() != kTagBytes || ciphertext.empty()) {
        return std::nullopt;
    }

    // Recompute the AAD from the header as written, so any edit to it fails the
    // tag check below.
    nlohmann::json header = file;
    header.erase("ciphertext");
    header.erase("tag");
    const std::string headerBytes = header.dump();
    const std::vector<uint8_t> aad(headerBytes.begin(), headerBytes.end());

    std::vector<uint8_t> aesKey;
    if (!deriveKey(passphrase, salt, aesKey)) return std::nullopt;

    std::vector<uint8_t> plaintext;
    const bool opened = aesGcmOpen(aesKey, iv, aad, ciphertext, tag, plaintext);
    scrub(aesKey);

    // Wrong passphrase and tampered file are the same answer on purpose.
    if (!opened || plaintext.size() != 32) {
        scrub(plaintext);
        return std::nullopt;
    }

    std::array<uint8_t, 32> seed{};
    std::copy(plaintext.begin(), plaintext.end(), seed.begin());
    scrub(plaintext);

    try {
        PrivateKey key = PrivateKey::fromRawSeed(seed);
        OPENSSL_cleanse(seed.data(), seed.size());
        if (key.id() != id) return std::nullopt;
        return key;
    } catch (const std::exception&) {
        OPENSSL_cleanse(seed.data(), seed.size());
        return std::nullopt;
    }
#else
    return std::nullopt;
#endif
}

bool KeyStore::contains(const SingularId& id) const {
    std::error_code ec;
    return std::filesystem::exists(pathFor(id), ec);
}

std::vector<SingularId> KeyStore::list() const {
    std::vector<SingularId> out;
    std::error_code ec;
    if (!std::filesystem::exists(_directory, ec)) return out;

    for (const auto& entry : std::filesystem::directory_iterator(_directory, ec)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".key") continue;

        std::ifstream in(entry.path(), std::ios::binary);
        if (!in.is_open()) continue;

        nlohmann::json file;
        try {
            in >> file;
        } catch (const std::exception&) {
            continue;
        }
        if (!file.is_object()) continue;

        SingularId id = SingularId::parse(stringField(file, "id"));
        if (id.canAuthenticate()) out.push_back(id);
    }
    return out;
}

bool KeyStore::remove(const SingularId& id) {
    std::error_code ec;
    return std::filesystem::remove(pathFor(id), ec);
}

} // namespace Identity
