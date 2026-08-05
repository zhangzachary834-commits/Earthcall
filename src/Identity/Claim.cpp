#include "Identity/Claim.hpp"

#include <openssl/rand.h>

#include <stdexcept>

namespace Identity {

namespace {

constexpr const char* kDomain = "earthcall-claim-v1";

void appendU32(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(v & 0xFF));
}

void appendU64(std::vector<uint8_t>& out, uint64_t v) {
    for (int shift = 56; shift >= 0; shift -= 8) {
        out.push_back(static_cast<uint8_t>((v >> shift) & 0xFF));
    }
}

// Length-prefixed so field boundaries are carried by the encoding rather than
// inferred from the content.
void appendField(std::vector<uint8_t>& out, const std::string& s) {
    appendU32(out, static_cast<uint32_t>(s.size()));
    out.insert(out.end(), s.begin(), s.end());
}

void appendField(std::vector<uint8_t>& out, const std::vector<uint8_t>& b) {
    appendU32(out, static_cast<uint32_t>(b.size()));
    out.insert(out.end(), b.begin(), b.end());
}

// json::value() throws when a key is present but of the wrong type, which on
// this path means malformed save data aborts the load instead of producing an
// unverifiable claim. Read the type first and fall back rather than trusting.
std::string stringField(const nlohmann::json& j, const char* key) {
    auto it = j.find(key);
    if (it == j.end() || !it->is_string()) return {};
    return it->get<std::string>();
}

int64_t intField(const nlohmann::json& j, const char* key) {
    auto it = j.find(key);
    if (it == j.end() || !it->is_number_integer()) return 0;
    return it->get<int64_t>();
}

} // namespace

std::vector<uint8_t> Claim::canonicalBytes() const {
    std::vector<uint8_t> out;
    appendField(out, std::string(kDomain));
    appendField(out, _issuer.toString());
    appendField(out, _subject.toString());
    appendField(out, _predicate);
    appendField(out, _object.toString());
    appendU64(out, static_cast<uint64_t>(_issuedAt));
    appendField(out, _nonce);
    return out;
}

Claim Claim::issue(const PrivateKey& signingKey,
                   const SingularId& subject,
                   const std::string& predicate,
                   const SingularId& object,
                   int64_t issuedAt) {
    if (!signingKey.isValid()) {
        throw std::runtime_error("Identity: cannot issue a claim without a key");
    }

    Claim c;
    c._issuer = signingKey.id();
    c._subject = subject;
    c._predicate = predicate;
    c._object = object;
    c._issuedAt = issuedAt;

    // A nonce makes two otherwise identical claims distinguishable, so a
    // re-issued right cannot be confused with a replay of the old one.
    c._nonce.resize(16);
    if (RAND_bytes(c._nonce.data(), static_cast<int>(c._nonce.size())) != 1) {
        throw std::runtime_error("Identity: CSPRNG unavailable, cannot issue claim");
    }

    c._signature = signingKey.sign(c.canonicalBytes());
    return c;
}

bool Claim::verify() const {
    if (!isWellFormed()) return false;

    PublicKey key = PublicKey::fromId(_issuer);
    if (!key.isValid()) return false;

    return key.verify(canonicalBytes(), _signature);
}

nlohmann::json Claim::toJson() const {
    return nlohmann::json{
        {"issuer", _issuer.toString()},
        {"subject", _subject.toString()},
        {"predicate", _predicate},
        {"object", _object.toString()},
        {"issuedAt", _issuedAt},
        {"nonce", hexEncode(_nonce)},
        {"signature", hexEncode(_signature)},
    };
}

Claim Claim::fromJson(const nlohmann::json& j) {
    Claim c;
    if (!j.is_object()) return c;

    // Type-checked throughout: this reads untrusted save data, so a missing or
    // wrongly-typed field must yield an unverifiable claim rather than an
    // exception mid-load.
    c._issuer = SingularId::parse(stringField(j, "issuer"));
    c._subject = SingularId::parse(stringField(j, "subject"));
    c._predicate = stringField(j, "predicate");
    c._object = SingularId::parse(stringField(j, "object"));
    c._issuedAt = intField(j, "issuedAt");
    c._nonce = hexDecode(stringField(j, "nonce"));
    c._signature = hexDecode(stringField(j, "signature"));
    return c;
}

} // namespace Identity
