#pragma once

#include "Identity/KeyPair.hpp"
#include "Identity/SingularId.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace Identity {

// ---------------------------------------------------------------------------
// Custody for the signing half of an identity.
//
// The private key is the Person. Anyone holding it can sign as them, and there
// is no revocation story on the far side -- every claim that identity ever made
// becomes forgeable at once. So the rules here are absolute rather than
// best-effort:
//
//   * It never enters a save file. Saves are shared, committed, and synced to
//     cloud storage; all three would publish it.
//   * It never lands inside the repository. saves/ is tracked by git, so the
//     default directory is deliberately outside the working tree entirely.
//   * It never sits on disk in the clear. At rest it is scrypt-derived and
//     AES-256-GCM sealed, and the file is 0600.
//
// On the KDF choice: the design called for Argon2id, which is what you would
// pick given a free hand. The vendored OpenSSL is 3.0.13 and Argon2 only
// arrives in 3.2, so this uses scrypt -- memory-hard in the same way and for
// the same reason, and actually available. Substituting a non-memory-hard KDF
// like plain PBKDF2 would NOT have been an acceptable trade, because the whole
// point is to make offline guessing against a stolen file expensive.
// ---------------------------------------------------------------------------
class KeyStore {
public:
    // Defaults to $EARTHCALL_HOME/identity, else ~/.earthcall/identity.
    // Explicitly outside the repo: see the note above.
    static std::filesystem::path defaultDirectory();

    explicit KeyStore(std::filesystem::path directory = defaultDirectory());

    // Seals the key under the passphrase and writes it 0600. Overwrites any
    // existing entry for the same identity.
    bool store(const PrivateKey& key, const std::string& passphrase);

    // Returns nothing on a wrong passphrase, a missing entry, or a file that
    // fails its authentication tag. Those are deliberately indistinguishable
    // to the caller: reporting "right passphrase, corrupt file" separately
    // would confirm a guess.
    std::optional<PrivateKey> load(const SingularId& id,
                                   const std::string& passphrase) const;

    bool contains(const SingularId& id) const;
    std::vector<SingularId> list() const;
    bool remove(const SingularId& id);

    const std::filesystem::path& directory() const { return _directory; }

private:
    std::filesystem::path pathFor(const SingularId& id) const;

    std::filesystem::path _directory;
};

} // namespace Identity
