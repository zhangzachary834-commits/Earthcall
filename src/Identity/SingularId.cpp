#include "Identity/SingularId.hpp"

#ifndef __EMSCRIPTEN__
#include <openssl/rand.h>
#endif

#include <stdexcept>

namespace Identity {

namespace {

constexpr char kAlphabet[] = "abcdefghijklmnopqrstuvwxyz234567";
constexpr const char* kOpaquePrefix = "ec1:";
constexpr const char* kKeyPrefix = "did:earthcall:";
constexpr size_t kOpaqueBytes = 16; // 128 bits
constexpr size_t kKeyBytes = 32;    // Ed25519 public key

} // namespace

std::string base32Encode(const std::vector<uint8_t>& data) {
    std::string out;
    out.reserve((data.size() * 8 + 4) / 5);

    uint32_t buffer = 0;
    int bitsLeft = 0;
    for (uint8_t byte : data) {
        buffer = (buffer << 8) | byte;
        bitsLeft += 8;
        while (bitsLeft >= 5) {
            out.push_back(kAlphabet[(buffer >> (bitsLeft - 5)) & 0x1F]);
            bitsLeft -= 5;
        }
    }
    if (bitsLeft > 0) {
        out.push_back(kAlphabet[(buffer << (5 - bitsLeft)) & 0x1F]);
    }
    return out;
}

bool base32Decode(const std::string& text, std::vector<uint8_t>& out) {
    out.clear();
    out.reserve(text.size() * 5 / 8);

    uint32_t buffer = 0;
    int bitsLeft = 0;
    for (char c : text) {
        const char* pos = nullptr;
        for (const char* p = kAlphabet; *p; ++p) {
            if (*p == c) { pos = p; break; }
        }
        if (!pos) return false; // reject rather than skip: silent tolerance
                                // here would let two spellings of one id exist
        buffer = (buffer << 5) | static_cast<uint32_t>(pos - kAlphabet);
        bitsLeft += 5;
        if (bitsLeft >= 8) {
            out.push_back(static_cast<uint8_t>((buffer >> (bitsLeft - 8)) & 0xFF));
            bitsLeft -= 8;
        }
    }
    return true;
}

std::string hexEncode(const std::vector<uint8_t>& data) {
    static const char* digits = "0123456789abcdef";
    std::string out;
    out.reserve(data.size() * 2);
    for (uint8_t b : data) {
        out.push_back(digits[b >> 4]);
        out.push_back(digits[b & 0x0F]);
    }
    return out;
}

std::vector<uint8_t> hexDecode(const std::string& hex) {
    if (hex.size() % 2 != 0) return {};

    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };

    std::vector<uint8_t> out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        int hi = nibble(hex[i]), lo = nibble(hex[i + 1]);
        if (hi < 0 || lo < 0) return {};
        out.push_back(static_cast<uint8_t>((hi << 4) | lo));
    }
    return out;
}

SingularId SingularId::mintOpaque() {
    std::vector<uint8_t> bytes(kOpaqueBytes);
#ifndef __EMSCRIPTEN__
    if (RAND_bytes(bytes.data(), static_cast<int>(bytes.size())) != 1) {
        // Falling back to a weaker source would hand out guessable identities
        // while looking like success. There is no safe degraded mode here.
        throw std::runtime_error("Identity: CSPRNG unavailable, cannot mint id");
    }
#else
    // There is no CSPRNG linked into the wasm build (OpenSSL is native-only;
    // see CMakeLists.txt's `if (NOT EMSCRIPTEN)` guard around find_package
    // (OpenSSL)). A constant id here would silently collide across every
    // call, every session, every user -- the same failure shape the native
    // branch above refuses. Fail the same way: throw, don't degrade.
    throw std::runtime_error(
        "Identity: no CSPRNG available on this platform (wasm build has no "
        "OpenSSL); cannot mint an id");
#endif
    return SingularId(Kind::Opaque, std::move(bytes));
}

SingularId SingularId::fromPublicKey(const std::array<uint8_t, 32>& publicKey) {
    return SingularId(Kind::Key, std::vector<uint8_t>(publicKey.begin(), publicKey.end()));
}

SingularId SingularId::parse(const std::string& text) {
    auto startsWith = [&text](const char* prefix) {
        return text.rfind(prefix, 0) == 0;
    };

    Kind kind;
    size_t prefixLen;
    size_t expectedBytes;

    if (startsWith(kKeyPrefix)) {
        kind = Kind::Key;
        prefixLen = std::string(kKeyPrefix).size();
        expectedBytes = kKeyBytes;
    } else if (startsWith(kOpaquePrefix)) {
        kind = Kind::Opaque;
        prefixLen = std::string(kOpaquePrefix).size();
        expectedBytes = kOpaqueBytes;
    } else {
        return SingularId{};
    }

    std::vector<uint8_t> bytes;
    if (!base32Decode(text.substr(prefixLen), bytes)) return SingularId{};

    // Length is part of the type. A short "key" would otherwise parse and then
    // fail confusingly at verification time instead of at the boundary.
    if (bytes.size() != expectedBytes) return SingularId{};

    return SingularId(kind, std::move(bytes));
}

std::string SingularId::toString() const {
    if (!_valid) return "";
    return std::string(_kind == Kind::Key ? kKeyPrefix : kOpaquePrefix) + base32Encode(_bytes);
}

std::string SingularId::abbreviated() const {
    if (!_valid) return "<invalid>";
    std::string full = base32Encode(_bytes);
    std::string tag = (_kind == Kind::Key ? "did:" : "ec1:");
    return tag + full.substr(0, 8) + "…";
}

} // namespace Identity
