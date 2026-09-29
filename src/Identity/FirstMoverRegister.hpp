#pragma once

#include "Identity/Claim.hpp"
#include "Identity/SingularId.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"
#include "json.hpp"

#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace Identity {

// ---------------------------------------------------------------------------
// The First Mover Register.
//
// Implements docs/architecture/law/FIRST_MOVER_AUTHORING.md 8a-8d, which specified
// this and left it unbuilt. A First Mover is any author who writes being
// directly into the serialization rather than causing it through in-world
// process: the engine, a human with a text editor, or a language model
// emitting a save file. They "differ in trustworthiness, not in kind" -- so
// the register is about recognition, not about capability.
//
// What this adds beyond the spec is FILE SCOPE. Recognition alone is too
// coarse for a model: "may write the substrate" should never mean "may write
// every save." Each mover carries an explicit list of path patterns, and a
// write outside them is refused even for an attested mover.
//
// Three floors from 8d, enforced here rather than documented:
//
//   * No mover may attest itself. The authorship chain terminates in a Person
//     or it does not terminate.
//   * Recognition never confers authority. Attestation authorizes writing
//     beings; raising a law's authority level stays a C++ change reviewed by a
//     Person. Collapsing the two is the tyranny the clamp exists to prevent.
//   * Scope cannot escape the save root, whatever the pattern says.
// ---------------------------------------------------------------------------

class FirstMover : public Singular {
public:
    enum class Kind { Person, Model };

    SingularId id;
    Kind kind = Kind::Model;

    // A label. Carries no authority, exactly like Person::displayName.
    std::string displayName;

    // Who recognised this mover. For a model this must be a Person: a model's
    // recognition is delegated and traceable, never self-originating.
    SingularId grantedBy;

    // Path patterns this mover may write. '*' matches within one path segment,
    // '**' matches across segments. An empty list means: recognised, but may
    // write nothing -- which is the correct default, not an error.
    std::vector<std::string> scopes;

    // grantedBy's signature over (mover, kind, scopes). Absent or failing
    // means the mover loads quarantined per 8c: present, listed, inert.
    Claim grant;

    std::string getIdentifier() const override {
        const std::string s = id.toString();
        if (!s.empty()) return s;
        return displayName.empty() ? "first-mover-unnamed" : displayName;
    }

    std::string propId() const { return id.toString(); }
    std::string propKind() const { return kind == Kind::Person ? "person" : "model"; }
    std::string propGrantedBy() const { return grantedBy.toString(); }
    std::string propScopes() const;

    // Rebuilds the exact subject bytes the grant must cover, so a scope cannot
    // be widened after the fact without invalidating the signature.
    std::string grantPredicate() const;

    nlohmann::json toJson() const;
    static FirstMover fromJson(const nlohmann::json& j);

protected:
    void buildProperties() override;
};

// Why a mover does or does not currently stand. Path-independent: this is
// the answer to "may this mover act at all right now?", and mayWrite() is
// that answer plus a path. explain()/isQuarantined()/foreign actuation all
// read this one value so diagnostics can never drift from enforcement.
enum class Standing {
    Recognized,
    NotRegistered,
    SelfAttested,
    GrantorNotPerson,
    // The grant verifies, but its grantor has not proved possession of their
    // Person key in THIS process. A serialized `kind: person` is not proof;
    // neither is a Person record on disk. Not quarantine -- nothing is
    // tampered -- but inert until the Person is present.
    GrantorNotAuthenticated,
    GrantInvalid,
    GrantSubjectMismatch,
    ScopeTampered,
    CryptoUnavailable,
};

const char* standingCode(Standing s);

class FirstMoverRegister {
public:
    // Constructible, not only a singleton: 8a specifies the register as a
    // serialized being belonging to a world, so a world owns one rather than
    // the process. instance() is the convenience handle for the active world.
    FirstMoverRegister() = default;

