#pragma once

#include "Identity/KeyStore.hpp"
#include "Identity/SingularId.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <string>

namespace Identity {

// ---------------------------------------------------------------------------
// The migration ledger: which legacy name became which identity.
//
// Before identities existed, a Person was addressed by a chosen string. Turning
// those worlds into identity-bearing ones means minting a keypair per distinct
// name -- and the same name must land on the same identity every time, or
// loading two saves that both say "Zach" would produce two unrelated Persons
// and split one being's history in half.
//
// This file is therefore continuity, not authority. After migration the name
// carries nothing; only the key does. But the ledger records the one moment
// where a name WAS taken at its word, which is precisely the trust-on-first-
// migration decision -- so it lives beside the key store, outside the repo, and
// deserves the same care. Anyone who can rewrite it can decide that the next
// world's "Zach" is an identity they hold the key for.
// ---------------------------------------------------------------------------
class IdentityLedger {
public:
    static std::filesystem::path defaultPath();

    explicit IdentityLedger(std::filesystem::path path = defaultPath());

    bool load();
    bool save() const;

    // The identity a legacy name already resolved to, if any.
    std::optional<SingularId> find(const std::string& legacyName) const;

    // Resolve a legacy name, minting and sealing a fresh keypair the first time
    // it is seen. Returns nothing if the key could not be stored -- minting an
    // identity whose private half was not persisted would hand back an id that
    // can never sign again, which is worse than failing.
    std::optional<SingularId> resolveOrMint(const std::string& legacyName,
                                            KeyStore& keys,
                                            const std::string& passphrase);

    // Record a mapping without minting. Used when a save already carries a
    // personId and only the association needs remembering.
    void record(const std::string& legacyName, const SingularId& id);

    const std::map<std::string, SingularId>& entries() const { return _byName; }
    size_t size() const { return _byName.size(); }
    const std::filesystem::path& path() const { return _path; }

private:
    std::filesystem::path _path;
    std::map<std::string, SingularId> _byName;
};

} // namespace Identity
