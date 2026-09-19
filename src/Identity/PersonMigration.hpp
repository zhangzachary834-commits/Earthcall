#pragma once

#include "Identity/IdentityLedger.hpp"
#include "Identity/KeyStore.hpp"
#include "json.hpp"

#include <string>
#include <vector>

namespace Identity {

// ---------------------------------------------------------------------------
// Turning a name-addressed world into an identity-bearing one.
//
// Legacy saves address Persons by a chosen string: zones[].owner is "Zach",
// deletability is keyed by "Zach", laws name "Zach" as an author. Every one of
// those is forgeable with a text editor, which is the hole this closes.
//
// The migration mints one identity per distinct Person name (through the
// ledger, so the name resolves the same way in every save), rewrites the
// references, and converts ownership from a bare string into a signed Claim.
// After it runs, editing the owner field no longer transfers anything: the
// signature stops matching and the claim is refused.
//
// TRUST BOUNDARY. Exactly one moment here takes a name at its word -- the
// moment a legacy "Zach" is granted a fresh keypair and the world's existing
// ownership is signed over to it. That is unavoidable: the old world contains
// no proof of anything, only assertions, so migration either trusts them once
// or discards the world. It is done once, recorded in the ledger, and never
// repeated. Everything after it requires a signature.
// ---------------------------------------------------------------------------

struct MigrationReport {
    bool ran = false;              // false when the save was already migrated
    int identitiesMinted = 0;
    int identitiesReused = 0;      // resolved from the ledger, not freshly minted
    int ownersRewritten = 0;
    int deletabilityRewritten = 0;
    int lawAuthorsRewritten = 0;
    std::vector<std::string> personNames;
    std::vector<std::string> warnings;

    std::string summary() const;
};

// Which names in this save are confirmably Persons.
//
// Deliberately conservative. A law's authors[] may name ANY Singular -- a Zone,
// a Concept, the transfer-policy -- and rewriting a non-Person author would
// detach the law from its author and silently render it Unauthored. So a name
// is treated as a Person only where the format guarantees it: zone ownership
// and per-person deletability. Law authors are rewritten only for names
// already established as Persons by those fields or by the ledger.
std::vector<std::string> discoverPersonNames(const nlohmann::json& save,
                                             const IdentityLedger& ledger);

// Rewrites `save` in place. Idempotent: a save already carrying the migration
// marker is returned untouched with ran == false.
MigrationReport migrateSave(nlohmann::json& save,
                            IdentityLedger& ledger,
                            KeyStore& keys,
                            const std::string& passphrase,
                            int64_t at);

// True if this save has already been through migration.
bool isMigrated(const nlohmann::json& save);

// Verify every ownership claim in a migrated save. Returns the zone names whose
// ownership does NOT verify -- the ones a text editor touched.
std::vector<std::string> verifyOwnership(const nlohmann::json& save);

} // namespace Identity