    static FirstMoverRegister& instance();

    // ------------------------------------------------------------------
    // Trusted Person roots.
    //
    // The grant chain must terminate in a Person who is PRESENT, meaning they
    // proved possession of their private key to this process. These roots
    // are never serialized and loadFromJson() cannot create them: a hostile
    // save can mint its own keypair, label it `kind: person`, and sign a
    // model grant, and every signature in that file would verify.
    //
    // The API takes the private key, not an id, so no caller can seed a root
    // from a public identifier alone. Production seeds exactly one: the
    // Person whose KeyStore entry unlocked and matched the loaded profile at
    // boot (EngineInit). See docs/plans/MCP_FIRST_MOVER_GOVERNANCE_
    // IMPLEMENTATION_PLAN_2026-09-18.md section 5.2-5.3.
    // ------------------------------------------------------------------
    bool trustAuthenticatedPerson(const PrivateKey& personKey);
    void revokeAuthenticatedPerson(const SingularId& person) { _authenticatedPersons.erase(person); }
    void clearAuthenticatedPersons() { _authenticatedPersons.clear(); }
    bool isAuthenticatedPerson(const SingularId& person) const {
        return _authenticatedPersons.count(person) != 0;
    }
    const std::set<SingularId>& authenticatedPersons() const { return _authenticatedPersons; }

    // Mint a grant. Refuses self-attestation, refuses to let a model attest
    // anyone, and refuses a grantor who is not an authenticated Person root.
    // Returns false without recording anything if any is attempted.
    bool recognize(const PrivateKey& grantorKey,
                   FirstMover::Kind grantorKind,
                   const SingularId& mover,
                   FirstMover::Kind moverKind,
                   const std::string& displayName,
                   const std::vector<std::string>& scopes,
                   int64_t at);

    // Recognition is a covenant, so ending it is too: only the Person who
    // granted it (holding their key, and authenticated) may withdraw it.
    // The FirstMover object itself is retired, not destroyed -- Laws it
    // authored keep a valid pointer to who authored them.
    bool revoke(const PrivateKey& grantorKey, const SingularId& mover);

    // Path-independent standing (see Standing).
    Standing standing(const SingularId& mover) const;
    std::string explainStanding(const SingularId& mover) const;

    // True only if the mover is registered, its grant verifies, its grantor is
    // an authenticated Person other than itself, and path falls inside both
    // the save root and one of its scopes. Every failure is a refusal; there
    // is no default-allow.
    bool mayWrite(const SingularId& mover, const std::filesystem::path& path) const;

    // Why a mayWrite() answer came out the way it did. For the audit surface --
    // 8c requires that a refusal be visible rather than silent.
    std::string explain(const SingularId& mover, const std::filesystem::path& path) const;

    // A mover whose grant does not verify. Per 8c these are neither discarded
    // nor honoured: they load, they are listed, and they cannot write.
    bool isQuarantined(const SingularId& mover) const;

    // Stable for the life of the process: movers are heap-owned and never
    // freed while the register lives (revoke/reload retire them instead), so
    // a Law's author Formation may hold this pointer.
    const FirstMover* find(const SingularId& mover) const;
    FirstMover* findMutable(const SingularId& mover);

    // Law-author rehydration: the mover a serialized author identifier names,
    // ONLY if it stands right now. A forged or unrooted mover claim in a save
    // must never satisfy Law::isAuthored(); such a Law stays Unauthored
    // (LawManager) or its Zone refuses activation (ZoneManager), loudly.
    // Non-cryptographic identifiers (slugs, legacy model-author Objects)
    // return nullptr and fall through to the ordinary resolvers.
    Singular* authorFor(const std::string& identifier);
    const std::vector<std::unique_ptr<FirstMover>>& movers() const { return _movers; }

    // The directory writes are confined to, whatever a scope pattern claims.
    void setSaveRoot(std::filesystem::path root) { _saveRoot = std::move(root); }
    const std::filesystem::path& saveRoot() const { return _saveRoot; }

