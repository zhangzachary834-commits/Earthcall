#include "Identity/FirstMoverRegister.hpp"

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

// --- FirstMoverRegister ----------------------------------------------------

FirstMoverRegister& FirstMoverRegister::instance() {
    static FirstMoverRegister reg;
    return reg;
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

    // 8d: no mover may attest itself.
    if (grantorKey.id() == mover) return false;

    FirstMover m;
    m.id = mover;
    m.kind = moverKind;
    m.displayName = displayName;
    m.grantedBy = grantorKey.id();
    m.scopes = scopes;
    m.grant = Claim::issue(grantorKey, mover, m.grantPredicate(), grantorKey.id(), at);

    auto it = std::find_if(_movers.begin(), _movers.end(),
                           [&](const FirstMover& e) { return e.id == mover; });
    if (it != _movers.end()) {
        *it = std::move(m);
    } else {
        _movers.push_back(std::move(m));
    }
    return true;
}

const FirstMover* FirstMoverRegister::find(const SingularId& mover) const {
    auto it = std::find_if(_movers.begin(), _movers.end(),
                           [&](const FirstMover& e) { return e.id == mover; });
    return it == _movers.end() ? nullptr : &*it;
}

namespace {

// Every gate, in one place, so mayWrite and explain can never disagree about
// why something was refused.
enum class Gate {
    Ok,
    NotRegistered,
    SelfAttested,
    GrantorNotPerson,
    GrantInvalid,
    GrantSubjectMismatch,
    ScopeTampered,
    OutsideSaveRoot,
    NoMatchingScope,
};

Gate evaluate(const FirstMover* m,
              const std::vector<FirstMover>& all,
              const std::filesystem::path& path,
              const std::filesystem::path& saveRoot,
              std::string* matchedRel) {
    if (!m) return Gate::NotRegistered;
    if (m->grantedBy == m->id) return Gate::SelfAttested;
    if (!m->grantedBy.canAuthenticate()) return Gate::GrantorNotPerson;

    // recognize() refuses a non-Person grantor, but a register arriving from a
    // save was not necessarily built by recognize(). A model holds a real key,
    // so a model-signed grant for another model VERIFIES -- the signature is
    // genuine, it is the authority behind it that is not. Checking the
    // grantor's kind here is what stops one compromised model from admitting
    // any number more, which is the delegation ceiling 8d exists to hold.
    for (const auto& candidate : all) {
        if (candidate.id == m->grantedBy && candidate.kind != FirstMover::Kind::Person) {
            return Gate::GrantorNotPerson;
        }
    }

    if (!m->grant.verify()) return Gate::GrantInvalid;

    // The signature must cover THIS mover, not some other entry's.
    if (m->grant.subject() != m->id) return Gate::GrantSubjectMismatch;
    if (m->grant.issuer() != m->grantedBy) return Gate::GrantSubjectMismatch;

    // Recomputing the predicate is what pins the scope list: widening scopes
    // in the file changes this string and the signature stops matching.
    if (m->grant.predicate() != m->grantPredicate()) return Gate::ScopeTampered;

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
        return Gate::OutsideSaveRoot;
    }

    // Patterns are written relative to the save root, and are matched against
    // the resolved path so both sides agree on what is being compared.
    const std::string rel = pathStr.substr(rootStr.size() + 1);
    const std::string rootName = saveRoot.filename().generic_string();

    for (const auto& scope : m->scopes) {
        // Accept patterns written with or without the leading "saves/".
        std::string pattern = scope;
        const std::string prefix = rootName + "/";
        if (pattern.rfind(prefix, 0) == 0) pattern = pattern.substr(prefix.size());

        if (matchesGlob(pattern, rel)) {
            if (matchedRel) *matchedRel = rel;
            return Gate::Ok;
        }
    }
    return Gate::NoMatchingScope;
}

} // namespace

bool FirstMoverRegister::mayWrite(const SingularId& mover,
                                  const std::filesystem::path& path) const {
    return evaluate(find(mover), _movers, path, _saveRoot, nullptr) == Gate::Ok;
}

bool FirstMoverRegister::isQuarantined(const SingularId& mover) const {
    const FirstMover* m = find(mover);
    if (!m) return false; // absent is not quarantined; it is simply unknown
    const Gate g = evaluate(m, _movers, _saveRoot / "probe", _saveRoot, nullptr);
    return g == Gate::SelfAttested || g == Gate::GrantorNotPerson ||
           g == Gate::GrantInvalid || g == Gate::GrantSubjectMismatch ||
           g == Gate::ScopeTampered;
}

std::string FirstMoverRegister::explain(const SingularId& mover,
                                        const std::filesystem::path& path) const {
    switch (evaluate(find(mover), _movers, path, _saveRoot, nullptr)) {
        case Gate::Ok: return "allowed: attested mover, path within a granted scope";
        case Gate::NotRegistered: return "refused: mover is not in the First Mover Register";
        case Gate::SelfAttested: return "refused: mover attested itself; the chain must terminate in a Person";
        case Gate::GrantorNotPerson: return "refused: grantor is not a Person";
        case Gate::GrantInvalid: return "refused: grant signature does not verify (quarantined)";
        case Gate::GrantSubjectMismatch: return "refused: grant was issued for a different mover";
        case Gate::ScopeTampered: return "refused: scope list does not match the signed grant";
        case Gate::OutsideSaveRoot: return "refused: path resolves outside the save root";
        case Gate::NoMatchingScope: return "refused: path matches none of the mover's granted scopes";
    }
    return "refused";
}

nlohmann::json FirstMoverRegister::toJson() const {
    nlohmann::json movers = nlohmann::json::array();
    for (const auto& m : _movers) movers.push_back(m.toJson());
    return nlohmann::json{{"movers", movers}};
}

void FirstMoverRegister::loadFromJson(const nlohmann::json& j) {
    _movers.clear();
    if (!j.is_object()) return;

    auto it = j.find("movers");
    if (it == j.end() || !it->is_array()) return;

    for (const auto& entry : *it) {
        FirstMover m = FirstMover::fromJson(entry);
        // Malformed ids are dropped; unverifiable grants are NOT. Per 8c an
        // unattested injection must be visible and inert rather than silently
        // discarded -- a world you cannot inspect is worse than a world with a
        // refused entry in it. mayWrite() is what keeps it inert.
        if (m.id.canAuthenticate()) _movers.push_back(std::move(m));
    }
}

} // namespace Identity
