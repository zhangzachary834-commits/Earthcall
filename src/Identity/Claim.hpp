#pragma once

#include "Identity/KeyPair.hpp"
#include "Identity/SingularId.hpp"
#include "json.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace Identity {

// ---------------------------------------------------------------------------
// A Claim is a signed assertion that one being stands in some relation to
// another: "this Person owns this Zone", "this Person authored this Law".
//
// The old model stored that as a bare field -- zj["owner"] = "Zach" -- so the
// claim and the assertion of it were the same bytes, and anyone who could
// write the file could write the right. A Claim separates them: the file
// carries the assertion *and* a signature over it, and loading verifies rather
// than trusts. Editing the owner field now invalidates the signature, and
// producing a valid one requires a private key the editor does not have.
//
// Note the boundary of what a signature proves. It proves the issuer really
// said this. It does NOT prove the issuer was *entitled* to say it -- that is
// authority policy, evaluated separately against who held the subject before.
// Conflating the two is how capability systems get broken.
// ---------------------------------------------------------------------------
class Claim {
public:
    Claim() = default;

    // Assert and sign in one step. issuer is derived from the signing key, so
    // a Claim cannot be minted in someone else's name.
    static Claim issue(const PrivateKey& signingKey,
                       const SingularId& subject,
                       const std::string& predicate,
                       const SingularId& object,
                       int64_t issuedAt);

    // True only if the signature checks out against the issuer's own id.
    bool verify() const;

    const SingularId& issuer() const { return _issuer; }
    const SingularId& subject() const { return _subject; }
    const std::string& predicate() const { return _predicate; }
    const SingularId& object() const { return _object; }
    int64_t issuedAt() const { return _issuedAt; }

    nlohmann::json toJson() const;

    // Parses without verifying -- callers must call verify(). Kept separate so
    // that "we read it" and "we believe it" are two visible steps in the code.
    static Claim fromJson(const nlohmann::json& j);

    bool isWellFormed() const {
        return _issuer.canAuthenticate() && _subject.isValid() &&
               !_predicate.empty() && _signature.size() == 64;
    }

private:
    // The exact bytes covered by the signature. Every field is length-prefixed
    // and the whole thing is domain-separated, so no two different claims can
    // ever produce the same preimage. A delimiter-joined string would not be
    // safe here: a predicate containing the delimiter could relocate a field
    // boundary and re-sign one claim as another.
    std::vector<uint8_t> canonicalBytes() const;

    SingularId _issuer;
    SingularId _subject;
    std::string _predicate;
    SingularId _object;
    int64_t _issuedAt = 0;
    std::vector<uint8_t> _nonce;
    std::vector<uint8_t> _signature;
};

} // namespace Identity