    // ------------------------------------------------------------------
    // The acting mover.
    //
    // Unset is the normal state and means the engine, or a Person gesturing
    // in-world, is acting through ordinary process -- which this layer does
    // not govern and must not obstruct. It becomes set only while a First
    // Mover is driving: an agent session, a fixture builder, a model emitting
    // a save. That is the window in which writes are checked.
    //
    // Deliberately fail-OPEN when unset and fail-CLOSED once set. An engine
    // save must never be blocked by an authorization layer it predates; an
    // agent's write must never succeed because nobody remembered to check.
    // ------------------------------------------------------------------
    void setActiveMover(const SingularId& mover) { _activeMover = mover; }
    void clearActiveMover() { _activeMover = SingularId{}; }
    const SingularId& activeMover() const { return _activeMover; }
    bool hasActiveMover() const { return _activeMover.canAuthenticate(); }

    // The one call a write path needs: true if this write is permitted right
    // now. Unset mover -> allowed. Set mover -> must pass every gate.
    bool permitsWrite(const std::filesystem::path& path) const {
        if (!hasActiveMover()) return true;
        return mayWrite(_activeMover, path);
    }

    std::string explainWrite(const std::filesystem::path& path) const {
        if (!hasActiveMover()) return "allowed: no First Mover session active";
        return explain(_activeMover, path);
    }

    nlohmann::json toJson() const;

    // Replaces the register from a save file. Entries arrive as claims and are
    // verified here; nothing is trusted because it was in the file. Note 8d:
    // injection may not GRANT recognition, so an entry whose grant does not
    // verify lands quarantined rather than recognised.
    void loadFromJson(const nlohmann::json& j);

    void clear();

    // The register's own durable path, relative to the save root. 8d: "the
    // register is not writable by injection" -- no mover scope, however
    // wide, may write anything under this directory.
    static constexpr const char* kRegisterDirectory = "identity";
    static constexpr const char* kRegisterFile = "identity/first-movers.json";

private:
    // Retire rather than free: a Law authored by a mover keeps pointing at it.
    void retire(std::unique_ptr<FirstMover> m);

    std::vector<std::unique_ptr<FirstMover>> _movers;
    std::vector<std::unique_ptr<FirstMover>> _retired;
    std::set<SingularId> _authenticatedPersons;   // runtime only, never serialized
    std::filesystem::path _saveRoot = "saves";
    SingularId _activeMover;
};

// The canonical bytes a foreign caller signs to prove it holds a mover's key
// for one transport session. Length-prefixed, domain-separated: shared by the
// engine (verifier) and the earthcall_first_mover signer so the two cannot
// disagree, and so a mover key asked to sign this can never be tricked into
// signing a Claim.
std::vector<uint8_t> foreignSessionTranscript(const std::string& challengeId,
                                              const std::string& nonce,
                                              const std::string& connection,
                                              const SingularId& mover);

// RAII window during which a First Mover is acting. Scoped rather than a bare
// setter because an agent session that forgets to clear would leave every
// later engine save being checked against the agent's scopes -- and the
// failure would look like a permissions bug, not a leaked session.
class FirstMoverSession {
public:
    FirstMoverSession(FirstMoverRegister& reg, const SingularId& mover)
        : _reg(reg), _previous(reg.activeMover()) {
        _reg.setActiveMover(mover);
    }
    ~FirstMoverSession() { _reg.setActiveMover(_previous); }

    FirstMoverSession(const FirstMoverSession&) = delete;
    FirstMoverSession& operator=(const FirstMoverSession&) = delete;

private:
    FirstMoverRegister& _reg;
    SingularId _previous;
};

// Exposed for testing. '*' stays within a segment, '**' spans them.
bool matchesGlob(const std::string& pattern, const std::string& path);

} // namespace Identity
