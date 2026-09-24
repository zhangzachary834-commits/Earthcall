#include "Identity/FirstMoverRegister.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"

#include <algorithm>
#include <sstream>

namespace Identity {

namespace {

const char* kindName(FirstMover::Kind k) {
    return k == FirstMover::Kind::Person ? "person" : "model";
}

FirstMover::Kind kindFromName(const std::string& s) {
    return s == "person" ? FirstMover::Kind::Person : FirstMover::Kind::Model;
}

std::string stringField(const nlohmann::json& j, const char* key) {
    auto it = j.find(key);
    if (it == j.end() || !it->is_string()) return {};
    return it->get<std::string>();
}

// Glob over '/'-separated paths. '**' spans separators, '*' does not -- so a
// scope of "saves/games/*" cannot be satisfied by "saves/games/../../etc".
// (That path is also blocked by normalisation below; the two are independent
// defences on purpose.)
bool globMatch(const std::string& pat, size_t pi, const std::string& str, size_t si) {
    while (pi < pat.size()) {
        if (pat[pi] == '*') {
            const bool doubled = (pi + 1 < pat.size() && pat[pi + 1] == '*');
            const size_t next = pi + (doubled ? 2 : 1);

            // Try every expansion, shortest first.
            for (size_t k = si; k <= str.size(); ++k) {
                if (globMatch(pat, next, str, k)) return true;
                if (k < str.size() && !doubled && str[k] == '/') break;
            }
            return false;
        }
        if (si >= str.size()) return false;
        if (pat[pi] != '?' && pat[pi] != str[si]) return false;
        ++pi;
        ++si;
    }
    return si == str.size();
}

} // namespace

bool matchesGlob(const std::string& pattern, const std::string& path) {
    return globMatch(pattern, 0, path, 0);
}

// --- FirstMover ------------------------------------------------------------

std::string FirstMover::grantPredicate() const {
    // The scopes are inside the signed predicate, not merely alongside it.
    // If they were signed separately, an attacker could keep a valid signature
    // and swap the scope list for a wider one.
    std::ostringstream os;
    os << "first-mover:" << kindName(kind) << ':';
    std::vector<std::string> sorted = scopes;
    std::sort(sorted.begin(), sorted.end()); // order must not change the meaning
    for (const auto& s : sorted) os << s.size() << ':' << s << ';';
    return os.str();
}

nlohmann::json FirstMover::toJson() const {
    return nlohmann::json{
        {"id", id.toString()},
        {"kind", kindName(kind)},
        {"displayName", displayName},
        {"grantedBy", grantedBy.toString()},
        {"scopes", scopes},
        {"grant", grant.toJson()},
    };
}

FirstMover FirstMover::fromJson(const nlohmann::json& j) {
    FirstMover m;
    if (!j.is_object()) return m;

    m.id = SingularId::parse(stringField(j, "id"));
    m.kind = kindFromName(stringField(j, "kind"));
    m.displayName = stringField(j, "displayName");
    m.grantedBy = SingularId::parse(stringField(j, "grantedBy"));

    auto scopesIt = j.find("scopes");
    if (scopesIt != j.end() && scopesIt->is_array()) {
        for (const auto& s : *scopesIt) {
            if (s.is_string()) m.scopes.push_back(s.get<std::string>());
        }
    }

    auto grantIt = j.find("grant");
    if (grantIt != j.end()) m.grant = Claim::fromJson(*grantIt);

    return m;
}

std::string FirstMover::propScopes() const {
    std::string out;
    for (const auto& sc : scopes) {
        if (!out.empty()) out += ',';
        out += sc;
    }
    return out;
}

void FirstMover::buildProperties() {
    registerProperty(std::make_unique<ComputedProperty<FirstMover, std::string>>(
        "id", this, &FirstMover::propId, nullptr));
    registerProperty(std::make_unique<PropertyRef<FirstMover, std::string>>(
        "displayName", this, &FirstMover::displayName));
    registerProperty(std::make_unique<ComputedProperty<FirstMover, std::string>>(
        "kind", this, &FirstMover::propKind, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FirstMover, std::string>>(
        "grantedBy", this, &FirstMover::propGrantedBy, nullptr));
    // Read-only: scopes are inside the signed predicate, so a property write
    // could only ever desynchronise them from the grant (ScopeTampered).
    // Widening a scope is a new grant from the Person, not a field edit.
    registerProperty(std::make_unique<ComputedProperty<FirstMover, std::string>>(
        "scopes", this, &FirstMover::propScopes, nullptr));
    // Not registered, named per NO_BLACK_BOX.md: `grant` (a signature blob;
    // its meaning is exposed as the register's Standing, not as bytes).
}

// --- FirstMoverRegister ----------------------------------------------------

FirstMoverRegister& FirstMoverRegister::instance() {
    static FirstMoverRegister reg;
    return reg;
}

bool FirstMoverRegister::trustAuthenticatedPerson(const PrivateKey& personKey) {
    if (!personKey.isValid()) return false;
    const SingularId id = personKey.id();
    if (!id.canAuthenticate()) return false;
    _authenticatedPersons.insert(id);
    return true;
}

bool FirstMoverRegister::recognize(const PrivateKey& grantorKey,
                                   FirstMover::Kind grantorKind,
                                   const SingularId& mover,
                                   FirstMover::Kind moverKind,
                                   const std::string& displayName,
                                   const std::vector<std::string>& scopes,
                                   int64_t at) {
    if (!grantorKey.isValid() || !mover.canAuthenticate()) return false;

    // 8d: the chain terminates in a Person. A model cannot widen the circle of
    // authors, or a compromised model could recognise a hundred more.
    if (grantorKind != FirstMover::Kind::Person) return false;

    // ...and that Person must be present. The caller's say-so that the key is
    // a Person's is exactly the label a hostile save can forge.
    if (!isAuthenticatedPerson(grantorKey.id())) return false;

    // 8d: no mover may attest itself.
    if (grantorKey.id() == mover) return false;

    FirstMover m;
    m.id = mover;
    m.kind = moverKind;
    m.displayName = displayName;
    m.grantedBy = grantorKey.id();
    m.scopes = scopes;
    m.grant = Claim::issue(grantorKey, mover, m.grantPredicate(), grantorKey.id(), at);

    // Re-granting updates the SAME object in place, so every Law already
    // pointing at this mover as author keeps pointing at it.
    if (FirstMover* existing = findMutable(mover)) {
        existing->kind = m.kind;
        existing->displayName = m.displayName;
        existing->grantedBy = m.grantedBy;
        existing->scopes = m.scopes;
        existing->grant = m.grant;
    } else {
        _movers.push_back(std::make_unique<FirstMover>(std::move(m)));
    }
    return true;
}

bool FirstMoverRegister::revoke(const PrivateKey& grantorKey, const SingularId& mover) {
    if (!grantorKey.isValid() || !isAuthenticatedPerson(grantorKey.id())) return false;
    auto it = std::find_if(_movers.begin(), _movers.end(),
                           [&](const std::unique_ptr<FirstMover>& e) { return e->id == mover; });
    if (it == _movers.end()) return false;
    if ((*it)->grantedBy != grantorKey.id()) return false;
    retire(std::move(*it));
    _movers.erase(it);
    if (_activeMover == mover) _activeMover = SingularId{};
    return true;
}

void FirstMoverRegister::retire(std::unique_ptr<FirstMover> m) {
    if (m) _retired.push_back(std::move(m));
}

void FirstMoverRegister::clear() {
    for (auto& m : _movers) retire(std::move(m));
    _movers.clear();
}

const FirstMover* FirstMoverRegister::find(const SingularId& mover) const {
    auto it = std::find_if(_movers.begin(), _movers.end(),
                           [&](const std::unique_ptr<FirstMover>& e) { return e->id == mover; });
    return it == _movers.end() ? nullptr : it->get();
}

Singular* FirstMoverRegister::authorFor(const std::string& identifier) {
    const SingularId id = SingularId::parse(identifier);
    if (!id.canAuthenticate()) return nullptr;
    if (standing(id) != Standing::Recognized) return nullptr;
    return findMutable(id);
}

FirstMover* FirstMoverRegister::findMutable(const SingularId& mover) {
    return const_cast<FirstMover*>(static_cast<const FirstMoverRegister*>(this)->find(mover));
}

namespace {

// Path gates, layered on top of Standing. Every gate lives in one of these
// two functions so mayWrite and explain can never disagree about a refusal.
enum class PathGate {
    Ok,
    OutsideSaveRoot,
    RegisterIsCovenant,
    NoMatchingScope,
};

Standing evaluateStanding(const FirstMover* m,
                          const std::vector<std::unique_ptr<FirstMover>>& all,
                          const std::set<SingularId>& authenticatedPersons) {
    if (!m) return Standing::NotRegistered;
    if (m->grantedBy == m->id) return Standing::SelfAttested;
    if (!m->grantedBy.canAuthenticate()) return Standing::GrantorNotPerson;

    // recognize() refuses a non-Person grantor, but a register arriving from a
    // save was not necessarily built by recognize(). A model holds a real key,
    // so a model-signed grant for another model VERIFIES -- the signature is
    // genuine, it is the authority behind it that is not. Checking the
    // grantor's kind here is what stops one compromised model from admitting
    // any number more, which is the delegation ceiling 8d exists to hold.
    for (const auto& candidate : all) {
        if (candidate->id == m->grantedBy && candidate->kind != FirstMover::Kind::Person) {
            return Standing::GrantorNotPerson;
        }
    }

    // Ask before verifying rather than catching a throw from grant.verify():
    // this keeps the gate a plain value computation with no dependency on
    // wasm exception-catching support, and it means we never actually invoke
    // a verify() that cannot deliver a real answer.
    if (!cryptoAvailable()) return Standing::CryptoUnavailable;

    if (!m->grant.verify()) return Standing::GrantInvalid;

    // The signature must cover THIS mover, not some other entry's.
    if (m->grant.subject() != m->id) return Standing::GrantSubjectMismatch;
    if (m->grant.issuer() != m->grantedBy) return Standing::GrantSubjectMismatch;

    // Recomputing the predicate is what pins the scope list: widening scopes
    // in the file changes this string and the signature stops matching.
    if (m->grant.predicate() != m->grantPredicate()) return Standing::ScopeTampered;

    // Last, deliberately: a tampered grant should be REPORTED as tampered
    // (quarantine) even while its Person is away. The absent-grantor case is
    // the 2026-08-20 trust-root finding: before this gate, a valid signature
    // from a grantor absent from the register was never proven to terminate
    // in a Person at all.
    if (authenticatedPersons.count(m->grantedBy) == 0) return Standing::GrantorNotAuthenticated;

    return Standing::Recognized;
}

PathGate evaluatePath(const FirstMover& m,
                      const std::filesystem::path& path,
                      const std::filesystem::path& saveRoot) {
    // Normalise before comparing. weakly_canonical resolves '..' and symlinks,
    // so a scope pattern cannot be satisfied by a path that lexically looks
    // inside the root but resolves outside it.
    std::error_code ec;
    std::filesystem::path absRoot = std::filesystem::weakly_canonical(saveRoot, ec);
    if (ec) absRoot = saveRoot.lexically_normal();
    std::filesystem::path absPath = std::filesystem::weakly_canonical(path, ec);
    if (ec) absPath = path.lexically_normal();

    const std::string rootStr = absRoot.generic_string();
    const std::string pathStr = absPath.generic_string();
    if (pathStr.size() <= rootStr.size() || pathStr.compare(0, rootStr.size(), rootStr) != 0 ||
        pathStr[rootStr.size()] != '/') {
        return PathGate::OutsideSaveRoot;
    }

    // Patterns are written relative to the save root, and are matched against
    // the resolved path so both sides agree on what is being compared.
    const std::string rel = pathStr.substr(rootStr.size() + 1);

    // 8d floor, checked before any scope: the register is not writable by
    // injection. A mover granted "**" still cannot grant itself more.
    const std::string regDir = std::string(FirstMoverRegister::kRegisterDirectory) + "/";
    if (rel.rfind(regDir, 0) == 0 || rel == FirstMoverRegister::kRegisterDirectory) {
        return PathGate::RegisterIsCovenant;
    }

    const std::string rootName = saveRoot.filename().generic_string();
    for (const auto& scope : m.scopes) {
        // Accept patterns written with or without the leading "saves/".
        std::string pattern = scope;
        const std::string prefix = rootName + "/";
        if (pattern.rfind(prefix, 0) == 0) pattern = pattern.substr(prefix.size());
        if (matchesGlob(pattern, rel)) return PathGate::Ok;
    }
    return PathGate::NoMatchingScope;
}

std::string standingText(Standing s) {
    switch (s) {
        case Standing::Recognized: return "allowed: attested mover with an authenticated Person grantor";
        case Standing::NotRegistered: return "refused: mover is not in the First Mover Register";
        case Standing::SelfAttested: return "refused: mover attested itself; the chain must terminate in a Person";
        case Standing::GrantorNotPerson: return "refused: grantor is not a Person";
        case Standing::GrantorNotAuthenticated:
            return "refused: the granting Person has not authenticated in this session "
                   "(unlock their key at boot with EARTHCALL_KEY_PASSPHRASE); a serialized "
                   "`kind: person` or a Person record on disk is not a trust root";
        case Standing::GrantInvalid: return "refused: grant signature does not verify (quarantined)";
        case Standing::GrantSubjectMismatch: return "refused: grant was issued for a different mover";
        case Standing::ScopeTampered: return "refused: scope list does not match the signed grant";
        case Standing::CryptoUnavailable:
            return "refused: no cryptographic verification is available on this platform "
                   "(wasm build has no CSPRNG/OpenSSL); this grant's validity cannot be "
                   "determined here, so the write is refused rather than assumed";
    }
    return "refused";
}

} // namespace

const char* standingCode(Standing s) {
    switch (s) {
        case Standing::Recognized: return "recognized";
        case Standing::NotRegistered: return "not-registered";
        case Standing::SelfAttested: return "self-attested";
        case Standing::GrantorNotPerson: return "grantor-not-person";
        case Standing::GrantorNotAuthenticated: return "grantor-not-authenticated";
        case Standing::GrantInvalid: return "grant-invalid";
        case Standing::GrantSubjectMismatch: return "grant-subject-mismatch";
        case Standing::ScopeTampered: return "scope-tampered";
        case Standing::CryptoUnavailable: return "crypto-unavailable";
    }
    return "refused";
}

Standing FirstMoverRegister::standing(const SingularId& mover) const {
    return evaluateStanding(find(mover), _movers, _authenticatedPersons);
}

std::string FirstMoverRegister::explainStanding(const SingularId& mover) const {
    return standingText(standing(mover));
}

bool FirstMoverRegister::mayWrite(const SingularId& mover,
                                  const std::filesystem::path& path) const {
    const FirstMover* m = find(mover);
    if (evaluateStanding(m, _movers, _authenticatedPersons) != Standing::Recognized) return false;
    return evaluatePath(*m, path, _saveRoot) == PathGate::Ok;
}

bool FirstMoverRegister::isQuarantined(const SingularId& mover) const {
    const FirstMover* m = find(mover);
    if (!m) return false; // absent is not quarantined; it is simply unknown
    const Standing s = evaluateStanding(m, _movers, _authenticatedPersons);
    // CryptoUnavailable and GrantorNotAuthenticated are deliberately
    // excluded: quarantine means "this grant looks tampered with," a judgment
    // about the data. An unverifiable platform or an absent Person is not a
    // judgment about the data. mayWrite() still refuses both (fail-closed).
    return s == Standing::SelfAttested || s == Standing::GrantorNotPerson ||
           s == Standing::GrantInvalid || s == Standing::GrantSubjectMismatch ||
           s == Standing::ScopeTampered;
}

std::string FirstMoverRegister::explain(const SingularId& mover,
                                        const std::filesystem::path& path) const {
    const FirstMover* m = find(mover);
    const Standing s = evaluateStanding(m, _movers, _authenticatedPersons);
    if (s != Standing::Recognized) return standingText(s);
    switch (evaluatePath(*m, path, _saveRoot)) {
        case PathGate::Ok: return "allowed: attested mover, path within a granted scope";
        case PathGate::OutsideSaveRoot: return "refused: path resolves outside the save root";
        case PathGate::RegisterIsCovenant:
            return "refused: the First Mover Register is not writable by injection; "
                   "recognition is a covenant granted by a Person, not a file write";
        case PathGate::NoMatchingScope: return "refused: path matches none of the mover's granted scopes";
    }
    return "refused";
}

nlohmann::json FirstMoverRegister::toJson() const {
    nlohmann::json movers = nlohmann::json::array();
    for (const auto& m : _movers) movers.push_back(m->toJson());
    return nlohmann::json{{"movers", movers}};
}

void FirstMoverRegister::loadFromJson(const nlohmann::json& j) {
    // Replace the ENTRIES, never the trusted Person roots: nothing in a file
    // can make a Person present. Existing mover objects are updated in place
    // (so Laws naming them stay valid); entries no longer listed are retired.
    std::vector<std::unique_ptr<FirstMover>> next;
    if (j.is_object()) {
        auto it = j.find("movers");
        if (it != j.end() && it->is_array()) {
            for (const auto& entry : *it) {
                FirstMover m = FirstMover::fromJson(entry);
                // Malformed ids are dropped; unverifiable grants are NOT. Per
                // 8c an unattested injection must be visible and inert rather
                // than silently discarded -- a world you cannot inspect is
                // worse than a world with a refused entry in it. standing()
                // is what keeps it inert.
                if (!m.id.canAuthenticate()) continue;
                const bool duplicate = std::any_of(next.begin(), next.end(),
                    [&](const std::unique_ptr<FirstMover>& e) { return e->id == m.id; });
                if (duplicate) continue;

                auto existing = std::find_if(_movers.begin(), _movers.end(),
                    [&](const std::unique_ptr<FirstMover>& e) { return e && e->id == m.id; });
                if (existing != _movers.end()) {
                    FirstMover& live = **existing;
                    live.kind = m.kind;
                    live.displayName = m.displayName;
                    live.grantedBy = m.grantedBy;
                    live.scopes = m.scopes;
                    live.grant = m.grant;
                    next.push_back(std::move(*existing));
                } else {
                    next.push_back(std::make_unique<FirstMover>(std::move(m)));
                }
            }
        }
    }
    for (auto& old : _movers) {
        if (old) retire(std::move(old));
    }
    _movers = std::move(next);
}

std::vector<uint8_t> foreignSessionTranscript(const std::string& challengeId,
                                              const std::string& nonce,
                                              const std::string& connection,
                                              const SingularId& mover) {
    // Every field length-prefixed so no attacker-chosen field can shift bytes
    // into its neighbour; the leading domain separator is not a Claim
    // encoding, so a signature over this can never double as a grant.
    std::vector<uint8_t> out;
    const auto put = [&](const std::string& field) {
        const std::string len = std::to_string(field.size()) + ":";
        out.insert(out.end(), len.begin(), len.end());
        out.insert(out.end(), field.begin(), field.end());
        out.push_back(';');
    };
    put("earthcall-first-mover-session-v1");
    put(challengeId);
    put(nonce);
    put(connection);
    put(mover.toString());
    return out;
}

} // namespace Identity
