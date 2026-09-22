#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Identity {

// ---------------------------------------------------------------------------
// A SingularId is what a Singular *is*, as distinct from what it is *called*.
//
// The old model made these the same thing: getIdentifier() returned a name a
// Person chose, ownership was that name compared with ==, and the name round
// tripped through the save file as plain text. Anything that could write the
// name could become the being -- by editing a save, or simply by picking the
// same soulName. A name is a label; labels are meant to be shareable and
// re-usable, so they can never carry authority.
//
// A SingularId carries no meaning a reader can choose. Two kinds exist:
//
//   Opaque - 128 random bits. For Objects, Zones, Laws, Lexemes: things that
//            need a stable unforgeable *reference* but cannot hold a secret.
//            Unguessable, but possession of the id is not proof of anything.
//
//   Key    - the 32-byte Ed25519 public key of a keypair. For Persons. The id
//            IS the key, so it is self-certifying: claiming it means producing
//            a signature only the holder of the private half can produce.
//            There is no registry to compromise and no name to squat.
//
// Text form is prefixed so the kind can never be silently reinterpreted:
//   ec1:<base32>          Opaque
//   did:earthcall:<base32> Key
// ---------------------------------------------------------------------------
class SingularId {
public:
    enum class Kind { Opaque, Key };

    // A default SingularId is deliberately invalid: identity must be minted or
    // parsed, never default-constructed into existence.
    SingularId() = default;

    // Mint a fresh unguessable reference. Uses the CSPRNG, not rand().
    static SingularId mintOpaque();

    // Build a Person-grade id from a raw 32-byte Ed25519 public key.
    static SingularId fromPublicKey(const std::array<uint8_t, 32>& publicKey);

    // Parse a text form. Returns an invalid id on anything malformed -- callers
    // must check isValid() rather than assuming, because this is exactly the
    // boundary untrusted save data crosses.
    static SingularId parse(const std::string& text);

    bool isValid() const { return _valid; }
    Kind kind() const { return _kind; }

    // Only a Key id can sign, so only a Key id can hold rights.
    bool canAuthenticate() const { return _valid && _kind == Kind::Key; }

    const std::vector<uint8_t>& bytes() const { return _bytes; }

    // Canonical text form. Stable across runs and platforms.
    std::string toString() const;

    // Short prefix for logs and UI. Never use this for comparison -- it is a
    // display convenience and prefixes collide by design.
    std::string abbreviated() const;

    bool operator==(const SingularId& o) const {
        return _valid == o._valid && _kind == o._kind && _bytes == o._bytes;
    }
    bool operator!=(const SingularId& o) const { return !(*this == o); }
    bool operator<(const SingularId& o) const {
        if (_kind != o._kind) return _kind < o._kind;
        return _bytes < o._bytes;
    }

private:
    SingularId(Kind kind, std::vector<uint8_t> bytes)
        : _bytes(std::move(bytes)), _kind(kind), _valid(true) {}

    std::vector<uint8_t> _bytes;
    Kind _kind = Kind::Opaque;
    bool _valid = false;
};

// Lowercase RFC 4648 base32, no padding. Chosen over hex for length and over
// base64 because these ids appear in filenames and logs, where case folding
// and '/' would both be hazards.
std::string base32Encode(const std::vector<uint8_t>& data);
bool base32Decode(const std::string& text, std::vector<uint8_t>& out);

// Hex for the binary fields inside serialized claims and key files. Decoding
// returns an empty vector on any malformed input rather than partial output --
// a half-decoded signature must never read as a short signature.
std::string hexEncode(const std::vector<uint8_t>& data);
std::vector<uint8_t> hexDecode(const std::string& hex);

} // namespace Identity

namespace std {
template <>
struct hash<Identity::SingularId> {
    size_t operator()(const Identity::SingularId& id) const noexcept {
        // FNV-1a over the raw bytes; ids are already uniformly random.
        uint64_t h = 1469598103934665603ull;
        for (uint8_t b : id.bytes()) {
            h ^= b;
            h *= 1099511628211ull;
        }
        return h;
    }
};
} // namespace std
