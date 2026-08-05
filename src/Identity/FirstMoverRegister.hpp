#pragma once

#include "Identity/Claim.hpp"
#include "Identity/SingularId.hpp"
#include "json.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace Identity {

// ---------------------------------------------------------------------------
// The First Mover Register.
//
// Implements docs/architecture/FIRST_MOVER_AUTHORING.md 8a-8d, which specified
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

class FirstMover {
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

    // Rebuilds the exact subject bytes the grant must cover, so a scope cannot
    // be widened after the fact without invalidating the signature.
    std::string grantPredicate() const;

    nlohmann::json toJson() const;
    static FirstMover fromJson(const nlohmann::json& j);
};

class FirstMoverRegister {
public:
    // Constructible, not only a singleton: 8a specifies the register as a
    // serialized being belonging to a world, so a world owns one rather than
    // the process. instance() is the convenience handle for the active world.
    FirstMoverRegister() = default;

    static FirstMoverRegister& instance();

    // Mint a grant. Refuses self-attestation and refuses to let a model attest
    // anyone. Returns false without recording anything if either is attempted.
    bool recognize(const PrivateKey& grantorKey,
                   FirstMover::Kind grantorKind,
                   const SingularId& mover,
                   FirstMover::Kind moverKind,
                   const std::string& displayName,
                   const std::vector<std::string>& scopes,
                   int64_t at);

    // True only if the mover is registered, its grant verifies, its grantor is
    // a Person other than itself, and path falls inside both the save root and
    // one of its scopes. Every failure is a refusal; there is no default-allow.
    bool mayWrite(const SingularId& mover, const std::filesystem::path& path) const;

    // Why a mayWrite() answer came out the way it did. For the audit surface --
    // 8c requires that a refusal be visible rather than silent.
    std::string explain(const SingularId& mover, const std::filesystem::path& path) const;

    // A mover whose grant does not verify. Per 8c these are neither discarded
    // nor honoured: they load, they are listed, and they cannot write.
    bool isQuarantined(const SingularId& mover) const;

    const FirstMover* find(const SingularId& mover) const;
    const std::vector<FirstMover>& movers() const { return _movers; }

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

    void clear() { _movers.clear(); }

private:
    std::vector<FirstMover> _movers;
    std::filesystem::path _saveRoot = "saves";
    SingularId _activeMover;
};

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
